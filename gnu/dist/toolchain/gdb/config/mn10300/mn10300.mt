# Target: Matsushita mn10300
TDEPFILES= mn10300-tdep.o
TM_FILE= tm-mn10300.h

SIM_OBS = remote-sim.o
SIM = ../sim/mn10300/libsim.a
