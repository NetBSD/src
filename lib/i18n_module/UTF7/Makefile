#	$NetBSD: Makefile,v 1.3 2008/08/29 00:02:22 gmcgarry Exp $

SRCPRE=citrus_
.include <bsd.lib.mk>

.if ${MACHINE_ARCH} == "vax" && defined(HAVE_GCC) && ${HAVE_GCC} > 3
COPTS.citrus_utf7.c+=	-O0
.endif
