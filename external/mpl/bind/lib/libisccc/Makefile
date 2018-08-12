#	$NetBSD: Makefile,v 1.1 2018/08/12 13:02:43 christos Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

LIBDPLIBS+=	isc	${.CURDIR}/../libisc

DIST=	${IDIST}/lib/isccc
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c lib.c result.c sexpr.c symtab.c \
	version.c

.include <bsd.lib.mk>
