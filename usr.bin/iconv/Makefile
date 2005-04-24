#	$NetBSD: Makefile,v 1.4 2005/04/24 17:46:06 christos Exp $

.include <bsd.own.mk>

WARNS?=		3
PROG=		iconv
SRCS=		iconv.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
MAN=		iconv.1

.include <bsd.prog.mk>
