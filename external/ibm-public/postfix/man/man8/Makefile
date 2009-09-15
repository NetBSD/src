#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:17 snj Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/man/man8
.PATH:	${DIST}

MAN=	anvil.8 bounce.8 cleanup.8 discard.8 error.8 local.8 \
	master.8 pickup.8 pipe.8 qmgr.8 \
	scache.8 showq.8 smtp.8 smtpd.8 spawn.8 trivial-rewrite.8 \
	oqmgr.8 spawn.8 flush.8 virtual.8 \
	verify.8 proxymap.8

.if ${MKCRYPTO} != "no"
MAN+=	tlsmgr.8
.endif

MLINKS=	bounce.8 defer.8
MLINKS+=bounce.8 trace.8
MLINKS+=smtp.8 lmtp.8

# XXX notyet:
# qmqpd.8
# We're not building it, yet.

.include <bsd.man.mk>
