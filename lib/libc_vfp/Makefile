#	$NetBSD: Makefile,v 1.2 2013/01/28 18:57:10 matt Exp $
#

LIB=		c_vfp

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

CPUFLAGS+=	-mfpu=vfp

SRCS=		vfpsf.S vfpdf.S

.include <bsd.lib.mk>
