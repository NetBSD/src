#	$NetBSD: Makefile,v 1.6 2001/12/12 01:48:51 tv Exp $

.if ${MACHINE} == "hp300"
PROG=	hilinfo
CPPFLAGS+= -I${.CURDIR}/../../sys/arch/hp300
.else
NOOBJ=	# defined
.endif
NOMAN=	# defined

.include <bsd.prog.mk>
