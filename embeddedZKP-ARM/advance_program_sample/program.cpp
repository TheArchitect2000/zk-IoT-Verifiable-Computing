#include "lib/fidesinnova.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <filesystem>
#include "lib/json.hpp"
#include <mosquitto.h>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cstring>
#include <sys/ioctl.h>
#include <net/if.h>
#include <vector>

using json = nlohmann::json;
namespace fs = std::filesystem;

std::string macAddress = "";
std::string macBase64 = "";
ordered_json proof;

std::string MQTT_HOST = "";
#define MQTT_PORT 8883
#define MQTT_TOPIC "test"
#define MQTT_KEEP_ALIVE 15
#define MQTT_QOS 0
#define MQTT_RETAIN false

// Base64 encoding table
const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::string &input) {
    std::string encoded_string;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    for (const auto &c : input) {
        char_array_3[i++] = c;
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) {
                encoded_string += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; j < i + 1; j++) {
            encoded_string += base64_chars[char_array_4[j]];
        }

        while (i++ < 3) {
            encoded_string += '=';
        }
    }

    return encoded_string;
}

/*std::vector<std::string> get_available_ports() {
    std::vector<std::string> ports;
    for (const auto &entry : fs::directory_iterator("/dev")) {
        if (entry.path().string().find("ttyUSB") != std::string::npos || 
            entry.path().string().find("ttyS") != std::string::npos ||
            entry.path().string().find("ttyACM") != std::string::npos) {
            ports.push_back(entry.path().string());
        }
    }
    return ports;
}*/

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


int open_serial_port(const std::string &port) {
    int fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        std::cerr << "Error: Unable to open serial port" << std::endl;
        return -1;
    }
    
    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error: Failed to get terminal attributes" << std::endl;
        close(fd);
        return -1;
    }
    
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &tty);
    
    return fd;
}


void on_connect(struct mosquitto *mosq, void *obj, int rc) {
    if (rc == 0) {
        std::cout << "✅ Connected to MQTT broker." << std::endl;
    } else {
        std::cerr << "❌ Failed to connect, return code: " << rc << " - " << mosquitto_strerror(rc) << std::endl;
        exit(EXIT_FAILURE);
    }
}
void on_publish(struct mosquitto *mosq, void *obj, int mid) {
    std::cout << "📢 Message ID " << mid << " published successfully!" << std::endl;
}
void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
    std::cerr << "⚠️ Disconnected! Reason: " << rc << " - " << mosquitto_strerror(rc) << std::endl;
}

std::string floatToStringOneDecimal(float value) {
    char buffer[32]; // Buffer to hold the formatted string
    std::snprintf(buffer, sizeof(buffer), "%.1f", value); // Format to 1 decimal place
    return std::string(buffer);
}

