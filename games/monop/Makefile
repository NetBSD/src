#	$NetBSD: Makefile,v 1.26 2003/10/21 10:01:19 lukem Exp $
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
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o initdeck ${.CURDIR}/initdeck.c

cards.pck: initdeck
	${_MKTARGET_CREATE}
	./initdeck ${.CURDIR}/cards.inp

.include <bsd.prog.mk>
