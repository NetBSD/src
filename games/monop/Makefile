#	$NetBSD: Makefile,v 1.14 1997/03/24 22:15:47 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	monop
SRCS=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c trade.c
MAN=	monop.6
DPADD+=	${LIBCOMPAT}
LDADD+=	-lcompat
HIDEGAME=hidegame
CLEANFILES+=initdeck cards.pck
FILES=cards.pck
FILESDIR=/usr/share/games

monop: cards.pck

cards.pck: initdeck
	${.OBJDIR}/initdeck ${.CURDIR}/cards.inp

initdeck: initdeck.c
	${CC} ${CFLAGS} ${LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${.CURDIR}/initdeck.c ${LDADD}

.include <bsd.prog.mk>
