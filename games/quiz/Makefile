#	from: @(#)Makefile	5.1 (Berkeley) 11/10/91
#	$Id: Makefile,v 1.5 1994/02/10 02:31:38 cgd Exp $

PROG=	quiz
MAN6=	quiz.0
SRCS=	quiz.c rxp.c
CATS=	africa america areas arith asia babies bard chinese collectives \
	ed elements europe greek inca index latin locomotive midearth morse \
	mult murders poetry posneg pres province seq-easy seq-hard sexes sov \
	spell state trek ucc
HIDEGAME=hidegame

beforeinstall:
	(cd ${.CURDIR}/datfiles; install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${CATS} ${DESTDIR}/usr/share/games/quiz.db)

.include <bsd.prog.mk>
