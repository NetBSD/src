#	$NetBSD: Makefile,v 1.3 2005/10/13 13:49:47 elad Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	gen_md5 gen_sha1 gen_rmd160 fpgen.sh
FILESDIR=/usr/share/examples/veriexecctl
.endif

.include <bsd.prog.mk>
