#	$NetBSD: Makefile,v 1.7 1996/10/18 02:46:02 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	fish
MAN=	fish.6
HIDEGAME=hidegame

beforeinstall: 
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/fish.instr \
	    ${DESTDIR}/usr/share/games/fish.instr

.include <bsd.prog.mk>
