#	$NetBSD: Makefile,v 1.16 1997/10/12 14:21:48 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

WARNS=	0
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
	${HOST_CC} ${HOST_CFLAGS} ${HOST_LDFLAGS} ${LDSTATIC} -o ${.TARGET} ${.CURDIR}/initdeck.c

.include <bsd.prog.mk>
