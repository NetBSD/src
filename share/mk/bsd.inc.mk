#	$NetBSD: bsd.inc.mk,v 1.2 1997/03/27 17:39:31 christos Exp $

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