int main() {
   /* std::vector<std::string> ports = get_available_ports();
    if (ports.empty()) {
        std::cerr << "No available UART ports found." << std::endl;
        return 1;
    }*/

    // std::cout << "Available UART ports:" << std::endl;
    // for (size_t i = 0; i < ports.size(); ++i) {
    //     std::cout << i + 1 << ": " << ports[i] << std::endl;
    // }

    // int choice;
    // std::cout << "Select a port (enter number): ";
    // std::cin >> choice;

    // if (choice < 1 || choice > static_cast<int>(ports.size())) {
    //     std::cerr << "Invalid selection." << std::endl;
    //     return 1;
    // }

    // std::string selected_port = ports[choice - 1];
    // int serial_fd = open_serial_port(selected_port);

    std::string interface = "eno2"; // "eno2" is the interface
    macAddress = getMacAddress(interface);
    macBase64 = base64_encode(macAddress);
    int serial_fd = open_serial_port("/dev/ttyACM0");
    if (serial_fd == -1) {
        return 1;
    }

    
    mosquitto_lib_init();
    struct mosquitto *mosq = mosquitto_new(macBase64.c_str(), true, nullptr);
    if (!mosq) {
        std::cerr << "❌ Failed to create MQTT client!" << std::endl;
        return EXIT_FAILURE;
    }
    // Configure TLS: provide the CA certificate file (adjust the path as necessary).
    if(mosquitto_tls_set(mosq, "/etc/ssl/certs/ca-certificates.crt", NULL, NULL, NULL, NULL) != MOSQ_ERR_SUCCESS) {
        std::cerr << "❌ Failed to set TLS options!" << std::endl;
        return EXIT_FAILURE;
    }
    mosquitto_tls_insecure_set(mosq, true);
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publish);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);
    mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V311);
    mosquitto_log_callback_set(mosq, [](mosquitto*, void*, int level, const char* msg) {
    });

    std::ifstream inFile("broker_host.txt");

    if (inFile.is_open()) {
        std::getline(inFile, MQTT_HOST);
        inFile.close();
    } else {
        std::cerr << "❌ Unable to open file to read MQTT_HOST." << std::endl;
        return EXIT_FAILURE;
    }
    int connect_res = mosquitto_connect(mosq, MQTT_HOST.c_str(), MQTT_PORT, MQTT_KEEP_ALIVE);
    if (connect_res != MOSQ_ERR_SUCCESS) {
        std::cerr << "❌ Connection failed: " << mosquitto_strerror(connect_res) << std::endl;
        return EXIT_FAILURE;
    }

    char buffer[256];
    while (true) {
        int bytes_read = read(serial_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string input_data(buffer);

            try {
                // std::cout << "Input Data: " << input_data << std::endl;
                json parsed_json = json::parse(input_data);
                
                asm volatile (
                    "mov x18, #1\n"
                    "mov x17, #1\n"
                    "mul x17, x17, x18\n"
                    "add x17, x17, #1\n"
                );
                if (parsed_json.contains("data")) {
                    mosquitto_loop_stop(mosq, true);
                    
                    auto data = parsed_json["data"];
                    float temperature = data.value("Temperature", 0.0f);
                    float humidity = data.value("Humidity", 0.0f);
                    std::string button_state = data.value("Button", "Unknown");

                    std::string docString;
                    ordered_json doc;
                    if(button_state == "Pressed") {
                        
                        const char* proofJsonFilePath = "data/proof.json";
                        nlohmann::json proofJsonData;
                        std::ifstream proofJsonFile(proofJsonFilePath);
                        proofJsonFile >> proofJsonData;
                        proofJsonFile.close();
                        if(temperature>120) temperature = 23.5;
                        if(humidity>100) humidity = 15.0;
                        doc.clear();
                        doc["from"] = macBase64;
                        doc["to"] = MQTT_HOST;
                        doc["data"]["Framware Version"] = "1.7";
                        doc["data"]["Hardware Version"] = "1";
                        doc["data"]["Root"] = true;
                        doc["data"]["Temperature"] = floatToStringOneDecimal(temperature) + " (°C)";
                        doc["data"]["Humidity"] = floatToStringOneDecimal(humidity) + " (%)";
                        doc["data"]["Button"] = button_state;
                        doc["data"]["proof"] = proofJsonData;
                        docString = doc.dump(4);
                    }
                    else {
                        doc.clear();
                        doc["from"] = macBase64;
                        doc["to"] = MQTT_HOST;
                        doc["data"]["Framware Version"] = "1.7";
                        doc["data"]["Hardware Version"] = "1";
                        doc["data"]["Root"] = true;
                        doc["data"]["Temperature"] = floatToStringOneDecimal(temperature) + " (°C)";
                        doc["data"]["Humidity"] = floatToStringOneDecimal(humidity) + " (%)";
                        doc["data"]["Button"] = button_state;
                        docString = doc.dump(4);
                    }
                    int mid;
                    int rc = mosquitto_publish(mosq, &mid, macBase64.c_str(), strlen(docString.c_str()), docString.c_str(), MQTT_QOS, MQTT_RETAIN);
                    if (rc != MOSQ_ERR_SUCCESS) {
                        std::cerr << "❌ Publish failed: " << mosquitto_strerror(rc) << std::endl;
                    } else {
                        std::cout << "✅ Message published successfully!" << std::endl;
                    }
                    mosquitto_loop_start(mosq);
 

                    // std::cout << "data: " << data << std::endl;
                } else {
                    std::cerr << "Invalid JSON format: Missing 'data' field" << std::endl;
                }
            } catch (json::parse_error &e) {
                std::cerr << "JSON Parse Error: " << e.what() << std::endl;
            }
        }
    }
    close(serial_fd);
    return 0;
}
