# $NetBSD: Makefile,v 1.1 2009/10/21 18:04:52 joerg Exp $

MAN=		mandoc_man.7 mandoc_char.7 mandoc_mdoc.7 manuals.7

.include <bsd.init.mk>

mandoc_man.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_CAT} ${DISTDIR}/man.7 > ${.TARGET}

mandoc_mdoc.7:
	${_MKTARGET_CREATE}
	rm -f ${.TARGET}
	${TOOL_CAT} ${DISTDIR}/mdoc.7 > ${.TARGET}

CLEANFILES+=	mandoc_man.7 mandoc_mdoc.7

.include <bsd.man.mk>
