// SPDX-License-Identifier: MIT
// Fail instead of silently claiming ASan when a compiler accepts but ignores it.
#ifndef __has_feature
#define __has_feature(x) 0
#endif
#if !defined(__SANITIZE_ADDRESS__) && !__has_feature(address_sanitizer)
#error "Compiler did not enable AddressSanitizer instrumentation"
#endif
extern void __asan_init(void);
int main(void) { __asan_init(); return 0; }
