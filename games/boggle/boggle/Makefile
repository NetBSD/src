#	$NetBSD: Makefile,v 1.5 1997/03/24 22:15:39 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
MAN=	boggle.6
FILES=	helpfile
FILESDIR=/usr/share/games/boggle

.include "../../Makefile.inc"
.include <bsd.prog.mk>
