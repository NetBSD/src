#	$NetBSD: Makefile,v 1.5 2001/12/12 12:24:21 lukem Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	SVR4_MAKEDEV netconfig nsswitch.conf
FILESDIR=/usr/share/examples/emul/svr4/etc
.endif

.include <bsd.prog.mk>
