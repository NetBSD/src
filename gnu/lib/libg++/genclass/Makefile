#	%W% (Berkeley) %G%

BINDIR=	/usr/bin

MAN1=	genclass.0

all genclass:

clean cleandir depend lint tags:

beforeinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
	    ${.CURDIR}/genclass.sh ${DESTDIR}${BINDIR}/genclass

.include <bsd.prog.mk>
