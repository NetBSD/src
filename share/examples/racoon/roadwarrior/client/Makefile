#	$NetBSD: Makefile,v 1.3 2017/05/21 15:28:42 riastradh Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	phase1-down.sh phase1-up.sh racoon.conf
FILESDIR=/usr/share/examples/racoon/roadwarrior/client
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior/client

.include <bsd.prog.mk>
