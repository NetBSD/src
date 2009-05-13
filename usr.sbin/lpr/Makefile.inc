#	$NetBSD: Makefile.inc,v 1.7.20.1 2009/05/13 19:20:28 jym Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# network client/server *and* setgid

LIBCOMMON != cd ${.CURDIR}/../common_source && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
