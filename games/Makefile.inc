#	$NetBSD: Makefile.inc,v 1.7.2.1 1997/11/19 21:05:51 mellon Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINGRP?=	games
BINMODE?=	550
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
WARNS?=		1
