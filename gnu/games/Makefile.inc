#	from: @(#)Makefile.inc	5.1 (Berkeley) 5/11/90
#	$Id: Makefile.inc,v 1.4 1997/11/19 08:26:35 mrg Exp $

.if defined(HIDEGAME) && defined(PROG)
BINDIR?=	/usr/games/hide
BINGRP?=	games
BINMODE?=	550
SYMLINKS+=	dm /usr/games/${PROG}
.else
BINDIR?=	/usr/games
.endif
