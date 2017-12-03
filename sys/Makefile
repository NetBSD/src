#	$NetBSD: Makefile,v 1.78.2.2 2017/12/03 11:35:43 jdolecek Exp $

.include <bsd.own.mk>

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netcan netipsec netinet netinet6 \
        netisdn netkey netmpls netnatm netsmb \
	nfs opencrypto sys ufs uvm

# interrupt implementation depends on the kernel within the port
#.if (${MACHINE} != "evbppc")
.if make(obj) || make(cleandir) || ${MKKMOD} != "no"
SUBDIR+=modules
.endif
#.endif

.if make(includes) || make(obj) || make(cleandir)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
