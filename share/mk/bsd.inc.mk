#	$NetBSD: bsd.inc.mk,v 1.9 1997/05/31 21:21:56 cjs Exp $

.PHONY:		incinstall
includes:	${INCS} incinstall

.if defined(INCS)
.for I in ${INCS}
incinstall:: ${DESTDIR}${INCSDIR}/$I

.PRECIOUS: ${DESTDIR}${INCSDIR}/$I
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
