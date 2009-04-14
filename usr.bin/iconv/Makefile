#	$NetBSD: Makefile,v 1.6 2009/04/14 22:15:21 lukem Exp $

.include <bsd.own.mk>

PROG=		iconv
SRCS=		iconv.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
MAN=		iconv.1

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
