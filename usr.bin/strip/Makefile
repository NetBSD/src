#	@(#)Makefile	5.4 (Berkeley) 5/11/90

PROG=	strip

realinstall:
	install ${COPY} -o ${BINOWN} -g ${BINOWN} -m ${BINMODE} strip \
	    ${DESTDIR}${BINDIR}
	./strip ${DESTDIR}${BINDIR}/strip

.include <bsd.prog.mk>
