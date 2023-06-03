#	$NetBSD: Makefile,v 1.11 2023/06/03 09:09:09 lukem Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

COPTS.hunt.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
