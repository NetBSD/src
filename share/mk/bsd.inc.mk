#	$NetBSD: bsd.inc.mk,v 1.3 1997/05/06 20:54:33 mycroft Exp $

.PHONY:		inclinstall

.if defined(INCS) && !empty(INCS)
.for I in ${INCS}
inclinstall:: ${DESTDIR}${INCSDIR}/$I

${DESTDIR}${INCSDIR}/$I: $I
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE} \
		${.ALLSRC} ${.TARGET})
.endfor
includes: inclinstall
.endif
