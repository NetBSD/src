#source: pr24151a.s
#as: --x32
#ld: -shared -melf32_x86_64
#error: .*relocation R_X86_64_PC32 against protected symbol `foo' can not be used when making a shared object
