#	$NetBSD: Makefile,v 1.1 2002/10/29 14:05:33 blymn Exp $

NOOBJ=	# defined

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILES=	gen_md5 gen_sha1
FILESDIR=/usr/share/examples/verifiedexec_load
.endif

.include <bsd.prog.mk>
