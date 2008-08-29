#	$NetBSD: Makefile,v 1.8 2008/08/29 00:02:25 gmcgarry Exp $

PROG=	mopprobe

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.mopprobe.c+=	-Wno-pointer-sign
.endif
