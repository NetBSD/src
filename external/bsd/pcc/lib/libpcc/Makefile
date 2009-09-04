#	$NetBSD: Makefile,v 1.2 2009/09/04 00:50:05 gmcgarry Exp $

.include <bsd.own.mk>

USE_FORT?= no

DIST=	${NETBSDSRCDIR}/external/bsd/pcc/dist/pcc-libs
SUBDIST=${DIST}/libpcc
.PATH:	${SUBDIST}

CPPFLAGS+=-I${.CURDIR}/../..
CPPFLAGS+=-I${SUBDIST}/arch/${TARGMACH}
CPPFLAGS+=-I${DESTDIR}/usr/include

LIB=	pcc

SRCS=	cmpdi2.c divdi3.c fixdfdi.c fixsfdi.c fixunsdfdi.c \
	fixunssfdi.c floatdidf.c floatdisf.c floatunsdidf.c \
	moddi3.c muldi3.c negdi2.c qdivrem.c \
	ucmpdi2.c udivdi3.c umoddi3.c 
SRCS+=	ashldi3.c ashrdi3.c lshrdi3.c
SRCS+=	ssp.c

.include "../../Makefile.inc"

.include <bsd.lib.mk>
