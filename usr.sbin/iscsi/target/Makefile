# $NetBSD: Makefile,v 1.2 2007/05/28 12:06:35 tls Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

.include <bsd.own.mk>

USE_FORT?=yes	# network server

DIST=	${.CURDIR}/../../../dist/iscsi
.PATH:	${DIST}/src

PROG= iscsi-target
SRCS= iscsi-target.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG -D_FILE_OFFSET_BITS=64
CPPFLAGS+= -I${DIST}/include
CPPFLAGS+= -pthread
LDADD+=  -liscsi
LDFLAGS+= -pthread
MAN=iscsi-target.8 targets.5
WARNS=4

.include <bsd.prog.mk>
