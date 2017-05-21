#	$NetBSD: Makefile,v 1.4 2017/05/21 15:28:42 riastradh Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	racoon.conf racoon.conf-radius
FILESDIR=/usr/share/examples/racoon/roadwarrior/server
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/server

.include <bsd.prog.mk>
