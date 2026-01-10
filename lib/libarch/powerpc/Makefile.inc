#	$NetBSD: Makefile.inc,v 1.2 2026/01/10 04:44:32 mrg Exp $

.include <bsd.own.mk>

.if (${MACHINE} == "evbppc" && ${MACHINE_ARCH} == "powerpc")
SUBDIR=	powerpc/espresso
.endif
