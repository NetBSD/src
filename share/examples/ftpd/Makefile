#	$NetBSD: Makefile,v 1.6 2001/12/12 12:24:22 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	ftpd.conf ftpusers
FILESDIR=/usr/share/examples/ftpd
.endif

.include <bsd.prog.mk>
