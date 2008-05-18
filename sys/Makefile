#	$NetBSD: Makefile,v 1.65.20.1 2008/05/18 12:31:18 yamt Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netsmb \
	nfs opencrypto sys ufs uvm

.if (${MACHINE} != "vax" \
  && ${MACHINE} != "evbppc")
SUBDIR+=	rump
.endif

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm modules
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
