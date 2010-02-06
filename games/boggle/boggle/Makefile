#	$NetBSD: Makefile,v 1.10 2010/02/06 23:45:25 he Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

.include <bsd.own.mk>

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame
MAN=	boggle.6
.if ${MKSHARE} != "no"
FILES=	helpfile
FILESDIR=/usr/share/games/boggle
.endif

.include "../../Makefile.inc"
.include <bsd.prog.mk>
