#	from: @(#)Makefile	6.11 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.10 1997/03/26 23:11:15 thorpej Exp $

MELIB=	${BINDIR}/me
TMLIB=	${BINDIR}/tmac
MESRCS=	acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me letterhead.me local.me null.me refer.me sh.me \
	tbl.me thesis.me
TMSRCS=	tmac.e
MAN=	me.7

realinstall: proginstall

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
	@rm -f ${.TARGET}
	sed -f ${.CURDIR}/strip.sed < ${.ALLSRC} > ${.TARGET}
	@chown ${BINOWN}:${BINGRP} ${.TARGET}
	@chmod 444 ${.TARGET}
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
	@rm -f ${.TARGET}
	sed -f ${.CURDIR}/strip.sed < ${.ALLSRC} > ${.TARGET}
	@chown ${BINOWN}:${BINGRP} ${.TARGET}
	@chmod 444 ${.TARGET}
.endfor

proginstall:: ${DESTDIR}${MELIB}/revisions
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${MELIB}/revisions
.endif
.if !defined(BUILD)
${DESTDIR}${MELIB}/revisions: .MADE
.endif

${DESTDIR}${MELIB}/revisions: revisions
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.ALLSRC} ${.TARGET}

