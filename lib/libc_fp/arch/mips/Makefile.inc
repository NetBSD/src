#	$NetBSD: Makefile.inc,v 1.2 2016/10/09 17:18:32 macallan Exp $
#

CPUFLAGS+=	-mhard-float

SRCS=		fpsf.S fpdf.S
AFLAGS.fpsf.S+=	-mhard-float
AFLAGS.fpdf.S+=	-mhard-float
