#	from: @(#)Makefile	6.13 (Berkeley) 3/20/91
#	$NetBSD: Makefile,v 1.14 1996/10/18 06:00:31 thorpej Exp $

BINDIR= /usr/share
FILES=	tmac.andoc
MSRCS=  doc doc-ditroff doc-common doc-nroff doc-syms
LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
NOOBJ=	noobj

STRIP_SED = ../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

all clean cleandir depend lint tags:

realinstall:
	for i in ${MSRCS}; do \
		rm -f ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		sed -f ${STRIP_SED} < $$i > ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chown ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chmod 444 ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
	done
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
		${DESTDIR}${BINDIR}/tmac

.include <bsd.prog.mk>
