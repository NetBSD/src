#	$NetBSD: Makefile,v 1.6 1996/10/18 02:45:34 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	cribbage
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN=	cribbage.6
HIDEGAME=hidegame

beforeinstall: 
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/cribbage.n \
	    ${DESTDIR}/usr/share/games/cribbage.instr

.include <bsd.prog.mk>
