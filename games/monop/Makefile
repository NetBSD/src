#	from: @(#)Makefile	5.8 (Berkeley) 9/2/92
#	$Id: Makefile,v 1.7 1993/08/10 02:44:47 mycroft Exp $

PROG=	monop
SRCS=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c trade.c
MAN6=	monop.0
DPADD+=	${LIBCOMPAT}
LDADD+=	-lcompat
HIDEGAME=hidegame
CLEANFILES+=initdeck cards.pck

all: ${PROG} ${MAN6}

monop: cards.pck

cards.pck: initdeck
	./initdeck ${.CURDIR}/cards.inp

initdeck: initdeck.c
	${CC} ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${.CURDIR}/initdeck.c ${LDADD}

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 cards.pck \
	    ${DESTDIR}/usr/share/games

.include <bsd.prog.mk>
