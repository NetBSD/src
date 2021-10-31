#	$NetBSD: Makefile.inc,v 1.12 2021/10/31 18:06:42 christos Exp $

USE_FORT?=yes	# network server
.include <bsd.own.mk>

.if ${MACHINE_CPU} == "mips"
CPPFLAGS+= -DNOAOUT
.endif

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
