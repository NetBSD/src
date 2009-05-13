#	$NetBSD: Makefile,v 1.5.26.1 2009/05/13 19:19:52 jym Exp $

.include <bsd.own.mk>

PROG=		iconv
SRCS=		iconv.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
MAN=		iconv.1

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
