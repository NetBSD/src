#	$NetBSD: Makefile,v 1.5 1996/10/18 02:45:01 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	caesar
MAN=	caesar.6
DPADD=	${LIBM}
LDADD=	-lm
MLINKS=	caesar.6 rot13.6

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/rot13.sh ${DESTDIR}/usr/games/rot13

.include <bsd.prog.mk>
