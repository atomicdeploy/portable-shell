/* Cross-check software division against native x86_64 arithmetic. */
unsigned __udivsi3(unsigned, unsigned);
unsigned __umodsi3(unsigned, unsigned);
int __divsi3(int, int);
int __modsi3(int, int);
int portable_runtime_checks(void) {
    unsigned i, n = 0xffffffffu, d;
    for (i = 0; i < 4096; ++i) {
        int a, b;
        n = n * 1664525u + 1013904223u;
        d = (n ^ (n >> 7) ^ (i << 25)) | 1u;
        if (__udivsi3(n, d) != n / d || __umodsi3(n, d) != n % d)
            return 0;
        a = (int)(n & 0x7fffffffu);
        b = (int)(d & 0x7fffffffu) | 1;
        if (i & 1)
            a = -a;
        if (i & 2)
            b = -b;
        if (__divsi3(a, b) != a / b || __modsi3(a, b) != a % b)
            return 0;
    }
    return __udivsi3(0xffffffffu, 1) == 0xffffffffu &&
           __umodsi3(0xffffffffu, 0x80000000u) == 0x7fffffffu &&
           __divsi3(-2147483647 - 1, 2) == -1073741824 && __modsi3(-2147483647 - 1, 3) == -2;
}
