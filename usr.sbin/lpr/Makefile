#	$NetBSD: Makefile,v 1.9 1997/10/16 05:13:00 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

SUBDIR=	common_source lp lpc lpd lpq lpr lprm lptest pac filters

.if make(install)
SUBDIR+= SMM.doc
.endif

all-lp all-lpc all-lpd all-lpq all-lpr all-lprm all-lptest \
    all-pac all-filters: all-common_source

.include <bsd.subdir.mk>
