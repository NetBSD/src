#	$NetBSD: Makefile,v 1.1.1.1 2004/05/18 00:06:58 christos Exp $

LIB=isccc

.include <bsd.own.mk>

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/lib/isccc
.include "${DIST}/api"

.PATH.c:	${DIST}

SRCS=	alist.c base64.c cc.c ccmsg.c lib.c result.c sexpr.c symtab.c \
	version.c

.include <bsd.lib.mk>
