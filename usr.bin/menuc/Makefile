# Menu system compiler Makefile
#

SRCS = main.c parse.y scan.l avl.c mdb.c util.c

DATA = menu_sys.def

DATADIR= /usr/share/misc

PROG=menuc

MAN=menuc.1

LDADD= -lfl

WARNS= 1

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

