#
#	$Id: Makefile,v 1.4 1993/09/30 23:31:28 jtc Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

beforeinstall:
	@install -d root.wheel ${DESTDIR}${BINDIR}/lkm

realinstall:
# XXX:
# Must do these separately if we want to preserve mod times on directories
# because -prune doesn't work correctly with -depth.
	find ${FILES} -name CVS -prune -o -type f -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm
	find ${FILES} -name CVS -prune -o -type d -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm

.include <bsd.prog.mk>
