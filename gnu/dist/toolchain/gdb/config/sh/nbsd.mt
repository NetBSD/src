# Target: Hitachi Super-H running NetBSD (with Simulator)
TDEPFILES= sh-tdep.o
TM_FILE= tm-nbsd.h

# XXXJRT Until mknative is fixed.
#SIM_OBS = remote-sim.o
#SIM = ../sim/sh/libsim.a
