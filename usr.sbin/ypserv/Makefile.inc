#	$NetBSD: Makefile.inc,v 1.6 2001/01/07 08:00:58 enami Exp $

.include <bsd.own.mk>

LIBCOMMON != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBCOMMON}/libcommon.a ${LIBUTIL}
LDADD+=	-L${LIBCOMMON} -lcommon -lutil

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
