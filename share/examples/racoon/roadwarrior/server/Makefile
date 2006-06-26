#	$NetBSD: Makefile,v 1.3 2006/06/26 19:50:57 mrg Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no" && ${MKCRYPTO} != "no"
FILES=	racoon.conf racoon.conf-radius
FILESDIR=/usr/share/examples/racoon/roadwarrior/server
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/server

.include <bsd.prog.mk>
