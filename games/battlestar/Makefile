#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.4 1994/12/22 09:33:04 cgd Exp $

PROG=	battlestar
SRCS=	battlestar.c com1.c com2.c com3.c com4.c com5.c com6.c com7.c \
	init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
	globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
MAN=	battlestar.6
DPADD=	${LIBCURSES} ${LIBTERMCAP}
LDADD=	-lcurses -ltermlib
HIDEGAME=hidegame

.include <bsd.prog.mk>
