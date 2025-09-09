// Copyright 2025 Fidesinnova.

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Use `sudo apt install libcurl4-openssl-dev` to install library 

#include <iostream>
#include <curl/curl.h>
#include "lib/json.hpp"
#include <algorithm>
#include <string>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <cstdio> 

using json = nlohmann::json;

std::string accessToken = "";
std::string macAddress = "";
std::string brokerUrl = "";

// Function to trim the URL for broker
std::string removeAppSuffix(std::string url) {
    // Remove "https://" from the beginning (if it exists)
    const std::string prefix = "https://";
    if (url.length() >= prefix.length() &&
        url.compare(0, prefix.length(), prefix) == 0) {
        url = url.substr(prefix.length());
    }

    // Remove "/app" from the end (if it exists)
    const std::string suffix = "/app";
    if (url.length() >= suffix.length() &&
        url.compare(url.length() - suffix.length(), suffix.length(), suffix) == 0) {
        url = url.substr(0, url.length() - suffix.length());
    }

    return url;
}

std::string getMacAddress(const std::string& interface) {
    struct ifreq ifr;
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        std::cerr << "Socket creation failed!" << std::endl;
        return "";
    }

    strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
        std::cerr << "Failed to get MAC address!" << std::endl;
        close(sock);
        return "";
    }

    close(sock);

    unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
    char macAddr[18];
    snprintf(macAddr, sizeof(macAddr), "%02X:%02X:%02X:%02X:%02X:%02X", 
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return std::string(macAddr);
}


// Function to handle the response from the API
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to trim quotes from a string
void trim_quotes(std::string& str) {
    if (!str.empty() && str.front() == '"') str.erase(str.begin());
    if (!str.empty() && str.back() == '"') str.pop_back();
}

// Function to check user credentials
bool check_credentials(const std::string& api_url, const std::string& email, const std::string& password) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string json_data = "{\"email\":\"" + email + "\",\"password\":\"" + password + "\"}";
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set up the POST request
        std::string api_url_credentials = api_url + "/v1/user/credential";
        curl_easy_setopt(curl, CURLOPT_URL, api_url_credentials.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Set the Content-Type header to application/json
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // Perform the request
        res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        // Parse the JSON response to check if the credentials are correct
        try {
            json response = json::parse(readBuffer);
            if (response["success"] == true) {
                accessToken = response["data"]["tokens"]["accessToken"];
                // std::cout << "\n\naccessToken: " << accessToken << std::endl;
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                return true;
            } else {
               std::cerr << "ERROR: " << response["message"] << std::endl;
            }
        } catch (const json::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return false;
}

bool install_device(const std::string& api_url) {
    CURL* curl;
    CURLcode res;
    
    std::string json_data = "{\"deviceName\":\"Siemens IOT2050\", \"deviceType\":\"Siemens_IOT2050\", \"mac\":\"" + macAddress + "\", \"hardwareVersion\":1, \"firmwareVersion\":1, \"parameters\":[{\"title\":\"Temperature\",\"ui\":\"text\",\"unit\":0},{\"title\":\"Humidity\",\"ui\":\"text\",\"unit\":0},{\"title\":\"Button\",\"ui\":\"text\",\"unit\":0}], \"isShared\":false}";
    std::string readBuffer;
    
    // Construct the Authorization header
    std::string authHeader = "Authorization: Bearer " + accessToken;
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        // Set up the POST request        
        std::string api_url_install_device = api_url + "/v1/device/insert";
        curl_easy_setopt(curl, CURLOPT_URL, api_url_install_device.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Set the Content-Type header to application/json
        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, authHeader.c_str());
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        
        // Perform the request
        res = curl_easy_perform(curl);
        
        // Check for errors
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            return false;
        }

        // Parse the JSON response to check if the credentials are correct
        try {
            json response = json::parse(readBuffer);
            // std::cout << response << std::endl;
            if (response["success"] == true) {
                curl_easy_cleanup(curl);
                curl_global_cleanup();
                
                brokerUrl = removeAppSuffix(api_url);
                if (std::remove("broker_host.txt") == 0) {
                    std::cout << "Existing file removed." << std::endl;
                }
                std::ofstream outFile("broker_host.txt");
                if (outFile.is_open()) {
                    outFile << brokerUrl;
                    outFile.close();
                    // std::cout << "Broker URL has been saved to broker_host.txt" << std::endl;
                }
                std::cout << "Device installed successfully." << std::endl;
                // else {
                //     std::cerr << "Unable to open file for writing." << std::endl;
                // }
                return true;
            } else {
                std::cerr << "ERROR: " << response["message"] << std::endl;
                
                brokerUrl = removeAppSuffix(api_url);
                if (std::remove("broker_host.txt") == 0) {
                    std::cout << "Existing file removed." << std::endl;
                }
                std::ofstream outFile("broker_host.txt");
                if (outFile.is_open()) {
                    outFile << brokerUrl;
                    outFile.close();
                    // std::cout << "Broker URL has been saved to broker_host.txt" << std::endl;
                }
                return false;
            }
        } catch (const json::exception& e) {
            std::cerr << "Error parsing JSON: " << e.what() << std::endl;
            return false;
        }
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return false;
}


int main() {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string email, password;

    std::string interface = "eno2"; // "eno2" is the interface
    macAddress = getMacAddress(interface);
    
    // if (!macAddress.empty()) {
    //     std::cout << "MAC Address of " << interface << ": " << macAddress << std::endl;
    // }

    
    // Path to the local JSON file
    std::string filePath = "data/nodes.json";

    // Read the file into a string
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonData = buffer.str();

    // Parse the JSON data
    try {
        json nodes = json::parse(jsonData);
        std::cout << "Available nodes: " << std::endl;

        // List the node names
        for (size_t i = 0; i < nodes.size(); ++i) {
            std::string nodeName = nodes[i]["Name"];
            trim_quotes(nodeName);
            std::cout << i + 1 << ". " << nodeName << std::endl;
        }

        // Prompt the user to select a node
        int choice;
        std::cout << "Enter the number of the node you'd like to choose: ";
        std::cin >> choice;

        if (choice >= 1 && choice <= nodes.size()) {
            std::string selectedNode = nodes[choice - 1]["Name"];
            trim_quotes(selectedNode);
            std::cout << "You selected: " << selectedNode << std::endl;

            // Ask for email and password
            std::cout << "Enter your email: ";
            std::cin >> email;
            std::cout << "Enter your password: ";
            std::cin >> password;


            // Check credentials using the selected node's API URL
            std::string api_url = "https://" + nodes[choice - 1]["API"].get<std::string>();

            if (check_credentials(api_url, email, password)) {
                // std::cout << "Credentials are correct!" << std::endl;
                install_device(api_url);
            } else {
                // std::cout << "Invalid credentials." << std::endl;
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    curl_global_cleanup();
    return 0;
}
