#	$NetBSD: bsd.inc.mk,v 1.1 1997/03/24 21:54:15 christos Exp $

.if !target(includes)
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
.else
includes: 
.endif
.endif
