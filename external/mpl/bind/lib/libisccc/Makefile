#	$NetBSD: Makefile,v 1.4 2024/02/21 22:52:54 christos Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c sexpr.c symtab.c

.include <bsd.lib.mk>
