#	$NetBSD: Makefile,v 1.9 2001/01/27 07:35:32 itojun Exp $

.include <bsd.own.mk>

# named-xfer and ndc depend on named/pathname.h

SUBDIR=	named-bootconf reload restart lib .WAIT dig dnskeygen dnsquery host \
	nslookup nsupdate named .WAIT named-xfer ndc
.if ${MKDOC} != no
SUBDIR+=doc
.endif


VER= 8.2.3

.include <bsd.subdir.mk>
