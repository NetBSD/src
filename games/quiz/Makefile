#	from: @(#)Makefile	5.1 (Berkeley) 11/10/91
#	$Id: Makefile,v 1.4 1993/10/01 00:02:17 jtc Exp $

PROG=	quiz
MAN6=	quiz.0
SRCS=	quiz.c rxp.c
CATS=	africa america areas arith asia babies bard chinese collectives \
	ed elements europe greek inca index latin locomotive midearth morse \
	mult murders poetry posneg pres province seq-easy seq-hard sexes sov \
	spell state trek ucc
HIDEGAME=hidegame

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}/usr/share/games/quiz.db
	(cd ${.CURDIR}/datfiles; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${CATS} ${DESTDIR}/usr/share/games/quiz.db)

.include <bsd.prog.mk>
