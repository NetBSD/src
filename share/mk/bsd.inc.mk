#	$NetBSD: bsd.inc.mk,v 1.12.4.1 1999/08/10 00:43:36 mcr Exp $

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
.if ${MORTALINSTALL} != "no"
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${PRESERVE} -c \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} ${PRESERVE} -c \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET})
.else
	@cmp -s ${.ALLSRC} ${.TARGET} > /dev/null 2>&1 || \
	    (echo "${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET}" && \
	     ${INSTALL} ${PRESERVE} -c -o ${BINOWN} -g ${BINGRP} \
		-m ${NONBINMODE} ${.ALLSRC} ${.TARGET})
.endif
.endfor
.endif

.if !target(incinstall)
incinstall::
.endif
