// test.cpp
// To compile for RISC-V use, use `riscv64-unknown-elf-g++`:
// riscv64-unknown-elf-gcc -o test.bin test.cpp -O2 -static

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

    int* ptr = (int*)0x2000;
    *ptr = i;                  // sw
    int j = *ptr;              // lw

    if (j == i) {              // beq
        a = 0;
    } else {
        a = 1;
    }
    return 0;
}
