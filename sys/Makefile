#	$NetBSD: Makefile,v 1.86 2020/05/18 21:19:35 jmcneill Exp $

.include <bsd.own.mk>

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netcan netipsec netinet netinet6 \
        netmpls \
	nfs opencrypto sys ufs uvm

.if make(obj) || make(cleandir) || ${MKDTB} != "no"
SUBDIR+=dtb
.endif

.if make(obj) || make(cleandir) || ${MKKMOD} != "no"
SUBDIR+=modules
.endif

.if make(includes) || make(obj) || make(cleandir)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
