#	$NetBSD: Makefile.inc,v 1.5 2001/01/07 08:00:58 enami Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common_source && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common_source
DPADD+=	${LIBCOMMON}/libcommon.a
LDADD+=	-L${LIBCOMMON} -lcommon

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
