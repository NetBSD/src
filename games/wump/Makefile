#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.2 1993/08/01 05:43:34 mycroft Exp $

PROG=	wump
MAN6=	wump.0
HIDEGAME=hidegame

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/wump.info \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
