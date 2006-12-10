#	$NetBSD: Makefile,v 1.3 2006/12/10 05:52:10 manu Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

SUBDIR= roadwarrior

.if ${MKSHARE} != "no" && ${MKCRYPTO} != "no"
FILES=	psk.txt.sample racoon.conf.sample 			\
	racoon.conf.sample-gssapi racoon.conf.sample-inherit	\
	racoon.conf.sample-natt	racoon.conf.sample-plainrsa	\
	FAQ README.certificate README.gssapi README.plainrsa
FILESDIR=/usr/share/examples/racoon
.endif

DIST= ${NETBSDSRCDIR}/crypto/dist/ipsec-tools
.PATH:	${DIST}/src/racoon/samples ${DIST}/src/racoon/doc

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
