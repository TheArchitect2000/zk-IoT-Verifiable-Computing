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


#include "../lib/polynomial.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "../lib/json.hpp"
using ordered_json = nlohmann::ordered_json;
#include <regex>

using namespace std;

void verifier() {
  bool verify = false;

  /*******************************  Read Commitment  ******************************/
  cout << "openning program_commitment.json" << endl;
  std::ifstream commitmentFileStream("program_commitment.json");
  if (!commitmentFileStream.is_open()) {
      std::cerr << "Could not open the file!" << std::endl;
  }
  nlohmann::json commitmentJsonData;
  commitmentFileStream >> commitmentJsonData;
  commitmentFileStream.close();
  uint64_t Class = commitmentJsonData["class"].get<uint64_t>();
  
  vector<uint64_t> rowA_x = commitmentJsonData["row_AHP_A"].get<vector<uint64_t>>();
  vector<uint64_t> colA_x = commitmentJsonData["col_AHP_A"].get<vector<uint64_t>>();
  vector<uint64_t> valA_x = commitmentJsonData["val_AHP_A"].get<vector<uint64_t>>();
  vector<uint64_t> rowB_x = commitmentJsonData["row_AHP_B"].get<vector<uint64_t>>();
  vector<uint64_t> colB_x = commitmentJsonData["col_AHP_B"].get<vector<uint64_t>>();
  vector<uint64_t> valB_x = commitmentJsonData["val_AHP_B"].get<vector<uint64_t>>();
  vector<uint64_t> rowC_x = commitmentJsonData["row_AHP_C"].get<vector<uint64_t>>();
  vector<uint64_t> colC_x = commitmentJsonData["col_AHP_C"].get<vector<uint64_t>>();
  vector<uint64_t> valC_x = commitmentJsonData["val_AHP_C"].get<vector<uint64_t>>();
  
  uint64_t Com0_AHP = commitmentJsonData["Com_AHP0"].get<uint64_t>();
  uint64_t Com1_AHP = commitmentJsonData["Com_AHP1"].get<uint64_t>();
  uint64_t Com2_AHP = commitmentJsonData["Com_AHP2"].get<uint64_t>();
  uint64_t Com3_AHP = commitmentJsonData["Com_AHP3"].get<uint64_t>();
  uint64_t Com4_AHP = commitmentJsonData["Com_AHP4"].get<uint64_t>();
  uint64_t Com5_AHP = commitmentJsonData["Com_AHP5"].get<uint64_t>();
  uint64_t Com6_AHP = commitmentJsonData["Com_AHP6"].get<uint64_t>();
  uint64_t Com7_AHP = commitmentJsonData["Com_AHP7"].get<uint64_t>();
  uint64_t Com8_AHP = commitmentJsonData["Com_AHP8"].get<uint64_t>();
  /*******************************  Read Commitment  ******************************/


  /*********************************  Read Setup  *********************************/
  cout << "openning data/setup" << to_string(Class) << ".json" << endl;
  string setupFileName = "data/setup";
  setupFileName += to_string(Class);
  setupFileName += ".json";
  std::ifstream setupFileStream(setupFileName);
  if (!setupFileStream.is_open()) {
      std::cerr << "Could not open the file!" << std::endl;
  }
  nlohmann::json setupJsonData;
  setupFileStream >> setupJsonData;
  setupFileStream.close();
  vector<uint64_t> ck = setupJsonData["ck"].get<vector<uint64_t>>();
  uint64_t vk = setupJsonData["vk"].get<uint64_t>();
  /*********************************  Read Setup  *********************************/


  /*********************************  Read Proof  *********************************/
  cout << "openning data/proof.json" << endl;
  std::ifstream proofFileStream("data/proof.json");
  if (!proofFileStream.is_open()) {
      std::cerr << "Could not open the file!" << std::endl;
  }
  nlohmann::json proofJsonData;
  proofFileStream >> proofJsonData;
  proofFileStream.close();
  uint64_t sigma1 =           proofJsonData["P_AHP1"].get<uint64_t>();
  vector<uint64_t> w_hat_x =  proofJsonData["P_AHP2"].get<vector<uint64_t>>();
  vector<uint64_t> z_hatA =   proofJsonData["P_AHP3"].get<vector<uint64_t>>();
  vector<uint64_t> z_hatB =   proofJsonData["P_AHP4"].get<vector<uint64_t>>();
  vector<uint64_t> z_hatC =   proofJsonData["P_AHP5"].get<vector<uint64_t>>();
  vector<uint64_t> h_0_x =    proofJsonData["P_AHP6"].get<vector<uint64_t>>();
  vector<uint64_t> s_x =      proofJsonData["P_AHP7"].get<vector<uint64_t>>();
  vector<uint64_t> g_1_x =    proofJsonData["P_AHP8"].get<vector<uint64_t>>();
  vector<uint64_t> h_1_x =    proofJsonData["P_AHP9"].get<vector<uint64_t>>();
  uint64_t sigma2 =           proofJsonData["P_AHP10"].get<uint64_t>();
  vector<uint64_t> g_2_x =    proofJsonData["P_AHP11"].get<vector<uint64_t>>();
  vector<uint64_t> h_2_x =    proofJsonData["P_AHP12"].get<vector<uint64_t>>();
  uint64_t sigma3 =           proofJsonData["P_AHP13"].get<uint64_t>();
  vector<uint64_t> g_3_x =    proofJsonData["P_AHP14"].get<vector<uint64_t>>();
  vector<uint64_t> h_3_x =    proofJsonData["P_AHP15"].get<vector<uint64_t>>();
  uint64_t y_prime =          proofJsonData["P_AHP16"].get<uint64_t>();
  uint64_t p_17_AHP =         proofJsonData["P_AHP17"].get<uint64_t>();

  vector<uint64_t> Com1_AHP_x = proofJsonData["Com_AHP1_x"].get<vector<uint64_t>>();
  uint64_t Com2_AHP_x = proofJsonData["Com_AHP2_x"].get<uint64_t>();
  uint64_t Com3_AHP_x = proofJsonData["Com_AHP3_x"].get<uint64_t>();
  uint64_t Com4_AHP_x = proofJsonData["Com_AHP4_x"].get<uint64_t>();
  uint64_t Com5_AHP_x = proofJsonData["Com_AHP5_x"].get<uint64_t>();
  uint64_t Com6_AHP_x = proofJsonData["Com_AHP6_x"].get<uint64_t>();
  uint64_t Com7_AHP_x = proofJsonData["Com_AHP7_x"].get<uint64_t>();
  uint64_t Com8_AHP_x = proofJsonData["Com_AHP8_x"].get<uint64_t>();
  uint64_t Com9_AHP_x = proofJsonData["Com_AHP9_x"].get<uint64_t>();
  uint64_t Com10_AHP_x = proofJsonData["Com_AHP10_x"].get<uint64_t>();
  uint64_t Com11_AHP_x = proofJsonData["Com_AHP11_x"].get<uint64_t>();
  uint64_t Com12_AHP_x = proofJsonData["Com_AHP12_x"].get<uint64_t>();
  uint64_t Com13_AHP_x = proofJsonData["Com_AHP13_x"].get<uint64_t>();
  
  uint64_t input_value = proofJsonData["input"].get<uint64_t>();
  uint64_t output_value = proofJsonData["output"].get<uint64_t>();
  // uint64_t ComP_AHP_x = proofJsonData["ComP_AHP_x"].get<uint64_t>();
  // string curve = proofJsonData["curve"];
  // string protocol = proofJsonData["protocol"];
  /*********************************  Read Proof  *********************************/


  /*********************************  Read Class  *********************************/
  cout << "openning class.json" << endl;
  std::ifstream classFileStream("class.json");
  if (!classFileStream.is_open()) {
      std::cerr << "Could not open the file!" << std::endl;
  }
  nlohmann::json classJsonData;
  classFileStream >> classJsonData;
  classFileStream.close();
  uint64_t n_i, n_g, m, n, p, g;
  string class_value = to_string(Class); // Convert uinteger to string class
  n_g = classJsonData[class_value]["n_g"].get<uint64_t>();
  n_i = classJsonData[class_value]["n_i"].get<uint64_t>();
  n   = classJsonData[class_value]["n"].get<uint64_t>();
  m   = classJsonData[class_value]["m"].get<uint64_t>();
  p   = classJsonData[class_value]["p"].get<uint64_t>();
  g   = classJsonData[class_value]["g"].get<uint64_t>();
  /*********************************  Read Class  *********************************/


  cout << "n: " << n << endl;
  cout << "m: " << m << endl;
  cout << "g: " << g << endl;



  

  uint64_t x_prime = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 22, p), p);

  uint64_t alpha = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 0, p), p);
  uint64_t etaA = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 1, p), p);
  uint64_t etaB = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 2, p), p);
  uint64_t etaC = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 3, p), p);

  uint64_t beta1 = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 8, p), p);
  uint64_t beta2 = Polynomial::hashAndExtractLower4Bytes(Polynomial::evaluatePolynomial(s_x, 9, p), p);
  uint64_t beta3 = Polynomial::generateRandomNumber({0}, 1000);

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



  uint64_t y, g_m;
  vector<uint64_t> K;
  K.push_back(1);
  g_m = ((p - 1) * Polynomial::pInverse(m, p)) % p;
  y = Polynomial::power(g, g_m, p);
  for (int64_t i = 1; i < m; i++) {
    K.push_back(Polynomial::power(y, i, p));
  }

  vector<uint64_t> H;
  uint64_t w, g_n;
  H.push_back(1);
  g_n = ((p - 1) / n) % p;
  w = Polynomial::power(g, g_n, p);
  for (int64_t i = 1; i < n; i++) {
    H.push_back(Polynomial::power(w, i, p));
  }

  uint64_t y_output = Polynomial::evaluatePolynomial(z_hatC, H[n-1], p);
  // cout << "y = " << y_output << endl;


  vector<uint64_t> vH_x(n + 1, 0);
  vH_x[0] = p - 1;
  // if (vH_x[0] < 0) {
  //   vH_x[0] += p;
  // }
  vH_x[n] = 1;
  Polynomial::printPolynomial(vH_x, "vH(x)");

  vector<uint64_t> vK_x = Polynomial::createLinearPolynomial(K[0]);
  for (size_t i = 1; i < K.size(); i++) {
    vector<uint64_t> nextPoly = Polynomial::createLinearPolynomial(K[i]);
    vK_x = Polynomial::multiplyPolynomials(vK_x, nextPoly, p);
  }
  Polynomial::printPolynomial(vK_x, "vK(x)");

  uint64_t vH_beta1 = Polynomial::evaluatePolynomial(vH_x, beta1, p);
  cout << "vH(beta1) = " << vH_beta1 << endl;

  uint64_t vH_beta2 = Polynomial::evaluatePolynomial(vH_x, beta2, p);
  cout << "vH(beta2) = " << vH_beta2 << endl;

  vector<uint64_t> poly_beta1 = { beta1 };
  vector<uint64_t> poly_beta2 = { beta2 };

  vector<uint64_t> poly_pi_a = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowA_x, poly_beta2, p), Polynomial::subtractPolynomials(colA_x, poly_beta1, p), p);
  vector<uint64_t> poly_pi_b = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowB_x, poly_beta2, p), Polynomial::subtractPolynomials(colB_x, poly_beta1, p), p);
  vector<uint64_t> poly_pi_c = Polynomial::multiplyPolynomials(Polynomial::subtractPolynomials(rowC_x, poly_beta2, p), Polynomial::subtractPolynomials(colC_x, poly_beta1, p), p);
  Polynomial::printPolynomial(poly_pi_a, "poly_pi_a(x)");
  Polynomial::printPolynomial(poly_pi_b, "poly_pi_b(x)");
  Polynomial::printPolynomial(poly_pi_c, "poly_pi_c(x)");

  vector<uint64_t> poly_etaA_vH_B2_vH_B1 = { (etaA * ((vH_beta2 * vH_beta1) % p)) % p };
  vector<uint64_t> poly_etaB_vH_B2_vH_B1 = { (etaB * ((vH_beta2 * vH_beta1) % p)) % p };
  vector<uint64_t> poly_etaC_vH_B2_vH_B1 = { (etaC * ((vH_beta2 * vH_beta1) % p)) % p };

  vector<uint64_t> poly_sig_a = Polynomial::multiplyPolynomials(poly_etaA_vH_B2_vH_B1, valA_x, p);
  vector<uint64_t> poly_sig_b = Polynomial::multiplyPolynomials(poly_etaB_vH_B2_vH_B1, valB_x, p);
  vector<uint64_t> poly_sig_c = Polynomial::multiplyPolynomials(poly_etaC_vH_B2_vH_B1, valC_x, p);
  Polynomial::printPolynomial(poly_sig_a, "poly_sig_a(x)");
  Polynomial::printPolynomial(poly_sig_b, "poly_sig_b(x)");
  Polynomial::printPolynomial(poly_sig_c, "poly_sig_c(x)");

  vector<uint64_t> a_x = Polynomial::addPolynomials(Polynomial::addPolynomials(Polynomial::multiplyPolynomials(poly_sig_a, Polynomial::multiplyPolynomials(poly_pi_b, poly_pi_c, p), p), Polynomial::multiplyPolynomials(poly_sig_b, Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_c, p), p), p), Polynomial::multiplyPolynomials(poly_sig_c, Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_b, p), p), p);

  vector<uint64_t> b_x = Polynomial::multiplyPolynomials(Polynomial::multiplyPolynomials(poly_pi_a, poly_pi_b, p), poly_pi_c, p);
  vector<uint64_t> r_alpha_x = Polynomial::calculatePolynomial_r_alpha_x(alpha, n, p);
  vector<uint64_t> etaA_z_hatA_x = Polynomial::multiplyPolynomialByNumber(z_hatA, etaA, p);
  vector<uint64_t> etaB_z_hatB_x = Polynomial::multiplyPolynomialByNumber(z_hatB, etaB, p);
  vector<uint64_t> etaC_z_hatC_x = Polynomial::multiplyPolynomialByNumber(z_hatC, etaC, p);
  vector<uint64_t> Sum_M_eta_M_z_hat_M_x = Polynomial::addPolynomials(Polynomial::addPolynomials(etaA_z_hatA_x, etaB_z_hatB_x, p), etaC_z_hatC_x, p);

  uint64_t t = n_i + 1;
  vector<uint64_t> zero_to_t_for_z;
  zero_to_t_for_z.push_back(1);
  for (int i = 0; i < 32; i++) {
    zero_to_t_for_z.push_back(Com1_AHP_x[i]);
  }
  vector<uint64_t> zero_to_t_for_H;
  for (int64_t i = 0; i < t; i++) {
    zero_to_t_for_H.push_back(H[i]);
  }

  vector<uint64_t> polyX_HAT_H = Polynomial::setupNewtonPolynomial(zero_to_t_for_H, zero_to_t_for_z, p, "x_hat(h)");

  vector<uint64_t> r_Sum_x = Polynomial::multiplyPolynomials(r_alpha_x, Sum_M_eta_M_z_hat_M_x, p);
  vector<uint64_t> v_H = Polynomial::expandPolynomials(zero_to_t_for_H, p);
  vector<uint64_t> z_hat_x = Polynomial::addPolynomials(Polynomial::multiplyPolynomials(w_hat_x, v_H, p), polyX_HAT_H, p);

  vector<uint64_t> p_x =
    Polynomial::addPolynomials(
      Polynomial::addPolynomials(Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(w_hat_x, eta_w_hat, p), Polynomial::multiplyPolynomialByNumber(z_hatA, eta_z_hatA, p), p),
                                 Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(z_hatB, eta_z_hatB, p), Polynomial::multiplyPolynomialByNumber(z_hatC, eta_z_hatC, p), p), p),
      Polynomial::addPolynomials(
        Polynomial::addPolynomials(Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(h_0_x, eta_h_0_x, p), Polynomial::multiplyPolynomialByNumber(s_x, eta_s_x, p), p),
                                   Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_1_x, eta_g_1_x, p), Polynomial::multiplyPolynomialByNumber(h_1_x, eta_h_1_x, p), p), p),
        Polynomial::addPolynomials(Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_2_x, eta_g_2_x, p), Polynomial::multiplyPolynomialByNumber(h_2_x, eta_h_2_x, p), p),
                                   Polynomial::addPolynomials(Polynomial::multiplyPolynomialByNumber(g_3_x, eta_g_3_x, p), Polynomial::multiplyPolynomialByNumber(h_3_x, eta_h_3_x, p), p), p),
        p),
      p);
      
  uint64_t ComP_AHP_x = 
  ((Com0_AHP * eta_row_ahp_a) % p + 
  ((Com1_AHP * eta_col_ahp_a) % p + 
  ((Com2_AHP * eta_val_ahp_a) % p + 
  ((Com3_AHP * eta_row_ahp_b) % p + 
  ((Com4_AHP * eta_col_ahp_b) % p + 
  ((Com5_AHP * eta_val_ahp_b) % p + 
  ((Com6_AHP * eta_row_ahp_c) % p + 
  ((Com7_AHP * eta_col_ahp_c) % p + 
  ((Com8_AHP * eta_val_ahp_c) % p + 
  ((Com2_AHP_x * eta_w_hat) % p + 
  ((Com3_AHP_x * eta_z_hatA) % p + 
  ((Com4_AHP_x * eta_z_hatB) % p + 
  ((Com5_AHP_x * eta_z_hatC) % p + 
  ((Com6_AHP_x * eta_h_0_x) % p + 
  ((Com7_AHP_x * eta_s_x) % p + 
  ((Com8_AHP_x * eta_g_1_x) % p + 
  ((Com9_AHP_x * eta_h_1_x) % p + 
  ((Com10_AHP_x * eta_g_2_x) % p + 
  ((Com11_AHP_x * eta_h_2_x) % p + 
  ((Com12_AHP_x * eta_g_3_x) % p + 
  (Com13_AHP_x * eta_h_3_x) % p) % p) %p) % p) % p) % p) % p) % p) % p) % p) % p) %p) % p) % p) % p) % p) % p) % p) % p) % p) % p;
  cout << "ComP_AHP_x = " << ComP_AHP_x << endl;
  

  Polynomial::printPolynomial(a_x, "a_x");
  Polynomial::printPolynomial(b_x, "b_x");
  Polynomial::printPolynomial(g_3_x, "g_3_x");
  cout << "beta3 = " << beta3 << endl;
  cout << "sigma3 = " << sigma3 << endl;

  cout << "\n\n\n";
  uint64_t eq11 = (Polynomial::evaluatePolynomial(h_3_x, beta3, p) * Polynomial::evaluatePolynomial(vK_x, beta3, p)) % p;
  uint64_t eq12 = Polynomial::subtractModP(Polynomial::evaluatePolynomial(a_x, beta3, p), ((Polynomial::evaluatePolynomial(b_x, beta3, p) * (beta3 * Polynomial::evaluatePolynomial(g_3_x, beta3, p) + (sigma3 * Polynomial::pInverse(m, p)) % p))), p);
  cout << eq11 << " = " << eq12 << endl;

  uint64_t eq21 = (Polynomial::evaluatePolynomial(r_alpha_x, beta2, p) * sigma3) % p;
  uint64_t eq22 = ((Polynomial::evaluatePolynomial(h_2_x, beta2, p) * Polynomial::evaluatePolynomial(vH_x, beta2, p)) % p + (beta2 * Polynomial::evaluatePolynomial(g_2_x, beta2, p)) % p + (sigma2 * Polynomial::pInverse(n, p)) % p) % p;
  cout << eq21 << " = " << eq22 << endl;

  uint64_t eq31 = Polynomial::subtractModP(Polynomial::evaluatePolynomial(s_x, beta1, p) + Polynomial::evaluatePolynomial(r_alpha_x, beta1, p) * Polynomial::evaluatePolynomial(Sum_M_eta_M_z_hat_M_x, beta1, p), (sigma2 * Polynomial::evaluatePolynomial(z_hat_x, beta1, p)), p);
  uint64_t eq32 = (Polynomial::evaluatePolynomial(h_1_x, beta1, p) * Polynomial::evaluatePolynomial(vH_x, beta1, p) + beta1 * Polynomial::evaluatePolynomial(g_1_x, beta1, p) + sigma1 * Polynomial::pInverse(n, p)) % p;
  cout << eq31 << " = " << eq32 << endl;

  uint64_t eq41 = Polynomial::subtractModP(((Polynomial::evaluatePolynomial(z_hatA, beta1, p) * Polynomial::evaluatePolynomial(z_hatB, beta1, p)) % p), Polynomial::evaluatePolynomial(z_hatC, beta1, p), p);
  uint64_t eq42 = (Polynomial::evaluatePolynomial(h_0_x, beta1, p) * Polynomial::evaluatePolynomial(vH_x, beta1, p)) % p;
  cout << eq41 << " = " << eq42 << endl;


  uint64_t eq51Buf = Polynomial::subtractModP(ComP_AHP_x, (g * y_prime), p);
  uint64_t eq51 = Polynomial::e_func(eq51Buf, g, g, p);

  uint64_t eq52BufP2 = Polynomial::subtractModP(vk, (g * x_prime), p);
  
  uint64_t eq52 = Polynomial::e_func(p_17_AHP, eq52BufP2, g, p);
  cout << eq51 << " = " << eq52 << endl;

  if (eq11 == eq12 && eq21 == eq22 && eq31 == eq32 && eq41 == eq42 && eq51 == eq52 && output_value == y_output) {
    verify = true;
  }

  cout << endl;
  if (verify) {
    cout << "verify!!!!!!!!!!" << endl;
  }
}


int main() {
  verifier();
  return 0;
}
