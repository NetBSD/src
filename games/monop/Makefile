#	$NetBSD: Makefile,v 1.18 1998/09/12 19:05:48 wrstuden Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	monop
SRCS=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c trade.c
MAN=	monop.6
HIDEGAME=hidegame
CLEANFILES+=initdeck cards.pck
FILES=cards.pck
FILESDIR=/usr/share/games

monop: cards.pck

initdeck: initdeck.c
	${HOST_LINK.c} -o initdeck ${.CURDIR}/initdeck.c

cards.pck: initdeck
	${.OBJDIR}/initdeck ${.CURDIR}/cards.inp

.include <bsd.prog.mk>
