#	$NetBSD: Makefile,v 1.13 1996/08/23 06:14:36 mrg Exp $
#	@(#)Makefile	5.3 (Berkeley) 6/22/90

FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.kmod.mk bsd.lib.mk bsd.man.mk \
	bsd.nls.mk bsd.obj.mk bsd.own.mk bsd.prog.mk bsd.subdir.mk bsd.sys.mk \
	sys.mk
NOOBJ=	noobj

install:
	install -c -o ${BINOWN} -g ${BINGRP} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>
