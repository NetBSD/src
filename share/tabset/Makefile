#	@(#)Makefile	5.3 (Berkeley) 10/22/90

FILES=	3101 9837 aa aed512 beehive diablo dtc382 hp700-wy ibm3101 std \
	stdcrt tandem653 teleray vt100 wyse-adds xerox1720 xerox1730 \
	xerox1730-lm zenith29
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@if [ ! -d ${DESTDIR}${BINDIR}/tabset ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/tabset ; \
                mkdir -p ${DESTDIR}${BINDIR}/tabset ; \
                chown root.wheel ${DESTDIR}${BINDIR}/tabset ; \
                chmod 755 ${DESTDIR}${BINDIR}/tabset ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/tabset

.include <bsd.prog.mk>
