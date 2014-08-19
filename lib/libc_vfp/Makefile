#	$NetBSD: Makefile,v 1.2.4.3 2014/08/20 00:02:16 tls Exp $
#

LIB=		c_vfp

USE_SHLIBDIR=	yes

.include <bsd.own.mk>

CPUFLAGS+=	-mfpu=vfp -marm

SRCS=		vfpsf.S vfpdf.S

.include <bsd.lib.mk>
