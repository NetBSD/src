# Target: AMD x86-64 running GNU/Linux
TDEPFILES= x86-64-tdep.o x86-64-linux-tdep.o dwarf2cfi.o \
	solib.o solib-svr4.o solib-legacy.o

GDB_MULTI_ARCH=GDB_MULTI_ARCH_TM

TM_FILE=tm-x86-64linux.h
