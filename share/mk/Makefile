#	from: @(#)Makefile	5.3 (Berkeley) 6/22/90
#	$Id: Makefile,v 1.9 1994/06/02 23:40:16 jtc Exp $

FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.lib.mk bsd.man.mk bsd.obj.mk \
	bsd.own.mk bsd.prog.mk bsd.subdir.mk sys.mk
NOOBJ=	noobj

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>
