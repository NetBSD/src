#
#	$NetBSD: Makefile,v 1.18 2000/12/11 09:14:28 jdolecek Exp $
#

.include <bsd.own.mk>		# for UNPRIVILEGED

STUFF=	README syscall misc
MKOBJ=	no

install:
	find ${STUFF} -follow -name CVS -prune -o -type f -print | \
	    cpio -pdumL ${DESTDIR}${BINDIR}/lkm
.if !defined(UNPRIVILEGED)
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
.endif
	find ${DESTDIR}${BINDIR}/lkm -type f | xargs chmod a=rX
	find ${DESTDIR}${BINDIR}/lkm -type d | xargs chmod u=rwX,go=rX

.include <bsd.prog.mk>
