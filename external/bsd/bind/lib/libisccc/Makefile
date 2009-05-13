#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:51:50 jym Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isccc
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c lib.c result.c sexpr.c symtab.c \
	version.c

.include <bsd.lib.mk>
