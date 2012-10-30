#	$NetBSD: Makefile,v 1.1.6.2 2012/10/30 18:59:34 yamt Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
