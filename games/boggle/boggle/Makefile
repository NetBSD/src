#	$NetBSD: Makefile,v 1.9 1999/02/13 02:54:20 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
MAN=	boggle.6
.if ${MKSHARE} != "no"
FILES=	helpfile
FILESDIR=/usr/share/games/boggle
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
