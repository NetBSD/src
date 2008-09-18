#	$NetBSD: Makefile,v 1.2.20.1 2008/09/18 04:39:19 wrstuden Exp $

SRCPRE=citrus_
.include <bsd.lib.mk>

.if ${MACHINE_ARCH} == "vax" && defined(HAVE_GCC) && ${HAVE_GCC} > 3
COPTS.citrus_utf7.c+=	-O0
.endif
