#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:27 matt Exp $

# The first four/five of these are libraries and have to be built first.

.include <bsd.own.mk>

.if ${MKCRYPTO} != "no"
SUBDIR=	util global dns masterlib tls .WAIT milter xsasl .WAIT
.else
SUBDIR=	util global dns masterlib milter xsasl .WAIT
.endif

.include <bsd.subdir.mk>
