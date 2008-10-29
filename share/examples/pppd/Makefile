#	$NetBSD: Makefile,v 1.2 2008/10/29 12:47:08 jmcneill Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	cingular3G cingular3G.chat
FILES+=	telus telus.chat
FILESDIR=/usr/share/examples/pppd
.endif

.include <bsd.prog.mk>
