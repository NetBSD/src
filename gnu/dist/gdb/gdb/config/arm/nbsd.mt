# Target: ARM running NetBSD
TDEPFILES= arm-tdep.o armnbsd-tdep.o solib.o solib-svr4.o nbsd-tdep.o

GDB_MULTI_ARCH=GDB_MULTI_ARCH_TM

TM_FILE=tm-nbsd.h
