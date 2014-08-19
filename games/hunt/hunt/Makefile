#	$NetBSD: Makefile,v 1.7.12.1 2014/08/20 00:00:23 tls Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

.include <bsd.prog.mk>
