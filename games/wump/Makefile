#	$NetBSD: Makefile,v 1.5 1996/10/18 02:51:24 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	wump
MAN=	wump.6
HIDEGAME=hidegame

beforeinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/wump.info \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
