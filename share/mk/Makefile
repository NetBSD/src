#	$NetBSD: Makefile,v 1.14 1996/10/18 05:58:40 thorpej Exp $
#	@(#)Makefile	5.3 (Berkeley) 6/22/90

FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.kmod.mk bsd.lib.mk bsd.man.mk \
	bsd.nls.mk bsd.obj.mk bsd.own.mk bsd.prog.mk bsd.subdir.mk bsd.sys.mk \
	sys.mk
NOOBJ=	noobj

install:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>
