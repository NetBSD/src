#	$NetBSD: Makefile,v 1.3 2021/02/19 16:42:24 christos Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c result.c sexpr.c symtab.c \
	version.c

.include <bsd.lib.mk>
