#
#	$Id: Makefile,v 1.6 1994/02/10 01:54:42 cgd Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

realinstall:
# XXX:
# Must do these separately if we want to preserve mod times on directories
# because -prune doesn't work correctly with -depth.
	find ${FILES} -name CVS -prune -o -type f -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm
	find ${FILES} -name CVS -prune -o -type d -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm

.include <bsd.prog.mk>
