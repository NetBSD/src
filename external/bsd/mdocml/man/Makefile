# $NetBSD: Makefile,v 1.3 2010/06/01 23:29:10 joerg Exp $

MAN=		mandoc_man.7 mandoc_char.7 mandoc_mdoc.7 mandoc_roff.7 \
		manuals.7

.include <bsd.init.mk>

mandoc_man.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_CAT} ${DISTDIR}/man.7 > ${.TARGET}

mandoc_mdoc.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_CAT} ${DISTDIR}/mdoc.7 > ${.TARGET}

mandoc_roff.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_CAT} ${DISTDIR}/roff.7 > ${.TARGET}

CLEANFILES+=	mandoc_man.7 mandoc_mdoc.7

.if (${MKHTML} != "no") && (${MKMAN} != "no")
FILES=		${DISTDIR}/style.css
FILESDIR=	/usr/share/man
.endif

.include <bsd.man.mk>
