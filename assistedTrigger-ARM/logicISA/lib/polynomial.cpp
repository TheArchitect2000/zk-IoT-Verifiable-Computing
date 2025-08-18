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


#include "polynomial.h"
#include <iostream>
#include <unordered_map>
#include <random>
// #include <openssl/evp.h>
#include <iomanip>


#include <cstdint>
#include <cstring>
#include <vector>
#include <cinttypes>
#include <set>

#include <complex>
#include <cmath>
#include <algorithm>

uint64_t Polynomial::power(uint64_t base, uint64_t exponent, uint64_t p) {
  uint64_t result = 1;
  base = base % p;  // Handle base larger than p

  while (exponent > 0) {
    // If exponent is odd, multiply the result by base
    if (exponent % 2 == 1) {
      result = (result * base) % p;
    }
    // Square the base and reduce exponent by half
    base = (base * base) % p;
    exponent /= 2;
  }

  return result;
}


// Function to compute the p exponentiation (a^b) % p
uint64_t Polynomial::pExp(uint64_t a, uint64_t b, uint64_t p) {
  uint64_t result = 1;
  a = a % p;
  while (b > 0) {
    if (b % 2 == 1) {  // If b is odd, multiply a with the result
      result = (result * a) % p;
    }
    b = b >> 1;         // Divide b by 2
    a = (a * a) % p;  // Change a to a^2
  }
  return result;
}

// Function to compute the p inverse using Fermat's Little Theorem
uint64_t Polynomial::pInverse(uint64_t a, uint64_t p) {
  return pExp(a, p - 2, p);
}

uint64_t Polynomial::generateRandomNumber(const std::vector<uint64_t>& H, uint64_t mod) {
    std::mt19937_64 rng(std::random_device{}());  // Use random_device to seed the generator
    std::uniform_int_distribution<uint64_t> dist(0, mod - 1);
    
    uint64_t randomNumber;
    do {
        randomNumber = dist(rng);
    } while (std::find(H.begin(), H.end(), randomNumber) != H.end());
    return randomNumber;
}

// Function to generate a random polynomial
vector<uint64_t> Polynomial::generateRandomPolynomial(size_t numTerms, size_t maxDegree, uint64_t p) {
    vector<uint64_t> polynomial(maxDegree + 1, 0); // Initialize polynomial with zeros

    // Generate random indices for the non-zero terms
    set<size_t> indices;
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<size_t> dis(0, maxDegree);

    while (indices.size() < numTerms) {
        indices.insert(dis(gen));
    }

    // Fill the polynomial with random values at the chosen indices
    for (size_t index : indices) {
        polynomial[index] = dis(gen) % p;
    }

    return polynomial;
}

// Add two polynomials with p arithmetic
vector<uint64_t> Polynomial::addPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p) {
  // Determine the size of the result polynomial (the max size of the two input polynomials)
  size_t maxSize = max(poly1.size(), poly2.size());
  vector<uint64_t> result(maxSize, 0);

  // Resize the smaller polynomial to match the size of the larger one
  vector<uint64_t> poly1_resized = poly1;
  vector<uint64_t> poly2_resized = poly2;
  poly1_resized.resize(maxSize, 0);
  poly2_resized.resize(maxSize, 0);

  // Add the polynomials
  for (size_t i = 0; i < maxSize; ++i) {
    result[i] = (poly1_resized[i] + poly2_resized[i]) % p;
  }

  return result;
}

// Subtract two polynomials with p arithmetic
vector<uint64_t> Polynomial::subtractPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p) {
  // Determine the size of the result polynomial (the max size of the two input polynomials)
  size_t maxSize = max(poly1.size(), poly2.size());
  vector<uint64_t> result(maxSize, 0);

  // Resize the smaller polynomial to match the size of the larger one
  vector<uint64_t> poly1_resized = poly1;
  vector<uint64_t> poly2_resized = poly2;
  poly1_resized.resize(maxSize, 0);
  poly2_resized.resize(maxSize, 0);

  // Subtract the polynomials
  for (size_t i = 0; i < maxSize; ++i) {
    result[i] = (poly1_resized[i] + p - poly2_resized[i]) % p;
  }

  return result;
}


