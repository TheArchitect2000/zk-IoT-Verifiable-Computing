// test.cpp
// riscv64-unknown-elf-gcc -O0 -static -g -o test.bin test.cpp

extern "C" int main() {
    volatile int a = 3;
    volatile int b = 4;
    volatile int c = a + b;    // add
    volatile int d = c - a;    // sub
    volatile int e = c << 1;   // sll
    volatile int f = e >> 1;   // srl
    volatile int g = e & d;    // and
    volatile int h = g | a;    // or
    volatile int i = h ^ f;    // xor

    return 0;
}