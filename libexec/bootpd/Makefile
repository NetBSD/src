# The next few lines may be uncommented and changed to alter the default
# filenames bootpd uses for its configuration and dump files.
#CONFFILE=-DCONFIG_FILE=\"/usr/etc/bootptab\"
DUMPFILE=-DDUMPTAB_FILE=\"/var/run/bootpd.dump\"
FILESPECS=${CONFFILE} ${DUMPFILE}

# Remove the -DVEND_CMU if you don't wish to support the "CMU vendor format"
# in addition to the RFC1048 format.

PROG=	bootpd
# Note: PRIVATE=static is defined by default in bootpd.h
CFLAGS+=-DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU ${FILESPECS}
SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c getif.c hwaddr.c report.c tzone.c
MAN5=	bootptab.0
MAN8=	bootpd.0

MLINKS= bootpd.8 bootpgw.8

BINDIR=/usr/sbin

.include <bsd.prog.mk>

# Additional targets (bootpd is done automatically)
all: bootpef bootpef.0 bootpgw bootptest bootptest.0
CLEANFILES+= bootpef bootpgw bootptest

bootpef: bootpef.o dovend.o readfile.o hash.o dumptab.o \
	 lookup.o hwaddr.o report.o tzone.o
	${CC} -o ${.TARGET} ${CFLAGS} ${LDFLAGS} ${.ALLSRC} ${LIBS}

bootpgw: bootpgw.o getif.o hwaddr.o report.o
	${CC} -o ${.TARGET} ${CFLAGS} ${LDFLAGS} ${.ALLSRC} ${LIBS}

print-bootp.o : print-bootp.c
	${CC} ${CFLAGS} -DBOOTPTEST -c ${.IMPSRC}

bootptest: bootptest.o print-bootp.o getif.o getether.o report.o
	${CC} -o ${.TARGET} ${CFLAGS} ${LDFLAGS} ${.ALLSRC} ${LIBS}

bootpef.0: bootpef.8
bootptest.0: bootptest.8

install:
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	     bootpef ${DESTDIR}${BINDIR}
	install ${COPY} ${STRIP} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	     bootpgw ${DESTDIR}${BINDIR}
	${MINSTALL} bootpef.0 ${DESTDIR}${MANDIR}8${MANSUBDIR}

