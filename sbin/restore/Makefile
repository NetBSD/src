#	from: @(#)Makefile	5.10 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.9 1993/11/14 15:36:05 cgd Exp $

PROG=	restore
SRCS=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c
ROBJS=	main.o interactive.o restore.o dirs.o symtab.o rtape.o utilities.o \
	dumprmt.o
MAN8=	restore.0 rrestore.0
.PATH:	${.CURDIR}/../dump
CLEANFILES+=dumprmt.o rtape.o rrestore

all: rrestore

rtape.o: tape.c ${LIBC}
	${CC} ${CFLAGS} -c -DRRESTORE ${.CURDIR}/tape.c -o ${.TARGET}

afterinstall:
	install ${COPY} ${STRIP} -o root -g ${BINGRP} -m 4755 rrestore \
	    ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>

rrestore: ${LIBCRT0} ${ROBJS} ${LIBC} ${DPADD}
	${CC} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${ROBJS} ${LDADD}
