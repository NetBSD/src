#	from: @(#)Makefile	5.14 (Berkeley) 9/15/92
#	$Id: Makefile,v 1.11 1993/09/30 23:31:43 jtc Exp $

# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#
MAN5=	termcap.0

CLEANFILES= termcap termcap.db
all: termcap termcap.db

termcap: reorder termcap.src
#	ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null
	-ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null

termcap.db: termcap
	cap_mkdb termcap

beforeinstall:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/misc 

realinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 termcap \
	     ${DESTDIR}${BINDIR}/misc
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 termcap.db \
	     ${DESTDIR}${BINDIR}/misc
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/map3270 \
	     ${DESTDIR}${BINDIR}/misc

	
.include <bsd.prog.mk>
