#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:59 lukem Exp $

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CPPFLAGS+= -I${.CURDIR}/../../sys/arch/hp300
.else
MKOBJ=	no
.endif
MKMAN=	no

.include <bsd.prog.mk>
