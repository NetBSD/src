#	$NetBSD: Makefile,v 1.2 2006/07/01 05:54:35 mrg Exp $

SRCPRE=citrus_
.include <bsd.lib.mk>

.if ${MACHINE_ARCH} == "vax" && ${HAVE_GCC} > 3
COPTS.citrus_utf7.c+=	-O0
.endif
