#	$NetBSD: Makefile.inc,v 1.7 2007/05/28 12:06:37 tls Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# network client/server *and* setgid

WARNS=4
LIBCOMMON != cd ${.CURDIR}/../common_source && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
