#	$NetBSD: Makefile,v 1.11 1997/03/24 22:15:48 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	quiz
MAN=	quiz.6
SRCS=	quiz.c rxp.c
CATS=	africa america areas arith asia babies bard chinese collectives \
	ed elements europe flowers greek inca index latin locomotive \
	midearth morse mult murders poetry posneg pres province seq-easy \
	seq-hard sexes sov spell state trek ucc
HIDEGAME=hidegame
FILES= ${CATS:S@^@${.CURDIR}/datfiles/@}
FILESDIR=/usr/share/games/quiz.db

.include <bsd.prog.mk>
