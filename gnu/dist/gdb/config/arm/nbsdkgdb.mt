# Target: Acorn RISC machine (ARM) with simulator
TDEPFILES= arm-tdep.o remote-bsd.o
TM_FILE= tm-armnetkgdb.h

SIM_OBS = remote-sim.o
SIM = ../sim/arm/libsim.a
