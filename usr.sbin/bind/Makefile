#	$NetBSD: Makefile,v 1.10 2001/02/11 06:45:59 itojun Exp $

.include <bsd.own.mk>

# named-xfer and ndc depend on named/pathname.h

SUBDIR=	named-bootconf reload restart lib .WAIT dig dnskeygen dnsquery host \
	nslookup nsupdate named .WAIT named-xfer ndc
.if ${MKDOC} != no
SUBDIR+=doc
.endif

.include <bsd.subdir.mk>
