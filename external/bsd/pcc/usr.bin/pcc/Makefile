#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:15:50 wrstuden Exp $

.include <bsd.own.mk>

USE_FORT?= yes

DIST=	${NETBSDSRCDIR}/external/bsd/pcc/dist/pcc
SUBDIST=${DIST}/cc/cc
.PATH:	${SUBDIST}

CPPFLAGS+=-I${.CURDIR}/../..
CPPFLAGS+=-I${DIST}/mip

PROG=	pcc
SRCS=	cc.c

BINDIR= /usr/bin

MAN=	cc.1

CLEANFILES+=	cc.cat1

.include "../../Makefile.inc"

.include <bsd.prog.mk>
