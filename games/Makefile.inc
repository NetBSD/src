#	$NetBSD: Makefile.inc,v 1.5 1997/10/11 19:55:46 veego Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93


BINOWN?=	games
.if defined(HIDEGAME)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
.if defined(PROG)
SYMLINKS+=	dm /usr/games/${PROG}
.endif
.else
BINDIR?=	/usr/games
.endif

