# $NetBSD: Makefile,v 1.4 2002/01/31 19:15:23 tv Exp $

FONTS=	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
	vt220l.816 vt220h.816 vt220l.810 vt220h.810

FILES=${FONTS}

CLEANFILES= 	${FONTS}
FILESDIR=	/usr/share/wscons/fonts

realall:	${FONTS}

UUDECODE?=	uudecode

.for i in ${FONTS}
$i: ${.CURDIR}/$i.uu
	${UUDECODE} ${.CURDIR}/$i.uu
.endfor

.include <bsd.prog.mk>
