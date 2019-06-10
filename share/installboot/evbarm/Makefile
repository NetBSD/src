#	$NetBSD: Makefile,v 1.1.2.2 2019/06/10 22:05:39 christos Exp $

NOOBJ=		# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=		boards.plist
FILESDIR=	/usr/share/installboot/evbarm
.endif

.include <bsd.prog.mk>
