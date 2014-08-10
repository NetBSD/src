#	$NetBSD: Makefile,v 1.6.22.1 2014/08/10 07:10:08 tls Exp $

.include <bsd.own.mk>

SUBDIR=		mkext .WAIT ccom cpp

.if ${MKCXX} != "no"
SUBDIR+=	cxxcom
.endif

.include <bsd.subdir.mk>
