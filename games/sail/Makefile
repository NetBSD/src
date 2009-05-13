#	$NetBSD: Makefile,v 1.15.12.1 2009/05/13 19:18:05 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	sail
SRCS=	main.c pl_main.c pl_1.c pl_2.c pl_3.c pl_4.c pl_5.c pl_6.c pl_7.c \
	dr_main.c dr_1.c dr_2.c dr_3.c dr_4.c dr_5.c lo_main.c \
	assorted.c game.c globals.c misc.c parties.c sync.c array.c version.c
MAN=	sail.6
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
