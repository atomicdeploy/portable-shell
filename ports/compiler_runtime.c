/* Optional 32-bit software division ABI helpers; no memory/runtime symbols. */
static unsigned divide(unsigned n, unsigned d, unsigned *remainder) {
    unsigned q = 0, r = 0, i = 32;
    if (!d) {
        *remainder = n;
        return 0;
    }
    while (i--) {
        unsigned carry = r >> 31;
        r = (r << 1) | ((n >> i) & 1u);
        if (carry || r >= d) {
            r -= d;
            q |= 1u << i;
        }
    }
    *remainder = r;
    return q;
}
unsigned __udivsi3(unsigned n, unsigned d) {
    unsigned r;
    return divide(n, d, &r);
}
unsigned __umodsi3(unsigned n, unsigned d) {
    unsigned r;
    divide(n, d, &r);
    return r;
}
int __divsi3(int n, int d) {
    unsigned a = (unsigned)n, b = (unsigned)d, r, q;
    if (n < 0)
        a = 0u - a;
    if (d < 0)
        b = 0u - b;
    q = divide(a, b, &r);
    return (int)(((n < 0) != (d < 0)) ? 0u - q : q);
}
int __modsi3(int n, int d) {
    unsigned a = (unsigned)n, b = (unsigned)d, r;
    if (n < 0)
        a = 0u - a;
    if (d < 0)
        b = 0u - b;
    divide(a, b, &r);
    return (int)(n < 0 ? 0u - r : r);
}
