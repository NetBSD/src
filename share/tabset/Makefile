#	from: @(#)Makefile	5.3 (Berkeley) 10/22/90
#	$Id: Makefile,v 1.4 1993/09/30 23:31:39 jtc Exp $

FILES=	3101 9837 aa aed512 beehive diablo dtc382 hp700-wy ibm3101 std \
	stdcrt tandem653 teleray vt100 wyse-adds xerox1720 xerox1730 \
	xerox1730-lm zenith29
NOOBJ=	noobj

all clean cleandir depend lint tags:

beforeinstall:
	@install -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/tabset

realinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/tabset

.include <bsd.prog.mk>
