#	$NetBSD: bsd.inc.mk,v 1.10 1997/06/03 16:00:30 cgd Exp $

.PHONY:		incinstall
includes:	${INCS} incinstall

.if defined(INCS)
.for I in ${INCS}
incinstall:: ${DESTDIR}${INCSDIR}/$I

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${INCSDIR}/$I
.endif
${DESTDIR}${INCSDIR}/$I: $I
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET})
.endfor
.endif

.if !target(incinstall)
incinstall::
.endif
