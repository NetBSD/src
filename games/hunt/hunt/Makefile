#	$NetBSD: Makefile,v 1.1 1997/10/04 09:11:18 mrg Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-ltermcap -lcurses
DPADD=	${LIBTERMCAP} ${LIBCURSES}

CFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
