# Target: AMD64 running NetBSD
TDEPFILES= x86-64-tdep.o x86-64nbsd-tdep.o nbsd-tdep.o \
	   dwarf2cfi.o solib.o solib-svr4.o
TM_FILE= tm-x86-64nbsd.h

GDB_MULTI_ARCH=GDB_MULTI_ARCH_PARTIAL
