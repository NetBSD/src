#	$NetBSD: Makefile.inc,v 1.9 2001/01/07 08:00:58 enami Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBKVM} ${LIBCOMMON}/libcommon.a
LDADD+=	-lkvm -L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
