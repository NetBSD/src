#	$NetBSD: Makefile.inc,v 1.7.2.2 1997/11/20 09:06:24 mrg Exp $
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
