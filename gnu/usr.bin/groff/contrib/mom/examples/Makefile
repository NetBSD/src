# $NetBSD: Makefile,v 1.8 2010/01/26 15:11:56 wiz Exp $

DIST_SUBDIR=	contrib/mom/examples

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${DOCDIR}/groff/mom
MOM_FILES=	typesetting.mom letter.mom sample_docs.mom
FILES=		README.txt elvis_syntax elvis_syntax.new penguin.ps ${MOM_FILES}
.endif	# MKSHARE

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
