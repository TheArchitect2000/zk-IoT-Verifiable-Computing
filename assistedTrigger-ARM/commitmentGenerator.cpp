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
// `g++ -std=c++17 commitmentGenerator.cpp lib/polynomial.cpp -o commitmentGenerator -lstdc++`


#include "lib/polynomial.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "lib/json.hpp"
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

  for (const auto& instr : instructions) {
    std::stringstream ss(instr);
    std::string opcode, rd, leftStr, rightStr;
    
    ss >> opcode >> rd;
    ss >> leftStr >> rightStr;
    leftStr = Polynomial::trim(leftStr);
    rightStr = Polynomial::trim(rightStr);
    leftStr = Polynomial::removeCommas(leftStr);
    rightStr = Polynomial::removeCommas(rightStr);
    // cout << "opcode: " << opcode << "\tleftStr: " << leftStr << "\trightStr: " << rightStr << "\n";
  }
  // cout << "Number of immediate instructions (n_i): " << n_i << endl;
  // cout << "Number of general instructions (n_g): " << n_g << endl;

  // Matrix order
  uint64_t t;
  // cout << "Matrix order: " << n << endl;

  t = n_i + 1;
  // m = (((Polynomial::power(n, 2, p) - n) / 2) - ((Polynomial::power(t, 2, p) - t) / 2)) % p;

  // Initialize matrices A, B, C
  vector<vector<uint64_t>> A(n, vector<uint64_t>(n, 0ll));
  vector<vector<uint64_t>> B(n, vector<uint64_t>(n, 0ll));
  vector<vector<uint64_t>> C(n, vector<uint64_t>(n, 0ll));
  
  vector<uint64_t> rd_latest_used(32, 0);

  // Fill matrices based on the instructions
  for (uint64_t i = 0; i < n_g; i++) {
    std::stringstream ss(instructions[i]);
    std::string opcode, rd, leftStr, rightStr;
    ss >> opcode >> rd;
    uint64_t li = 0;
    uint64_t ri = 0;

    if (opcode == "add" || opcode == "addi" || opcode == "mul" || opcode == "sub" || opcode == "sdiv" ||opcode == "udiv") {
      ss >> leftStr >> rightStr;

      // Remove commas
      rd = Polynomial::removeCommas(rd);
      leftStr = Polynomial::removeCommas(leftStr);
      rightStr = Polynomial::removeCommas(rightStr);
      // Trim spaces
      rd = Polynomial::trim(rd);
      leftStr = Polynomial::trim(leftStr);
      rightStr = Polynomial::trim(rightStr);

      uint64_t leftInt, rightInt;
      
      if(opcode == "add" || opcode == "addi" || opcode == "sub" || opcode == "mul") {
        C[1+n_i+i][1+n_i+i] = 1;
      }

      if (opcode == "add" || opcode == "addi") {
        A[1+n_i+i][0] = 1;
        if (std::isdigit(leftStr[0]) || (leftStr[0] == '#' && leftStr.size() > 1 && std::isdigit(leftStr[1]))) {
          leftInt = std::stoi(leftStr[0] == '#' ? leftStr.substr(1) : leftStr);
          B[1+n_i+i][0] = leftInt;
        }
        else {
          if(rd_latest_used[registerMap[leftStr]] == 0){
            li = (registerMap[leftStr] + 1);
          }
          else {
            li = rd_latest_used[registerMap[leftStr]];
          }
          B[1+n_i+i][li] = 1;
        }
        
        if (std::isdigit(rightStr[0]) || (rightStr[0] == '#' && rightStr.size() > 1 && std::isdigit(rightStr[1]))) {
          rightInt = std::stoi(rightStr[0] == '#' ? rightStr.substr(1) : rightStr);
          B[1+n_i+i][0] = rightInt;
        }
        else {
          if(rd_latest_used[registerMap[rightStr]] == 0){
            ri = (registerMap[rightStr] + 1);
          }
          else {
            ri = rd_latest_used[registerMap[rightStr]];
          }
          B[1+n_i+i][ri] = 1;
        }

    } else if (opcode == "mul") {
        if (std::isdigit(leftStr[0]) || (leftStr[0] == '#' && leftStr.size() > 1 && std::isdigit(leftStr[1]))) {
          leftInt = std::stoi(leftStr[0] == '#' ? leftStr.substr(1) : leftStr);
          
          A[1+n_i+i][0] = leftInt;
        }
        else {
          if(rd_latest_used[registerMap[leftStr]] == 0){
            li = (registerMap[leftStr] + 1);
          }
          else {
            li = rd_latest_used[registerMap[leftStr]];
          }
          A[1+n_i+i][li] = 1;
        }
        if (std::isdigit(rightStr[0]) || (rightStr[0] == '#' && rightStr.size() > 1 && std::isdigit(rightStr[1]))) {
          rightInt = std::stoi(rightStr[0] == '#' ? rightStr.substr(1) : rightStr);
          
          B[1+n_i+i][0] = rightInt;
        }
        else {
          if(rd_latest_used[registerMap[rightStr]] == 0){
            ri = (registerMap[rightStr] + 1);
          }
          else {
            ri = rd_latest_used[registerMap[rightStr]];
          }
          B[1+n_i+i][ri] = 1;
        }
      } else if (opcode == "sub") {
        A[1+n_i+i][0] = 1;
        if (std::isdigit(leftStr[0]) || (leftStr[0] == '#' && leftStr.size() > 1 && std::isdigit(leftStr[1]))) {
          leftInt = std::stoi(leftStr[0] == '#' ? leftStr.substr(1) : leftStr);
          B[1+n_i+i][0] = leftInt % p;
        }
        else {
          if(rd_latest_used[registerMap[leftStr]] == 0){
            li = (registerMap[leftStr] + 1);
          }
          else {
            li = rd_latest_used[registerMap[leftStr]];
          }
          B[1+n_i+i][li] = 1;
        }
        
        if (std::isdigit(rightStr[0]) || (rightStr[0] == '#' && rightStr.size() > 1 && std::isdigit(rightStr[1]))) {
          rightInt = std::stoi(rightStr[0] == '#' ? rightStr.substr(1) : rightStr);
          B[1+n_i+i][0] = ((rightInt * (p - 1)) % p);
        }
        else {
          if(rd_latest_used[registerMap[rightStr]] == 0){
            ri = (registerMap[rightStr] + 1);
          }
          else {
            ri = rd_latest_used[registerMap[rightStr]];
          }
          B[1+n_i+i][ri] = (p - 1);
        }
        
      } else if (opcode == "sdiv" || opcode == "udiv") {
        if(rd_latest_used[registerMap[leftStr]] == 0){
          li = (registerMap[leftStr] + 1);
        }
        else {
          li = rd_latest_used[registerMap[leftStr]];
        }
        C[1+n_i+i][li] = 1;
        A[1+n_i+i][1+n_i+i] = 1;
        if (std::isdigit(rightStr[0]) || (rightStr[0] == '#' && rightStr.size() > 1 && std::isdigit(rightStr[1]))) {
          rightInt = std::stoi(rightStr[0] == '#' ? rightStr.substr(1) : rightStr);
          B[1+n_i+i][0] = rightInt;
        }
        else {
          if(rd_latest_used[registerMap[rightStr]] == 0){
            ri = (registerMap[rightStr] + 1);
          }
          else {
            ri = rd_latest_used[registerMap[rightStr]];
          }
          B[1+n_i+i][ri] = 1;
        }
      }
      rd_latest_used[registerMap[rd]] = (1 + n_i + i);
    } else {
      throw std::runtime_error("Error: Fides commitmentGenerator program cannot recognize the opcode << " + opcode + " >> within the specified code_block range. The code_block range is defined in the device_config.json file.\n");
    }
  }

  Polynomial::printMatrix(A, "A");
  Polynomial::printMatrix(B, "B");
  Polynomial::printMatrix(C, "C");

  // Vector H to store powers of w
  vector<uint64_t> H;
  uint64_t w, g_n;

  H.push_back(1);
  g_n = ((p - 1) / n) % p;
  w = Polynomial::power(g, g_n, p);
  for (uint64_t i = 1; i < n; i++) {
    H.push_back(Polynomial::power(w, i, p));
  }
  cout << "H[n]: ";
  for (uint64_t i = 0; i < n; i++) {
    cout << H[i] << " ";
  }
  cout << endl;

  uint64_t y, g_m;

  // Vector K to store powers of y
  vector<uint64_t> K;
  K.push_back(1);
  g_m = ((p - 1) * Polynomial::pInverse(m, p)) % p;
  y = Polynomial::power(g, g_m, p);
  for (uint64_t i = 1; i < m; i++) {
    K.push_back(Polynomial::power(y, i, p));
  }
  cout << "K[m]: ";
  for (uint64_t i = 0; i < m; i++) {
    cout << K[i] << " ";
  }
  cout << endl;
  
  // Create a polynomial vector vH_x of size (n + 1) initialized to 0
  vector<uint64_t> vH_x(n + 1, 0);
  vH_x[0] = p - 1;
  vH_x[n] = 1;
  Polynomial::printPolynomial(vH_x, "vH(x)");

 // Create a mapping for the non-zero rows using parameters K and H
  vector<vector<uint64_t>> nonZeroRowsA = Polynomial::getNonZeroRows(A);
  vector<vector<uint64_t>> rowA = Polynomial::createMapping(K, H, nonZeroRowsA);
  
  Polynomial::printMapping(rowA, "row_A");
  vector<vector<uint64_t>> nonZeroColsA = Polynomial::getNonZeroCols(A);
  vector<vector<uint64_t>> colA = Polynomial::createMapping(K, H, nonZeroColsA);
  
  Polynomial::printMapping(colA, "col_A");
  vector<vector<uint64_t>> valA = Polynomial::valMapping(K, H, nonZeroRowsA, nonZeroColsA, p);
  Polynomial::printMapping(valA, "val_A");

  vector<vector<uint64_t>> nonZeroRowsB = Polynomial::getNonZeroRows(B);
  vector<vector<uint64_t>> rowB = Polynomial::createMapping(K, H, nonZeroRowsB);
  
  Polynomial::printMapping(rowB, "row_B");
  vector<vector<uint64_t>> nonZeroColsB = Polynomial::getNonZeroCols(B);
  vector<vector<uint64_t>> colB = Polynomial::createMapping(K, H, nonZeroColsB);
  
  Polynomial::printMapping(colB, "col_B");
  vector<vector<uint64_t>> valB = Polynomial::valMapping(K, H, nonZeroRowsB, nonZeroColsB, p);
  Polynomial::printMapping(valB, "val_B");

  vector<vector<uint64_t>> nonZeroRowsC = Polynomial::getNonZeroRows(C);
  vector<vector<uint64_t>> rowC = Polynomial::createMapping(K, H, nonZeroRowsC);
  
  Polynomial::printMapping(rowC, "row_C");
  vector<vector<uint64_t>> nonZeroColsC = Polynomial::getNonZeroCols(C);
  vector<vector<uint64_t>> colC = Polynomial::createMapping(K, H, nonZeroColsC);
  
  Polynomial::printMapping(colC, "col_C");
  vector<vector<uint64_t>> valC = Polynomial::valMapping(K, H, nonZeroRowsC, nonZeroColsC, p);
  Polynomial::printMapping(valC, "val_C");


  vector<uint64_t> rowA_x = Polynomial::setupNewtonPolynomial(rowA[0], rowA[1], p, "rowA(x)");
  vector<uint64_t> colA_x = Polynomial::setupNewtonPolynomial(colA[0], colA[1], p, "colA(x)");
  vector<uint64_t> valA_x = Polynomial::setupNewtonPolynomial(valA[0], valA[1], p, "valA(x)");

  vector<uint64_t> rowB_x = Polynomial::setupNewtonPolynomial(rowB[0], rowB[1], p, "rowB(x)");
  vector<uint64_t> colB_x = Polynomial::setupNewtonPolynomial(colB[0], colB[1], p, "colB(x)");
  vector<uint64_t> valB_x = Polynomial::setupNewtonPolynomial(valB[0], valB[1], p, "valB(x)");

  vector<uint64_t> rowC_x = Polynomial::setupNewtonPolynomial(rowC[0], rowC[1], p, "rowC(x)");
  vector<uint64_t> colC_x = Polynomial::setupNewtonPolynomial(colC[0], colC[1], p, "colC(x)");
  vector<uint64_t> valC_x = Polynomial::setupNewtonPolynomial(valC[0], valC[1], p, "valC(x)");

  vector<uint64_t> O_AHP;

  O_AHP.insert(O_AHP.end(), rowA_x.begin(), rowA_x.end());
  O_AHP.insert(O_AHP.end(), colA_x.begin(), colA_x.end());
  O_AHP.insert(O_AHP.end(), valA_x.begin(), valA_x.end());

  O_AHP.insert(O_AHP.end(), rowB_x.begin(), rowB_x.end());
  O_AHP.insert(O_AHP.end(), colB_x.begin(), colB_x.end());
  O_AHP.insert(O_AHP.end(), valB_x.begin(), valB_x.end());

  O_AHP.insert(O_AHP.end(), rowC_x.begin(), rowC_x.end());
  O_AHP.insert(O_AHP.end(), colC_x.begin(), colC_x.end());
  O_AHP.insert(O_AHP.end(), valC_x.begin(), valC_x.end());

  cout << "O_AHP = {";
  for (uint64_t i = 0; i < O_AHP.size(); i++) {
    cout << O_AHP[i];
    if (i != O_AHP.size() - 1) {
      cout << ", ";
    }
  }
  cout << "}" << endl;

  uint64_t Com0_AHP = 0, Com1_AHP = 0, Com2_AHP = 0, Com3_AHP = 0, Com4_AHP = 0, Com5_AHP = 0, Com6_AHP = 0, Com7_AHP = 0, Com8_AHP = 0;

  for (uint64_t i = 0; i < rowA_x.size(); i++) {
    Com0_AHP += (ck[i] * rowA_x[i]) % p;
    Com1_AHP += (ck[i] * colA_x[i]) % p;
    Com2_AHP += (ck[i] * valA_x[i]) % p;
    
    Com3_AHP += (ck[i] * rowB_x[i]) % p;
    Com4_AHP += (ck[i] * colB_x[i]) % p;
    Com5_AHP += (ck[i] * valB_x[i]) % p;
    
    Com6_AHP += (ck[i] * rowC_x[i]) % p;
    Com7_AHP += (ck[i] * colC_x[i]) % p;
    Com8_AHP += (ck[i] * valC_x[i]) % p;

    Com0_AHP %= p;
    Com1_AHP %= p;
    Com2_AHP %= p;
    Com3_AHP %= p;
    Com4_AHP %= p;
    Com5_AHP %= p;
    Com6_AHP %= p;
    Com7_AHP %= p;
    Com8_AHP %= p;
  }
  cout << "Com0_AHP = " << Com0_AHP << endl;
  cout << "Com1_AHP = " << Com1_AHP << endl;
  cout << "Com2_AHP = " << Com2_AHP << endl;
  cout << "Com3_AHP = " << Com3_AHP << endl;
  cout << "Com4_AHP = " << Com4_AHP << endl;
  cout << "Com5_AHP = " << Com5_AHP << endl;
  cout << "Com6_AHP = " << Com6_AHP << endl;
  cout << "Com7_AHP = " << Com7_AHP << endl;
  cout << "Com8_AHP = " << Com8_AHP << endl;

