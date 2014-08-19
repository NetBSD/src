#	$NetBSD: Makefile.inc,v 1.10.40.1 2014/08/20 00:05:10 tls Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# network server

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
