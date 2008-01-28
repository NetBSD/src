#	$NetBSD: Makefile,v 1.9 2008/01/28 01:06:19 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

PROG=	wump
MAN=	wump.6
HIDEGAME=hidegame
.if ${MKSHARE} != "no"
FILES=wump.info
FILESDIR=/usr/share/games
.endif
WARNS=	4

.include <bsd.prog.mk>
