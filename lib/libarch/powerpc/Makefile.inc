#	$NetBSD: Makefile.inc,v 1.1 2026/01/09 22:54:26 jmcneill Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "evbppc")
SUBDIR=	powerpc/espresso
.endif
