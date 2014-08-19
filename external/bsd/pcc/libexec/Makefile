#	$NetBSD: Makefile,v 1.6.10.1 2014/08/19 23:52:10 tls Exp $

.include <bsd.own.mk>

SUBDIR=		mkext .WAIT ccom cpp

.if ${MKCXX} != "no"
SUBDIR+=	cxxcom
.endif

.include <bsd.subdir.mk>
