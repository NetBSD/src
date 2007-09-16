#	$NetBSD: Makefile,v 1.62 2007/09/16 21:42:23 pooka Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netnatm netsmb \
	nfs opencrypto sys ufs uvm

.if (${MACHINE} != "x68k" \
  && ${MACHINE} != "vax" \
  && ${MACHINE} != "sun3" \
  && ${MACHINE} != "sun2" \
  && ${MACHINE} != "next68k" \
  && ${MACHINE} != "news68k" \
  && ${MACHINE} != "mvme68k" \
  && ${MACHINE} != "mmeye" \
  && ${MACHINE} != "mac68k" \
  && ${MACHINE} != "luna68k" \
  && ${MACHINE} != "landisk" \
  && ${MACHINE} != "hpcsh" \
  && ${MACHINE} != "hp300" \
  && ${MACHINE} != "evbsh3-sh3el" \
  && ${MACHINE} != "evbsh3-sh3eb" \
  && ${MACHINE} != "evbppc" \
  && ${MACHINE} != "dreamcast" \
  && ${MACHINE} != "cesfic" \
  && ${MACHINE} != "atari" \
  && ${MACHINE} != "amiga")
SUBDIR+=	rump
.endif

.if (${MACHINE} != "evbppc")
SUBDIR+=lkm
.endif

# Speedup stubs for some subtrees that don't need to run these rules
includes-lkm:
	@true

.include <bsd.kinc.mk>
