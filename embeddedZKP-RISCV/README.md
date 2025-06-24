<p align="center">
  <a href="https://fidesinnova.io/" target="blank"><img src="docs/images/g-c-web-back.png" /></a>
</p>

# zk-IoT: Enabling Verifiable Computing for C++ Programs

<a href="https://discord.com/invite/NQdM6JGwcs" target="_blank"><img src="https://img.shields.io/badge/discord-online-brightgreen.svg" alt="Discord"/></a>
<a href="https://twitter.com/FidesInnova" target="_blank"><img src="https://img.shields.io/twitter/follow/nestframework.svg?style=social&label=Follow"></a>

This guide will teach you how to:
- **Step A:** Create a commitment for computation from a C++ program and submit it to the blockchain.
- **Step B:** Execute the computation, generate a ZK proof to demonstrate the correctness of the execution, and submit it to the blockchain.
- **Step C:** Verify the ZK proof using a blockchain explorer or a verifier program.

# Table of Contents
- [Step A. Commitment Generation](#step-a-commitment-generation)
  - [A.1. Install GCC compiler and emulator to compile and execute C++ programs](#a1-install-gcc-compiler-and-emulator-to-compile-and-execute-c-programs)
    - [A.1.1. Ubuntu](#a11-ubuntu)
    - [A.1.2. macOS](#a12-macos)
  - [A.2. Clone the zkIoT repository](#a2-clone-the-zkiot-repository)
  - [A.3. Writing a C++ program](#a3-writing-a-c-program)
  - [A.4. Compile the C++ program](#a4-compile-the-c-program)
  - [A.5. Edit `device_config.json` in the zkiot-riscv-qemu-executable folder](#a5-edit-device_configjson-in-the-zkiot-folder)
  - [A.6. Check the Commitment folder structure](#a6-check-the-commitment-folder-structure)
  - [A.7. Execute `commitmentGenerator`](#a7-execute-commitmentgenerator)
    - [A.7.1 Ubuntu](#a71-ubuntu)
    - [A.7.2 macOS](#a72-macos)
  - [A.8. Submit the commitment on blockchain](#a8-submit-the-commitment-on-blockchain)
- [Step B. Proof Generation](#step-b-proof-generation)
  - [B.1. Compile the `program_new.s`](#b1-compile-the-programnews)
  - [B.2. Run the executable](#b2-run-the-executable)
    - [B.2.1. Ubuntu](#b21-ubuntu)
    - [B.2.2. macOS](#b22-macos)
  - [B.3. Submit the generated proof on blockchain](#b3-submit-the-generated-proof-on-blockchain)
- [Step C. Proof Verification](#step-c-proof-verification)
  - [C.1. Verifier program](#c1-verifier-program)
    - [C.1.1. Ubuntu](#c11-ubuntu)
    - [C.1.2. macOS](#c12-macos)
  - [C.2. Web ZKP Explorer](#c2-web-zkp-explorer)




# Step A. Commitment Generation
## A.1. Install GCC compiler and emualtor to compile and execute C++ programms
### A.1.1. Ubuntu:
- Install the RISC-V GNU Compiler and Toolchain follow the instructions from https://github.com/riscv-collab/riscv-gnu-toolchain or use the instructions below
```
sudo apt update
git clone https://github.com/riscv/riscv-gnu-toolchain
```
- This repository uses submodules, but submodules will fetch automatically on demand, so `--recursive` or `git submodule update --init --recursive` is not needed. Several standard packages are needed to build the toolchain which requires to run below commands:
```
sudo apt-get install autoconf automake autotools-dev curl python3 python3-pip libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev ninja-build git cmake libglib2.0-dev libslirp-dev
```
- Go to the riscv-gnu-toolchain directory run the below commands to configure and build the Newlib cross-compiler.
```
cd riscv-gnu-toolchain
./configure --prefix=/opt/riscv
make
```
- Add the following path to the PATH system variable.
```
echo 'export PATH=/opt/riscv/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```
- Install the RISC-V QEMU emulator.
```
apt-get install qemu-user-static
```
### A.1.2. macOS:
- Install Homebrew
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
- Add the following path to the system.
```
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zshrc
source ~/.zshrc
```
- Verify the Homebrew installation
```
brew --version
```
- Install RISC-V Toolchain
```
brew tap riscv-software-src/riscv
```
- Build the toolchain
```
brew install riscv-tools
```
- Verify the Installation
```
brew test riscv-tools
```
- Install Spike RISC-V Simulator to run RISC-V binaries
```
git clone https://github.com/riscv/riscv-isa-sim.git
cd riscv-isa-sim
mkdir build
cd build
../configure --prefix=/usr/local
make
sudo make install
```

## A.2. Clone the zkiot-riscv-qemu-executable repository
```
git clone https://github.com/FidesInnova/zkiot-riscv-qemu-executable.git
cd zkiot-riscv-qemu-executable
```
### A.2.1 macOS permission
To allow `commitmentGeneratorForMac` and `verifierForMac` to be executable on macOS, you need to adjust their permissions
```
chmod +x commitmentGeneratorForMac
chmod +x verifierForMac
```
## A.3. Writing a C++ program
Write a C++ program for the GCC Compiler and save it as `program.cpp` in the `zkiot-riscv-qemu-executable` project folder.
**A sample program, program.cpp, is provided in the zkiot repository for testing purposes.**

## A.4. Compile the C++ program
 ```
 riscv64-unknown-elf-g++ -S program.cpp -o program.s -lstdc++
 ```

## A.5. Edit `device_config.json` in the `zkiot-riscv-qemu-executable` folder
```
{
  "class": 5, // 32-bit Integer
  "iot_developer_name": "My Company", // "String"
  "iot_device_name": "Smart Device", // "String"
  "device_hardware_version": "1.0", // "String"
  "firmware_version": "1.0", // "String"
  "code_block": [26087, 26118] // "64-bit Array"
}
```
- To learn about these parameters, please refer to the [Fidesinnova Documentation](https://fidesinnova-1.gitbook.io/fidesinnova-docs/zero-knowledge-proof-zkp-scheme/1-setup-phase).

<!-- 
### Step 4. The Necessary Files
Inside the `data` folder, you will find the `setupN.json` files, where `N` corresponds to your class number. 
The `class.json` file contains critical information, including:
- **number of gates (n_g)**: The total number of gates in your ZKP.
- **number of inputs (n_i)**: The number of inputs to the ZKP.
- **n**: Calculated as `n = n_i + n_g + 1`.
- **m**: Calculated as `m = 2 * n_g`.
- **field size (p)**: The size of the finite field used in the ZKP.
- **generator (g)**: The generator of the field.

Ensure you have the correct `setupN.json` file for your class to proceed with the ZKP setup. -->

## A.6. Check the Commitment folder structure
Ensure your directory structure looks like below and contains `program.s`, `commitmentGenerator`, `class.json`, `device_config.json`, and `data/setupN.json`:
```
zkiot-riscv-qemu-executable/|
      ├── class.json
      ├── commitmentGenerator
      ├── device_config.json
      ├── data/ |
                ├── setup1.json │
                ├── setup2.json │
                ├── setup3.json │
                └── ... (other setupN.json files)
      ├── program.cpp
      ├── program.s
      └── ... (other project files)
```
## A.7. Execute `commitmentGenerator` 
### A.7.1. Ubuntu
```
./commitmentGenerator
```
### A.7.2. macOS
```
./commitmentGeneratorForMac
```
The `commitmentGenerator` will create the following files:
- `data/program_commitment.json`: The commitment file
- `data/program_param.json`: An additional parameters file that accelerates the proof generation process.
- `program_new.s`: A newly generated assembly file with added macros.

## A.8. Submit the commitment on blockchain 
Using Fidesinnova's node web app (e.g., panel.zksensor.tech) submit the generated commitment.

### A.8.1 How to Acquire Developer Privileges
You must first obtain developer permissions to submit commitments and proofs to the blockchain using the IoT Developer Console.
<p align="center">
  <img src="docs/images/14.png" />
</p>
<p align="center">
  <img src="docs/images/15.png" />
</p>
After clicking the 'Update' button, please email your request, including your email address, to the [node admin](info@fidesinnova.io). This step is necessary to upgrade your account to an "IoT Developer Account."

### A.8.2. Connect your wallet to Fidesinnova's node web app.
<p align="center">
  <img src="docs/images/16.png" />
</p>
<p align="center">
  <img src="docs/images/21.png" />
</p>
<p align="center">
  <img src="docs/images/17m.png" />
</p>
<p align="center">
  <img src="docs/images/18m1.png" />
</p>

### A.8.3. Request tokens from the faucet.
<p align="center">
  <img src="docs/images/22.png" />
</p>
<p align="center">
  <img src="docs/images/23.png" />
</p>

### A.8.4. Go to the **IoT Developer Console**
<p align="center">
  <img src="docs/images/A-8-4.png" />
</p>

### A.8.5. Click on **ZKP Commitment Submiter**
<p align="center">
  <img src="docs/images/A-8-5-n.png" />
</p>

### A.8.6. Click on **Upload**
<p align="center">
  <img src="docs/images/A-8-6-1.png" />
</p>
<p align="center">
  <img src="docs/images/A-8-6-2.png" />
</p>
<p align="center">
  <img src="docs/images/A-8-6-3.png" />
</p>

### A.8.7. Click on **Submit**

<p align="center">
  <img src="docs/images/A-8-7-1.png" />
</p>
<p align="center">
  <img src="docs/images/A-8-7-2.png" />
</p>

# Step B. Proof Generation
## B.1. Compile the `program_new.s`
```
riscv64-unknown-elf-g++ program_new.s lib/polynomial.cpp -o program -lstdc++
```

## B.2. Run the executable
### B.2.1. Ubuntu
```
qemu-riscv64-static program
```
- After running the code in QEMU, you will be prompted to enter the contents of `data/program_commitment.json`, `data/program_param.json`, `class.json`, and `data/setupN.json`. After copying the files contents in your terminal, press 'Enter'.
- QEMU will print the proof as a JSON in the terminal. Copy this output into the `data/proof.json` file.
###  B.2.2. macOS
```
spike pk program
```
The proof will be stored in the `data/proof.json` file.
## B.3. Submit the generated proof on blockchain 
Using Fidesinnova's node web app (e.g., panel.zksensor.tech) submit the generated ZK proof file.

### B.3.1. Goto the **IoT Developer Console**.
<p align="center">
  <img src="docs/images/B-3-1.png" />
</p>

### B.3.2. Click on **ZKP Proof Submiter**.
<p align="center">
  <img src="docs/images/B-3-2-n.png" />
</p>

### B.3.3. Select the **IoT Device Type**.
<p align="center">
  <img src="docs/images/B-3-3.png" />
</p>

### B.3.4. Click on **Upload Proof**.
<p align="center">
  <img src="docs/images/B-3-4-1-1.png" />
</p>
<p align="center">
  <img src="docs/images/B-3-4-1-2.png" />
</p>
<p align="center">
  <img src="docs/images/B-3-4-1-3.png" />
</p>

# Step C. Proof Verification
## C.1. Verifier program
Simply run the verifier program.
### C.1.1. Ubuntu
```
./verifier
```
### C.1.2. macOS
```
./verifierForMac
```
## C.2. Web ZKP Explorer
Go to the [Fidesinnova Explorer](https://explorer.fidesinnova.io), look for the submitted proof, and choose "Verify Proof" from the menu.

### C.2.1. Locate the proof
In the [Fidesinnova Explorer](https://explorer.fidesinnova.io), fide the submitted proof using the Transaction ID.
<p align="center">
  <img src="docs/images/9.png" />
</p>

### C.2.2. Verify the proof
At the transaction location, click the ` ⋮ ` and select "Verify Proof" from the menu.
<p align="center">
  <img src="docs/images/10.png" />
</p>
