# $NetBSD: Makefile,v 1.7 2025/09/18 03:35:59 mrg Exp $

.sinclude "${.CURDIR}/../Makefile.inc"

.include <bsd.own.mk>

USE_FORT?=yes	# network client/local server

ISCSIDIST=	${.CURDIR}/../dist
.PATH:	${ISCSIDIST}/src/initiator

PROG=iscsi-initiator
SRCS=iscsi-initiator.c virtdir.c libkmod.c
LDADD+= -lrefuse -liscsi -lpuffs
DPADD+= ${LIBREFUSE} ${LIBISCSI} ${LIBPUFFS}
CPPFLAGS+= -I${ISCSIDIST}/include -D_KERNTYPES
CPPFLAGS+= -pthread
LDFLAGS+= -pthread
MAN=iscsi-initiator.8
WARNS=4

.ifdef MODULAR_KERNEL
CPPFLAGS+= -DUSE_LIBKMOD
LDADD+= -lprop
DPADD+= ${LIBPROP}
.endif

COPTS.iscsi-initiator.c+=	${CC_WNO_CALLOC_TRANSPOSED_ARGS}
COPTS.virtdir.c+=		${CC_WNO_CALLOC_TRANSPOSED_ARGS}

.include <bsd.prog.mk>
