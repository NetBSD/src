#	$NetBSD: Makefile,v 1.4 1997/11/19 07:08:01 mrg Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-lcurses -ltermcap
DPADD=	${LIBTERMCAP} ${LIBCURSES}
HIDEGAME=hidegame

CPPFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
