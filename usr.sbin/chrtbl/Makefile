#	$NetBSD: Makefile,v 1.3 2001/03/17 20:43:55 christos Exp $

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${.CURDIR}/../../lib/libc/locale

CPPFLAGS+=-I${.CURDIR}/../../lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
