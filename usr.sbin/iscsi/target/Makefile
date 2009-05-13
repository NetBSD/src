# $NetBSD: Makefile,v 1.3.14.1 2009/05/13 19:20:25 jym Exp $

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