// Perform NTT or inverse NTT
void NTT(vector<uint64_t>& a, bool invert, uint64_t p, uint64_t root) {
    int n = a.size();
    uint64_t root_inv = Polynomial::pExp(root, p - 2, p);  // Inverse of root mod p
    uint64_t root_pw = Polynomial::pExp(root, (p - 1) / n, p);  // root^(n/pw) mod p
    if (invert) root_pw = Polynomial::pExp(root_pw, p - 2, p);

    // Bit-reversal permutation
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        if (i < j) swap(a[i], a[j]);
    }

    // NTT computation
    for (int len = 2; len <= n; len <<= 1) {
        uint64_t wlen = Polynomial::pExp(root_pw, n / len, p);
        for (int i = 0; i < n; i += len) {
            uint64_t w = 1;
            for (int j = 0; j < len / 2; j++) {
                uint64_t u = a[i + j];
                uint64_t v = (a[i + j + len / 2] * w) % p;
                a[i + j] = (u + v) % p;
                a[i + j + len / 2] = (u - v + p) % p;
                w = (w * wlen) % p;
            }
        }
    }

    // Scale for inverse NTT
    if (invert) {
        uint64_t inv_n = Polynomial::pExp(n, p - 2, p);
        for (uint64_t& x : a) x = (x * inv_n) % p;
    }
}


// Function to multiply two polynomials
// vector<uint64_t> Polynomial::multiplyPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p) {
//   size_t n = 1;
//   while (n < poly1.size() + poly2.size() - 1) n <<= 1;

//   vector<uint64_t> a(poly1.begin(), poly1.end());
//   vector<uint64_t> b(poly2.begin(), poly2.end());
//   a.resize(n);
//   b.resize(n);

//   // Define a primitive root for the NTT
//   uint64_t root = 3; // This should be a primitive root modulo p

//   // Perform NTT on both polynomials
//   NTT(a, false, p, root);
//   NTT(b, false, p, root);

//   // Point-wise multiplication
//   for (size_t i = 0; i < n; ++i) {
//     a[i] = (a[i] * b[i]) % p;
//   }

//   // Perform inverse NTT
//   NTT(a, true, p, root);

//   // Resize the result to the correct size
//   a.resize(poly1.size() + poly2.size() - 1);
//   return a;
// }
vector<uint64_t> Polynomial::multiplyPolynomials(const vector<uint64_t>& poly1, const vector<uint64_t>& poly2, uint64_t p) {
  vector<uint64_t> result(poly1.size() + poly2.size() - 1, 0);

  for (size_t i = 0; i < poly1.size(); i++) {
    for (size_t j = 0; j < poly2.size(); j++) {
      result[i + j] = (result[i + j] + poly1[i] * poly2[j]) % p;
    }
  }
  return result;
}


// Function to divide two polynomials
vector<vector<uint64_t>> Polynomial::dividePolynomials(const vector<uint64_t>& dividend, const vector<uint64_t>& divisor, uint64_t p) {
  vector<uint64_t> quotient(dividend.size(), 0);  // Initialize with size equal to dividend size
  vector<uint64_t> remainder = dividend;
  vector<vector<uint64_t>> result;

  uint64_t n = remainder.size();
  uint64_t m = divisor.size();

  // If the divisor is larger than the dividend, the quotient is zero
  if (m > n) {
    quotient.resize(1, 0);  // Just one term, 0
    result.push_back(quotient);
    result.push_back(remainder);
    return result;
  }

  // Normalize the divisor
  uint64_t inv_lead = Polynomial::pExp(divisor.back(), p - 2, p);
  vector<uint64_t> normalized_divisor(m);
  for (size_t i = 0; i < m; i++) {
    normalized_divisor[i] = (divisor[i] * inv_lead) % p;
  }

  // Perform the division
  for (int i = n - m; i >= 0; i--) {
    quotient[i] = (remainder[i + m - 1] * inv_lead) % p;
    for (size_t j = 0; j < m; j++) {
      remainder[i + j] = (remainder[i + j] + p - (quotient[i] * normalized_divisor[j]) % p) % p;
    }
  }

  // Remove leading zeros from the remainder
  while (!remainder.empty() && remainder.back() == 0) {
    remainder.pop_back();
  }

  result.push_back(quotient);
  result.push_back(remainder);
  return result;
}

