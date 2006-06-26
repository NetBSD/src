#	$NetBSD: Makefile,v 1.2 2006/06/26 19:50:57 mrg Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no" && ${MKCRYPTO} != "no"
FILES=	phase1-down.sh phase1-up.sh racoon.conf
FILESDIR=/usr/share/examples/racoon/roadwarrior/client
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/client

.include <bsd.prog.mk>
