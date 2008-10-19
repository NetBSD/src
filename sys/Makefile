#	$NetBSD: Makefile,v 1.66.6.1 2008/10/19 22:15:38 haad Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netsmb \
	nfs opencrypto sys ufs uvm

# interrupt implementation depends on the kernel within the port
.if (${MACHINE} != "evbppc")
SUBDIR+=lkm modules
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.if make(obj)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
