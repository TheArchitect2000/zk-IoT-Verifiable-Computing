// Copyright 2025 Fidesinnova.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "../lib/fidesinnova.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../lib/json.hpp"
#include <sstream>
#include <unordered_map>
using ordered_json = nlohmann::ordered_json;
#include <regex>
#include <random>
#include <chrono>
#include <cstdlib>

using namespace std;
using namespace chrono;

vector<int64_t> z_array;
int64_t input_value = 0;
int64_t output_value = 0;

// Function to clean up the GDB output file
void cleanupTraceFile(const std::string& filename) {
  std::ifstream inFile(filename);
  if (!inFile) {
      std::cerr << "Failed to open trace file for cleaning." << std::endl;
  }

  std::vector<std::string> lines;
  std::string line;
  bool isBlockActive = false;
  std::string currentBlock;
  bool initialRegistersProcessed = false;

  // Read the file line by line
  while (std::getline(inFile, line)) {
      // Handle the initial register dump (before the first instruction)
      if (!initialRegistersProcessed && (line.find("x") == 0 || line.find("sp") == 0 || line.find("pc") == 0 ||
                                        line.find("cpsr") == 0 || line.find("fpsr") == 0 || line.find("fpcr") == 0)) {
          lines.push_back(line + "\n");
          continue;
      }

      // Start a new block when an instruction line is found
      if (line.find("=>") != std::string::npos) {
          if (isBlockActive) {
              // Save the current block
              lines.push_back(currentBlock);
              lines.push_back("-----------------------------------------------------\n"); // Separator between blocks
          }
          // Start a new block
          currentBlock = line + "\n";
          isBlockActive = true;
          initialRegistersProcessed = true; // Mark that initial registers have been processed
      }
      // Add register lines to the current block
      else if (line.find("x") == 0 || line.find("sp") == 0 || line.find("pc") == 0 ||
               line.find("cpsr") == 0 || line.find("fpsr") == 0 || line.find("fpcr") == 0) {
          if (isBlockActive) {
              currentBlock += line + "\n";
          }
      }
  }

  // Save the last block if it exists
  if (isBlockActive) {
      lines.push_back(currentBlock);
  }
  inFile.close();

  // Write the cleaned lines back to the file
  std::ofstream outFile(filename);
  if (!outFile) {
      std::cerr << "Failed to open trace file for writing cleaned data." << std::endl;
  }

  for (const auto& cleanedLine : lines) {
      outFile << cleanedLine;
  }
  outFile.close();
}

void run_the_user_program(int argc, char* argv[]) {
  if (argc < 2) {
      std::cerr << "Usage: " << argv[0] << " <program_to_execute>" << std::endl;
  }

  std::string program = argv[1];
  std::string gdbCommand = "gdb --batch --command=gdb_commands.txt " + program + " > /dev/null 2>&1";
  std::string outputFile = "execution_trace.txt";

  // Create a GDB command file
  std::ofstream gdbCommands("gdb_commands.txt");
  if (!gdbCommands) {
      std::cerr << "Failed to create GDB command file." << std::endl;
  }

  gdbCommands << "set logging file " << outputFile << std::endl;
  gdbCommands << "set logging overwrite on" << std::endl; // Overwrite the file
  gdbCommands << "set logging on" << std::endl;
  
  gdbCommands << "break zkp_start" << std::endl; // Set a breakpoint at zkp_start
  gdbCommands << "run" << std::endl; // Run the program until zkp_start
  gdbCommands << "stepi" << std::endl; // Move to the first instruction after zkp_start
  
  gdbCommands << "break zkp_end" << std::endl; // Set a breakpoint at zkp_end (stop before executing it)
  gdbCommands << "while $pc != zkp_end" << std::endl; // Iterate until the last instruction BEFORE zkp_end
  gdbCommands << "info registers" << std::endl; // Log all registers before executing each instruction
  gdbCommands << "x/i $pc" << std::endl; // Log the current instruction
  gdbCommands << "stepi" << std::endl; // Step to the next instruction
  gdbCommands << "end" << std::endl;
  
  gdbCommands << "info registers" << std::endl; // Capture all registers BEFORE reaching zkp_end (last meaningful instruction)
  
  gdbCommands << "set logging off" << std::endl;
  gdbCommands << "quit" << std::endl;
  

  gdbCommands.close();

  // Execute the GDB command and suppress terminal output
  int result = std::system(gdbCommand.c_str());
  if (result != 0) {
      std::cerr << "GDB execution failed." << std::endl;
  }

  // Clean up the trace file
  cleanupTraceFile(outputFile);

  std::cout << "Execution trace saved and cleaned in " << outputFile << std::endl;
}


