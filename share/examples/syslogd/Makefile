#	$NetBSD: Makefile,v 1.4 2001/12/18 01:44:36 mrg Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	all.debug all.newsyslog.conf
FILESDIR=/usr/share/examples/syslogd
.endif

.include <bsd.prog.mk>
