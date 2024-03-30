#	$NetBSD: Makefile,v 1.1 2024/03/30 06:29:01 thorpej Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	99-ucom-symlinks
FILESDIR=/usr/share/examples/devpubd/hooks
.endif

.include <bsd.prog.mk>
