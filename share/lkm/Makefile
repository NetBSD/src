#
#	$NetBSD: Makefile,v 1.15 1999/02/13 02:54:35 lukem Exp $
#

FILES=	README syscall vfs misc
MKOBJ=	no

install:
	find ${FILES} -follow -name CVS -prune -o -type f -print | \
	    cpio -pdumL ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
	find ${DESTDIR}${BINDIR}/lkm -type f | xargs chmod a=rX
	find ${DESTDIR}${BINDIR}/lkm -type d | xargs chmod u=rwX,go=rX

.include <bsd.prog.mk>
