#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.7 1993/08/10 00:50:50 mycroft Exp $

PROG=	strip

realinstall:
	install -c -o ${BINOWN} -g ${BINOWN} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	-[ "${COPY}" = "" ] && rm -f ./strip

.include <bsd.prog.mk>
