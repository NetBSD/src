#	$NetBSD: Makefile,v 1.16 1997/05/10 14:30:38 mycroft Exp $
#	@(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	strip

proginstall::	.NOTMAIN hackinstall
.PHONY:		hackinstall

.include <bsd.prog.mk>

PROGNAME?= ${PROG}
hackinstall:: ${DESTDIR}${BINDIR}/${PROGNAME}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${PROGNAME}
.endif
.if !defined(BUILD)
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROG}
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 600 ${.ALLSRC} ${.TARGET}
	-if [ "${STRIPFLAG}" != "" ]; then ./strip ${.TARGET}; fi
	chmod ${BINMODE} ${.TARGET}
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi
