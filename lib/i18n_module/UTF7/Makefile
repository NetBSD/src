#	$NetBSD: Makefile,v 1.3.18.1 2011/06/23 14:18:35 cherry Exp $

SRCPRE=citrus_
.include <bsd.lib.mk>

.if ${MACHINE_ARCH} == "vax" && defined(HAVE_GCC)
COPTS.citrus_utf7.c+=	-O0
.endif
