#
#	$NetBSD: Makefile,v 1.16 1999/02/14 10:53:06 lukem Exp $
#

STUFF=	README syscall vfs misc
MKOBJ=	no

install:
	find ${STUFF} -follow -name CVS -prune -o -type f -print | \
	    cpio -pdumL ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
	find ${DESTDIR}${BINDIR}/lkm -type f | xargs chmod a=rX
	find ${DESTDIR}${BINDIR}/lkm -type d | xargs chmod u=rwX,go=rX

.include <bsd.prog.mk>
