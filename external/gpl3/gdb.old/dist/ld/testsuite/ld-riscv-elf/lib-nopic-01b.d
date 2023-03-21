#name: link non-pic code into a shared library
#source: lib-nopic-01b.s
#as:
#ld: -shared tmpdir/lib-nopic-01a.so
#error: .*relocation R_RISCV_JAL against `func1' can not be used when making a shared object; recompile with -fPIC
