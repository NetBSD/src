#	from: @(#)Makefile	5.5 (Berkeley) 5/21/91
#	$Id: Makefile,v 1.1 1993/10/16 22:02:58 pk Exp $

CFLAGS+=	-DLIBC_SCCS
OBJS=		crt0.o gcrt0.o
CLEANFILES+=	gmon.o moncrt0.o core a.out

all: ${OBJS}

crt0.o: crt0.c
	${CC} ${CFLAGS} -c -DCRT0 ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

moncrt0.o: crt0.c
	${CC} ${CFLAGS} -c -DMCRT0 ${.ALLSRC} -o ${.TARGET}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

gcrt0.o: moncrt0.o gmon.o
	${LD} -x -r -o ${.TARGET} moncrt0.o gmon.o

gmon.o: gmon.c gmon.h
	${CC} ${CFLAGS} -c ${.IMPSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:
	install -d -o ${BINOWN} -g ${BINGRP} -m 0755 ${DESTDIR}/usr/lib
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

depend lint tags:

.include <bsd.prog.mk>
