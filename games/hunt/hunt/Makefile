#	$NetBSD: Makefile,v 1.9 2014/03/30 02:26:09 dholland Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.prog.mk>
