#	from: @(#)Makefile	5.3 (Berkeley) 6/22/90
#	$Id: Makefile,v 1.5 1993/08/15 20:49:25 mycroft Exp $

FILES=	bsd.README bsd.doc.mk bsd.dep.mk bsd.lib.mk bsd.man.mk bsd.obj.mk \
	bsd.own.mk bsd.prog.mk bsd.subdir.mk sys.mk
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
