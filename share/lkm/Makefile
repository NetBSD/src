#
#	$Id: Makefile,v 1.1 1993/06/08 03:53:19 cgd Exp $
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
	tar cf - ${FILES} | (cd ${DESTDIR}${BINDIR}/lkm ; tar xf -)

.include <bsd.prog.mk>
