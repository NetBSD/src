#	$NetBSD: Makefile,v 1.1 2012/08/22 06:45:16 spz Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
