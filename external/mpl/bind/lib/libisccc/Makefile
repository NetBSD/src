#	$NetBSD: Makefile,v 1.2 2019/02/24 20:01:33 christos Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c result.c sexpr.c symtab.c \
	version.c

.include <bsd.lib.mk>