// Getting the current timestamp as a string
  auto now = std::chrono::system_clock::now();
  auto in_time_t = std::chrono::system_clock::to_time_t(now);
  // std::cout << "in_time_t: " << in_time_t << std::endl;

  // Concatenate the strings
  std::stringstream commitment_id_ss;
  commitment_id_ss << deviceType << deviceIdType << deviceModel << manufacturer << softwareVersion << in_time_t;
  std::string concatenatedString = commitment_id_ss.str();
  char* concatenatedStringCStr = const_cast<char*>(concatenatedString.c_str());

  commitmentID = Polynomial::SHA256(concatenatedStringCStr);

  ordered_json commitment;
  commitment.clear();
  commitment["commitmentId"] = commitmentID;
  commitment["deviceType"] = deviceType;
  commitment["deviceIdType"] = deviceIdType;
  commitment["deviceModel"] = deviceModel;
  commitment["manufacturer"] = manufacturer;
  commitment["softwareVersion"] = softwareVersion;
  commitment["class"] = Class;
  commitment["m"] = m;
  commitment["n"] = n;
  commitment["p"] = p;
  commitment["g"] = g;
  commitment["row_AHP_A"] = rowA_x;
  commitment["col_AHP_A"] = colA_x;
  commitment["val_AHP_A"] = valA_x;
  commitment["row_AHP_B"] = rowB_x;
  commitment["col_AHP_B"] = colB_x;
  commitment["val_AHP_B"] = valB_x;
  commitment["row_AHP_C"] = rowC_x;
  commitment["col_AHP_C"] = colC_x;
  commitment["val_AHP_C"] = valC_x;
  commitment["Com_AHP0"] = Com0_AHP;
  commitment["Com_AHP1"] = Com1_AHP;
  commitment["Com_AHP2"] = Com2_AHP;
  commitment["Com_AHP3"] = Com3_AHP;
  commitment["Com_AHP4"] = Com4_AHP;
  commitment["Com_AHP5"] = Com5_AHP;
  commitment["Com_AHP6"] = Com6_AHP;
  commitment["Com_AHP7"] = Com7_AHP;
  commitment["Com_AHP8"] = Com8_AHP;
  commitment["curve"] = "bn128";
  commitment["polynomialCommitment"] = "KZG";

  // Serialize JSON object to a string
  std::string commitmentString = commitment.dump(4);
  // Write JSON object to a file
  std::ofstream commitmentFile(commitmentFileName);
  if (commitmentFile.is_open()) {
      commitmentFile << commitmentString;
      commitmentFile.close();
      std::cout << commitmentFileName << " is created successfully\n";
  } else {
    throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + commitmentFileName + " for writing proposes\n");\
  }

  vector<vector<uint64_t>> nonZeroB;
  for(uint64_t i = 0; i < nonZeroRowsB[0].size(); i++){
    nonZeroB.push_back({nonZeroRowsB[0][i], nonZeroColsB[0][i], nonZeroColsB[1][i]});
  }
  ordered_json program_param;
  program_param.clear();
  program_param["A"] = nonZeroColsA[0];
  program_param["B"] = nonZeroB;
  program_param["C"] = nonZeroColsC[0];
  program_param["rA"] = rowA[1];
  program_param["cA"] = colA[1];
  program_param["vA"] = valA[1];
  program_param["rB"] = rowB[1];
  program_param["cB"] = colB[1];
  program_param["vB"] = valB[1];
  program_param["rC"] = rowC[1];
  program_param["cC"] = colC[1];
  program_param["vC"] = valC[1];


  // Serialize JSON object to a string
  std::string program_paramString = program_param.dump(4);
  // Write JSON object to a file
  std::ofstream program_paramFile(paramFileName);
  if (program_paramFile.is_open()) {
      program_paramFile << program_paramString;
      program_paramFile.close();
      std::cout << paramFileName << " is created successfully\n";
  } else {
    throw std::runtime_error("Error: Fides commitmentGenerator cannot open " + paramFileName + " for writing proposes\n");
  }
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
