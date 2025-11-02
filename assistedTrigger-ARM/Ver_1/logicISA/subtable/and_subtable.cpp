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
// `g++ and_subtable.cpp -o and_subtable -lstdc++`

#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <bitset>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <bits> <output_file>\n";
        return 1;
    }

    int bits = std::stoi(argv[1]);
    if (bits <= 0 || bits > 16) {
        std::cerr << "Error: bits must be between 1 and 16\n";
        return 1;
    }

    std::string filename = argv[2];
    std::ofstream table(filename);
    if (!table.is_open()) {
        std::cerr << "Error: could not open file " << filename << " for writing\n";
        return 1;
    }

    uint32_t maxVal = 1u << bits;

    for (uint32_t a = 0; a < maxVal; a++) {
        for (uint32_t b = 0; b < maxVal; b++) {
            uint32_t c = a & b;

            // print as binary strings with leading zeros
            table << std::bitset<16>(a).to_string().substr(16 - bits) << " "
                  << std::bitset<16>(b).to_string().substr(16 - bits) << " "
                  << std::bitset<16>(c).to_string().substr(16 - bits) << "\n";
        }
    }

    table.close();
    std::cout << "Binary subtable generated with " << maxVal * maxVal
              << " entries in " << filename << "\n";
    return 0;
}


// #include <iostream>
// #include <fstream>
// #include <cstdint>
// #include <string>

// int main(int argc, char* argv[]) {
//     if (argc < 3) {
//         std::cerr << "Usage: " << argv[0] << " <bits> <output_file>\n";
//         return 1;
//     }

//     int bits = std::stoi(argv[1]);
//     if (bits <= 0 || bits > 16) { // prevent huge tables by accident
//         std::cerr << "Error: bits must be between 1 and 16\n";
//         return 1;
//     }

//     std::string filename = argv[2];
//     std::ofstream table(filename);
//     if (!table.is_open()) {
//         std::cerr << "Error: could not open file " << filename << " for writing\n";
//         return 1;
//     }

//     uint32_t maxVal = 1u << bits;

//     for (uint32_t a = 0; a < maxVal; a++) {
//         for (uint32_t b = 0; b < maxVal; b++) {
//             uint32_t c = a & b;
//             table << a << " " << b << " " << c << "\n";
//         }
//     }

//     table.close();
//     std::cout << "Subtable generated with " << maxVal * maxVal
//               << " entries in " << filename << "\n";
//     return 0;
// }

