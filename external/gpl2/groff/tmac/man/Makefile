# $NetBSD: Makefile,v 1.2 2018/01/13 14:00:50 uwe Exp $

DIST_SUBDIR=tmac

.include "${.CURDIR}/../../Makefile.inc"

# For ms.7
USETBL=

MAN=            groff_ms.7 groff_man.7 groff_me.7 groff_mdoc.7
MAN+=           groff_trace.7 groff_www.7
MLINKS=         groff_ms.7 ms.7
MLINKS+=        groff_man.7 man.7
MLINKS+=        groff_me.7 me.7
MLINKS+=        groff_mdoc.7 mdoc.7 groff_mdoc.7 mdoc.samples.7

cleandir: tmac_manclean

tmac_manclean:
	rm -f ${MAN}

.include <bsd.man.mk>
