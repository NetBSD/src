#	$NetBSD: Makefile,v 1.8.18.1 2011/06/23 14:20:48 cherry Exp $

PROG=	mopprobe

.include <bsd.prog.mk>

.if defined(HAVE_GCC) || defined(HAVE_PCC)
COPTS.mopprobe.c+=	-Wno-pointer-sign
.endif
