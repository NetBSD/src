#	@(#)Makefile	5.2 (Berkeley) 5/6/91

#
# set `OBSCENE' to "real" to have obscene fortunes installed,
# fake to have a note stating that they're not installed
# isntalled in their place
#
OBSCENE=real

DATFILES=fortunes.dat startrek.dat zippy.dat fortunes-o.dat
CLEANFILES+=${DATFILES}

install: ${DATFILES}
	install -o ${BINOWN} -g ${BINGRP} -m 444 ${DATFILES} \
	    ${DESTDIR}/usr/share/games/fortune

fortunes-o.dat: ${.TARGET:R}.${OBSCENE}.rot13
	${.CURDIR}/../strfile/obj/strfile -rsx \
	    ${.CURDIR}/${.TARGET:R}.${OBSCENE}.rot13 ${.TARGET}

fortunes.dat startrek.dat zippy.dat: ${.TARGET:R}
	${.CURDIR}/../strfile/obj/strfile -rs \
	    ${.CURDIR}/${.TARGET:R} ${.TARGET}

.include <bsd.prog.mk>
