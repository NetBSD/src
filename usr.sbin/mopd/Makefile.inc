#	$NetBSD: Makefile.inc,v 1.10 2007/05/28 12:06:37 tls Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# network server

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
