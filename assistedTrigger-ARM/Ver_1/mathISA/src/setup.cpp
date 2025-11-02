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


#include <stdint.h>
#include <fstream>
#include "../lib/json.hpp"
using ordered_json = nlohmann::ordered_json;
#include <regex>
#include <iostream>
#include <random>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

void setup() {
    vector<uint64_t> ck;
    uint64_t vk;

    nlohmann::json classJsonData;
    // Open the JSON file for reading (class.json)
    std::ifstream classFile("../class.json");
    if (!classFile.is_open()) {
        cerr << "Could not open class.json!" << endl;
        return; // Exit if the file cannot be opened
    }

    // Parse the JSON data from class.json
    classFile >> classJsonData;
    classFile.close();

    // Loop to create multiple setup files
    for (int class_value = 1; class_value <= 16; class_value++) {
        string Class = to_string(class_value); // Convert integer to string class
        // Get n_g for current file
        if (classJsonData.contains(Class)) { // Check if the class exists
            uint64_t n_g = classJsonData[Class]["n_g"].get<uint64_t>();
            uint64_t n_i = classJsonData[Class]["n_i"].get<uint64_t>();
            uint64_t n   = classJsonData[Class]["n"].get<uint64_t>();
            uint64_t m   = classJsonData[Class]["m"].get<uint64_t>();
            uint64_t p   = classJsonData[Class]["p"].get<uint64_t>();
            uint64_t g   = classJsonData[Class]["g"].get<uint64_t>();

            cout << "class: " << class_value << endl;
            cout << "  n_g: " << n_g << endl;
            cout << "  n_i: " << n_i << endl;
            cout << "  n: " << n << endl;
            cout << "  m: " << m << endl;
            cout << "  p: " << p << endl;
            cout << "  g: " << g << endl;

            // Validate p and g
            if (p <= 1) {
                cout << "Invalid p value for Class " << class_value << ": " << p << endl;
            } else {
                uint64_t d_AHP = max(((12 * n_g) - 6), ((3 * n_g) + (2 * n_i) + 1));

                // Calculate new power for g based on d_AHP
                uint64_t pMinusOne = p - 1;

                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<uint64_t> dis(1, pMinusOne);
                uint64_t tau = dis(gen);
                tau %= pMinusOne;

                // Push values into ck
                ck.clear(); // Clear previous values

                for (uint64_t i = 0; i < d_AHP; i++) {
                    ck.push_back(g);
                    g = (g * tau) % p;
                }

                // Output ck for verification
                cout << "ck = {";
                for (uint64_t i = 0; i < ck.size(); i++) {
                    cout << ck[i];
                    if (ck.size() - i > 1) {
                        cout << ", ";
                    }
                }
                cout << "}" << endl;

                // Retrieve verifying key
                if (ck.size() > 1) {
                    vk = ck[1];
                    cout << "vk = " << vk << endl;
                } else {
                    cout << "Error: ck does not have enough elements." << endl;
                    vk = 0; // Set vk to 0 if ck is insufficient
                }

                // Create JSON object
                ordered_json setupJson;
                setupJson["class"] = class_value;
                setupJson["ck"] = ck;
                setupJson["vk"] = vk;

                // Serialize JSON object to a string
                string setupString = setupJson.dump(4); // Pretty print with 4-space indentation

                // Write JSON object to a file
                // Check if the "data" directory exists, if not, create it
                struct stat info;
                if (stat("data", &info) != 0) {
                    cout << "Data directory does not exist. Creating 'data' directory." << endl;
                    if (mkdir("data", 0777) == -1) {
                        cerr << "Error creating data directory!" << endl;
                        return;
                    }
                } else if (!(info.st_mode & S_IFDIR)) {
                    cerr << "'data' exists but is not a directory!" << endl;
                    return;
                }
                std::ofstream setupFile("data/setup" + to_string(class_value) + ".json");
                if (setupFile.is_open()) {
                    setupFile << setupString;
                    setupFile.close();
                    cout << "JSON data has been written to setup" << class_value << ".json\n";
                } else {
                    cerr << "Error opening file for writing setup" << class_value << ".json\n";
                }
            }
        } else {
            cout << "Class " << class_value << " not found in JSON.\n";
        }
    }
}

int main() {
    setup();
}
