# Target: TI TMS320C80 (MVP) processor
TDEPFILES= tic80-tdep.o
TM_FILE= tm-tic80.h
SIM_OBS = remote-sim.o
SIM = ../sim/tic80/libsim.a
GDBSERVER_DEPFILES= low-sim.o
GDBSERVER_LIBS = ../../sim/tic80/libsim.a ../../bfd/libbfd.a ../../libiberty/libiberty.a -lm
