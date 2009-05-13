#	$NetBSD: Makefile.inc,v 1.4.44.1 2009/05/13 19:19:43 jym Exp $

.include <bsd.own.mk>

LIBAUDIO != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
