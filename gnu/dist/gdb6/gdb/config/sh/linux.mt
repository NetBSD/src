# Target: GNU/Linux Super-H
TDEPFILES= sh-tdep.o sh64-tdep.o sh-linux-tdep.o \
	monitor.o sh3-rom.o remote-e7000.o ser-e7kpc.o dsrec.o \
	solib.o solib-svr4.o symfile-mem.o
DEPRECATED_TM_FILE= tm-linux.h

SIM_OBS = remote-sim.o
SIM = ../sim/sh/libsim.a
