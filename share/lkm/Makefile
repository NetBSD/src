#
#	$Id: Makefile,v 1.2 1993/07/09 04:24:28 mycroft Exp $
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
	find ${FILES} -name CVS -prune -o -type f -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm
	find ${FILES} -name CVS -prune -o -type d -print | cpio -pdum ${DESTDIR}${BINDIR}/lkm

.include <bsd.prog.mk>
