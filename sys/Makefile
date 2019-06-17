#	$NetBSD: Makefile,v 1.84 2019/06/17 17:01:50 sevan Exp $

.include <bsd.own.mk>

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netcan netipsec netinet netinet6 \
        netmpls netsmb \
	nfs opencrypto sys ufs uvm

.if make(obj) || make(cleandir) || ${MKKMOD} != "no"
SUBDIR+=modules
.endif

.if make(includes) || make(obj) || make(cleandir)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
