#	$NetBSD: Makefile,v 1.75.4.1 2010/08/17 06:43:48 uebayasi Exp $

SUBDIR=	altq arch compat dev fs miscfs \
	net net80211 netatalk netbt netipsec netinet netinet6 \
        netisdn netiso netkey netmpls netnatm netsmb \
	nfs opencrypto sys ufs uvm

# interrupt implementation depends on the kernel within the port
.if (${MACHINE} != "evbppc")
.if make(obj) || make(cleandir)
SUBDIR+=modules
.endif
.endif

.if make(includes) || make(obj) || make(cleandir)
SUBDIR+= rump
.endif

.include <bsd.kinc.mk>
