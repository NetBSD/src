#	$NetBSD: Makefile,v 1.4 1997/05/09 07:50:10 mycroft Exp $
#	from: @(#)Makefile	5.6 (Berkeley) 5/22/91

CFLAGS+=	-DLIBC_SCCS -I${.CURDIR}/..
OBJS=		crt0.o gcrt0.o scrt0.o mcrt0.o
CLEANFILES+=	core a.out

all: ${OBJS}

crt0.o: crt0.c
	${COMPILE.c} -DCRT0 -DDYNAMIC ${.ALLSRC}
#	${LD} -x -r ${.TARGET}
#	mv a.out ${.TARGET}

gcrt0.o: crt0.c
	${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}
#	${LD} -x -r ${.TARGET}
#	mv a.out ${.TARGET}

mcrt0.o: crt0.c
	${COMPILE.c} -DMCRT0 ${.ALLSRC} -o ${.TARGET}
#	${LD} -x -r ${.TARGET}
#	mv a.out ${.TARGET}

scrt0.o: crt0.c
	${COMPILE.c} -DSCRT0 ${.ALLSRC} -o ${.TARGET}
#	${LD} -x -r ${.TARGET}
#	mv a.out ${.TARGET}

install:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>
