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
#include <bitset>

using namespace std;
using namespace chrono;

// Map of ARM 32-bit registers
std::unordered_map<std::string, int> registerMap = {
    {"r0", 0},   {"r1", 1},   {"r2", 2},   {"r3", 3},
    {"r4", 4},   {"r5", 5},   {"r6", 6},   {"r7", 7},
    {"r8", 8},   {"r9", 9},   {"r10", 10}, {"r11", 11},
    {"r12", 12}, {"r13", 13}, {"r14", 14}, {"r15", 15},
    {"sp", 31},  // Stack Pointer (alias for r13)
    {"lr", 14},  // Link Register (alias for r14)
    {"pc", 15},  // Program Counter (alias for r15)
    {"x0", 0},   {"x1", 1},   {"x2", 2},   {"x3", 3},
    {"x4", 4},   {"x5", 5},   {"x6", 6},   {"x7", 7},
    {"x8", 8},   {"x9", 9},   {"x10", 10}, {"x11", 11},
    {"x12", 12}, {"x13", 13}, {"x14", 14}, {"x15", 15},
    {"x16", 16},   {"x17", 17},   {"x18", 18},   {"x19", 19},
    {"x20", 20},   {"x21", 21},   {"x22", 22},   {"x23", 23},
    {"x24", 24},   {"x25", 25},   {"x26", 26},   {"x27", 27},
    {"x28", 28},   {"x29", 29},   {"x30", 30}
};

struct DimEntry {
    int i;        // index (1..8)
    std::vector<bool> indices; // flexible indices
    uint8_t value; // 8-bit interleaved nibble pair
};


vector<int64_t> z_array;
vector<std::string> src_reg1_array;
vector<std::string> src_reg2_array;
vector<std::string> dest_reg_array;

vector<uint64_t> reg_val_buffer;
vector<uint64_t> src_val1_array;
vector<uint64_t> src_val2_array;
vector<uint64_t> dest_val_array;

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
      reg_val_buffer.push_back(int_val);
      z_array.push_back(int_val);
      if(line_number == 31) {
        z_array.push_back(0);
      }
    }
    if (line.find("=>") != string::npos) {
      
      line_number = 100;
      ss.ignore(256, ':');
      ss >> instruction >> dest_reg >> src_reg1 >> src_reg2 >> immediate;
      
      dest_reg = Polynomial::trim(dest_reg);
      dest_reg = Polynomial::removeCommas(dest_reg);

      src_reg1 = Polynomial::trim(src_reg1);
      src_reg1 = Polynomial::removeCommas(src_reg1);

      src_reg2 = Polynomial::trim(src_reg2);
      src_reg2 = Polynomial::removeCommas(src_reg2);
      src_reg1_array.push_back(src_reg1);
      src_reg2_array.push_back(src_reg2);
      dest_reg_array.push_back(dest_reg);
      src_val1_array.push_back(reg_val_buffer[registerMap[src_reg1]]);
      src_val2_array.push_back(reg_val_buffer[registerMap[src_reg2]]);

      // empty reg_val_buffer
      reg_val_buffer.clear();
    }
    if(line_number >= 101 && line_number <= 131) {
      ss >> reg_name >> hex_val >> int_val;
      reg_val_buffer.push_back(int_val);
      if(dest_reg == reg_name) {
        if(first_instruction) {
          input_value = z_array[line_number-100];
          first_instruction = false;
        }
        output_value = int_val;
        z_array.push_back(int_val);
        dest_val_array.push_back(int_val);
      }
    }
  }

  file.close();

  // Output results as integers
  for (const auto &val : z_array) {
      cout << val << endl;  // Output as decimal integers
  }
}



