#	$NetBSD: Makefile.inc,v 1.12.4.1 2025/08/02 05:58:50 perseant Exp $

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