// Function to multiply a polynomial by a number
vector<uint64_t> Polynomial::multiplyPolynomialByNumber(const vector<uint64_t>& H, uint64_t h, uint64_t p) {
  vector<uint64_t> result(H.size(), 0);  // Use long long to avoid overflow during multiplication

  for (uint64_t i = 0; i < H.size(); i++) {
    result[i] = (H[i] * h) % p;
  }
  return result;
}


uint64_t Polynomial::subtractModP(uint64_t a, uint64_t b, uint64_t p) {
    return (a >= b) ? (a - b) % p : (p - (b - a) % p) % p;
}


vector<uint64_t> Polynomial::newtonDividedDifferences(const vector<uint64_t>& x_values, const vector<uint64_t>& y_values, uint64_t p) {
    uint64_t n = x_values.size();
    vector<vector<uint64_t>> divided_diff(n, vector<uint64_t>(n, 0));

    // Fill the first column with y_values
    for (uint64_t i = 0; i < n; i++) {
        divided_diff[i][0] = y_values[i] % p;
    }

    // Compute the divided differences
    for (uint64_t j = 1; j < n; j++) {
        for (uint64_t i = 0; i < n - j; i++) {
            uint64_t numerator = (divided_diff[i + 1][j - 1] - divided_diff[i][j - 1] + p) % p;
            uint64_t denominator = (x_values[i + j] - x_values[i] + p) % p;
            divided_diff[i][j] = (numerator * pInverse(denominator, p)) % p;
        }
    }

    // Extract the coefficients for the Newton polynomial
    vector<uint64_t> coefficients(n, 0);
    for (uint64_t i = 0; i < n; i++) {
        coefficients[i] = divided_diff[0][i];
    }
    return coefficients;
}

vector<uint64_t> Polynomial::newtonPolynomial(const vector<uint64_t>& coefficients, const vector<uint64_t>& x_values, uint64_t p) {
    vector<uint64_t> result = {coefficients[0]}; // Start with the first term
    vector<uint64_t> current_term = {1};        // Tracks the product (x - x_0)(x - x_1)...
    
    for (size_t i = 1; i < coefficients.size(); i++) {
        vector<uint64_t> term = {(p - x_values[i - 1]) % p, 1}; // (x - x_i)
        current_term = multiplyPolynomials(current_term, term, p);
        
        for (size_t j = 0; j < current_term.size(); j++) {
            if (j >= result.size()) {
                result.push_back(0);
            }
            result[j] = (result[j] + current_term[j] * coefficients[i]) % p;
        }
    }
    return result;
}

vector<uint64_t> Polynomial::setupNewtonPolynomial(const vector<uint64_t>& x_values, const vector<uint64_t>& y_values, uint64_t p, const std::string& name) {
    // Compute coefficients using divided differences
    vector<uint64_t> coefficients = newtonDividedDifferences(x_values, y_values, p);

    // Construct the polynomial from the coefficients
    vector<uint64_t> polynomial = newtonPolynomial(coefficients, x_values, p);

    // Print and return the polynomial
    printPolynomial(polynomial, name);
    return polynomial;
}


// Function to parse the polynomial string and evaluate it
uint64_t Polynomial::evaluatePolynomial(const vector<uint64_t>& polynomial, uint64_t x, uint64_t p) {
  uint64_t result = 0;
  uint64_t power_of_x = 1; // x^0 initially

  for (size_t i = 0; i < polynomial.size(); i++) {
    result = (result + polynomial[i] * power_of_x) % p;
    power_of_x = (power_of_x * x) % p;
  }

  return result;
}

// Function to compute the sum of polynomial evaluations at multiple points
uint64_t Polynomial::sumOfEvaluations(const vector<uint64_t>& poly, const vector<uint64_t>& points, uint64_t p) {
  uint64_t totalSum = 0;

  for (uint64_t point : points) {
    uint64_t evlpol = evaluatePolynomial(poly, point, p);
    totalSum = (totalSum + evlpol) % p;
  }
  totalSum %= p;
  return totalSum;
}

// Function to create a polynomial for (x - root)
vector<uint64_t> Polynomial::createLinearPolynomial(uint64_t root) {
  return { root, 1 };  // Represents (x - root)
}

