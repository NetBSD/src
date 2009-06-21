# $NetBSD: Makefile,v 1.1 2009/06/21 21:11:16 agc Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

USE_FORT?=yes	# network client

PROG=iscsi-initiator
SRCS=iscsi-initiator.c virtdir.c initiator.c libkmod.c
LDADD+= -lrefuse -liscsi
DPADD+= ${LIBREFUSE} ${LIBISCSI}
CPPFLAGS+= -g -I${ISCSI_DIST}/include
MAN=iscsi-initiator.8
WARNS=4

ISCSI_DIST=${.CURDIR}/../dist

.ifdef MODULAR_KERNEL
CPPFLAGS+= -DUSE_LIBKMOD
LDADD+= -lprop
DPADD+= ${LIBPROP}
.endif

.PATH: ${ISCSI_DIST}/src

.include <bsd.prog.mk>
