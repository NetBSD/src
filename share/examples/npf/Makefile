#	$NetBSD: Makefile,v 1.1.6.3 2014/05/22 11:37:45 yamt Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		host-npf.conf soho_gw-npf.conf hashtablefile treetablefile
FILES+=		l2tp_gw-npf.conf
FILESDIR=	/usr/share/examples/npf
.endif

.include <bsd.prog.mk>
