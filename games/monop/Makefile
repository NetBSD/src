#	$NetBSD: Makefile,v 1.23 2003/04/21 01:23:07 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	monop
SRCS=	monop.c cards.c execute.c getinp.c houses.c jail.c malloc.c misc.c \
	morg.c print.c prop.c rent.c roll.c spec.c trade.c
MAN=	monop.6
HIDEGAME=hidegame
CLEANFILES+=initdeck cards.pck
.if ${MKSHARE} != "no"
FILES=cards.pck
FILESDIR=/usr/share/games
.endif

realall: ${FILES}

initdeck: initdeck.c
	${HOST_LINK.c} -o initdeck ${.CURDIR}/initdeck.c

cards.pck: initdeck
	./initdeck ${.CURDIR}/cards.inp

.include <bsd.prog.mk>
