#
#	$Id: Makefile,v 1.3 1993/07/09 04:27:33 mycroft Exp $
#

FILES=	README syscall vfs misc
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@@if [ ! -d ${DESTDIR}${BINDIR}/lkm ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/lkm ; \
                mkdir -p ${DESTDIR}${BINDIR}/lkm ; \
                chown root.wheel ${DESTDIR}${BINDIR}/lkm ; \
                chmod 755 ${DESTDIR}${BINDIR}/lkm ; \
        else \
                true ; \
        fi
# XXX:
# Must do these separately if we want to preserve mod times on directories
# because -prune doesn't work correctly with -depth.
	find ${FILES} -name CVS -prune -o -type f -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm
	find ${FILES} -name CVS -prune -o -type d -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm

.include <bsd.prog.mk>
