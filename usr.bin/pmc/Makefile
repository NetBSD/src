#	$NetBSD: Makefile,v 1.5 2017/07/12 17:10:09 maxv Exp $

.if (${MACHINE_ARCH} == "i386") || (${MACHINE_ARCH} == "x86_64")
PROG=	pmc
.else
MAN=	pmc.1
.endif

.include <bsd.prog.mk>
