#	$NetBSD: Makefile,v 1.6 2004/04/27 05:34:34 heas Exp $

.include <bsd.own.mk>

DIST=	${NETBSDSRCDIR}/gnu/dist/postfix/man/man8
.PATH:	${DIST}

MAN=	bounce.8 cleanup.8 error.8 local.8 \
	lmtp.8 master.8 pickup.8 pipe.8 qmgr.8 \
	showq.8 smtp.8 smtpd.8 trivial-rewrite.8 \
	oqmgr.8 spawn.8 flush.8 virtual.8 \
	verify.8 trace.8 proxymap.8

MLINKS=	bounce.8 defer.8
MLINKS+=bounce.8 trace.8

# XXX notyet:
# qmqpd.8
# We're not building it, yet.

.include <bsd.man.mk>
