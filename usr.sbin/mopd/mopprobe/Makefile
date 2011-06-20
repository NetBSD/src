#	$NetBSD: Makefile,v 1.9 2011/06/20 07:44:03 mrg Exp $

PROG=	mopprobe

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.mopprobe.c+=	-Wno-pointer-sign
.endif
