# FidesInnova — Verifiable ARM64 Execution (Commit → Prove → Verify)

`fidesinnova` is a minimal, production-style demo of verifiable computation on **ARM64 Ubuntu**.  
It commits to your program’s **assembly bytes**, generates a **proof** from an actual run of the binary via **GDB/MI**, and **verifies** the proof with **KZG commitments on BN254** (via [MCL](https://github.com/herumi/mcl)).

- Deterministic SRS (no setup files).  
- AArch64-aware tracer for a subset of ops (`mov`, `add`, `sub`, `mul`, `and`, `orr`).  
- Optional lookup-table (LUT) checks for `and`/`orr`.

---

## Features

- **Three-phase pipeline**
  - **Commit** `(-c)`: hash + KZG commitment to assembly bytes.
  - **Prove** `(-p)`: trace real execution with GDB and produce a proof.
  - **Verify** `(-v)`: verify proof against the published commitment.
- **Deterministic KZG SRS** from `SHA256("fidesinnova_srs")` ensures reproducible commitments across processes.
- **Random spot-checks**: verifier samples opcode/row openings using Fiat–Shamir.
- **Session binding**: per-run blinded commitment derived from a domain tag; prevents replay.

---

## Build

### Prereqs (ARM64 Ubuntu)
```bash
sudo apt update
sudo apt install -y g++ make git libssl-dev gdb
```

### Compile `fidesinnova`
```bash
g++ -std=gnu++17 -O2 fidesinnova.cpp -lmcl -lcrypto -o fidesinnova
```
## Usage
```php-template
./fidesinnova -c <program_name.s>                    # → writes <program_name>.com
./fidesinnova -p <program_name>                      # → reads <program_name>.com, writes <program_name>.prf
./fidesinnova -v <program_name>.com <program_name>.prf
```
### Notes
* -c takes the assembly file produced by objdump -d <binary> > <program_name>.s

* -p takes the executable binary (must exist) and expects <program_name>.com next to it.

* -v verifies the proof against the commitment.

## Quick Start with the sample program
### 1) Create `sample.cpp`
```
#include <iostream>
#include <cstdint>

int main() {
    // Simple arithmetic with a couple of logical ops so the tracer sees a mix:
    uint64_t a = 5, b = 7, c = 2;
    uint64_t t1 = a + b;        // add
    uint64_t t2 = t1 - c;       // sub
    uint64_t t3 = t2 * 3;       // mul
    uint64_t t4 = (t3 & 0xF) | 1; // and / orr (low nibble)
    std::cout << "Result: " << t4 << std::endl;
    return 0;
}
```
### 2) Build the sample assembly and binary (keep it simple to trace)
```bash
g++ -S -O0 -g -fno-inline -fno-builtin -fno-omit-frame-pointer sample.cpp -o sample.s
```
```bash
g++ -O0 -g -fno-pie -no-pie sample.cpp -o sample
```


<!---
### Prereqs (one time)
Install cmake (if you don’t have it):
```
sudo apt install cmake
```
Install clang (if you don’t have it):
```
sudo apt update
sudo apt install clang
```
If you need a newer clang (sometimes mcl / crypto libs like newer ones):
```
sudo apt install clang-18   # or clang-17 depending on your Ubuntu
```
Install GMP dev:
```
sudo apt install libgmp-dev
```
# get mcl (if you don't have it yet)
git clone https://github.com/herumi/mcl.git
cd mcl
mkdir build && cd build
cmake -DCMAKE_C_COMPILER=clang-18 -DCMAKE_CXX_COMPILER=clang++-18 ..
make -j
sudo make install
# this installs headers in a standard place on Ubuntu
```
install OpenSSL dev package
```
sudo apt-get update
sudo apt-get install -y libssl-dev
```
Install build deps
```
sudo apt-get update
sudo apt-get install -y build-essential cmake libgmp-dev
```
### To build and run the code
```
g++ -std=gnu++17 -O2 jolt_style_vm.cpp -lmcl -lcrypto -o jolt_demo
./jolt_demo
```
--->