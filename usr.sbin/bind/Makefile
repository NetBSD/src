#	$NetBSD: Makefile,v 1.4.4.4 2002/07/01 17:17:52 he Exp $

.include <bsd.own.mk>

SUBDIR=	named-bootconf reload restart lib dig dnskeygen dnsquery host \
	nslookup nsupdate named named-xfer ndc
.if ${MKDOC} != no
SUBDIR+=doc
.endif

.include <bsd.subdir.mk>
