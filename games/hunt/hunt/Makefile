#	$NetBSD: Makefile,v 1.10 2019/10/13 07:28:09 mrg Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c server.c
MAN=	hunt.6
LDADD=	-lcurses -lterminfo
DPADD=	${LIBCURSES} ${LIBTERMINFO}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../include

COPTS.hunt.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
