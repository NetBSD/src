#	from: @(#)Makefile	5.11 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.3 1993/08/01 05:45:43 mycroft Exp $

PROG=	cribbage
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
SRCS=	extern.c crib.c cards.c instr.c io.c score.c support.c
MAN6=	cribbage.0
HIDEGAME=hidegame

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/cribbage.n \
	    ${DESTDIR}/usr/share/games/cribbage.instr

.include <bsd.prog.mk>
