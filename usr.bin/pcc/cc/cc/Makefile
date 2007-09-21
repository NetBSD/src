#	$NetBSD: Makefile,v 1.1 2007/09/21 21:10:55 abs Exp $

.include <bsd.own.mk>

USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/dist/pcc
SUBDIST=${DIST}/cc/cc
.PATH:	${SUBDIST}

PROG=	pcc
SRCS=	cc.c

MAN=	cc.1

CLEANFILES+=	cc.cat1

.include <bsd.prog.mk>
