#	$NetBSD: Makefile.inc,v 1.13 2024/10/23 00:45:58 kalvisd Exp $

USE_FORT?=yes	# network server
.include <bsd.own.mk>

.if ${MACHINE_CPU} == "mips"
CPPFLAGS+= -DNOAOUT
.endif

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
.ifndef HOSTPROG
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon
.endif

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
