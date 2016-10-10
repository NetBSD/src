#	$NetBSD: Makefile.inc,v 1.3 2016/10/10 18:49:30 macallan Exp $
#

CPUFLAGS+=	-mhard-float
MKSOFTFLOAT=	no

SRCS=		fpsf.S fpdf.S
AFLAGS.fpsf.S+=	-mhard-float
AFLAGS.fpdf.S+=	-mhard-float
