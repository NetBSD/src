#
#	$Id: Makefile,v 1.8 1994/02/10 05:06:01 cgd Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	find ${FILES} -name CVS -prune -o -type f -print | \
	    cpio -pdum ${DESTDIR}${BINDIR}/lkm
	chown -R ${BINOWN}.${BINGRP} ${TZDIR}
	find ${TZDIR} -type f | xargs chmod a-w

.include <bsd.prog.mk>
