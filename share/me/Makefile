#	@(#)Makefile	8.2 (Berkeley) 3/27/94
#	$NetBSD: Makefile,v 1.20 2002/03/16 10:19:58 tron Exp $

MELIB=	${BINDIR}/me
TMLIB=	${BINDIR}/tmac
MESRCS=	acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me letterhead.me local.me null.me refer.me sh.me \
	tbl.me thesis.me
TMSRCS=	tmac.e
MAN=	me.7

.include <bsd.prog.mk>

.for M in ${MESRCS}

proginstall:: ${DESTDIR}${MELIB}/${M}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${MELIB}/${M}
.endif
.if !defined(BUILD)
${DESTDIR}${MELIB}/${M}: .MADE
.endif

${DESTDIR}${MELIB}/${M}: ${M}
	@rm -f ${.TARGET} ${M}.tmp
	sed -f ${.CURDIR}/strip.sed < ${.ALLSRC} > ${M}.tmp
	${INSTALL_FILE:N-c} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${M}.tmp ${.TARGET}
.endfor

.for M in ${TMSRCS}

proginstall:: ${DESTDIR}${TMLIB}/${M}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${TMLIB}/${M}
.endif
.if !defined(BUILD)
${DESTDIR}${TMLIB}/${M}: .MADE
.endif

${DESTDIR}${TMLIB}/${M}: ${M}
	@rm -f ${.TARGET} ${M}.tmp
	sed -f ${.CURDIR}/strip.sed < ${.ALLSRC} > ${M}.tmp
	${INSTALL_FILE:N-c} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${M}.tmp ${.TARGET}
.endfor

proginstall:: ${DESTDIR}${MELIB}/revisions
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${MELIB}/revisions
.endif
.if !defined(BUILD)
${DESTDIR}${MELIB}/revisions: .MADE
.endif

${DESTDIR}${MELIB}/revisions: revisions
	${INSTALL_FILE:N-c} -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
	    ${.ALLSRC} ${.TARGET}

