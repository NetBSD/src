#	from: @(#)Makefile	5.2 (Berkeley) 5/6/91
#	$Id: Makefile,v 1.7 1993/09/30 23:57:08 jtc Exp $

#
# set `OBSCENE' to "real" to have obscene fortunes installed,
# fake to have a note stating that they're not installed
# isntalled in their place
#
OBSCENE=real

NORMDAT= fortunes.dat startrek.dat zippy.dat
OBSDAT=	fortunes-o.dat
DATFILES= ${NORMDAT} ${OBSDAT}

CLEANFILES+=${DATFILES}

all: ${DATFILES}

install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}/usr/share/games/fortune
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${DATFILES} \
	    ${DESTDIR}/usr/share/games/fortune
	(cd ${.CURDIR} && install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${NORMDAT:R} ${DESTDIR}/usr/share/games/fortune)
	(cd ${.CURDIR} && install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${OBSDAT:R}.${OBSCENE}.rot13 \
		${DESTDIR}/usr/share/games/fortune/${OBSDAT:R})

fortunes-o.dat: ${.TARGET:R}.${OBSCENE}.rot13
	${.CURDIR}/../strfile/obj/strfile -rsx \
	    ${.CURDIR}/${.TARGET:R}.${OBSCENE}.rot13 ${.TARGET}

fortunes.dat startrek.dat zippy.dat: ${.TARGET:R}
	${.CURDIR}/../strfile/obj/strfile -rs \
	    ${.CURDIR}/${.TARGET:R} ${.TARGET}

.include <bsd.prog.mk>
