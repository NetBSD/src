# $NetBSD: Makefile,v 1.4 2009/04/22 15:23:04 lukem Exp $

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

.include <bsd.prog.mk>
