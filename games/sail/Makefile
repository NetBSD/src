#	$NetBSD: Makefile,v 1.8 1997/11/20 00:12:40 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	sail
SRCS=	main.c pl_main.c pl_1.c pl_2.c pl_3.c pl_4.c pl_5.c pl_6.c pl_7.c \
	dr_main.c dr_1.c dr_2.c dr_3.c dr_4.c dr_5.c lo_main.c \
	assorted.c game.c globals.c misc.c parties.c sync.c version.c
MAN=	sail.6
DPADD=	${LIBCURSES} ${LIBTERM}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
