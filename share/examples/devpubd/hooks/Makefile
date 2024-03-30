#	$NetBSD: Makefile,v 1.2 2024/03/30 06:42:10 thorpej Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	99-ucom-symlinks
FILES+=	99-ugen-perms-tigard
FILESDIR=/usr/share/examples/devpubd/hooks
.endif

.include <bsd.prog.mk>
