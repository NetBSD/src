#	$NetBSD: Makefile,v 1.2 2005/04/21 09:57:03 xtraeme Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	gen_md5 gen_sha1 gen_rmd160
FILESDIR=/usr/share/examples/veriexecctl
.endif

.include <bsd.prog.mk>
