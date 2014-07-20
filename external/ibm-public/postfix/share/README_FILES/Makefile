#	$NetBSD: Makefile,v 1.10 2014/07/20 22:58:02 tron Exp $

.include <bsd.own.mk>

.if ${MKDOC} != "no"

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/README_FILES
.PATH:	${DIST}

.include "../readme.mk"
FILES=	AAAREADME RELEASE_NOTES ${PFIX_README_FILES}
.endif

.include <bsd.prog.mk>
