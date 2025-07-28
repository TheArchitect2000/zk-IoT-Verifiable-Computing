// test.cpp
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

    return 0;
}

extern "C" void _start() {
    int r = main();
    __asm__ volatile("li a7, 93; ecall"); // exit syscall
    (void)r;
}