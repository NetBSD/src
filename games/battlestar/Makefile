#	$NetBSD: Makefile,v 1.9 2001/10/19 03:06:09 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	battlestar
SRCS=	battlestar.c command1.c command2.c command3.c command4.c \
	command5.c command6.c command7.c \
	init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
	globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
MAN=	battlestar.6
DPADD=	${LIBCURSES}
LDADD=	-lcurses
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
