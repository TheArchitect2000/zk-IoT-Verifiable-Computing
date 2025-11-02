# Copyright 2025 Fidesinnova.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/bash

total_steps=8

echo "What would you like to do?"
echo "1. Install Device"
echo "2. Generate Commitment"
echo "3. Generate ZKP"
echo "4. Verify ZKP"
read -p "Enter your choice (1, 2, 3, or 4): " user_choice


case $user_choice in
    1)
        ./install_device
        ;;
    
    2)
    
        # Step 1: Compile the program.cpp to assembly
        echo "[1/$total_steps] Compiling program.cpp to assembly"
        g++ -std=c++17 -S program.cpp -o program.s -lstdc++ -lmosquitto -lpthread
        if [ $? -ne 0 ]; then
            echo "Compilation failed"
            exit 1
        fi

        # Step 2: Find the line number that uses 'mul' after '#APP' in program.s
        echo "[2/$total_steps] Finding the first instruction after '#APP' in program.s"
        line_number=$(awk '/#APP/{flag=1; next} flag && /add|mul/{print NR; exit}' program.s)
        if [ -z "$line_number" ]; then
            echo "No relevent instruction found after '#APP'"
            exit 1
        fi

        # Check if jq is installed
        if ! command -v jq &> /dev/null; then
            echo "jq is not installed. Installing jq..."

            # Detect the package manager and install jq
            if command -v apt-get &> /dev/null; then
                sudo apt-get update
                sudo apt-get install -y jq
            elif command -v yum &> /dev/null; then
                sudo yum install -y jq
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y jq
            elif command -v brew &> /dev/null; then
                brew install jq
            else
                echo "Unsupported package manager. Please install jq manually."
                exit 1
            fi

            # Verify installation
            if ! command -v jq &> /dev/null; then
                echo "Failed to install jq. Please install it manually and try again."
                exit 1
            fi
        fi

        # Rest of your script
        # echo "jq is installed. Proceeding with the script..."


        # Step 3: Read the class value from device_config.json
        echo "[3/$total_steps] Reading class value from device_config.json"
        config_file="device_config.json"
        class_value=$(jq -r '.class' "$config_file")
        if [ -z "$class_value" ]; then
            echo "Class value not found in device_config.json"
            exit 1
        fi

        # Step 4: Read n_g from class.json based on the class value
        echo "[4/$total_steps] Reading relevant info from class.json based on class value"
        class_file="class.json"
        n_g=$(jq --arg class_value "$class_value" '.[$class_value].n_g' "$class_file")
        if [ -z "$n_g" ]; then
            echo "n_g not found for class $class_value in class.json"
            exit 1
        fi

        # Step 5: Calculate the new values for code_block
        echo "[5/$total_steps] Calculating new values for code_block"
        second_value=$((line_number + n_g -1))

        # Step 6: Update device_config.json with the new values
        echo "[6/$total_steps] Updating device_config.json 'code_block' with new values"
        temp_file=$(mktemp)
        jq --argjson line_number "$line_number" --argjson second_value "$second_value" '.code_block = [$line_number, $second_value]' "$config_file" > "$temp_file" && mv "$temp_file" "$config_file"

        # Step 7: Run the commitmentGenerator and store the output logs
        echo "[7/$total_steps] Running commitmentGenerator"
        log_dir="log"
        if [ ! -d "$log_dir" ]; then
            mkdir -p "$log_dir"
        fi
        ./commitmentGenerator > log/commitmentGenerator.log 2>&1
        if [ $? -ne 0 ]; then
            echo "commitmentGenerator execution failed"
            exit 1
        fi

        # Step 8: Build the program_new.s using the updated codes and store the output logs
        echo "[8/$total_steps] Build the executable from program_new.s"
        g++ -std=c++17 program_new.s lib/polynomial.cpp -o program -lstdc++ -g -lmosquitto -lpthread
        if [ $? -ne 0 ]; then
            echo "Build failed"
            exit 1
        fi

        ;;
    3)
        # Step 9: Execute the program and store the output logs
        # echo "[9/$total_steps] Executing program"
        # ./program > log/proofGeneration.log 2>&1
        echo "Generating ZKP."
        # while true; do
        ./proofGenerator ./program 2>&1  # Redirect both stdout and stderr to /dev/null
        # if [ $? -eq 0 ]; then
        #     break  # Exit the loop if the program exits successfully
        # else
        #     echo "Restarting Program..."
        # fi
        # done
        ;;
    
    4)
        echo "Verify ZKP."
        ./verifier > log/verifier.log 2>&1
        if [ $? -ne 0 ]; then
            echo "Verifier execution failed"
            exit 1
        fi
        echo "Checking verification result"
        if grep -q 'verify!' log/verifier.log; then
            echo "Verification: true"
        else
            echo "Verification: false"
        fi
esac

# echo "Script completed successfully"
