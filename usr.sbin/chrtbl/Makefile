#	$NetBSD: Makefile,v 1.4 2002/09/18 03:54:28 lukem Exp $

.include <bsd.own.mk>

PROG=	chrtbl
MAN=	chrtbl.8

.PATH:	${NETBSDSRCDIR}/lib/libc/locale

CPPFLAGS+=-I${NETBSDSRCDIR}/lib/libc/locale

# ctypeio.c pulled in from ../../lib/libc/locale
SRCS=	chrtbl.c ctypeio.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.include <bsd.prog.mk>
