#	$NetBSD: Makefile,v 1.3 2017/05/21 15:28:41 riastradh Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

SUBDIR= client server

.if ${MKSHARE} != "no"
FILES=	README
FILESDIR=/usr/share/examples/racoon/roadwarrior
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples/roadwarrior

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
