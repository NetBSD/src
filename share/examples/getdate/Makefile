#	$NetBSD: Makefile,v 1.1 2009/05/14 02:37:36 ginsbach Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	datemsk.template posixmsk.example3 posixmsk.example4
FILESDIR=/usr/share/examples/getdate
.endif

.include <bsd.prog.mk>
