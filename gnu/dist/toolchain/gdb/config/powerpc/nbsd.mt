# Target: Big-endian PowerPC running NetBSD
TDEPFILES= rs6000-tdep.o
TM_FILE= tm-nbsd.h

SIM_OBS = remote-sim.o
SIM = ../sim/ppc/libsim.a
