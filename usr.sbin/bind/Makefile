#	$NetBSD: Makefile,v 1.8 2001/01/09 03:13:42 lukem Exp $

.include <bsd.own.mk>

# named-xfer and ndc depend on named/pathname.h

SUBDIR=	named-bootconf reload restart lib .WAIT dig dnskeygen dnsquery host \
	nslookup nsupdate named .WAIT named-xfer ndc
.if ${MKDOC} != no
SUBDIR+=doc
.endif


VER= 8.2.2-P7

.include <bsd.subdir.mk>
