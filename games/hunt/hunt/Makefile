#	$NetBSD: Makefile,v 1.7.6.1 2014/05/22 11:36:23 yamt Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.prog.mk>
