#	$NetBSD: Makefile,v 1.8 1998/02/18 22:37:30 jtc Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	battlestar
SRCS=	battlestar.c com1.c com2.c com3.c com4.c com5.c com6.c com7.c \
	init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
	globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
MAN=	battlestar.6
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