uint64_t subtable_search(const std::bitset<8>& inputBits) {
    std::ifstream file("subtable/and_4bit.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open subtable file\n";
        return 0;
    }

    // Convert bitset to integer
    uint64_t input = inputBits.to_ulong();

    // Split into two 4-bit nibbles
    uint64_t inputA = (input >> 4) & 0xF; // upper 4 bits
    uint64_t inputB = input & 0xF;        // lower 4 bits

    std::string a_str, b_str, r_str;

    while (file >> a_str >> b_str >> r_str) {
        uint64_t a = std::stoi(a_str, nullptr, 2);
        uint64_t b = std::stoi(b_str, nullptr, 2);
        uint64_t r = std::stoi(r_str, nullptr, 2);

        if (a == inputA && b == inputB) {
            std::cout << std::bitset<4>(inputA) << " " << std::bitset<4>(inputB) << " -> "
                      << std::bitset<4>(r_str) << endl;
            return r;
        }
    }

    std::cout << "No match for dim[] {" << inputA << "," << inputB << "}\n";
    return 0;
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
  for(uint64_t i = 0; i < (1 + n_i + n_g) -1; i++) {
    cout << z[i] << ", ";
  }
  cout << z[(1 + n_i + n_g) - 1] << "]" << endl;

  uint64_t t = n_i + 1;

  size_t dest_reg_array_size = dest_reg_array.size();
  for (size_t idx = 0; idx < dest_reg_array_size; ++idx) {
    cout << dest_reg_array[idx] << " = " 
        << src_reg1_array[idx] << " & " 
        << src_reg2_array[idx] << endl;

    cout << dest_val_array[idx] << " = " 
        << src_val1_array[idx] << " & " 
        << src_val2_array[idx] << endl;
  }

  std::vector<DimEntry> dim;
  dim.reserve(8 * 4); // 8 is the number of registers devided, 4 is placeholders for m

  for (int idx = 0; idx < 8; idx++) {
    for (int counter = 0; counter < 4; counter++) {
      std::vector<bool> indices;
      uint8_t nibbleA = (src_val2_array[counter] >> (4 * idx)) & 0xF;
      uint8_t nibbleB = (src_val1_array[counter] >> (4 * idx)) & 0xF;
      uint8_t combined = (nibbleB << 4) | nibbleA;
      for (int bit = 1; bit >= 0; bit--) { 
        indices.push_back((counter >> bit) & 1);
      }
      dim.push_back({idx, indices, combined});
    }
  }

  // Print to verify
  for (auto& entry : dim) {
    std::cout << "dim[" << entry.i+1 << "]{";
    for (size_t j=0; j<entry.indices.size(); j++) {
      std::cout << entry.indices[j];
      if (j < entry.indices.size()-1) std::cout << ",";
    }
    std::cout << "}=" << std::bitset<8>(entry.value) << "\n";
  }


  vector<uint64_t> C_dim(8, 0);
  for (int i = 0; i < 8; i++) {
    C_dim[i] = Polynomial::hashAndExtractLower4Bytes(((Polynomial::hashAndExtractLower4Bytes(Polynomial::power(g, dim[i].indices[0,0], p) * Polynomial::power(g, dim[i].indices[0,1], p), p) % p << 8) | (Polynomial::hashAndExtractLower4Bytes(Polynomial::power(g, dim[i].indices[1,0], p) * Polynomial::power(g, dim[i].indices[1,1], p), p) % p)), p);
  }

  for (int i = 0; i < 8; i++) {
    cout << "C_dim[" << to_string(i+1) << "] = " << C_dim[i] << endl;
  }


  vector<uint64_t> random_poly = Polynomial::generateRandomPolynomial(4, 4, p);
  uint64_t r = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(random_poly, 4, p), p) % 2;

  uint64_t w = 32;
  uint64_t c = 8;
  uint64_t s = 4;
  uint64_t alpha = 8;

  std::vector<DimEntry> E;
  E.reserve(8 * 4); // 8 is the number of registers devided, 4 is placeholders for m

  for (int idx = 0; idx < 8; idx++) {
    for (int counter = 0; counter < 4; counter++) {
      std::vector<bool> indices;
      uint8_t nibbleD = (dest_val_array[counter] >> (4 * idx)) & 0xF;
      for (int bit = 1; bit >= 0; bit--) { 
        indices.push_back((counter >> bit) & 1);
      }
      E.push_back({idx, indices, nibbleD});
    }
  }
  // Print to verify
  for (auto& entry : E) {
    std::cout << "E[" << entry.i+1 << "]{";
    for (size_t j=0; j<entry.indices.size(); j++) {
      std::cout << entry.indices[j];
      if (j < entry.indices.size()-1) std::cout << ",";
    }
    std::cout << "}=" << std::bitset<4>(entry.value) << "\n";
  }

  vector<uint64_t> C_E(8, 0);
  for (int i = 0; i < 8; i++) {
    C_E[i] = Polynomial::hashAndExtractLower4Bytes(((Polynomial::hashAndExtractLower4Bytes(Polynomial::power(g, dim[i].indices[0,0], p) * Polynomial::power(g, dim[i].indices[0,1], p), p) % p << 8) | (Polynomial::hashAndExtractLower4Bytes(Polynomial::power(g, dim[i].indices[1,0], p) * Polynomial::power(g, dim[i].indices[1,1], p), p) % p)), p);
  }

  for (int i = 0; i < 8; i++) {
    cout << "C_E[" << to_string(i+1) << "] = " << C_E[i] << endl;
  }


  uint64_t v = 0;

  // for (auto& entry : dim) {
  //   std::cout << "dim[" << entry.i+1 << "]{";
  //   for (size_t j=0; j<entry.indices.size(); j++) {
  //     std::cout << entry.indices[j];
  //     if (j < entry.indices.size()-1) std::cout << ",";
  //   }
  //   std::cout << "}=" << std::bitset<8>(entry.value) << "\n";
  // }
  for (auto& entry : dim) {
    if (entry.indices[0] == 1 && entry.indices[1] == 0) {
      cout << "dim[" << entry.i << "]{1,0} = "
          << std::bitset<8>(entry.value) << endl;

      uint64_t v_buffer = (Polynomial::power(2, entry.i, p) *
                          subtable_search(std::bitset<8>(entry.value))) % p;
      v = (v + v_buffer) % p;
    }
  }
  cout << "v = " << v << endl;

  uint64_t h1_x1 = 0;

  for (auto& entry : E) {
    if (entry.indices[0] == 1 && entry.indices[1] == 0) {
      cout << "E[" << entry.i << "]{1,0} = "
          << std::bitset<8>(entry.value) << endl;
    }
  }
  cout << "h1_x1 = " << h1_x1 << endl;
}

void verifyProof() {
  

}

int main(int argc, char* argv[]) {
  run_the_user_program(argc, argv);
  process_execution_trace_file();
  proofGenerator();
  verifyProof();
  return 0;
}
