#	$NetBSD: Makefile,v 1.7 1998/02/18 22:37:31 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
MAN=	boggle.6
FILES=	helpfile
FILESDIR=/usr/share/games/boggle

.include "../../Makefile.inc"
.include <bsd.prog.mk>
