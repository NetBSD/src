#	$NetBSD: Makefile,v 1.2 1997/10/22 05:05:27 lukem Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-ltermcap -lcurses
DPADD=	${LIBTERMCAP} ${LIBCURSES}

CPPFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
