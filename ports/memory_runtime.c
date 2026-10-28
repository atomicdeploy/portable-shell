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
/* ARM Runtime ABI memory helpers. Alignment suffixes promise alignment; the
   byte implementation also handles unaligned pointers. None return a value.
   ABI reference: ARM-software/abi-aa, rtabi32, Memory copying/clearing/filling. */
#if defined(__ARM_EABI__)
void __aeabi_memcpy(void *d,const void *s,size_t n) { ps_portable_copy(d,s,n); }
void __aeabi_memcpy4(void *d,const void *s,size_t n) { ps_portable_copy(d,s,n); }
void __aeabi_memcpy8(void *d,const void *s,size_t n) { ps_portable_copy(d,s,n); }
void __aeabi_memmove(void *d,const void *s,size_t n) { ps_portable_move(d,s,n); }
void __aeabi_memmove4(void *d,const void *s,size_t n) { ps_portable_move(d,s,n); }
void __aeabi_memmove8(void *d,const void *s,size_t n) { ps_portable_move(d,s,n); }
void __aeabi_memset(void *d,size_t n,int value) { ps_portable_fill(d,value,n); }
void __aeabi_memset4(void *d,size_t n,int value) { ps_portable_fill(d,value,n); }
void __aeabi_memset8(void *d,size_t n,int value) { ps_portable_fill(d,value,n); }
void __aeabi_memclr(void *d,size_t n) { ps_portable_fill(d,0,n); }
void __aeabi_memclr4(void *d,size_t n) { ps_portable_fill(d,0,n); }
void __aeabi_memclr8(void *d,size_t n) { ps_portable_fill(d,0,n); }
#endif
