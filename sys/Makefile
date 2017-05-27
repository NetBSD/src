#	$NetBSD: Makefile,v 1.80 2017/05/27 21:02:55 bouyer Exp $

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
