#	$NetBSD: Makefile,v 1.85 2020/04/04 15:43:21 jdolecek Exp $

.include <bsd.own.mk>

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netcan netipsec netinet netinet6 \
        netmpls \
	nfs opencrypto sys ufs uvm

.if make(obj) || make(cleandir) || ${MKKMOD} != "no"
SUBDIR+=modules
.endif

.if make(includes) || make(obj) || make(cleandir)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
