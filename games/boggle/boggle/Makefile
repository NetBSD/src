#	$NetBSD: Makefile,v 1.8 1998/09/29 07:33:21 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
MAN=	boggle.6
.if !defined(NOSHARE)
FILES=	helpfile
FILESDIR=/usr/share/games/boggle
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
