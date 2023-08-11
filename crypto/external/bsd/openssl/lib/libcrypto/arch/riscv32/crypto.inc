
COPTS.threads_pthread.c += -DBROKEN_CLANG_ATOMICS       # no atomic 64 bit ops

.include "../../crypto.inc"