void process_execution_trace_file() {
  ifstream file("execution_trace.txt");
  if (!file) {
      cerr << "Error opening file" << endl;
  }

  string line;
  int line_number = 0;
  
  string reg_name;
  string hex_val;
  int64_t int_val;

  stringstream ss(line);
  string instruction;
  string dest_reg;
  string src_reg1;
  string src_reg2;
  string immediate;
  bool first_instruction = true;
  while (getline(file, line)) {
    line_number++;
    stringstream ss(line);
    if (line_number <= 31) {
      if(line_number == 1) {
        z_array.push_back(1);
      }
      ss >> reg_name >> hex_val >> int_val;
      z_array.push_back(int_val);
      if(line_number == 31) {
        z_array.push_back(0);
      }
    }
    if (line.find("=>") != string::npos) {
      
      line_number = 100;
      ss.ignore(256, ':');
      ss >> instruction >> dest_reg >> src_reg1;
      dest_reg = Polynomial::trim(dest_reg);
      dest_reg = Polynomial::removeCommas(dest_reg);
    }
    if(line_number >= 101 && line_number <= 131) {
      ss >> reg_name >> hex_val >> int_val;
      if(dest_reg == reg_name) {
        if(first_instruction) {
          input_value = z_array[line_number-100];
          first_instruction = false;
        }
        output_value = int_val;
        z_array.push_back(int_val);
      }
    }
  }

  file.close();

  // Output results as integers
  for (const auto &val : z_array) {
      cout << val << endl;  // Output as decimal integers
  }
}


