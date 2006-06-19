#	$NetBSD: Makefile,v 1.58 2006/06/19 15:44:44 gdamore Exp $

SUBDIR=	altq arch compat crypto dev fs miscfs \
	net net80211 netatalk netbt netccitt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netns netsmb \
	nfs opencrypto sys ufs uvm

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
