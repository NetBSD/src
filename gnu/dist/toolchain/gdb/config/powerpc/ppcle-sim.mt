# Target: PowerPC running eabi in little endian mode under the simulator
TDEPFILES= ser-ocd.o rs6000-tdep.o monitor.o dsrec.o ppcbug-rom.o ppc-bdm.o ocd.o
TM_FILE= tm-ppcle-eabi.h

SIM_OBS = remote-sim.o
SIM = ../sim/ppc/libsim.a
