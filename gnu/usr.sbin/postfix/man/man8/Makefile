#	$NetBSD: Makefile,v 1.8 2004/09/04 22:28:26 lukem Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/man/man8
.PATH:	${DIST}

MAN=	bounce.8 cleanup.8 error.8 flush.8 lmtp.8 local.8 \
	master.8 oqmgr.8 pickup.8 pipe.8 proxymap.8 qmgr.8 \
	showq.8 smtp.8 smtpd.8 spawn.8 trivial-rewrite.8 \
	verify.8 virtual.8

MLINKS=	bounce.8 defer.8
MLINKS+=bounce.8 trace.8

# XXX notyet:
# qmqpd.8
#	We're not building it, yet.

.include <bsd.man.mk>
