#	$NetBSD: Makefile,v 1.3 2010/08/07 20:47:27 christos Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	cingular3G cingular3G.chat
FILES+=	cosmote3G cosmote3G.chat
FILES+=	telus telus.chat
FILESDIR=/usr/share/examples/pppd
.endif

.include <bsd.prog.mk>
