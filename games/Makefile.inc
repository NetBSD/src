#	$NetBSD: Makefile.inc,v 1.14.8.1 2012/10/30 18:58:16 yamt Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

MKHIDEGAME?=	no

.if defined(HIDEGAME) && (${MKHIDEGAME} != no) && defined(PROG)
BINDIR=		/usr/games/hide
BINGRP=		games
.if defined(SETGIDGAME)
USE_FORT?=	yes
BINMODE=	2550
.else
BINMODE=	550
.endif
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR=		/usr/games
.if defined(SETGIDGAME)
BINGRP=		games
BINMODE=	2555
.endif
.endif
WARNS?=		5
