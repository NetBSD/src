#	$NetBSD: Makefile,v 1.3 1997/11/05 12:12:32 ragge Exp $

PROG=	hunt
SRCS=	connect.c hunt.c otto.c playit.c pathname.c
MAN=	hunt.6
LDADD=	-lcurses -ltermcap
DPADD=	${LIBTERMCAP} ${LIBCURSES}

CPPFLAGS+=-I${.CURDIR}/../huntd

.PATH: ${.CURDIR}/../huntd

.include <bsd.prog.mk>
