# Target: Motorola MCore processor
TDEPFILES= mcore-tdep.o  mcore-rom.o monitor.o dsrec.o
TM_FILE= tm-mcore.h
SIM_OBS = remote-sim.o
SIM = ../sim/mcore/libsim.a
