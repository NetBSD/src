# Target: Intel IA-64 running GNU/Linux
TDEPFILES= ia64-tdep.o
TM_FILE= tm-linux.h

GDBSERVER_DEPFILES= low-linux.o
GDBSERVER_LIBS= -lc -lnss_dns -lnss_files -lresolv -lc
