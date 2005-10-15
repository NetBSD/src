#	$NetBSD: Makefile,v 1.1.6.2 2005/10/15 21:03:10 riz Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	gen_md5 gen_sha1 gen_rmd160 fpgen.sh
FILESDIR=/usr/share/examples/veriexecctl
.endif

.include <bsd.prog.mk>
