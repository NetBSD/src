# $NetBSD: Makefile,v 1.3 2007/10/26 19:04:21 agc Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

.include <bsd.own.mk>

USE_FORT?=yes	# network server

DIST=	${.CURDIR}/../../../dist/iscsi
.PATH:	${DIST}/src

PROG= iscsi-target
SRCS= iscsi-target.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG
CPPFLAGS+= -I${DIST}/include
CPPFLAGS+= -pthread
LDADD+=  -liscsi
LDFLAGS+= -pthread
MAN=iscsi-target.8 targets.5
WARNS=4

.include <bsd.prog.mk>
