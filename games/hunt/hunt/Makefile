#	$NetBSD: Makefile,v 1.1.2.1 1997/11/06 03:29:49 mellon Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-lcurses -ltermcap
DPADD=	${LIBTERMCAP} ${LIBCURSES}

CFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
