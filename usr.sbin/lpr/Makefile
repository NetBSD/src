#	$NetBSD: Makefile,v 1.10 2001/01/09 03:13:42 lukem Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

SUBDIR=	common_source .WAIT lp lpc lpd lpq lpr lprm lptest pac filters

.if make(install)
SUBDIR+= SMM.doc
.endif

.include <bsd.subdir.mk>
