#	$NetBSD: Makefile,v 1.72 2008/11/12 12:35:54 ad Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netsmb \
	nfs opencrypto sys ufs uvm

# interrupt implementation depends on the kernel within the port
.if (${MACHINE} != "evbppc")
SUBDIR+=modules
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-modules:
	@true

.if make(obj)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
