#	$NetBSD: Makefile,v 1.63 2007/09/19 20:36:33 he Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netsmb \
	nfs opencrypto sys ufs uvm

.if (${MACHINE} != "vax" \
  && ${MACHINE} != "sun2" \
  && ${MACHINE} != "mmeye" \
  && ${MACHINE} != "landisk" \
  && ${MACHINE} != "hpcsh" \
  && ${MACHINE} != "evbsh3-sh3el" \
  && ${MACHINE} != "evbsh3-sh3eb" \
  && ${MACHINE} != "evbppc" \
  && ${MACHINE} != "dreamcast")
SUBDIR+=	rump
.endif

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
