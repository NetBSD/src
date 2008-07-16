# $NetBSD: Makefile,v 1.6 2008/07/16 16:34:36 perry Exp $

DIST_SUBDIR=	contrib/mom/examples

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR=	${DOCDIR}/groff/mom
MOM_FILES=	typesetting.mom letter.mom sample_docs.mom
FILES=		README.txt elvis_syntax elvis_syntax.new penguin.ps ${MOM_FILES}

.if ${MKMAN} != "no"
PROCESSED_FILES=	${MOM_FILES:S/.mom/.ps/}
FILES+=			${PROCESSED_FILES}
CLEANFILES+=		${PROCESSED_FILES}

.for i in ${PROCESSED_FILES}
$i: ${i:S/.ps/.mom/}
	${TOOL_GROFF} -I${FILESDIR} -Tps -mom ${.ALLSRC} | \
		${TOOL_SED} 's/^\(%%CreationDate:\).*/\1 unknown/' >${.TARGET}
.endfor

dependall: ${PROCESSED_FILES}
.endif	# MKMAN
.endif	# MKSHARE

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
