#	$NetBSD: Makefile,v 1.3 2003/07/10 11:21:03 lukem Exp $

.include <bsd.own.mk>

WARNS?=		2
PROG=		iconv
SRCS=		iconv.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
MAN=		iconv.1

.include <bsd.prog.mk>
