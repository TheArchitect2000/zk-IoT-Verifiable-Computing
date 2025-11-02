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


#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <string>

using namespace std;

class Polynomial {
public:
  // Function to compute the p exponentiation (base^exponent) % p
  static uint64_t power(uint64_t base, uint64_t exponent, uint64_t p);

  // Function to compute the p exponentiation (a^b) % p
  static uint64_t pExp(uint64_t a, uint64_t b, uint64_t p);

  // Function to compute the p inverse using Fermat's Little Theorem
  static uint64_t pInverse(uint64_t a, uint64_t p);

  // Function to generate a random number in p
  static uint64_t generateRandomNumber(const vector<uint64_t>& H, uint64_t p);

  // Function to generate a random polynomial
  static vector<uint64_t> generateRandomPolynomial(size_t numTerms, size_t maxDegree, uint64_t p);

  // Add two polynomials with p arithmetic
  static vector<uint64_t> addPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p);

  // Subtract two polynomials with p arithmetic
  static vector<uint64_t> subtractPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p);

  // Function to multiply two polynomials
  static vector<uint64_t> multiplyPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p);

  // Function to divide two polynomials
  static vector<vector<uint64_t>> dividePolynomials(const vector<uint64_t>& dividend, const vector<uint64_t>& divisor, uint64_t p);

  // Function to multiply a polynomial by a number
  static vector<uint64_t> multiplyPolynomialByNumber(const vector<uint64_t>& H, uint64_t h, uint64_t p);

  // Function to subtract two number in p
  static uint64_t subtractModP(uint64_t a, uint64_t b, uint64_t p);

  // Function to compute Lagrange basis polynomial L_i(x)
  static vector<uint64_t> LagrangePolynomial(uint64_t i, const vector<uint64_t>& x_values, uint64_t p);

  // Function to compute Lagrange polynomial(x, y)
  static vector<uint64_t> setupNewtonPolynomial(const vector<uint64_t>& x_values, const vector<uint64_t>& y_values, uint64_t p, const std::string& name);

  // Function to parse the polynomial string and evaluate it
  static uint64_t evaluatePolynomial(const vector<uint64_t>& polynomial, uint64_t x, uint64_t p);

  // Function to compute the sum of polynomial evaluations at multiple points
  static uint64_t sumOfEvaluations(const vector<uint64_t>& poly, const vector<uint64_t>& points, uint64_t p);

  // Function to create a polynomial for (x - root)
  static vector<uint64_t> createLinearPolynomial(uint64_t root);

  // Function to calculate Polynomial r(α,x) = (alpha^n - x^n) / (alpha - x)
  static vector<uint64_t> calculatePolynomial_r_alpha_x(uint64_t alpha, uint64_t n, uint64_t p);

  // Function to calculate Polynomial r(α,x) = (alpha^n - x^n) / (alpha - x)
  static uint64_t calculatePolynomial_r_alpha_k(uint64_t alpha, uint64_t k, uint64_t n, uint64_t p);

  // Function to expand polynomials given the roots
  static vector<uint64_t> expandPolynomials(const vector<uint64_t>& roots, uint64_t p);

  // Function to print polynomial in serial
  static void printPolynomial(const vector<uint64_t>& coefficients, const std::string& name);

  // Utility functions for trimming
  static std::string trim(const std::string& str);

  // Utility functions for removing commas
  static std::string removeCommas(const std::string& str);

  // Utility functions to print a Matrix
  static void printMatrix(vector<vector<uint64_t>>& matrix, const std::string& name);

  // Function to get the row indices of non-zero entries in matrix
  static vector<vector<uint64_t>> getNonZeroRows(const vector<vector<uint64_t>>& matrix);

  // Function to get the col indices of non-zero entries in matrix
  static vector<vector<uint64_t>> getNonZeroCols(const vector<vector<uint64_t>>& matrix);

  // Function to create the mapping
  static vector<vector<uint64_t>> createMapping(const vector<uint64_t>& K, const vector<uint64_t>& H, const vector<vector<uint64_t>>& nonZero);

  // Function to print the mapping
  static void printMapping(vector<vector<uint64_t> >& row, const std::string& name);

  // Function to create the val mapping
  static vector<vector<uint64_t>> valMapping(const vector<uint64_t>& K, const vector<uint64_t>& H, vector<vector<uint64_t>>& nonZeroRows, vector<vector<uint64_t>>& nonZeroCols, uint64_t p);

  // Function to calculate log in p
  static uint64_t log_p(uint64_t a, uint64_t b, uint64_t p);

  // Function to calculate e_func in p
  static uint64_t e_func(uint64_t a, uint64_t b, uint64_t g, uint64_t p);

  // Function to calculate KZG in p
  static uint64_t KZG_Commitment(vector<uint64_t> a, vector<uint64_t> b, uint64_t p);

  // Function to compute the SHA-256 hash of an uint64_t and return the lower 4 bytes as uint64_t, applying a modulo operation
  static uint64_t hashAndExtractLower4Bytes(uint64_t inputNumber, uint64_t p);

  // Function to compute the SHA-256 hash of an uint64_t and return the lower 4 bytes as uint64_t, applying a modulo operation
  static string SHA256(char* data);

  // Function to divided difference for polynomial using newton
  static vector<uint64_t> newtonDividedDifferences(const vector<uint64_t>& x_values, const vector<uint64_t>& y_values, uint64_t p);

  // Function to calculate polynomial using newton
  static vector<uint64_t> newtonPolynomial(const vector<uint64_t>& coefficients, const vector<uint64_t>& x_values, uint64_t p);
};

#endif  // POLYNOMIAL_H
