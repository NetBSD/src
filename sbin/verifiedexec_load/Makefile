PROG    = verifiedexec_load
MAN   	= verifiedexec_load.8
SRCS    = verifiedexec_parse.y verifiedexec_conf.l verifiedexec_load.c

CFLAGS += -g
LDADD      =  -ll -static
YFLAGS += -d
LDFLAGS += -g
CLEANFILES = *.o *~ y.tab.h

.include <bsd.prog.mk>
