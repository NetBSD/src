#
#	$Id: Makefile,v 1.9 1994/02/10 05:25:18 cgd Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	find ${FILES} -name CVS -prune -o -type f -print | \
	    cpio -pdum ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}:${BINGRP} ${DESTDIR}${BINDIR}/lkm
	find ${DESTDIR}${BINDIR}/lkm -type f | xargs chmod a-w

.include <bsd.prog.mk>
