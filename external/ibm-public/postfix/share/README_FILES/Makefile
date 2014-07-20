#	$NetBSD: Makefile,v 1.8 2014/07/20 15:58:06 tron Exp $

.include <bsd.own.mk>

.if ${MKDOC} != "no"

BINDIR=	${PFIX_EXAMPLEDIR}

DIST=	${NETBSDSRCDIR}/external/ibm-public/postfix/dist/README_FILES
.PATH:	${DIST}

.include "../readme.mk"
FILES=	${PFIX_README_FILES}
.endif

.include <bsd.prog.mk>
