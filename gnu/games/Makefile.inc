#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.3.2.1 1997/11/19 21:07:21 mellon Exp $

.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINGRP?=	games
BINMODE?=	550
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
