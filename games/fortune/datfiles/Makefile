#	from: @(#)Makefile	5.2 (Berkeley) 5/6/91
#	$Id: Makefile,v 1.10 1994/02/10 02:31:32 cgd Exp $

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

STRFILE!=cd $(.CURDIR)/../strfile; \
	printf "xxx:\n\techo \$${.OBJDIR}/strfile\n" | ${MAKE} -r -s -f - xxx

all: ${DATFILES}

install:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m 444 ${DATFILES} \
	    ${DESTDIR}/usr/share/games/fortune
	(cd ${.CURDIR} && install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${NORMDAT:R} ${DESTDIR}/usr/share/games/fortune)
	(cd ${.CURDIR} && install -c -o ${BINOWN} -g ${BINGRP} -m 444 \
		${OBSDAT:R}.${OBSCENE}.rot13 \
		${DESTDIR}/usr/share/games/fortune/${OBSDAT:R})

fortunes-o.dat: ${.TARGET:R}.${OBSCENE}.rot13
	${STRFILE} -rsx ${.CURDIR}/${.TARGET:R}.${OBSCENE}.rot13 ${.TARGET}

fortunes.dat startrek.dat zippy.dat: ${.TARGET:R}
	${STRFILE} -rs ${.CURDIR}/${.TARGET:R} ${.TARGET}

.include <bsd.prog.mk>
