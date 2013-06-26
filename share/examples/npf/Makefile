#	$NetBSD: Makefile,v 1.3 2013/06/26 21:36:39 christos Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILES+=		l2tp_gw-npf.conf
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
