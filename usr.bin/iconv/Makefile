#	$NetBSD: Makefile,v 1.3.6.1 2005/05/01 18:32:58 tron Exp $

.include <bsd.own.mk>

WARNS?=		3
PROG=		iconv
SRCS=		iconv.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc
MAN=		iconv.1

.include <bsd.prog.mk>
