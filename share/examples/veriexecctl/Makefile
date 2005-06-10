#	$NetBSD: Makefile,v 1.1.6.1 2005/06/10 14:50:42 tron Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	gen_md5 gen_sha1 gen_rmd160
FILESDIR=/usr/share/examples/veriexecctl
.endif

.include <bsd.prog.mk>
