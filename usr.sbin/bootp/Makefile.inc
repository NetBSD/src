#	$NetBSD: Makefile.inc,v 1.3 2007/05/28 12:06:34 tls Exp $

.include <bsd.own.mk>

USE_FORT?= yes	# network client/server

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
