# Target: Intel 960 rom monitor
TDEPFILES= i960-tdep.o monitor.o mon960-rom.o ttyflush.o xmodem.o dsrec.o
TM_FILE= tm-mon960.h
SIM_OBS = remote-sim.o
SIM = ../sim/i960/libsim.a

