#
#	$Id: Makefile,v 1.10 1994/02/16 05:50:47 cgd Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	find ${FILES} -name CVS -prune -o -type f -print | \
	    cpio -pdum ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
	find ${DESTDIR}${BINDIR}/lkm | xargs chmod a=rX

.include <bsd.prog.mk>
