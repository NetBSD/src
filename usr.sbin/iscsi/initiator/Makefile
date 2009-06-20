# $NetBSD: Makefile,v 1.4 2009/06/20 17:58:21 agc Exp $

.sinclude "${.CURDIR}/../../Makefile.inc"

.include <bsd.own.mk>

PROG=iscsi-initiator
SRCS=iscsi-initiator.c virtdir.c initiator.c libkmod.c
LDADD+= -lrefuse -liscsi
DPADD+= ${LIBREFUSE} ${LIBISCSI}
CPPFLAGS+= -g -I${NETBSDSRCDIR}/dist/iscsi/include
MAN=iscsi-initiator.8
WARNS=4

.ifdef MODULAR_KERNEL
CPPFLAGS+= -DUSE_LIBKMOD
LDADD+= -lprop
DPADD+= ${LIBPROP}
.endif

.PATH: ${NETBSDSRCDIR}/dist/iscsi/src

.include <bsd.prog.mk>