// Function to calculate Polynomial r(α,x) = (alpha^n - x^n) / (alpha - x)
vector<uint64_t> Polynomial::calculatePolynomial_r_alpha_x(uint64_t alpha, uint64_t n, uint64_t p) {
  vector<uint64_t> P(n, 0);

  // Calculate each term of the polynomial P(x)
  uint64_t currentPowerOfAlpha = 1;  // alpha^0
  for (uint64_t i = 0; i < n; i++) {
    P[n - 1 - i] = currentPowerOfAlpha;  // alpha^(n-1-i)
    currentPowerOfAlpha = (currentPowerOfAlpha * alpha) % p;
  }

  return P;
}

// This function calculates the value of the polynomial at a specific point 𝑘 rather than returning the coefficients.
uint64_t Polynomial::calculatePolynomial_r_alpha_k(uint64_t alpha, uint64_t k, uint64_t n, uint64_t p) {
  uint64_t result = 1;
  result = subtractModP(power(alpha, n, p), power(k, n, p), p);
  uint64_t buff = subtractModP(alpha, k, p);

  result *= pInverse(buff, p);
  result %= p;
  return result;
}

// Function to expand polynomials given the roots
vector<uint64_t> Polynomial::expandPolynomials(const vector<uint64_t>& roots, uint64_t p) {
  vector<uint64_t> result = { 1 };  // Start with the polynomial "1"

  for (uint64_t root : roots) {
    // Multiply the current result polynomial by (x - root)
    vector<uint64_t> temp(result.size() + 1, 0);
    for (size_t i = 0; i < result.size(); i++) {
      temp[i] += result[i];  // x^n term
      temp[i] %= p;
      temp[i + 1] = subtractModP(temp[i + 1], (result[i] * root), p);
      // temp[i + 1] -= result[i] * root;  // -root * x^(n-1) term
      // temp[i + 1] %= p;
      // if (temp[i + 1] < 0) temp[i + 1] += p;
    }
    result = temp;
  }
  reverse(result.begin(), result.end());
  return result;
}

// Function to print polynomial in serial
void Polynomial::printPolynomial(const vector<uint64_t>& coefficients, const std::string& name) {
  // Iterate through the coefficients and print each term of the polynomial
  vector<int64_t> coefficientsBuf;
  for (size_t i = 0; i < coefficients.size(); i++)
  {
    coefficientsBuf.push_back(coefficients[i]);
  }
  
  cout << name  << " = ";
  bool first = true;
  for (int64_t i = coefficientsBuf.size() - 1; i >= 0; i--) {
    if (coefficientsBuf[i] == 0) continue;  // Skip zero coefficients

    // Print the sign for all terms except the first
    if (!first) {
      if (coefficientsBuf[i] > 0) {
        cout << " + ";
      } else {
        cout << " - ";
      }
    } else {
      first = false;
    }

    // Print the absolute value of the coefficient
    cout << abs(coefficientsBuf[i]);

    // Print the variable and the exponent
    cout << "x^" << i;
  }
  cout << endl;
}

// Utility functions for trimming and removing commas
std::string Polynomial::trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
}

// Helper function to Remove commas from a string
std::string Polynomial::removeCommas(const std::string& str) {
    size_t first = str.find_first_not_of(',');
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(',');
    return str.substr(first, last - first + 1);
}

void Polynomial::printMatrix(vector<vector<uint64_t>>& matrix, const std::string& name) {
  cout << "Matrix " << name << ":" << endl;
  for (const auto& row : matrix) {
    for (uint64_t val : row) {
      cout << val << " ";
    }
    cout << endl;
  }
}


// Function to get the row indices of non-zero entries in matrix
vector<vector<uint64_t>> Polynomial::getNonZeroRows(const vector<vector<uint64_t>>& matrix) {
  vector<vector<uint64_t>> nonZeroRows(2);
  for (uint64_t i = 0; i < matrix.size(); i++) {
    for (uint64_t j = 0; j < matrix[i].size(); j++) {
      if (matrix[i][j] != 0) {
        nonZeroRows[0].push_back(i);  // Storing row index
        nonZeroRows[1].push_back(matrix[i][j]);
      }
    }
  }
  return nonZeroRows;
}

// Function to get the col indices of non-zero entries in matrix
vector<vector<uint64_t>> Polynomial::getNonZeroCols(const vector<vector<uint64_t>>& matrix) {
  vector<vector<uint64_t>> nonZeroCols(2);
  for (uint64_t i = 0; i < matrix.size(); i++) {
    for (uint64_t j = 0; j < matrix[i].size(); j++) {
      if (matrix[i][j] != 0) {
        nonZeroCols[0].push_back(j);  // Storing col index
        nonZeroCols[1].push_back(matrix[i][j]);
      }
    }
  }
  return nonZeroCols;
}


