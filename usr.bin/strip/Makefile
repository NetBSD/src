#	from: @(#)Makefile	5.4 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.6 1993/08/07 04:44:25 mycroft Exp $

PROG=	strip

install: maninstall
	install -c -o ${BINOWN} -g ${BINOWN} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip
	-[ "${COPY}" = "" ] && rm -f ./strip

.include <bsd.prog.mk>
