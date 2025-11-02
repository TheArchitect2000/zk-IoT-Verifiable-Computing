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