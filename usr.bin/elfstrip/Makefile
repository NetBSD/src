#	$NetBSD: Makefile,v 1.5 1997/05/07 16:06:01 mycroft Exp $
#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	elfstrip

.include <bsd.prog.mk>

proginstall:: ${DESTDIR}${BINDIR}/${PROG}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${PROG}
.endif
.if !defined(UPDATE)
${DESTDIR}${BINDIR}/${PROG}: .MADE
.endif

${DESTDIR}${BINDIR}/${PROG}: ${PROG}
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${.ALLSRC} ${.TARGET}
	./strip ${.TARGET}
	chmod ${BINMODE} ${.TARGET}
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi
