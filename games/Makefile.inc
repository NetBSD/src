#	$NetBSD: Makefile.inc,v 1.4 1997/03/27 09:08:58 mycroft Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

BINOWN?=	games
.if defined(HIDEGAME)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
