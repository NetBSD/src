#	from: @(#)Makefile	5.8 (Berkeley) 9/2/92
#	$Id: Makefile,v 1.10 1994/01/04 21:41:36 pk Exp $

PROG=	monop
SRCS=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c trade.c
MAN6=	monop.0
DPADD+=	${LIBCOMPAT}
LDADD+=	-lcompat
HIDEGAME=hidegame
CLEANFILES+=initdeck cards.pck

monop: cards.pck

cards.pck: initdeck
	${.OBJDIR}/initdeck ${.CURDIR}/cards.inp

initdeck: initdeck.c
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${.CURDIR}/initdeck.c ${LDADD}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 cards.pck \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
