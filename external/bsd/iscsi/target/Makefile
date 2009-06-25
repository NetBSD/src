# $NetBSD: Makefile,v 1.2 2009/06/25 13:47:12 agc Exp $

.sinclude "${.CURDIR}/../Makefile.inc"

.include <bsd.own.mk>

USE_FORT?=yes	# network server

ISCSIDIST=	${.CURDIR}/../dist
.PATH:	${ISCSIDIST}/src/target

PROG= iscsi-target
SRCS= iscsi-target.c
CPPFLAGS+= -DCONFIG_ISCSI_DEBUG
CPPFLAGS+= -I${ISCSIDIST}/include
CPPFLAGS+= -pthread
LDADD+=  -liscsi
LDFLAGS+= -pthread
MAN=iscsi-target.8 targets.5

.include <bsd.prog.mk>
