#	$NetBSD: Makefile.inc,v 1.1.2.1 2016/11/04 14:48:53 pgoyette Exp $
#

CPUFLAGS+=	-mhard-float
MKSOFTFLOAT=	no

SRCS=		fpsf.S fpdf.S
AFLAGS.fpsf.S+=	-mhard-float
AFLAGS.fpdf.S+=	-mhard-float
