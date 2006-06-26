#	$NetBSD: Makefile,v 1.2 2006/06/26 19:50:57 mrg Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

SUBDIR= client server

.if ${MKSHARE} != "no" && ${MKCRYPTO} != "no"
FILES=	README
FILESDIR=/usr/share/examples/racoon/roadwarrior
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
