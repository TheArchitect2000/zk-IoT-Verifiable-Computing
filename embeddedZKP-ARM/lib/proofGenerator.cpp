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


#include "fidesinnova.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "json.hpp"
using ordered_json = nlohmann::ordered_json;
#include <regex>
#include <random>
#include <chrono>

using namespace std;
using namespace chrono;

int64_t input_value = 0;
int64_t output_value = 0;

extern "C" void store_register_instances();

extern "C" void proofGenerator() {
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
    // return;
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
    // return;
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
      // return;
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
      // return;
  }
  vector<uint64_t> ck = setupJsonData["ck"].get<vector<uint64_t>>();
  uint64_t vk = setupJsonData["vk"].get<uint64_t>();


  // Measure the start time
  auto start_time = high_resolution_clock::now();

  extern uint64_t z_array[];
  
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

  cout << "Initialize matrices A, B, C" << endl;
  // Initialize matrices A, B, C
  vector<vector<uint64_t>> A(n, vector<uint64_t>(n, 0ll));
  vector<vector<uint64_t>> B(n, vector<uint64_t>(n, 0ll));
  vector<vector<uint64_t>> C(n, vector<uint64_t>(n, 0ll));

  cout << "rowMatA" << endl;
  uint64_t rowMatA = n_i;
  for (uint64_t i = 0; i < nonZeroA.size(); i++) {
    int64_t col = nonZeroA[i];
    // Set the value in the matrix A
    A[i + n_i + 1][col] = 1;
  }
  // Polynomial::printMatrix(A, "A");
  

  for (const auto& entry : nonZeroB) {
    int64_t row = entry[0];
    int64_t col = entry[1];
    int64_t val = entry[2];
    // Set the value in the matrix B
    B[row][col] = val;
  }
  // for (uint64_t i = 0; i < nonZeroB.size(); i++) {
  //   int64_t row = nonZeroB[i + n_i + 1][0];
  //   int64_t col = nonZeroB[i + n_i + 1][1];
  //   int64_t val = nonZeroB[i + n_i + 1][2];    
  //   // Set the value in the matrix
  //   B[row][col] = val;
  // }
  // Polynomial::printMatrix(B, "B");

  cout << "rowMatC" << endl;
  uint64_t rowMatC = n_i;
  for (uint64_t i = 0; i < nonZeroC.size(); i++) {
    int64_t col = nonZeroC[i];
    // Set the value in the matrix A
    C[i + n_i + 1][col] = 1;
  }
  // Polynomial::printMatrix(C, "C");

  // vector<uint64_t> z;

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


  vector<vector<uint64_t>> Az(n, vector<uint64_t>(1, 0));
  vector<vector<uint64_t>> Bz(n, vector<uint64_t>(1, 0));
  vector<vector<uint64_t>> Cz(n, vector<uint64_t>(1, 0));
  cout << "n_i: " << n_i << endl;
  
  // Matrix multiplication with modulo
  for (uint64_t i = 0; i < n; i++) {
    for (uint64_t j = 0; j < 1; j++) {
      for (uint64_t k = 0; k < n; k++) {
        Az[i][j] = (Az[i][j] + (A[i][k] * z[k]) % p) % p;
        Bz[i][j] = (Bz[i][j] + (B[i][k] * z[k]) % p) % p;
        Cz[i][j] = (Cz[i][j] + (C[i][k] * z[k]) % p) % p;
      }
    }
  }
  // cout << "Matrice Az under modulo " << p << " is: ";
  // for (uint64_t i = 0; i < n; i++) {
  //   cout << Az[i][0] << " ";
  // }
  // cout << endl;

  // cout << "Matrice Bz under modulo " << p << " is: ";
  // for (uint64_t i = 0; i < n; i++) {
  //   cout << Bz[i][0] << " ";
  // }
  // cout << endl;

  // cout << "Matrice Cz under modulo " << p << " is: ";
  // for (uint64_t i = 0; i < n; i++) {
  //   cout << Cz[i][0] << " ";
  // }
  // cout << endl;



  vector<vector<uint64_t>> zA(2);
  // cout << "zA(x):" << endl;
  for (uint64_t i = 0; i < n + b; i++) {
  // for (uint64_t i = 0; i < n; i++) {
    if (i < n) {
      zA[0].push_back(H[i]);
      zA[1].push_back(Az[i][0]);
    } else {
      zA[0].push_back(Polynomial::generateRandomNumber(H, p - n));
      zA[1].push_back(Polynomial::generateRandomNumber(H, p - n));
    }
    // cout << "zA(" << zA[0][i] << ")= " << zA[1][i] << endl;
  }

  vector<uint64_t> z_hatA = Polynomial::setupNewtonPolynomial(zA[0], zA[1], p, "z_hatA(x)");


  vector<vector<uint64_t>> zB(2);
  // cout << "zB(x):" << endl;
  for (uint64_t i = 0; i < n + b; i++) {
  // for (uint64_t i = 0; i < n; i++) {
    if (i < n) {
      zB[0].push_back(H[i]);
      zB[1].push_back(Bz[i][0]);
    } else {
      zB[0].push_back(zA[0][i]);
      zB[1].push_back(Polynomial::generateRandomNumber(H, p - n));
    }
    // cout << "zB(" << zB[0][i] << ")= " << zB[1][i] << endl;
  }
  vector<uint64_t> z_hatB = Polynomial::setupNewtonPolynomial(zB[0], zB[1], p, "z_hatB(x)");

  vector<vector<uint64_t>> zC(2);
  // cout << "zC(x):";
  for (uint64_t i = 0; i < n + b; i++) {
  // for (uint64_t i = 0; i < n; i++) {
    if (i < n) {
      zC[0].push_back(H[i]);
      zC[1].push_back(Cz[i][0]);
    } else {
      zC[0].push_back(zA[0][i]);
      zC[1].push_back(Polynomial::generateRandomNumber(H, p - n));
    }
    // cout << "zC(" << zC[0][i] << ")= " << zC[1][i] << endl;
  }
  vector<uint64_t> z_hatC = Polynomial::setupNewtonPolynomial(zC[0], zC[1], p, "z_hatC(x)");


  vector<uint64_t> zero_to_t_for_H;
  vector<uint64_t> t_to_n_for_H;
  vector<uint64_t> zero_to_t_for_z;
  vector<uint64_t> t_to_n_for_z;
  for (uint64_t i = 0; i < t; i++) {
    zero_to_t_for_H.push_back(H[i]);
    zero_to_t_for_z.push_back(z[i]);
  }
  vector<uint64_t> polyX_HAT_H = Polynomial::setupNewtonPolynomial(zero_to_t_for_H, zero_to_t_for_z, p, "x_hat(h)");

  for (uint64_t i = 0; i < n - t; i++) {
    t_to_n_for_H.push_back(H[i + t]);
    t_to_n_for_z.push_back(z[i + t]);
  }
  // cout << "w_bar(h):" << endl;
  vector<uint64_t> w_bar(n - t + b);
  vector<uint64_t> w_bar_numerator(n - t, 1);
  vector<uint64_t> w_bar_denominator(n - t, 1);
  for (uint64_t i = 0; i < n - t; i++) {
    w_bar_numerator[i] = Polynomial::subtractModP(t_to_n_for_z[i], (Polynomial::evaluatePolynomial(polyX_HAT_H, t_to_n_for_H[i], p)), p);

    for (uint64_t j = 0; j < zero_to_t_for_H.size(); j++) {
      w_bar_denominator[i] *= Polynomial::subtractModP(t_to_n_for_H[i], zero_to_t_for_H[j], p);
      // Apply pulus to keep the number within the bounds
      w_bar_denominator[i] %= p;

      // // If result becomes negative, convert it to a positive equivalent under modulo
      // if (w_bar_denominator[i] < 0) {
      //   w_bar_denominator[i] += p;
      // }
    }
    w_bar_denominator[i] = Polynomial::pInverse(w_bar_denominator[i], p);
    w_bar[i] = (w_bar_numerator[i] * w_bar_denominator[i]) % p;

    // if (w_bar[i] < 0) {
    //   w_bar[i] += p;
    // }
    // cout << "w_bar(" << t_to_n_for_H[i] << ")= " << w_bar[i] << endl;
  }

  // cout << "w_hat(x):" << endl;
  vector<vector<uint64_t>> w_hat(2);
  for (uint64_t i = 0; i < n - t; i++) {
    w_hat[0].push_back(t_to_n_for_H[i]);
    w_hat[1].push_back(w_bar[i]);
    // cout << "w_hat(" << w_hat[0][i] << ")= " << w_hat[1][i] << endl;
  }
  for (uint64_t i = n; i < n + b; i++) {
    w_hat[0].push_back(zA[0][i]);
    w_hat[1].push_back(Polynomial::generateRandomNumber(H, p));
    // cout << "w_hat(" << w_hat[0][i - b] << ")= " << w_hat[1][i - b] << endl;
  }
  vector<uint64_t> w_hat_x = Polynomial::setupNewtonPolynomial(w_hat[0], w_hat[1], p, "w_hat(x)");

  vector<uint64_t> productAB = Polynomial::multiplyPolynomials(z_hatA, z_hatB, p);
  vector<uint64_t> zAzB_zC = Polynomial::subtractPolynomials(productAB, z_hatC, p);
  Polynomial::printPolynomial(zAzB_zC, "zA(x)zB(x)-zC(x)");


  vector<uint64_t> vH_x(n + 1, 0);
  vH_x[0] = p - 1;
  vH_x[n] = 1;
  Polynomial::printPolynomial(vH_x, "vH(x)");


  vector<uint64_t> vK_x = Polynomial::createLinearPolynomial(K[0]);
  // Multiply (x - K) for all other K
  for (size_t i = 1; i < K.size(); i++) {
    vector<uint64_t> nextPoly = Polynomial::createLinearPolynomial(K[i]);
    vK_x = Polynomial::multiplyPolynomials(vK_x, nextPoly, p);
  }
  Polynomial::printPolynomial(vK_x, "vK(x)");

  // Dividing the product of zAzB_zC by vH_x
  vector<uint64_t> h_0_x = Polynomial::dividePolynomials(zAzB_zC, vH_x, p)[0];
  Polynomial::printPolynomial(h_0_x, "h0(x)");

  vector<uint64_t> s_x = Polynomial::generateRandomPolynomial(n, (2*n)+b-1, p);
  // vector<uint64_t> s_x = { 115, 3, 0, 0, 20, 1, 0, 17, 101, 0, 5 };
  Polynomial::printPolynomial(s_x, "s(x)");

  uint64_t sigma1 = Polynomial::sumOfEvaluations(s_x, H, p);
  cout << "sigma1 = " << sigma1 << endl;

  uint64_t alpha = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 0, p), p);
  uint64_t etaA = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 1, p), p);
  uint64_t etaB = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 2, p), p);
  uint64_t etaC = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 3, p), p);

  uint64_t beta1 = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 8, p), p);
  uint64_t beta2 = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 9, p), p);


  cout << "alpha = " << alpha << endl;
  cout << "beta1 = " << beta1 << endl;
  cout << "beta2 = " << beta2 << endl;
  cout << "etaA = " << etaA << endl;
  cout << "etaB = " << etaB << endl;
  cout << "etaC = " << etaC << endl;


  vector<uint64_t> etaA_z_hatA_x = Polynomial::multiplyPolynomialByNumber(z_hatA, etaA, p);
  vector<uint64_t> etaB_z_hatB_x = Polynomial::multiplyPolynomialByNumber(z_hatB, etaB, p);
  vector<uint64_t> etaC_z_hatC_x = Polynomial::multiplyPolynomialByNumber(z_hatC, etaC, p);
  Polynomial::printPolynomial(etaA_z_hatA_x, "etaA_z_hatA(x)");
  Polynomial::printPolynomial(etaB_z_hatB_x, "etaB_z_hatB(x)");
  Polynomial::printPolynomial(etaC_z_hatC_x, "etaC_z_hatC(x)");

  vector<uint64_t> Sum_M_eta_M_z_hat_M_x = Polynomial::addPolynomials(Polynomial::addPolynomials(etaA_z_hatA_x, etaB_z_hatB_x, p), etaC_z_hatC_x, p);
  Polynomial::printPolynomial(Sum_M_eta_M_z_hat_M_x, "Sum_M_z_hatM(x)");

  vector<uint64_t> r_alpha_x = Polynomial::calculatePolynomial_r_alpha_x(alpha, n, p);
  Polynomial::printPolynomial(r_alpha_x, "r(alpha, x)");

  vector<uint64_t> r_Sum_x = Polynomial::multiplyPolynomials(r_alpha_x, Sum_M_eta_M_z_hat_M_x, p);
  Polynomial::printPolynomial(r_Sum_x, "r(alpha, x)Sum_M_z_hatM(x)");

  vector<uint64_t> v_H = Polynomial::expandPolynomials(zero_to_t_for_H, p);
  Polynomial::printPolynomial(v_H, "v_H");
  vector<uint64_t> z_hat_x = Polynomial::addPolynomials(Polynomial::multiplyPolynomials(w_hat_x, v_H, p), polyX_HAT_H, p);
  Polynomial::printPolynomial(z_hat_x, "z_hat(x)");

  vector<uint64_t> A_hat(2);
  for (uint64_t i = 0; i < nonZeroA.size(); i++) {
    vector<uint64_t> buff_n = Polynomial::calculatePolynomial_r_alpha_x(rowA[i], H.size(), p);
    int64_t eval = Polynomial::evaluatePolynomial(buff_n, rowA[i], p);
    vector<uint64_t> buff = Polynomial::calculatePolynomial_r_alpha_x(colA[i], H.size(), p);
    eval = (eval * valA[i]) % p;
    buff = Polynomial::multiplyPolynomialByNumber(buff, eval, p);
    buff = Polynomial::multiplyPolynomialByNumber(buff, Polynomial::calculatePolynomial_r_alpha_k(alpha, rowA[i], H.size(), p), p);
    if (i > 0) {
      A_hat = Polynomial::addPolynomials(A_hat, buff, p);
    } else {
      A_hat = buff;
    }
  }
  Polynomial::printPolynomial(A_hat, "A_hat(x)");

  vector<uint64_t> B_hat(2);
  for (uint64_t i = 0; i < nonZeroB.size(); i++) {
    vector<uint64_t> buff_n = Polynomial::calculatePolynomial_r_alpha_x(rowB[i], H.size(), p);
    int64_t eval = Polynomial::evaluatePolynomial(buff_n, rowB[i], p);
    vector<uint64_t> buff = Polynomial::calculatePolynomial_r_alpha_x(colB[i], H.size(), p);
    eval = (eval * valB[i]) % p;
    buff = Polynomial::multiplyPolynomialByNumber(buff, eval, p);
    buff = Polynomial::multiplyPolynomialByNumber(buff, Polynomial::calculatePolynomial_r_alpha_k(alpha, rowB[i], H.size(), p), p);
    if (i > 0) {
      B_hat = Polynomial::addPolynomials(B_hat, buff, p);
    } else {
      B_hat = buff;
    }
  }
  Polynomial::printPolynomial(B_hat, "B_hat(x)");
  
  vector<uint64_t> C_hat(2);
  for (uint64_t i = 0; i < n_g; i++) {
    vector<uint64_t> buff_n = Polynomial::calculatePolynomial_r_alpha_x(rowC[i], H.size(), p);
    int64_t eval = Polynomial::evaluatePolynomial(buff_n, rowC[i], p);
    vector<uint64_t> buff = Polynomial::calculatePolynomial_r_alpha_x(colC[i], H.size(), p);
    eval = (eval * valC[i]) % p;
    buff = Polynomial::multiplyPolynomialByNumber(buff, eval, p);
    buff = Polynomial::multiplyPolynomialByNumber(buff, Polynomial::calculatePolynomial_r_alpha_k(alpha, rowC[i], H.size(), p), p);
    if (i > 0) {
      C_hat = Polynomial::addPolynomials(C_hat, buff, p);
    } else {
      C_hat = buff;
    }
  }
  Polynomial::printPolynomial(C_hat, "C_hat(x)");

