#	from: @(#)Makefile	5.5 (Berkeley) 1/18/91
#	$Id: Makefile,v 1.5 1995/01/12 19:06:53 jtc Exp $

PROG=	fish
MAN=	fish.6
HIDEGAME=hidegame

beforeinstall: 
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/fish.instr \
		${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
