# $NetBSD: Makefile,v 1.1.4.2 2025/11/24 16:48:29 martin Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		xorg.conf
FILESDIR=	${X11ROOTDIR}/share/examples/xorg
.endif

.include <bsd.prog.mk>
