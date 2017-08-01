#	$NetBSD: Makefile,v 1.4.4.1 2017/08/01 23:18:31 snj Exp $

.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "x86_64")
PROG=	pmc
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
