#	@(#)Makefile	5.14 (Berkeley) 9/15/92

# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#
MAN5=	termcap.0

all: termcap termcap.db

termcap: reorder termcap.src
#	ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null
	-ex - ${.CURDIR}/termcap.src < ${.CURDIR}/reorder > /dev/null

termcap.db: termcap
	cap_mkdb termcap

clean:
	rm -f termcap termcap.db

beforeinstall:
	@if [ ! -d ${DESTDIR}${BINDIR}/misc ]; then \
		/bin/rm -f ${DESTDIR}${BINDIR}/misc ; \
		mkdir -p ${DESTDIR}${BINDIR}/misc ; \
		chown root.wheel ${DESTDIR}${BINDIR}/misc ; \
		chmod 755 ${DESTDIR}${BINDIR}/misc ; \
	else \
		true ; \
	fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 termcap \
	     ${DESTDIR}${BINDIR}/misc
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 termcap.db \
	     ${DESTDIR}${BINDIR}/misc
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${.CURDIR}/map3270 \
	     ${DESTDIR}${BINDIR}/misc

	
.include <bsd.prog.mk>
