#	$NetBSD: Makefile.inc,v 1.1 2016/07/14 01:59:18 matt Exp $
#

CPUFLAGS+=	-mhard-float

SRCS=		fpsf.S fpdf.S
AFLAGS.vfpsf.S+=	-mhard-float
AFLAGS.vfpdf.S+=	-mhard-float
