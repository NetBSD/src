#	$NetBSD: Makefile,v 1.8 2014/03/29 21:24:26 dholland Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.prog.mk>
