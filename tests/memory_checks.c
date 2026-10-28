/* Test actual standard-name symbols through volatile pointers, and private
   wrappers separately. Both backends must preserve the same core semantics. */
#include "../src/portable_primitives.h"
#include <stddef.h>
#ifndef PS_USE_LIBC
void *memcpy(void *, const void *, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
int memcmp(const void *, const void *, size_t);
#endif
static void *(*volatile call_copy)(void *, const void *, size_t) = memcpy;
static void *(*volatile call_move)(void *, const void *, size_t) = memmove;
static void *(*volatile call_fill)(void *, int, size_t) = memset;
static int (*volatile call_compare)(const void *, const void *, size_t) = memcmp;
int portable_memory_checks(void) {
    unsigned char a[64], expected[64], source[64];
    unsigned from, to, n, i, mode;
    char text[8];
    for (i = 0; i < sizeof source; ++i)
        source[i] = (unsigned char)(i * 17 + 129);
    for (mode = 0; mode < 2; ++mode)
        for (from = 0; from < 24; ++from)
            for (to = 0; to < 24; ++to)
                for (n = 0; n <= 32; ++n) {
                    for (i = 0; i < 64; ++i)
                        a[i] = expected[i] = source[i];
                    for (i = 0; i < n; ++i)
                        expected[to + i] = source[from + i];
                    if (mode)
                        move(a + to, a + from, n);
                    else if (call_move(a + to, a + from, n) != a + to)
                        return 0;
                    for (i = 0; i < 64; ++i)
                        if (a[i] != expected[i])
                            return 0;
                }
    for (to = 0; to < 16; ++to)
        for (n = 0; n <= 32; ++n) {
            for (i = 0; i < 64; ++i)
                a[i] = 0x55;
            if (call_copy(a + to, source + 1, n) != a + to || call_compare(a + to, source + 1, n))
                return 0;
            for (i = 0; i < 64; ++i)
                if ((i < to || i >= to + n) && a[i] != 0x55)
                    return 0;
            if (call_fill(a + to, 0x180, n) != a + to)
                return 0;
            for (i = 0; i < 64; ++i)
                if (a[i] != (i >= to && i < to + n ? 0x80 : 0x55))
                    return 0;
        }
    a[0] = 0xff;
    a[1] = 0x7f;
    if (call_compare(a, a + 1, 1) <= 0 || call_compare(a + 1, a, 1) >= 0 ||
        call_compare(a, a + 1, 0))
        return 0;
    move(a, source, 64); /* unrelated objects */
    if (call_compare(a, source, 64))
        return 0;
    copy(a, source, 64);
    copy(a, a, 64); /* core wrapper accepts self-copy even in libc mode */
    zero(a + 1, 62);
    if (a[0] != source[0] || a[63] != source[63])
        return 0;
    for (i = 1; i < 63; ++i)
        if (a[i])
            return 0;
    copy(0, 0, 0);
    move(0, 0, 0);
    zero(0, 0);
    str(0, 0, 0);
    str(text, "123456789", sizeof text);
    if (!equal(text, "1234567") || length(text) != 7 || length(0) != 0 || !prefix("abcd", "ab") ||
        prefix("ab", "abcd") || !contains("abcdef", "cde") || contains("ab", "abc") ||
        !contains("", ""))
        return 0;
    str(text, 0, sizeof text);
    return text[0] == 0;
}
