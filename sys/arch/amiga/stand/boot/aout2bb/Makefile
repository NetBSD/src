PROG=aout2bb
SRCS=aout2bb.c chksum.c
NOMAN=notyet
INCLUDES=aout2bb.h chksum.h

COPTS += -Wall -Wstrict-prototypes -g


ci: 
	ci -l $(SRCS) $(INCLUDES) Makefile

.include <bsd.prog.mk>
