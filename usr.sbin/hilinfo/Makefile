#	$NetBSD: Makefile,v 1.4 1997/10/25 06:58:04 lukem Exp $

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CPPFLAGS+= -I${.CURDIR}/../../sys/arch/hp300
.else
NOOBJ=
.endif
NOMAN=

.include <bsd.prog.mk>