// Function to create the mapping
vector<vector<uint64_t>> Polynomial::createMapping(const vector<uint64_t>& K, const vector<uint64_t>& H, const vector<vector<uint64_t>>& nonZero) {
  vector<vector<uint64_t>> row(2);
  for (uint64_t i = 0; i < nonZero[0].size(); i++) {
    row[0].push_back(K[i]);
    row[1].push_back(H[nonZero[0][i]]);
  }
  for (uint64_t i = nonZero[0].size(); i < K.size(); i++) {
    row[0].push_back(K[i]);
    row[1].push_back(H[i % H.size()]);
  }

  return row;
}

// Function to print the mapping
void Polynomial::printMapping(vector<vector<uint64_t>>& row, const std::string& name) {
  for (uint64_t i = 0; i < row[0].size(); i++) {
    cout << name << "(" << row[0][i] << ") = " << row[1][i] << endl;
  }
  cout << endl;
}

// Function to create the val mapping
vector<vector<uint64_t>> Polynomial::valMapping(const vector<uint64_t>& K, const vector<uint64_t>& H, vector<vector<uint64_t>>& nonZeroRows, vector<vector<uint64_t>>& nonZeroCols, uint64_t p) {
  vector<vector<uint64_t>> val(2);

  for (uint64_t i = 0; i < K.size(); i++) {
    if (i < nonZeroRows[0].size()) {
      val[0].push_back(K[i]);
      val[1].push_back((nonZeroRows[1][i] * Polynomial::pInverse(((H.size() * Polynomial::power(H[nonZeroRows[0][i]], H.size() - 1, p)) % p) * ((H.size() * Polynomial::power(H[nonZeroCols[0][i]], H.size() - 1, p)) % p), p)) % p);
    } else {
      val[0].push_back(K[i]);
      val[1].push_back(0);
    }
  }

  return val;
}

// Function to calculate log in p
uint64_t Polynomial::log_p(uint64_t a, uint64_t b, uint64_t p) {
  uint64_t m = static_cast<uint64_t>(ceil(sqrt(static_cast<double>(p - 1))));

  // Precompute a^i (p P) for i = 0..m and store in a map
  std::unordered_map<int64_t, uint64_t> tbl;
  for (uint64_t i = 0; i < m; ++i) {
    tbl[Polynomial::pExp(a, i, p)] = i;
  }

  uint64_t c = Polynomial::pExp(a, m * (p - 2), p);  // c = a^(-m) p P

  // Check if we can find the solution in the baby-step giant-step manner
  for (uint64_t j = 0; j < m; ++j) {
    uint64_t y = (b * Polynomial::pExp(c, j, p)) % p;
    if (tbl.find(y) != tbl.end()) {
      uint64_t num = tbl[y];
      return j * m + num;
    }
  }
  return 0;  // Return 0 if no solution is found
}

// Function to calculate e_func in p
uint64_t Polynomial::e_func(uint64_t a, uint64_t b, uint64_t g, uint64_t p) {
  uint64_t buf1 = (a * Polynomial::pInverse(g, p)) % p;
  uint64_t buf2 = (b * Polynomial::pInverse(g, p)) % p;
  return (3 * (buf1 * buf2)) % p;
  return (buf1 * buf2) % p;
}

  // Function to calculate KZG in p
uint64_t Polynomial::KZG_Commitment(vector<uint64_t> a, vector<uint64_t> b, uint64_t p) {
  uint64_t res = 0;
  for (uint64_t i = 0; i < b.size(); i++) {
    res += (a[i] * b[i]) % p;
    res %= p;
  }
  return res;
}


// Function to compute the SHA-256 hash of an uint64_t and return the lower 4 bytes as uint64_t, applying a modulo operation
uint64_t Polynomial::hashAndExtractLower4Bytes(uint64_t inputNumber, uint64_t p) {
  char* inputData = (char*)malloc(21);
  snprintf(inputData, 21, "%" PRId64, inputNumber);

  // Compute the hash
  string hashStr = Polynomial::SHA256(inputData);
  string last4BytesHex = hashStr.substr(56, 8);
  
  uint64_t result = 0;
  std::stringstream ss;
  ss << std::hex << last4BytesHex;
  ss >> result;

  // Apply modulo operation
  result = result % p;
  return result;
}



