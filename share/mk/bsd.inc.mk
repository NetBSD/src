#	$NetBSD: bsd.inc.mk,v 1.4 1997/05/06 21:29:36 mycroft Exp $

.PHONY:		inclinstall

.if defined(INCS) && !empty(INCS)
.for I in ${INCS}
inclinstall:: ${DESTDIR}${INCSDIR}/$I

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
${DESTDIR}${INCSDIR}/$I: $I
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET})
.endfor
includes: inclinstall
.endif
