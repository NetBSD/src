#	from: @(#)Makefile	6.11 (Berkeley) 7/1/90
#	$Id: Makefile,v 1.4 1993/09/30 23:04:06 jtc Exp $

MELIB=	${BINDIR}/me
TMLIB=	${BINDIR}/tmac
MESRCS=	acm.me chars.me deltext.me eqn.me float.me footnote.me \
	index.me letterhead.me local.me null.me refer.me sh.me \
	tbl.me thesis.me
TMSRCS=	tmac.e
MAN7=	me.0

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${MELIB}
	@install -d -o root -g wheel -m 755 ${DESTDIR}${TMLIB}

realinstall:
	cd ${.CURDIR}; for i in ${MESRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${MELIB}/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${MELIB}/$$i; \
		chmod 444 ${DESTDIR}${MELIB}/$$i; \
	done
	cd ${.CURDIR}; for i in ${TMSRCS}; do \
		sed -f strip.sed < $$i > ${DESTDIR}${TMLIB}/$$i; \
		chown ${BINOWN}.${BINGRP} ${DESTDIR}${TMLIB}/$$i; \
		chmod 444 ${DESTDIR}${TMLIB}/$$i; \
	done
	cd ${.CURDIR}; install -c -o ${BINOWN} -g ${BINGRP} -m 444 revisions \
	    ${DESTDIR}${MELIB}/revisions

.include <bsd.prog.mk>
