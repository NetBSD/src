#	from: @(#)Makefile	5.14 (Berkeley) 9/15/92
#	$Id: Makefile,v 1.18 1996/10/18 06:00:06 thorpej Exp $

MAN=	termcap.5

CLEANFILES= termcap.db
all: termcap.db

termcap.db: termcap.src
	cap_mkdb -f termcap ${.ALLSRC}

realinstall:
	${INSTALL} ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 termcap.db \
	     ${DESTDIR}${BINDIR}/misc/termcap.db
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/termcap.src \
	     ${DESTDIR}${BINDIR}/misc/termcap
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/map3270 \
	     ${DESTDIR}${BINDIR}/misc/map3270


.include <bsd.prog.mk>
