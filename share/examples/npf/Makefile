#	$NetBSD: Makefile,v 1.2 2013/06/26 21:33:56 christos Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILES+=		l2tp-gw.conf
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
