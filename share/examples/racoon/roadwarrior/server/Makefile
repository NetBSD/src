#	$NetBSD: Makefile,v 1.2 2005/05/10 10:23:13 manu Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	racoon.conf racoon.conf-radius
FILESDIR=/usr/share/examples/racoon/roadwarrior/server
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/server

.include <bsd.prog.mk>
