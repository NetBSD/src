#	from: @(#)Makefile	5.3 (Berkeley) 6/22/90
#	$Id: Makefile,v 1.6 1993/08/15 21:09:20 mycroft Exp $

FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.lib.mk bsd.man.mk bsd.obj.mk \
	bsd.own.mk bsd.prog.mk bsd.subdir.mk sys.mk
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@install -d -o root -g wheel -m 755 ${DESTDIR}${BINDIR}/mk
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>
