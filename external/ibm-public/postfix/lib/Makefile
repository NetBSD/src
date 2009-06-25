#	$NetBSD: Makefile,v 1.1 2009/06/25 18:21:52 tron Exp $

# The first four/five of these are libraries and have to be built first.

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"
SUBDIR=	util global dns masterlib tls .WAIT milter xsasl .WAIT
.else
SUBDIR=	util global dns masterlib milter xsasl .WAIT
.endif

.include <bsd.subdir.mk>
