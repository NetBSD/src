# Message system compiler Makefile
#

SRCS = msgmain.c msgparse.y msgscan.l msgdb.c util.c avl.c

.PATH: ${.CURDIR}/../menuc

PROG = msgc

DATA = msg_sys.def

DATADIR = /usr/share/misc

MAN = msgc.1

LDADD = -lfl

WARNS = 1

COPTS = -g

CLEANFILES += y.tab.h

CFLAGS += -I. -I${.CURDIR}

.include<bsd.prog.mk>

.for M in ${DATA}

proginstall:: ${DESTDIR}${DATADIR}/${M}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${DATADIR}/${M}
.endif
.if !defined(BUILD)
${DESTDIR}${DATADIR}/${M}: .MADE
.endif

${DESTDIR}${DATADIR}/${M}: ${M}
	$(INSTALL) -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.ALLSRC} ${.TARGET}
.endfor
