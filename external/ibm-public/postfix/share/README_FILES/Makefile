#	$NetBSD: Makefile,v 1.2.10.2 2014/08/19 23:59:46 tls Exp $

.include <bsd.own.mk>

.if ${MKDOC} != "no"

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/README_FILES
.PATH:	${DIST}

.include "../readme.mk"
FILES=	AAAREADME RELEASE_NOTES ${PFIX_README_FILES}
.endif

.include <bsd.prog.mk>
