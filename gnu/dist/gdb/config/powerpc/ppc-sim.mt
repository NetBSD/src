# Target: PowerPC running eabi and including the simulator
# XXX Obsolete now that we use AC_SUBST to configure the simulator
TDEPFILES= ser-ocd.o rs6000-tdep.o monitor.o dsrec.o ppcbug-rom.o dink32-rom.o ppc-bdm.o ocd.o remote-sds.o
TM_FILE= tm-ppc-eabi.h
