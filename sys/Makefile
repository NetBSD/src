#	$NetBSD: Makefile,v 1.56 2005/08/20 15:42:03 elad Exp $

SUBDIR=	altq arch compat crypto dev fs miscfs \
	net net80211 netatalk netccitt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netns netsmb \
	nfs opencrypto sys ufs uvm

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
