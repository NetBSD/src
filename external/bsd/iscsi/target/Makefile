# $NetBSD: Makefile,v 1.1 2009/06/21 21:11:17 agc Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

.include <bsd.own.mk>

USE_FORT?=yes	# network server

ISCSI_DIST=	${.CURDIR}/../dist
.PATH:	${ISCSI_DIST}/src

PROG= iscsi-target
SRCS= iscsi-target.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG
CPPFLAGS+= -I${ISCSI_DIST}/include
CPPFLAGS+= -pthread
LDADD+=  -liscsi
LDFLAGS+= -pthread
MAN=iscsi-target.8 targets.5

.include <bsd.prog.mk>
