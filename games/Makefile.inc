#	$NetBSD: Makefile.inc,v 1.10 2002/01/27 00:08:49 perry Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.if defined(HIDEGAME) && defined(PROG)
BINDIR=		/usr/games/hide
BINGRP=		games
.if defined(SETGIDGAME)
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
WARNS?=		1
