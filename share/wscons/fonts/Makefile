# $NetBSD: Makefile,v 1.1 2000/07/25 17:13:53 drochner Exp $

FONTS	= 	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
		vt220l.816 vt220h.816 vt220l.810 vt220h.810

LIBMODE	=	644
LIBOWN	=	bin
LIBGRP	=	bin
CLEANFILES= 	${FONTS}
FONTDIR=	/usr/share/wscons/fonts

all: $(FONTS)

install: ${FONTS}
	for i in ${FONTS}; do \
		${INSTALL} -c -m ${LIBMODE} -o ${LIBOWN} -g ${LIBGRP} \
			${.OBJDIR}/$$i ${DESTDIR}${FONTDIR}; \
	done

clean:
	rm -f ${CLEANFILES}

.include <bsd.prog.mk>

# this seems to be the lowest common denominator

vt100pc.814: ${.CURDIR}/vt100pc.814.uu
	uudecode ${.CURDIR}/$@.uu

vt100sg.814: ${.CURDIR}/vt100sg.814.uu
	uudecode ${.CURDIR}/$@.uu

vt220l.814: ${.CURDIR}/vt220l.814.uu
	uudecode ${.CURDIR}/$@.uu

vt220h.814: ${.CURDIR}/vt220h.814.uu
	uudecode ${.CURDIR}/$@.uu

vt220l.808: ${.CURDIR}/vt220l.808.uu
	uudecode ${.CURDIR}/$@.uu

vt220h.808: ${.CURDIR}/vt220h.808.uu
	uudecode ${.CURDIR}/$@.uu

vt220l.816: ${.CURDIR}/vt220l.816.uu
	uudecode ${.CURDIR}/$@.uu

vt220h.816: ${.CURDIR}/vt220h.816.uu
	uudecode ${.CURDIR}/$@.uu

vt220l.810: ${.CURDIR}/vt220l.810.uu
	uudecode ${.CURDIR}/$@.uu

vt220h.810: ${.CURDIR}/vt220h.810.uu
	uudecode ${.CURDIR}/$@.uu
