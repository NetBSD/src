#	$NetBSD: Makefile,v 1.10.46.1 2020/04/13 08:03:06 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	battlestar
SRCS=	battlestar.c command1.c command2.c command3.c command4.c \
	command5.c command6.c command7.c \
	init.c cypher.c getcom.c parse.c room.c save.c fly.c misc.c \
	globals.c dayfile.c nightfile.c dayobjs.c nightobjs.c words.c
MAN=	battlestar.6
DPADD=	${LIBCURSES} ${LIBTERMINFO}
LDADD=	-lcurses -lterminfo
HIDEGAME=hidegame
SETGIDGAME=yes

.if !empty(MACHINE_ARCH:Msh3*)
COPTS.parse.c+=-Wno-restrict
.endif

.include <bsd.prog.mk>
