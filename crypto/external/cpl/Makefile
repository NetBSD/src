#	$NetBSD: Makefile,v 1.2.2.2 2012/11/20 02:57:29 tls Exp $

.include <bsd.own.mk>

.if (${MKTPM} != "no")
SUBDIR+= trousers .WAIT tpm-tools
.endif

.include <bsd.subdir.mk>