/*
// Check for overflow risk for etaA, etaB and etaC and ensure p is applied correctly
// vector<uint64_t> etaA_z_hatA_x = Polynomial::multiplyPolynomialByNumber(A_hatA, etaA % p, p);
// vector<uint64_t> etaB_z_hatB_x = Polynomial::multiplyPolynomialByNumber(B_hatB, etaB % p, p);
// vector<uint64_t> etaC_z_hatC_x = Polynomial::multiplyPolynomialByNumber(C_hatC, etaC % p, p);*/

  vector<uint64_t> eta_A_hat = Polynomial::multiplyPolynomialByNumber(A_hat, etaA, p);
  vector<uint64_t> eta_B_hat = Polynomial::multiplyPolynomialByNumber(B_hat, etaB, p);
  vector<uint64_t> eta_C_hat = Polynomial::multiplyPolynomialByNumber(C_hat, etaC, p);
  Polynomial::printPolynomial(eta_A_hat, "eta_A_hat: ");
  Polynomial::printPolynomial(eta_B_hat, "eta_B_hat: ");
  Polynomial::printPolynomial(eta_C_hat, "eta_C_hat: ");

  // Calculate the sum of the three polynomials and print the result
  vector<uint64_t> Sum_M_eta_M_r_M_alpha_x = Polynomial::addPolynomials(Polynomial::addPolynomials(eta_A_hat, eta_B_hat, p), eta_C_hat, p);
  Polynomial::printPolynomial(Sum_M_eta_M_r_M_alpha_x, "Sum_M_eta_M_r_M(alpha ,x)");

  // Multiply the sum by another polynomial z_hat_x and print the result
  vector<uint64_t> Sum_M_eta_M_r_M_alpha_x_z_hat_x = Polynomial::multiplyPolynomials(Sum_M_eta_M_r_M_alpha_x, z_hat_x, p);
  Polynomial::printPolynomial(Sum_M_eta_M_r_M_alpha_x_z_hat_x, "Sum_M_eta_M_r_M(alpha ,x)z-hat(x)");

  // Calculate the sum for the check protocol, subtracting the pified sum from s_x
  vector<uint64_t> Sum_check_protocol = Polynomial::addPolynomials(s_x, (Polynomial::subtractPolynomials(r_Sum_x, Sum_M_eta_M_r_M_alpha_x_z_hat_x, p)), p);
  Polynomial::printPolynomial(Sum_check_protocol, "Sum_check_protocol");

  // Divide the sum check protocol by vH_x to get two results: h1(x) and g1(x)
  vector<uint64_t> h_1_x = Polynomial::dividePolynomials(Sum_check_protocol, vH_x, p)[0];
  Polynomial::printPolynomial(h_1_x, "h1(x)");

  // Get the second part of the division result, g1(x), and erase the first element
  vector<uint64_t> g_1_x = Polynomial::dividePolynomials(Sum_check_protocol, vH_x, p)[1];
  g_1_x.erase(g_1_x.begin());
  Polynomial::printPolynomial(g_1_x, "g1(x)");

  // Calculate sigma2 using the evaluations of the polynomials A_hat, B_hat, and C_hat and print the result of sigma2
  uint64_t sigma2 = ((etaA * Polynomial::evaluatePolynomial(A_hat, beta1, p)) % p + (etaB * Polynomial::evaluatePolynomial(B_hat, beta1, p)) % p + (etaC * Polynomial::evaluatePolynomial(C_hat, beta1, p)) % p) % p;
  cout << "sigma2 = " << sigma2 << endl;

  // Initialize vectors for the pified polynomial results with zeros
  vector<uint64_t> A_hat_M_hat(H.size(), 0);
  vector<uint64_t> B_hat_M_hat(H.size(), 0);
  vector<uint64_t> C_hat_M_hat(H.size(), 0);

  // Loop through non-zero rows for matrix A and calculate the pified polynomial A_hat_M_hat
  for (uint64_t i = 0; i < nonZeroA.size(); i++) {
    vector<uint64_t> buff_nA = Polynomial::calculatePolynomial_r_alpha_x(colA[i], H.size(), p);
    int64_t evalA = Polynomial::evaluatePolynomial(buff_nA, beta1, p);
    vector<uint64_t> buffA = Polynomial::calculatePolynomial_r_alpha_x(rowA[i], H.size(), p);
    evalA = (evalA * valA[i]) % p;
    buffA = Polynomial::multiplyPolynomialByNumber(buffA, evalA, p);

    if (i > 0) {
      A_hat_M_hat = Polynomial::addPolynomials(A_hat_M_hat, buffA, p);
    } else {
      A_hat_M_hat = buffA;
    }
  }
  for (uint64_t i = 0; i < nonZeroB.size(); i++) {
    vector<uint64_t> buff_nB = Polynomial::calculatePolynomial_r_alpha_x(colB[i], H.size(), p);
    int64_t evalB = Polynomial::evaluatePolynomial(buff_nB, beta1, p);
    vector<uint64_t> buffB = Polynomial::calculatePolynomial_r_alpha_x(rowB[i], H.size(), p);
    evalB = (evalB * valB[i]) % p;
    buffB = Polynomial::multiplyPolynomialByNumber(buffB, evalB, p);

    if (i > 0) {
      B_hat_M_hat = Polynomial::addPolynomials(B_hat_M_hat, buffB, p);
    } else {
      B_hat_M_hat = buffB;
    }
  }
  for (uint64_t i = 0; i < n_g; i++) {
    vector<uint64_t> buff_nC = Polynomial::calculatePolynomial_r_alpha_x(colC[i], H.size(), p);
    int64_t evalC = Polynomial::evaluatePolynomial(buff_nC, beta1, p);
    vector<uint64_t> buffC = Polynomial::calculatePolynomial_r_alpha_x(rowC[i], H.size(), p);
    evalC = (evalC * valC[i]) % p;
    buffC = Polynomial::multiplyPolynomialByNumber(buffC, evalC, p);

    if (i > 0) {
      C_hat_M_hat = Polynomial::addPolynomials(C_hat_M_hat, buffC, p);
    } else {
      C_hat_M_hat = buffC;
    }
  }
  // Print the final pified polynomials for A, B, and C
  Polynomial::printPolynomial(A_hat_M_hat, "A_hat_M_hat");
  Polynomial::printPolynomial(B_hat_M_hat, "B_hat_M_hat");
  Polynomial::printPolynomial(C_hat_M_hat, "C_hat_M_hat");

  // Multiply the pified polynomials by their respective eta values and print
  vector<uint64_t> eta_A_hat_M_hat = Polynomial::multiplyPolynomialByNumber(A_hat_M_hat, etaA, p);
  vector<uint64_t> eta_B_hat_M_hat = Polynomial::multiplyPolynomialByNumber(B_hat_M_hat, etaB, p);
  vector<uint64_t> eta_C_hat_M_hat = Polynomial::multiplyPolynomialByNumber(C_hat_M_hat, etaC, p);
  Polynomial::printPolynomial(eta_A_hat_M_hat, "eta_A_hat_M_hat: ");
  Polynomial::printPolynomial(eta_B_hat_M_hat, "eta_B_hat_M_hat: ");
  Polynomial::printPolynomial(eta_C_hat_M_hat, "eta_C_hat_M_hat: ");

  // Calculate the final result for r_Sum_M_eta_M_M_hat_x_beta1
  vector<uint64_t> r_Sum_M_eta_M_M_hat_x_beta1 = Polynomial::multiplyPolynomials(Polynomial::addPolynomials(Polynomial::addPolynomials(eta_A_hat_M_hat, eta_B_hat_M_hat, p), eta_C_hat_M_hat, p), r_alpha_x, p);
  Polynomial::printPolynomial(r_Sum_M_eta_M_M_hat_x_beta1, "r_Sum_M_eta_M_M_hat_x_beta1");

  // Divide the final result by vH_x to get h2(x) and g2(x)
  vector<uint64_t> h_2_x = Polynomial::dividePolynomials(r_Sum_M_eta_M_M_hat_x_beta1, vH_x, p)[0];
  Polynomial::printPolynomial(h_2_x, "h2(x)");

  vector<uint64_t> g_2_x = Polynomial::dividePolynomials(r_Sum_M_eta_M_M_hat_x_beta1, vH_x, p)[1];
  g_2_x.erase(g_2_x.begin());//remove the first item
  Polynomial::printPolynomial(g_2_x, "g2(x)");

  // Generate Lagrange polynomials for row, column, and value vectors for matrices A and B
  // vector<uint64_t> rowA_x = Polynomial::setupNewtonPolynomial(rowA[0], rowA[1], p, "rowA(x)");
  // vector<uint64_t> colA_x = Polynomial::setupNewtonPolynomial(colA[0], colA[1], p, "colA(x)");
  // vector<uint64_t> valA_x = Polynomial::setupNewtonPolynomial(valA[0], valA[1], p, "valA(x)");

  // vector<uint64_t> rowB_x = Polynomial::setupNewtonPolynomial(rowB[0], rowB[1], p, "rowB(x)");
  // vector<uint64_t> colB_x = Polynomial::setupNewtonPolynomial(colB[0], colB[1], p, "colB(x)");
  // vector<uint64_t> valB_x = Polynomial::setupNewtonPolynomial(valB[0], valB[1], p, "valB(x)");

  // vector<uint64_t> rowC_x = Polynomial::setupNewtonPolynomial(rowC[0], rowC[1], p, "rowC(x)");
  // vector<uint64_t> colC_x = Polynomial::setupNewtonPolynomial(colC[0], colC[1], p, "colC(x)");
  // vector<uint64_t> valC_x = Polynomial::setupNewtonPolynomial(valC[0], valC[1], p, "valC(x)");

  // Evaluate polynomial vH at beta1 and beta2
  uint64_t vH_beta1 = Polynomial::evaluatePolynomial(vH_x, beta1, p);
  cout << "vH(beta1) = " << vH_beta1 << endl;

  uint64_t vH_beta2 = Polynomial::evaluatePolynomial(vH_x, beta2, p);
  cout << "vH(beta2) = " << vH_beta2 << endl;

  // Initialize vectors for function points and sigma value
  vector<uint64_t> points_f_3(K.size(), 0);
  uint64_t sigma3 = 0;

  // Loop over K to compute delta and signature values for A, B, and C
  for (uint64_t i = 0; i < K.size(); i++) {
    int64_t deA = (Polynomial::subtractModP(beta2, Polynomial::evaluatePolynomial(rowA_x, K[i], p), p) * Polynomial::subtractModP(beta1, Polynomial::evaluatePolynomial(colA_x, K[i], p), p)) % p;
    int64_t deB = (Polynomial::subtractModP(beta2, Polynomial::evaluatePolynomial(rowB_x, K[i], p), p) * Polynomial::subtractModP(beta1, Polynomial::evaluatePolynomial(colB_x, K[i], p), p)) % p;
    int64_t deC = (Polynomial::subtractModP(beta2, Polynomial::evaluatePolynomial(rowC_x, K[i], p), p) * Polynomial::subtractModP(beta1, Polynomial::evaluatePolynomial(colC_x, K[i], p), p)) % p;

    int64_t sig3_A = (etaA * (vH_beta2 * vH_beta1 % p) % p * Polynomial::evaluatePolynomial(valA_x, K[i], p) % p * Polynomial::pInverse(deA, p)) % p;
    int64_t sig3_B = (etaB * (vH_beta2 * vH_beta1 % p) % p * Polynomial::evaluatePolynomial(valB_x, K[i], p) % p * Polynomial::pInverse(deB, p)) % p;
    int64_t sig3_C = (etaC * (vH_beta2 * vH_beta1 % p) % p * Polynomial::evaluatePolynomial(valC_x, K[i], p) % p * Polynomial::pInverse(deC, p)) % p;

    points_f_3[i] = (sig3_A + sig3_B + sig3_C) % p;
    sigma3 += points_f_3[i];
    sigma3 %= p;
  }
  cout << "sigma3 = " << sigma3 << endl;

  // Create polynomials for beta1 and beta2
  vector<uint64_t> poly_beta1 = { beta1 };
  vector<uint64_t> poly_beta2 = { beta2 };

  // Compute polynomial products for sigma
  vector<uint64_t> poly_pi_a = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowA_x, poly_beta2, p), Polynomial::subtractPolynomials(colA_x, poly_beta1, p), p);
  vector<uint64_t> poly_pi_b = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowB_x, poly_beta2, p), Polynomial::subtractPolynomials(colB_x, poly_beta1, p), p);
  vector<uint64_t> poly_pi_c = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowC_x, poly_beta2, p), Polynomial::subtractPolynomials(colC_x, poly_beta1, p), p);
  Polynomial::printPolynomial(poly_pi_a, "poly_pi_a");
  Polynomial::printPolynomial(poly_pi_b, "poly_pi_b");
  Polynomial::printPolynomial(poly_pi_c, "poly_pi_c");

  // Compute polynomials for signature multipliers
  vector<uint64_t> poly_etaA_vH_B2_vH_B1 = { (etaA * ((vH_beta2 * vH_beta1) % p)) % p };
  vector<uint64_t> poly_etaB_vH_B2_vH_B1 = { (etaB * ((vH_beta2 * vH_beta1) % p)) % p };
  vector<uint64_t> poly_etaC_vH_B2_vH_B1 = { (etaC * ((vH_beta2 * vH_beta1) % p)) % p };

  // Calculate sigma
  vector<uint64_t> poly_sig_a = Polynomial::multiplyPolynomials(poly_etaA_vH_B2_vH_B1, valA_x, p);
  vector<uint64_t> poly_sig_b = Polynomial::multiplyPolynomials(poly_etaB_vH_B2_vH_B1, valB_x, p);
  vector<uint64_t> poly_sig_c = Polynomial::multiplyPolynomials(poly_etaC_vH_B2_vH_B1, valC_x, p);
  Polynomial::printPolynomial(poly_sig_a, "poly_sig_a");
  Polynomial::printPolynomial(poly_sig_b, "poly_sig_b");
  Polynomial::printPolynomial(poly_sig_c, "poly_sig_c");

  vector<uint64_t> a_x = Polynomial::addPolynomials(Polynomial::addPolynomials(Polynomial::multiplyPolynomials(poly_sig_a, Polynomial::multiplyPolynomials(poly_pi_b, poly_pi_c, p), p), Polynomial::multiplyPolynomials(poly_sig_b, Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_c, p), p), p), Polynomial::multiplyPolynomials(poly_sig_c, Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_b, p), p), p);
  Polynomial::printPolynomial(a_x, "a(x)");

  vector<uint64_t> b_x = Polynomial::multiplyPolynomials(Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_b, p), poly_pi_c, p);
  Polynomial::printPolynomial(b_x, "b(x)");

  // Set up polynomial for f_3 using K
  vector<uint64_t> poly_f_3x = Polynomial::setupNewtonPolynomial(K, points_f_3, p, "poly_f_3(x)");

  vector<uint64_t> g_3_x = poly_f_3x;
  g_3_x.erase(g_3_x.begin());
  Polynomial::printPolynomial(g_3_x, "g3(x)");

  // Calculate sigma_3_set_k based on sigma3 and K.size()
  vector<uint64_t> sigma_3_set_k;
  sigma_3_set_k.push_back((sigma3 * Polynomial::pInverse(K.size(), p)) % p);
  cout << "sigma_3_set_k = " << sigma_3_set_k[0] << endl;

  // Update polynomial f_3 by subtracting sigma_3_set_k
  vector<uint64_t> poly_f_3x_new = Polynomial::subtractPolynomials(poly_f_3x, sigma_3_set_k, p);
  Polynomial::printPolynomial(poly_f_3x_new, "f3(x)new");

  // Calculate polynomial h_3(x) using previous results
  vector<uint64_t> h_3_x = Polynomial::dividePolynomials(Polynomial::subtractPolynomials(a_x, Polynomial::multiplyPolynomials(b_x, Polynomial::addPolynomials(poly_f_3x_new, sigma_3_set_k, p), p), p), vK_x, p)[0];
  Polynomial::printPolynomial(h_3_x, "h3(x)");

  // Define random values based on s_x
  uint64_t eta_row_ahp_a = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 10, p), p);
  uint64_t eta_col_ahp_a = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 11, p), p);
  uint64_t eta_val_ahp_a = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 12, p), p);

  uint64_t eta_row_ahp_b = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 13, p), p);
  uint64_t eta_col_ahp_b = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 14, p), p);
  uint64_t eta_val_ahp_b = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 15, p), p);

  uint64_t eta_row_ahp_c = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 16, p), p);
  uint64_t eta_col_ahp_c = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 17, p), p);
  uint64_t eta_val_ahp_c = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 18, p), p);

  uint64_t eta_w_hat = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 19, p), p);
  uint64_t eta_z_hatA = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 20, p), p);
  uint64_t eta_z_hatB = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 21, p), p);
  uint64_t eta_z_hatC = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 22, p), p);
  uint64_t eta_h_0_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 23, p), p);
  uint64_t eta_s_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 24, p), p);
  uint64_t eta_g_1_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 25, p), p);
  uint64_t eta_h_1_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 26, p), p);
  uint64_t eta_g_2_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 27, p), p);
  uint64_t eta_h_2_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 28, p), p);
  uint64_t eta_g_3_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 29, p), p);
  uint64_t eta_h_3_x = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 30, p), p);
  
  // Initialize the polynomial p(x) by performing several polynomial operations and print
  vector<uint64_t> p_x =
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(rowA_x, eta_row_ahp_a, p), Polynomial::multiplyPolynomialByNumber(colA_x, eta_col_ahp_a, p), p),
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(valA_x, eta_val_ahp_a, p), Polynomial::multiplyPolynomialByNumber(rowB_x, eta_row_ahp_b, p), p), p),
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(colB_x, eta_col_ahp_b, p), Polynomial::multiplyPolynomialByNumber(valB_x, eta_val_ahp_b, p), p),
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(rowC_x, eta_row_ahp_c, p), Polynomial::multiplyPolynomialByNumber(colC_x, eta_col_ahp_c, p), p), p), p),
  Polynomial::multiplyPolynomialByNumber(valC_x, eta_val_ahp_c, p), p),
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(w_hat_x, eta_w_hat, p), Polynomial::multiplyPolynomialByNumber(z_hatA, eta_z_hatA, p), p),
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(z_hatB, eta_z_hatB, p), Polynomial::multiplyPolynomialByNumber(z_hatC, eta_z_hatC, p), p), p), p),
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(h_0_x, eta_h_0_x, p), Polynomial::multiplyPolynomialByNumber(s_x, eta_s_x, p), p),
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_1_x, eta_g_1_x, p), Polynomial::multiplyPolynomialByNumber(h_1_x, eta_h_1_x, p), p), p), p),
  Polynomial::addPolynomials(
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_2_x, eta_g_2_x, p), Polynomial::multiplyPolynomialByNumber(h_2_x, eta_h_2_x, p), p),
  Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_3_x, eta_g_3_x, p), Polynomial::multiplyPolynomialByNumber(h_3_x, eta_h_3_x, p), p), p), p);

  Polynomial::printPolynomial(p_x, "p(x)");
  
  uint64_t x_prime = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 22, p), p);
  uint64_t y_prime = Polynomial::evaluatePolynomial(p_x, x_prime, p);
  cout << "y_prime = " << y_prime << endl;

  // p(x) - y'  =>  p(x)  !!!!!!!
  vector<uint64_t> q_xBuf;
  q_xBuf.push_back(p - x_prime);
  q_xBuf.push_back(1);
  Polynomial::printPolynomial(q_xBuf, "div = ");

  vector<uint64_t> q_x = Polynomial::dividePolynomials(p_x, q_xBuf, p)[0];
  Polynomial::printPolynomial(q_x, "q(x)");

  // Generate a KZG commitment for q(x) using the provided verification key (ck)
  uint64_t p_17_AHP = Polynomial::KZG_Commitment(ck, q_x, p);
  cout << "p_17_AHP = " << p_17_AHP << endl;

  // Generate KZG commitments for various polynomials and print
  vector<uint64_t> Com1_AHP_x;
  for (int i = 1; i < 33; i++) {
    Com1_AHP_x.push_back(z[i]);
  }
  uint64_t Com2_AHP_x = Polynomial::KZG_Commitment(ck, w_hat_x, p);
  uint64_t Com3_AHP_x = Polynomial::KZG_Commitment(ck, z_hatA, p);
  uint64_t Com4_AHP_x = Polynomial::KZG_Commitment(ck, z_hatB, p);
  uint64_t Com5_AHP_x = Polynomial::KZG_Commitment(ck, z_hatC, p);
  uint64_t Com6_AHP_x = Polynomial::KZG_Commitment(ck, h_0_x, p);
  uint64_t Com7_AHP_x = Polynomial::KZG_Commitment(ck, s_x, p);
  uint64_t Com8_AHP_x = Polynomial::KZG_Commitment(ck, g_1_x, p);
  uint64_t Com9_AHP_x = Polynomial::KZG_Commitment(ck, h_1_x, p);
  uint64_t Com10_AHP_x = Polynomial::KZG_Commitment(ck, g_2_x, p);
  uint64_t Com11_AHP_x = Polynomial::KZG_Commitment(ck, h_2_x, p);
  uint64_t Com12_AHP_x = Polynomial::KZG_Commitment(ck, g_3_x, p);
  uint64_t Com13_AHP_x = Polynomial::KZG_Commitment(ck, h_3_x, p);

  // Measure the end time
  auto end_time = high_resolution_clock::now();


  cout << "Com2_AHP_x = " << Com2_AHP_x << endl;
  cout << "Com3_AHP_x = " << Com3_AHP_x << endl;
  cout << "Com4_AHP_x = " << Com4_AHP_x << endl;
  cout << "Com5_AHP_x = " << Com5_AHP_x << endl;
  cout << "Com6_AHP_x = " << Com6_AHP_x << endl;
  cout << "Com7_AHP_x = " << Com7_AHP_x << endl;
  cout << "Com8_AHP_x = " << Com8_AHP_x << endl;
  cout << "Com9_AHP_x = " << Com9_AHP_x << endl;
  cout << "Com10_AHP_x = " << Com10_AHP_x << endl;
  cout << "Com11_AHP_x = " << Com11_AHP_x << endl;
  cout << "Com12_AHP_x = " << Com12_AHP_x << endl;
  cout << "Com13_AHP_x = " << Com13_AHP_x << endl;

  // Generate a KZG commitment for the combined polynomial p(x)
  // int64_t ComP_AHP_x = Polynomial::KZG_Commitment(ck, p_x, p);
  // cout << "ComP_AHP = " << ComP_AHP_x << endl;


  ordered_json proof;
  proof.clear(); 
  proof["commitmentId"] = commitmentID;
  proof["class"] = Class;
  proof["P_AHP1"] = sigma1;
  proof["P_AHP2"] = w_hat_x;
  proof["P_AHP3"] = z_hatA;
  proof["P_AHP4"] = z_hatB;
  proof["P_AHP5"] = z_hatC;
  proof["P_AHP6"] = h_0_x;
  proof["P_AHP7"] = s_x;
  proof["P_AHP8"] = g_1_x;
  proof["P_AHP9"] = h_1_x;
  proof["P_AHP10"] = sigma2;
  proof["P_AHP11"] = g_2_x;
  proof["P_AHP12"] = h_2_x;
  proof["P_AHP13"] = sigma3;
  proof["P_AHP14"] = g_3_x;
  proof["P_AHP15"] = h_3_x;
  proof["P_AHP16"] = y_prime;
  proof["P_AHP17"] = p_17_AHP;
  proof["Com_AHP1_x"] = Com1_AHP_x;
  proof["Com_AHP2_x"] = Com2_AHP_x;
  proof["Com_AHP3_x"] = Com3_AHP_x;
  proof["Com_AHP4_x"] = Com4_AHP_x;
  proof["Com_AHP5_x"] = Com5_AHP_x;
  proof["Com_AHP6_x"] = Com6_AHP_x;
  proof["Com_AHP7_x"] = Com7_AHP_x;
  proof["Com_AHP8_x"] = Com8_AHP_x;
  proof["Com_AHP9_x"] = Com9_AHP_x;
  proof["Com_AHP10_x"] = Com10_AHP_x;
  proof["Com_AHP11_x"] = Com11_AHP_x;
  proof["Com_AHP12_x"] = Com12_AHP_x;
  proof["Com_AHP13_x"] = Com13_AHP_x;
  // proof["ComP_AHP_x"] = ComP_AHP_x;\

  cout << "\n\n\n\n" << proof << "\n\n\n\n";

  // Calculate the duration
  auto duration = duration_cast<milliseconds>(end_time - start_time);
  // Print the time taken
  cout << "Time taken: " << duration.count() << " milliseconds" << endl;

  std::string proofString = proof.dump(4);
  std::ofstream proofFile("data/proof.json");
  if (proofFile.is_open()) {
      proofFile << proofString;
      proofFile.close();
      std::cout << "JSON data has been written at data/proof.json\n";
      exit(0);
  } else {
      // std::cerr << "Error opening file for writing proof.json\n";
  }
}
