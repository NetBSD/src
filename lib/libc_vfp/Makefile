#	$NetBSD: Makefile,v 1.3.4.2 2014/05/22 11:36:55 yamt Exp $
#

LIB=		c_vfp

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

CPUFLAGS+=	-mfpu=vfp -marm

SRCS=		vfpsf.S vfpdf.S

.include <bsd.lib.mk>
