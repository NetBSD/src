#	from: @(#)Makefile	6.13 (Berkeley) 3/20/91
#	$Id: Makefile,v 1.8 1993/09/30 22:16:52 jtc Exp $

BINDIR= /usr/share
FILES=	tmac.andoc
MSRCS=  doc doc-ditroff doc-common doc-nroff doc-syms
LINKS=	${BINDIR}/tmac/tmac.andoc ${BINDIR}/tmac/tmac.an
NOOBJ=	noobj

STRIP_SED = ../me/strip.sed

# old macros not currently installed...
OFILES=	tmac.an.v6compat tmac.an6n tmac.an6t tmac.a tmac.cp tmac.doc.old

all clean cleandir depend lint tags:

beforeinstall:
	@if [ ! -d ${DESTDIR}${BINDIR}/tmac ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/tmac ; \
                mkdir -p ${DESTDIR}${BINDIR}/tmac ; \
                chown root.wheel ${DESTDIR}${BINDIR}/tmac ; \
                chmod 755 ${DESTDIR}${BINDIR}/tmac ; \
        else \
                true ; \
        fi
	for i in ${MSRCS}; do \
		sed -f ${STRIP_SED} < $$i > ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
		chmod 444 ${DESTDIR}${BINDIR}/tmac/tmac.$$i; \
	done
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
		${DESTDIR}${BINDIR}/tmac

.include <bsd.prog.mk>