void proofGenerator() {
  cout << "\n\n\n\n*** Start proof generation ***" << endl;

  // Hardcoded file path
  const char* commitmentJsonFilePath = "program_commitment.json";

  // Parse the JSON file
  nlohmann::json commitmentJsonData;
  try {
    std::ifstream commitmentJsonFile(commitmentJsonFilePath);
    commitmentJsonFile >> commitmentJsonData;
    commitmentJsonFile.close();
  } catch (nlohmann::json::parse_error& e) {
    cout << "Enter the content of program_commitment.json file! (end with a blank line):" << endl;
    string commitmentJsonInput;
    string commitmentJsonLines;
    while (getline(cin, commitmentJsonLines)) {
      if (commitmentJsonLines.empty()) break;
      commitmentJsonInput += commitmentJsonLines + "\n";
    }
    commitmentJsonData = nlohmann::json::parse(commitmentJsonInput);
    // std::cerr << "Error: " << e.what() << std::endl;
  }

  // Extract data from the parsed JSON
  uint64_t Class = commitmentJsonData["class"].get<uint64_t>();
  std::string commitmentID = commitmentJsonData["commitmentId"].get<std::string>();
  std::vector<uint64_t> rowA_x = commitmentJsonData["row_AHP_A"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> colA_x = commitmentJsonData["col_AHP_A"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> valA_x = commitmentJsonData["val_AHP_A"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> rowB_x = commitmentJsonData["row_AHP_B"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> colB_x = commitmentJsonData["col_AHP_B"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> valB_x = commitmentJsonData["val_AHP_B"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> rowC_x = commitmentJsonData["row_AHP_C"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> colC_x = commitmentJsonData["col_AHP_C"].get<std::vector<uint64_t>>();
  std::vector<uint64_t> valC_x = commitmentJsonData["val_AHP_C"].get<std::vector<uint64_t>>();


  // Hardcoded file path
  const char* paramJsonFilePath = "program_param.json";

  // Parse the JSON file
  nlohmann::json paramJsonData;
  try {
    std::ifstream paramJsonFile(paramJsonFilePath);
    paramJsonFile >> paramJsonData;
    paramJsonFile.close();
  } catch (nlohmann::json::parse_error& e) {
    cout << "Enter the content of program_param.json file! (end with a blank line):" << endl;
    string paramJsonInput;
    string paramJsonLines;
    while (getline(cin, paramJsonLines)) {
      if (paramJsonLines.empty()) break;
      paramJsonInput += paramJsonLines + "\n";
    }
    paramJsonData = nlohmann::json::parse(paramJsonInput);
    // std::cerr << "Error: " << e.what() << std::endl;
  }
  vector<uint64_t> nonZeroA = paramJsonData["A"].get<vector<uint64_t>>();
  vector<vector<uint64_t>> nonZeroB = paramJsonData["B"].get<vector<vector<uint64_t>>>();
  vector<uint64_t> nonZeroC = paramJsonData["C"].get<vector<uint64_t>>();
  vector<uint64_t> rowA = paramJsonData["rA"].get<vector<uint64_t>>();
  vector<uint64_t> colA = paramJsonData["cA"].get<vector<uint64_t>>();
  vector<uint64_t> valA = paramJsonData["vA"].get<vector<uint64_t>>();
  vector<uint64_t> rowB = paramJsonData["rB"].get<vector<uint64_t>>();
  vector<uint64_t> colB = paramJsonData["cB"].get<vector<uint64_t>>();
  vector<uint64_t> valB = paramJsonData["vB"].get<vector<uint64_t>>();
  vector<uint64_t> rowC = paramJsonData["rC"].get<vector<uint64_t>>();
  vector<uint64_t> colC = paramJsonData["cC"].get<vector<uint64_t>>();
  vector<uint64_t> valC = paramJsonData["vC"].get<vector<uint64_t>>();



  const char* classJsonFilePath = "class.json";

  // Parse the JSON file
  nlohmann::json classJsonData;
  try {
      std::ifstream classJsonFile(classJsonFilePath);
      classJsonFile >> classJsonData;
      classJsonFile.close();
  } catch (nlohmann::json::parse_error& e) {
    cout << "Enter the content of class.json file! (end with a blank line):" << endl;
    string classJsonInput;
    string classJsonLines;
    while (getline(cin, classJsonLines)) {
      if (classJsonLines.empty()) break;
      classJsonInput += classJsonLines + "\n";
    }
    classJsonData = nlohmann::json::parse(classJsonInput);
      // std::cerr << "Error: " << e.what() << std::endl;
  }
  uint64_t n_i, n_g, m, n, p, g;
  string class_value = to_string(Class); // Convert integer to string class
  n_g = classJsonData[class_value]["n_g"].get<uint64_t>();
  n_i = classJsonData[class_value]["n_i"].get<uint64_t>();
  n   = classJsonData[class_value]["n"].get<uint64_t>();
  m   = classJsonData[class_value]["m"].get<uint64_t>();
  p   = classJsonData[class_value]["p"].get<uint64_t>();
  g   = classJsonData[class_value]["g"].get<uint64_t>();

  uint64_t upper_limit = (n_g < 10) ? n_g - 1 : 9;
  // Set up random number generation
  std::random_device rd;  // Seed
  std::mt19937_64 gen(rd()); // Random number engine
  std::uniform_int_distribution<uint64_t> dis(0, upper_limit);
  int64_t b = dis(gen);

  // Hardcoded file path
  std::string setupJsonFilePath = "data/setup" + class_value + ".json";
  const char* setupJsonFilePathCStr = setupJsonFilePath.c_str();

  // Parse the JSON file
  nlohmann::json setupJsonData;
  try {
      std::ifstream setupJsonFile(setupJsonFilePath);
      setupJsonFile >> setupJsonData;
      setupJsonFile.close();
  } catch (nlohmann::json::parse_error& e) {
    cout << "Enter the content of setup" << class_value << ".json file! (end with a blank line):" << endl;
    string setupJsonInput;
    string setupJsonLines;
    while (getline(cin, setupJsonLines)) {
      if (setupJsonLines.empty()) break;
      setupJsonInput += setupJsonLines + "\n";
    }
    setupJsonData = nlohmann::json::parse(setupJsonInput);
      // std::cerr << "Error: " << e.what() << std::endl;
  }
  vector<uint64_t> ck = setupJsonData["ck"].get<vector<uint64_t>>();
  uint64_t vk = setupJsonData["vk"].get<uint64_t>();


  // Measure the start time
  auto start_time = high_resolution_clock::now();
  vector<uint64_t> z;
  for(uint64_t i = 0; i < (1 + n_i + n_g); i++) {
    cout << "z_array" << "[" << i << "] = " << z_array[i] % p << endl;
    int64_t bufferZ = z_array[i] % p;
    if (bufferZ < 0) {
      bufferZ += p;
    }
    z.push_back(bufferZ);
  }

  cout << "\n\n" << endl;
  cout << "z" << "[";
  for(uint64_t i = 0; i < (1 + n_i + n_g); i++) {
    cout << z[i] << ", ";
  }
  cout << "]" << endl;

  uint64_t t = n_i + 1;

  vector<vector<vector<uint64_t>>> dim;

  for(uint64_t i = 1; i<=8; i++) {
    for(uint64_t j = 1; j <= 4; j++) {
      
      dim[i][j].push_back(0);
    }
  }
  for(auto &i : dim) {
    i = (i + t) % (n + 1);
  }
}


int main(int argc, char* argv[]) {
  run_the_user_program(argc, argv);
  process_execution_trace_file();
  proofGenerator();
  return 0;
}
