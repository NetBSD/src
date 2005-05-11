#	$NetBSD: Makefile,v 1.1.2.1 2005/05/11 17:52:03 tron Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	racoon.conf racoon.conf-radius
FILESDIR=/usr/share/examples/racoon/roadwarrior/server
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/server

.include <bsd.prog.mk>
