#	$NetBSD: Makefile,v 1.4 1996/10/18 06:22:06 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	tput
DPADD=	${LIBTERMCAP}
LDADD=	-ltermcap
MLINKS=	tput.1 clear.1

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/clear.sh ${DESTDIR}/usr/bin/clear

.include <bsd.prog.mk>
