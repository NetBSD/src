#
#	$Id: Makefile,v 1.11 1994/07/21 07:33:01 cgd Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	find ${FILES} -name CVS -prune -o -type f -print | \
	    cpio -pdum ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
	find ${DESTDIR}${BINDIR}/lkm -type f | xargs chmod a=rX
	find ${DESTDIR}${BINDIR}/lkm -type d | xargs chmod u=rwX,go=rX

.include <bsd.prog.mk>
