#	$NetBSD: Makefile.inc,v 1.8 1997/11/19 08:23:16 mrg Exp $
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
