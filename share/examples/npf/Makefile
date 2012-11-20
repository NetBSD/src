#	$NetBSD: Makefile,v 1.1.8.2 2012/11/20 23:13:35 matt Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
