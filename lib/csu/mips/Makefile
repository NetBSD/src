#	from: @(#)Makefile	8.1 (Berkeley) 6/1/93
#	$Id: Makefile,v 1.3 1995/01/07 23:21:41 mycroft Exp $

CFLAGS=		-DLIBC_SCCS
OBJS=		crt0.o gcrt0.o
CLEANFILES+=	core a.out

all: ${OBJS}

crt0.o: crt0.S
	${COMPILE.S} -DCRT0 ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

gcrt0.o: crt0.S
	${COMPILE.S} -DMCRT0 ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
