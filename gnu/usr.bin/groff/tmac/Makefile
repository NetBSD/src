#	$NetBSD: Makefile,v 1.6 1996/08/13 07:08:12 explorer Exp $

# XXX -- this depends on ../include/defs.h
TMACDIR?=	/usr/share/tmac

TMACOWN?=	bin
TMACGRP?=	bin
TMACMODE?=	444

MAN=		groff_ms.7 msafer.7
MLINKS=		groff_ms.7 ms.7

FILES=		tmac.pic tmac.ps tmac.psnew tmac.psold tmac.pspic tmac.psatk\
		tmac.dvi tmac.tty tmac.tty-char tmac.X tmac.Xps tmac.latin1\
		tmac.lj4 tmac.safer tmac.a4 man.local eqnrc troffrc

afterinstall:
	for f in ${FILES}; do \
		install -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
			${.CURDIR}/$$f ${DESTDIR}${TMACDIR}; \
	done
	install -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
		${.CURDIR}/tmac.s ${DESTDIR}${TMACDIR}/tmac.s
	install -c -o ${TMACOWN} -g ${TMACGRP} -m ${TMACMODE} \
		${.CURDIR}/tmac.an ${DESTDIR}${TMACDIR}/tmac.an.old

.include "../Makefile.cfg"
.include <bsd.prog.mk>
