#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.2 1993/08/01 05:43:18 mycroft Exp $

BINOWN?=	games
.if defined(HIDEGAME)
BINDIR?=	/usr/games/hide
BINMODE?=	4700
.else
BINDIR?=	/usr/games
.endif
