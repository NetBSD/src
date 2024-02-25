#	$NetBSD: Makefile,v 1.3.6.1 2024/02/25 15:47:57 martin Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c sexpr.c symtab.c

.include <bsd.lib.mk>
