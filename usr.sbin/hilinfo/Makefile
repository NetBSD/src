#	$NetBSD: Makefile,v 1.3 1997/06/24 07:55:11 mikel Exp $

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CFLAGS+= -I${.CURDIR}/../../sys/arch/hp300
.else
NOOBJ=
.endif
NOMAN=

.include <bsd.prog.mk>
