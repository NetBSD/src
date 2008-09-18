#	$NetBSD: Makefile,v 1.7.20.1 2008/09/18 04:30:07 wrstuden Exp $

PROG=	mopprobe

.include <bsd.prog.mk>

.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
COPTS.mopprobe.c+=	-Wno-pointer-sign
.endif
