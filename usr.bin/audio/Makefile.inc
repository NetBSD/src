#	$NetBSD: Makefile.inc,v 1.4 2002/01/15 23:48:51 mrg Exp $

.include <bsd.own.mk>

WARNS=3
LIBAUDIO != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif
