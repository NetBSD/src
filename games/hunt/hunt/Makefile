#	$NetBSD: Makefile,v 1.9.26.1 2020/04/13 08:03:07 martin Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

COPTS.hunt.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
