#	$NetBSD: Makefile.inc,v 1.7 1997/10/12 14:21:44 lukem Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

BINOWN?=	games
.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
WARNS?=		1
