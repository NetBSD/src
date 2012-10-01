#	$NetBSD: Makefile,v 1.1.4.2 2012/10/01 20:15:34 riz Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
