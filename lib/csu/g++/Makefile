#	$Id: Makefile,v 1.3 1994/01/05 23:47:35 mycroft Exp $

CFLAGS+=	-DLIBC_SCCS -fpic
OBJS=		c++rt0.o

all: ${OBJS}

c++rt0.o: c++rt0.c
	${CC} ${CFLAGS} -c ${.ALLSRC}

install:
	install -d -o ${BINOWN} -g ${BINGRP} -m 0755 ${DESTDIR}/usr/lib
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${OBJS} \
	    ${DESTDIR}/usr/lib

.include <bsd.prog.mk>
