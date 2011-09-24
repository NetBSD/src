#ld: --export-dynamic
#error: .*dynamic STT_GNU_IFUNC symbol `foo' with pointer equality in `.*.o' can not be used when making an executable; recompile with -fPIE and relink with -pie
#target: x86_64-*-* i?86-*-*
