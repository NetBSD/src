#	$NetBSD: Makefile.inc,v 1.2 2000/12/30 14:54:39 sommerfeld Exp $

.include <bsd.own.mk>

LIBAUDIO != cd ${.CURDIR}/../common && ${PRINTOBJDIR}
CPPFLAGS+=-I${.CURDIR}/../common
DPADD+=	${LIBAUDIO}/libaudio.a
LDADD+=	-L${LIBAUDIO} -laudio

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

COPTS+= -g
