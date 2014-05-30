#	$NetBSD: Makefile.inc,v 1.11 2014/05/30 02:16:10 joerg Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# network server

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
