#	@(#)Makefile	5.1 (Berkeley) 5/23/90

MAN1=	shar.0

NOPROG=noprog

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/shar.sh ${DESTDIR}/usr/bin/shar

.include <bsd.prog.mk>
