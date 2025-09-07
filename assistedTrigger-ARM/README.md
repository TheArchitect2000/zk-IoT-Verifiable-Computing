Executing on GDB based on ARM processor:

In the assistedTrigger method, QEMU executes the user’s program and stores the resulting execution trace. The proof generation module then reads this trace to produce a zero-knowledge proof (ZKP), which is subsequently verified by the verifier program.


# Zero-Knowledge Proof Workflow
This guide explains how to generate a commitment, create a zero-knowledge proof (ZKP), and verify it using C++ and accompanying utilities.

---

## 1. Generate Commitment
### Step 1: Compile the Commitment Generator
Execute the following command to generate a _commitmentGenerator_ executable file on your machine.
```
g++ -std=c++17 commitmentGenerator.cpp lib/polynomial.cpp -o commitmentGenerator -lstdc++
```
### Step 1.1: Compile C++ Source to Assembly
Here’s a sample C++ program that includes inline assembly. You can modify it as needed:
```
int main() {
    asm volatile (
        "mov x18, #25\n"
        "mov x17, #159\n"
        "mul x17, x17, x18\n"
        "add x17, x17, #28\n"
    );
    return 0;
}
```
Save it as YOUR_PROGRAM.cpp and generate the assembly file:
```
g++ -std=c++17 -S program.cpp -o program.s -lstdc++
```
### Step 1.2: Find the Instruction Line
Open `program.s` and locate the first supported instruction line (like `mul` or `add`) after the `#APP` directive. This line number will be used to define the `code_block` in the device configuration.

### Step 1.3: Update the `device_config.json` File
```
{
  "class": 1,                       // 32-bit Integer
  "deviceType": "Sensor",           // String
  "deviceIdType": "MAC",            // String
  "deviceModel": "Siemense 2.5",    // String
  "manufacturer": "My Company",     // String
  "softwareVersion": "1.2",         // String
  "code_block": [14, 15]            // 64-bit Array
}
```
* **`commitmentId`**: Unique identifier for the commitment.
* **`deviceType`**: Type of the IoT device (e.g., 'Sensor', 'Actuator', 'Car').
* **`deviceIdType`**: Type of the device identifier (e.g., 'MAC', 'VIN').
* **`deviceModel`**: Model of the IoT device.
* **`manufacturer`**: Manufacturer of the IoT device (e.g., 'Siemens', 'Tesla').
* **`softwareVersion`**: Software or firmware version of the device.
* **`code_block`**: Line range in the assembly where the critical operations occur.

### Step 1.4: Run the Commitment Generator
Ensure `program.s`, the `data` folder, `Class.json`, `device_config.json` and the `commitmentGenerator` binary are in the same directory:
```
./commitmentGenerator program.s
```
### Step 1.5: Build an Executable from the Updated Assembly
After `commitmentGenerator` generates `program_AddedFidesProofGen.s`, compile it into an executable:
```
g++ -std=c++17 program_AddedFidesProofGen.s lib/polynomial.cpp -o program -lstdc++ -g
```
## 2. Generate Zero-Knowledge Proof
### Step 2.1: Compile the Proof Generator
```
g++ -std=c++17 proofGenerator.cpp lib/polynomial.cpp -o proofGenerator -lstdc++
```
### Step 2.2: Execute the Program
Execute your program using the `proofGenerator`
```
./proofGenerator ./program
```
## 3. Verify Zero-Knowledge Proof
### Step 3.1: Compile the Verifier
Compile your code for your operating system
```
g++ -std=c++17 verifier.cpp lib/polynomial.cpp -o verifier -lstdc++
```
### Step 3.2: Run the Verifier
```
./verifier
```

## Notes
- Ensure all necessary files and folders (`program.s`, `device_config.json`, `class.json`, `data/`, etc.) are in place.
- Make binaries executable with `chmod +x <binary>` if needed.


<!-- # Trace Execution Tool
## Prerequisites
* GCC Compiler: Ensure you have the GCC compiler installed on your system.

* C++ Program: You should have a C++ program (program.cpp) that you want to trace.

## Compile and Run
### Step 1: Save the Updated C++ Program
Make sure your C++ program (program.cpp) is saved and ready to be compiled.

### Step 2: Compile the Trace Execution Tool
First, compile the `trace_execution` tool using the following command:

```
g++ -o trace_execution trace_execution.cpp
```
This will generate an executable named `trace_execution`.

### Step 3: Compile Your C++ Program with Debugging Symbols
Next, compile your C++ program (`program.cpp`) with debugging symbols enabled. This allows the tracer to capture detailed information about the program's execution.

```
g++ -g -o program program.cpp
```
This will generate an executable named `program` with debugging symbols included.

### Step 4: Run the Tracer
Now, run the `trace_execution` tool with your compiled program as an argument:

```
./trace_execution ./program
```
This will execute your program and generate an execution trace.

### Step 5: Check the Output
The execution trace will be saved in a file named `execution_trace.txt`. You can open this file to review the cleaned output and analyze the flow of your program. -->
