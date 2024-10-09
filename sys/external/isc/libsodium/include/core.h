/* This overwrites dist/src/libsodium/include/sodium/core.h */

#include "../dist/src/libsodium/include/sodium/export.h"
#define sodium_misuse()	panic("sodium_misuse")
