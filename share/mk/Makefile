#	@(#)Makefile	5.3 (Berkeley) 6/22/90

FILES=	bsd.doc.mk bsd.lib.mk bsd.man.mk bsd.prog.mk bsd.subdir.mk sys.mk
NOOBJ=	noobj

all clean cleandir depend lint tags:

install:
	@if [ ! -d ${DESTDIR}${BINDIR}/mk ]; then \
                /bin/rm -f ${DESTDIR}${BINDIR}/mk ; \
                mkdir -p ${DESTDIR}${BINDIR}/mk ; \
                chown root.wheel ${DESTDIR}${BINDIR}/mk ; \
                chmod 755 ${DESTDIR}${BINDIR}/mk ; \
        else \
                true ; \
        fi
	install -c -o ${BINOWN} -g ${BINOWN} -m 444 ${FILES} \
	    ${DESTDIR}${BINDIR}/mk

.include <bsd.prog.mk>