#define uchar unsigned char
#define uint unsigned int

#define DBL_INT_ADD(a,b,c) if (a > 0xffffffff - (c)) ++b; a += c;
#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

typedef struct {
	uchar data[64];
	uint datalen;
	uint bitlen[2];
	uint state[8];
} SHA256_CTX;

uint k[64] = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

void SHA256Transform(SHA256_CTX *ctx, uchar data[])
{
	uint a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

	for (i = 0, j = 0; i < 16; ++i, j += 4)
		m[i] = (data[j] << 24) | (data[j + 1] << 16) | (data[j + 2] << 8) | (data[j + 3]);
	for (; i < 64; ++i)
		m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

	a = ctx->state[0];
	b = ctx->state[1];
	c = ctx->state[2];
	d = ctx->state[3];
	e = ctx->state[4];
	f = ctx->state[5];
	g = ctx->state[6];
	h = ctx->state[7];

	for (i = 0; i < 64; ++i) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + m[i];
		t2 = EP0(a) + MAJ(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->state[0] += a;
	ctx->state[1] += b;
	ctx->state[2] += c;
	ctx->state[3] += d;
	ctx->state[4] += e;
	ctx->state[5] += f;
	ctx->state[6] += g;
	ctx->state[7] += h;
}

void SHA256Init(SHA256_CTX *ctx)
{
	ctx->datalen = 0;
	ctx->bitlen[0] = 0;
	ctx->bitlen[1] = 0;
	ctx->state[0] = 0x6a09e667;
	ctx->state[1] = 0xbb67ae85;
	ctx->state[2] = 0x3c6ef372;
	ctx->state[3] = 0xa54ff53a;
	ctx->state[4] = 0x510e527f;
	ctx->state[5] = 0x9b05688c;
	ctx->state[6] = 0x1f83d9ab;
	ctx->state[7] = 0x5be0cd19;
}

void SHA256Update(SHA256_CTX *ctx, uchar data[], uint len)
{
	for (uint i = 0; i < len; ++i) {
		ctx->data[ctx->datalen] = data[i];
		ctx->datalen++;
		if (ctx->datalen == 64) {
			SHA256Transform(ctx, ctx->data);
			DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], 512);
			ctx->datalen = 0;
		}
	}
}

void SHA256Final(SHA256_CTX *ctx, uchar hash[])
{
	uint i = ctx->datalen;

	if (ctx->datalen < 56) {
		ctx->data[i++] = 0x80;

		while (i < 56)
			ctx->data[i++] = 0x00;
	}
	else {
		ctx->data[i++] = 0x80;

		while (i < 64)
			ctx->data[i++] = 0x00;

		SHA256Transform(ctx, ctx->data);
		memset(ctx->data, 0, 56);
	}

	DBL_INT_ADD(ctx->bitlen[0], ctx->bitlen[1], ctx->datalen * 8);
	ctx->data[63] = ctx->bitlen[0];
	ctx->data[62] = ctx->bitlen[0] >> 8;
	ctx->data[61] = ctx->bitlen[0] >> 16;
	ctx->data[60] = ctx->bitlen[0] >> 24;
	ctx->data[59] = ctx->bitlen[1];
	ctx->data[58] = ctx->bitlen[1] >> 8;
	ctx->data[57] = ctx->bitlen[1] >> 16;
	ctx->data[56] = ctx->bitlen[1] >> 24;
	SHA256Transform(ctx, ctx->data);

	for (i = 0; i < 4; ++i) {
		hash[i] = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 4] = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 8] = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
		hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
	}
}

string Polynomial::SHA256(char* data) {
	int strLen = strlen(data);
	SHA256_CTX ctx;
	unsigned char hash[32];
	string hashStr = "";

	SHA256Init(&ctx);
	SHA256Update(&ctx, (unsigned char*)data, strLen);
	SHA256Final(&ctx, hash);

	char s[3];
	for (int i = 0; i < 32; i++) {
//		sprintf(s, "%02x", hash[i]);
        snprintf(s, sizeof(s), "%02x", hash[i]);
		hashStr += s;
	}

	return hashStr;
}
