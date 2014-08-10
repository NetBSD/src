# $NetBSD: Makefile,v 1.8.22.1 2014/08/10 06:51:36 tls Exp $

DIST_SUBDIR=	contrib/mom/examples

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	/usr/share/doc/reference/ref1/roff/mom_examples
MOM_FILES=	typesetting.mom letter.mom sample_docs.mom
FILES=		README.txt elvis_syntax elvis_syntax.new penguin.ps ${MOM_FILES}
.endif	# MKSHARE

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
