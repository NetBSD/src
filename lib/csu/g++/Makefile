#	$Id: Makefile,v 1.1 1994/01/05 21:05:33 pk Exp $

CFLAGS+=	-DLIBC_SCCS
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${CC} ${CFLAGS} -c ${.ALLSRC}
	${LD} -x -r ${.TARGET}
	mv a.out ${.TARGET}

install:
	install -d -o ${BINOWN} -g ${BINGRP} -m 0755 ${DESTDIR}/usr/lib
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>
