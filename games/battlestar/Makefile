#	$NetBSD: Makefile,v 1.6 1997/11/20 00:12:32 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	battlestar
SRCS=	battlestar.c com1.c com2.c com3.c com4.c com5.c com6.c com7.c \
	init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
	globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
MAN=	battlestar.6
DPADD=	${LIBCURSES} ${LIBTERMCAP}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame
SETGIDGAME=yes

.include <bsd.prog.mk>
