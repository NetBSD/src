#	$NetBSD: Makefile,v 1.54 2003/10/13 06:49:14 dyoung Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netccitt netinet netinet6 netisdn netiso netkey \
	netnatm netns netsmb nfs opencrypto sys ufs uvm

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
