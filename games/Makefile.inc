#	$NetBSD: Makefile.inc,v 1.6 1997/10/11 20:22:10 mycroft Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

BINOWN?=	games
.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
