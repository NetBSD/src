#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.8 1993/10/23 00:23:07 cgd Exp $

PROG=	strip

realinstall:
	install -c -o ${BINOWN} -g ${BINOWN} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	-if [ "${COPY}" = "" ]; then rm -f ./strip; fi

.include <bsd.prog.mk>
