#	$NetBSD: Makefile,v 1.2 2008/10/30 21:37:55 mrg Exp $

SCRIPTS=ching.sh
MAN=	ching.6

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	hexagrams macros
FILESDIR=/usr/share/games/ching
.endif

.include <bsd.prog.mk>
