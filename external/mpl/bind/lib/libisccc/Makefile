#	$NetBSD: Makefile,v 1.2.4.1 2024/02/29 12:36:04 martin Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c sexpr.c symtab.c

.include <bsd.lib.mk>
