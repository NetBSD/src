# $NetBSD: Makefile,v 1.3 2006/02/12 00:18:30 christos Exp $

DIST_SUBDIR=    contrib/mom/examples

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=       ${DOCDIR}/groff/mom
MOM_FILES=	typesetting.mom letter.mom sample_docs.mom
FILES=          README.txt elvis_syntax.new penguin.ps ${MOM_FILES}

PROCESSED_FILES= ${MOM_FILES:S/.mom/.ps/}
FILES+=			${PROCESSED_FILES}
CLEANFILES+=		${PROCESSED_FILES}

.for i in ${PROCESSED_FILES}
$i: ${i:S/.ps/.mom/}
	${TOOL_GROFF} -I${FILESDIR} -Tps -mom ${.ALLSRC} >${.TARGET}
.endfor

dependall: ${PROCESSED_FILES}

.endif

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
