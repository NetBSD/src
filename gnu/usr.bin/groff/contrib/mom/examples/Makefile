# $NetBSD: Makefile,v 1.1 2006/02/06 18:29:48 wiz Exp $

DIST_SUBDIR=    contrib/mom/examples

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=       ${DOCDIR}/groff/mom
FILES=          README.txt elvis_syntax.new penguin.ps typesetting.mom \
		elvis_syntax letter.mom sample_docs.mom

PROCESSED_FILES=	letter.ps sample_docs.ps typesetting.ps
FILES+=			${PROCESSED_FILES}
CLEANFILES+=		${PROCESSED_FILES} penguin.ps
.endif

.SUFFIXES: .mom .ps
.mom.ps:
	${TOOL_GROFF} -Tps -mom $< >$@

dependall: ${PROCESSED_FILES}
${PROCESSED_FILES}: ${.OBJDIR}/penguin.ps

${.OBJDIR}/penguin.ps: penguin.ps
	cp ${.ALLSRC} ${.TARGET}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
