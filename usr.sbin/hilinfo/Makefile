#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 15:11:42 lukem Exp $

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CPPFLAGS+= -I${.CURDIR}/../../sys/arch/hp300
.else
NOOBJ=
.endif
NOMAN=

.include <bsd.prog.mk>
