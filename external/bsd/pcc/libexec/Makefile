#	$NetBSD: Makefile,v 1.8 2014/07/24 20:55:45 plunky Exp $

.include <bsd.own.mk>

SUBDIR=		mkext .WAIT ccom cpp

.if ${MKCXX} != "no"
SUBDIR+=	cxxcom
.endif

.include <bsd.subdir.mk>
