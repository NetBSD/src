#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.3 1997/03/27 09:13:14 mycroft Exp $

BINOWN?=	games
.if defined(HIDEGAME)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
