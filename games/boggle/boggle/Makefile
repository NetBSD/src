#	$NetBSD: Makefile,v 1.6 1998/02/05 12:19:53 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/11/93

PROG=	boggle
SRCS=	bog.c help.c mach.c prtable.c timer.c word.c
DPADD=	${LIBCURSES} ${LIBTERMCAP}
LDADD=	-lcurses -ltermcap
HIDEGAME=hidegame
MAN=	boggle.6
FILES=	helpfile
FILESDIR=/usr/share/games/boggle

.include "../../Makefile.inc"
.include <bsd.prog.mk>
