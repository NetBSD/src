#	$NetBSD: Makefile.inc,v 1.9 1997/11/20 00:12:30 mrg Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINGRP?=	games
.if defined(SETGIDGAME)
BINMODE?=	2550
.else
BINMODE?=	550
.endif
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
WARNS?=		1
