# Target: PowerPC running eabi in little endian mode under the simulator
# XXX Obsolete now that we use AC_SUBST to configure the simulator
TDEPFILES= ser-ocd.o rs6000-tdep.o monitor.o dsrec.o ppcbug-rom.o ppc-bdm.o ocd.o
TM_FILE= tm-ppcle-eabi.h
