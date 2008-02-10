#	$NetBSD: Makefile,v 1.2 2008/02/10 20:32:25 ragge Exp $

.include <bsd.own.mk>

USE_FORT?= yes

CPPFLAGS+=-I../..
DIST=	${NETBSDSRCDIR}/dist/pcc
SUBDIST=${DIST}/cc/cc
.PATH:	${SUBDIST}

PROG=	pcc
SRCS=	cc.c

MAN=	cc.1

CLEANFILES+=	cc.cat1

.include <bsd.prog.mk>
