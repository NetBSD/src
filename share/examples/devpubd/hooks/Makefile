#	$NetBSD: Makefile,v 1.3 2024/03/30 16:47:55 thorpej Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	99-ucom-symlinks
FILES+=	99-ugen-perms-minipro
FILES+=	99-ugen-perms-tigard
FILESDIR=/usr/share/examples/devpubd/hooks
.endif

.include <bsd.prog.mk>
