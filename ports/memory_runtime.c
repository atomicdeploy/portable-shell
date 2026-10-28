/* Exact C signatures for compiler-generated memory calls. Not linked in libc
   mode. Keep this translation unit outside LTO, freestanding, without builtins. */
#define PS_MEMORY_VOLATILE
#include "../src/portable_memory.h"
void *memcpy(void *dst, const void *src, size_t n) {
    return ps_portable_copy(dst, src, n);
}
void *memmove(void *dst, const void *src, size_t n) {
    return ps_portable_move(dst, src, n);
}
void *memset(void *dst, int value, size_t n) {
    return ps_portable_fill(dst, value, n);
}
int memcmp(const void *a, const void *b, size_t n) {
    return ps_portable_compare(a, b, n);
}
