# Target: Motorola m68k running NetBSD
TDEPFILES= m68k-tdep.o m68knbsd-tdep.o nbsd-thread.o corelow.o nbsd-tdep.o \
	solib.o solib-svr4.o
TM_FILE= tm-nbsd.h
