# $NetBSD: Makefile,v 1.3 2001/01/28 18:45:59 veego Exp $

FONTS=	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
	vt220l.816 vt220h.816 vt220l.810 vt220h.810

FILES=${FONTS}

CLEANFILES= 	${FONTS}
FILESDIR=	/usr/share/wscons/fonts

realall:	${FONTS}

.for i in ${FONTS}
$i: ${.CURDIR}/$i.uu
	uudecode ${.CURDIR}/$i.uu
.endfor

.include <bsd.prog.mk>
