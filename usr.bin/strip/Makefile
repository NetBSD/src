#	$NetBSD: Makefile,v 1.17 1997/10/18 15:04:56 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

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
