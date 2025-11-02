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

// Compile your code based on your operating system.
// `g++ -std=c++17 commitmentGenerator.cpp ../lib/polynomial.cpp -o commitmentGenerator -lstdc++`


#include "../lib/polynomial.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../lib/json.hpp"
using ordered_json = nlohmann::ordered_json;
#include <regex>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std;

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

uint64_t n_i, n_g, m, n, p, g;

std::string configFilePath = "device_config.json", setupFilePath, assemblyFilePath = "program.s", newAssemblyFile = "program_new.s", commitmentFileName, paramFileName;

std::vector<std::string> instructions;
uint64_t Class;
string commitmentID;
string deviceType;
string deviceIdType;
string deviceModel;
string manufacturer;
string softwareVersion;


// Function to parse the device configuration
std::pair<uint64_t, uint64_t> parseDeviceConfig(const std::string &configFilePath, nlohmann::json &config) {
  std::ifstream configFileStream(configFilePath, std::ifstream::binary);
  if (!configFileStream.is_open()) {
      throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + configFilePath + " for reading proposes.\n");
  }

  configFileStream >> config;
  configFileStream.close();

  std::vector<uint64_t> linesToRead;

  uint64_t startLine = config["code_block"][0].get<uint64_t>();
  uint64_t endLine = config["code_block"][1].get<uint64_t>();
  Class = config["class"].get<uint64_t>();
  deviceType = config["deviceType"].get<string>();
  deviceIdType = config["deviceIdType"].get<string>();
  deviceModel = config["deviceModel"].get<string>();
  manufacturer = config["manufacturer"].get<string>();
  softwareVersion = config["softwareVersion"].get<string>();

  std::ifstream classFileStream("class.json");
  if (!classFileStream.is_open()) {
      throw std::runtime_error("Error: Fides commitmentGenerator cannot open class.json for reading proposes.\n");
  }
  nlohmann::json classJsonData;
  classFileStream >> classJsonData;
  classFileStream.close();
  string class_value = to_string(Class); // Convert integer to string class
  n_g = classJsonData[class_value]["n_g"].get<uint64_t>();
  n_i = classJsonData[class_value]["n_i"].get<uint64_t>();
  n   = classJsonData[class_value]["n"].get<uint64_t>();
  m   = classJsonData[class_value]["m"].get<uint64_t>();
  p   = classJsonData[class_value]["p"].get<uint64_t>();
  g   = classJsonData[class_value]["g"].get<uint64_t>();

  return {startLine, endLine};
}

// Function to read specified lines from assembly file
std::vector<std::string> readAssemblyLines(const std::string &assemblyFilePath, uint64_t startLine, uint64_t endLine) {
  std::ifstream assemblyFileStream(assemblyFilePath);
  if (!assemblyFileStream.is_open()) {
    throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + assemblyFilePath + " for reading proposes.\n");
  }

  std::vector<std::string> selectedLines;
  std::string line;
  uint64_t currentLineNumber = 1;

  while (std::getline(assemblyFileStream, line)) {
      // if (currentLineNumber >= startLine && currentLineNumber <= endLine) {
          selectedLines.push_back(line);
      // }
      ++currentLineNumber;
  }

  assemblyFileStream.close();
  
  if (selectedLines.empty()) {
    throw std::runtime_error("Error: The code_block range contains blank lines. Please check the device_config.json file.");
  }

  return selectedLines;
}

vector<vector<uint64_t>> vector_z(2, vector<uint64_t>(2, 0ll));


// Function to modify assembly code and return the modified lines
std::vector<std::string> modifyAssembly(const std::vector<std::string> &originalLines, uint64_t startLine, uint64_t endLine) {
    std::vector<std::string> modifiedLines;

    for (size_t i = 0; i < originalLines.size(); ++i) {
      if (i + 1 == startLine) {
          modifiedLines.push_back(".global zkp_start");
          modifiedLines.push_back("zkp_start: nop");
          modifiedLines.push_back(originalLines[i]);
          instructions.push_back(originalLines[i]);
      } else if (i + 1 > startLine && i + 1 <= endLine) {
          modifiedLines.push_back(originalLines[i]);
          instructions.push_back(originalLines[i]);
      } else if (i + 1 == endLine + 1) {
          modifiedLines.push_back(".global zkp_end");
          modifiedLines.push_back("zkp_end: nop");
          modifiedLines.push_back(originalLines[i]);
      } else {
          modifiedLines.push_back(originalLines[i]);
      }
    }

    return modifiedLines;
}

// Function to write a vector of strings to a file
void writeToFile(const std::string &filePath, const std::vector<std::string> &content) {
    std::ofstream fileStream(filePath);
    if (!fileStream.is_open()) {
        throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + filePath + "for writing proposes\n");
    }

    for (const auto &line : content) {
        fileStream << line << "\n";
    }
    
    cout << filePath << " is created successfully\n";
}

void commitmentGenerator() {
  setupFilePath = "data/setup";
  setupFilePath += to_string(Class);
  setupFilePath += ".json";
  std::ifstream setupFileStream(setupFilePath);
  if (!setupFileStream.is_open()) {
      throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + setupFilePath + " for reading proposes.\n");
  }
  nlohmann::json setupJsonData;
  setupFileStream >> setupJsonData;
  setupFileStream.close();
  vector<uint64_t> ck = setupJsonData["ck"].get<vector<uint64_t>>();
  uint64_t vk = setupJsonData["vk"].get<uint64_t>();

}

int main(int argc, char* argv[]) {
  // TODO: Remove the hard coded file names and use the inputs from user

  // std::string configFilePath, setupFilePath, assemblyFilePath, newAssemblyFile;
  // Input filenames
  // std::cout << "Enter the device config file name: ";
  // std::cin >> configFilePath;
  // std::cout << "Enter setup file name: ";
  // std::cin >> setupFilePath;
  // std::cout << "Enter the program assembly file name: ";
  // std::cin >> assemblyFilePath;
  // std::cout << "Enter the output file name for modified assembly: ";
  // std::cin >> newAssemblyFile;
  assemblyFilePath = argv[1];
  newAssemblyFile = assemblyFilePath;// + "_new.s";
  newAssemblyFile = newAssemblyFile.substr(0, newAssemblyFile.find_last_of('.')) + "_AddedFidesProofGen.s";
  commitmentFileName = assemblyFilePath;
  commitmentFileName = commitmentFileName.substr(0, commitmentFileName.find_last_of('.')) + "_commitment.json";
  paramFileName = assemblyFilePath;
  paramFileName = paramFileName.substr(0, paramFileName.find_last_of('.')) + "_param.json";
  nlohmann::json config;
  auto [startLine, endLine] = parseDeviceConfig(configFilePath, config);
  if((endLine - startLine)+1 != n_g) {
    throw std::runtime_error(
      "Error: The 'code_block' range in device_config.json does not match the number of supported instructions (n_g) for the selected 'class'. "
      "Please verify the 'code_block' and 'class' values in device_config.json."
    );
  }
  cout << "startLine: " << startLine << endl;
  cout << "endLine: " << endLine << endl;
  // modifyAndSaveAssembly(assemblyFilePath, newAssemblyFile, startLine, endLine);

  auto originalLines = readAssemblyLines(assemblyFilePath, startLine, endLine);
  // for (const auto& i : originalLines) {
  //     std::cout << "originalLines: " << i << std::endl;
  // }

  auto modifiedLines = modifyAssembly(originalLines, startLine, endLine);
  uint64_t startLineIndex = (startLine > 3) ? startLine - 3 : 0;
  uint64_t endLineIndex = (endLine < modifiedLines.size()) ? endLine : modifiedLines.size() - 1;
  for (uint64_t i = startLineIndex; i <= startLineIndex + 5; i++) {
      if(i == startLine-1 || i == startLine) {
        std::cout << to_string(i+1) << "(added)\t" << modifiedLines[i] << std::endl;
      }
      else if(i > startLine) {
        std::cout << to_string(i+1) << "(" << to_string(i-1) << ")\t" << modifiedLines[i] << std::endl;
      }
      else {
        std::cout << to_string(i+1) << "\t" << modifiedLines[i] << std::endl;
      }
  }
  std::cout << "..." << std::endl;
  for (uint64_t i = endLineIndex; i <= endLineIndex + 5; i++) {
      if(i < endLineIndex + 2) {
        std::cout << to_string(i+1) << "(" << to_string(i-1) << ")\t" << modifiedLines[i] << std::endl;
      }
      else if(i >= endLineIndex + 4) {
        std::cout << to_string(i+1) << "(" << to_string(i-1) << ")\t" << modifiedLines[i] << std::endl;
      }
      else {
        std::cout << to_string(i+1) << "(added)\t" << modifiedLines[i] << std::endl;
      }
  }
  
  // for (const auto& j : modifiedLines) {
  //     std::cout << "modifiedLines: " << j << std::endl;
  // }


  // TODO: update this part to be dynamic
  commitmentGenerator();

  writeToFile(newAssemblyFile, modifiedLines);
  
  return 0;
}
