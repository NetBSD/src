# Target: Mitsubishi m32r processor
TDEPFILES= m32r-tdep.o monitor.o m32r-rom.o dsrec.o
TM_FILE= tm-m32r.h
SIM_OBS = remote-sim.o
SIM = ../sim/m32r/libsim.a
