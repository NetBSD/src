#	$NetBSD: Makefile,v 1.1.2.3 1997/11/27 09:10:11 veego Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-lcurses -ltermcap
DPADD=	${LIBTERMCAP} ${LIBCURSES}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
