#	$NetBSD: Makefile,v 1.1.2.2 1997/11/08 06:06:59 lukem Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-lcurses -ltermcap
DPADD=	${LIBTERMCAP} ${LIBCURSES}

CPPFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
