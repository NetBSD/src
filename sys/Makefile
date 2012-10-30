#	$NetBSD: Makefile,v 1.77.2.1 2012/10/30 17:18:35 yamt Exp $

.include <bsd.own.mk>

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netmpls netnatm netsmb \
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
