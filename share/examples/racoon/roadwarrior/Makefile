#	$NetBSD: Makefile,v 1.1 2005/02/19 16:55:05 thorpej Exp $

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
