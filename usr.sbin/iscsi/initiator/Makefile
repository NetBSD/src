# $NetBSD: Makefile,v 1.3 2009/06/20 04:12:55 agc Exp $

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
