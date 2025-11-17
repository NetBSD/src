# $NetBSD: Makefile,v 1.1 2025/11/17 00:14:23 nia Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		xorg.conf
FILESDIR=	${X11ROOTDIR}/share/examples/xorg
.endif

.include <bsd.prog.mk>
