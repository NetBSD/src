// RUN: %clang_cc1 -fsyntax-only -verify %s

void* my_malloc(unsigned char) __attribute__((alloc_size(1)));
void* my_calloc(unsigned char, short) __attribute__((alloc_size(1,2)));
void* my_realloc(void*, unsigned) __attribute__((alloc_size(2)));


void* fn1(int) __attribute__((alloc_size("xpto"))); // expected-error{{'alloc_size' attribute requires parameter 1 to be an integer constant}}

void* fn2(void*) __attribute__((alloc_size(1))); // expected-error{{'alloc_size' attribute requires an integer constant}}

void* fn3(unsigned) __attribute__((alloc_size(0))); // expected-error{{attribute parameter 1 is out of bounds}}
void* fn4(unsigned) __attribute__((alloc_size(2))); // expected-error{{attribute parameter 1 is out of bounds}}

void fn5(unsigned) __attribute__((alloc_size(1))); // expected-warning{{only applies to functions that return a pointer}}
char fn6(unsigned) __attribute__((alloc_size(1))); // expected-warning{{only applies to functions that return a pointer}}

void* fn7(unsigned) __attribute__((alloc_size)); // expected-error {{attribute takes at least 1 argument}}

void *fn8(int, int) __attribute__((alloc_size(1, 1))); // OK

void* fn9(unsigned) __attribute__((alloc_size(12345678901234567890123))); // expected-error {{integer constant is larger than the largest unsigned integer type}} // expected-error {{attribute parameter 1 is out of bounds}}

void* fn10(size_t, size_t) __attribute__((alloc_size(1,2))); // expected-error{{redefinition of parameter}} \
                                                             // expected-error{{a parameter list without types is only allowed in a function definition}} \
                                                             // expected-error{{attribute parameter 1 is out of bounds}}
void* fn11() __attribute__((alloc_size(1))); // expected-error{{attribute parameter 1 is out of bounds}}
