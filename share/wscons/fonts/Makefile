# $NetBSD: Makefile,v 1.8 2003/10/21 10:01:20 lukem Exp $

FONTS=	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
	vt220l.816 vt220h.816 vt220l.810 vt220h.810 \
	donna.816 flori.816 orator.816

FILES=${FONTS}

CLEANFILES= 	${FONTS}
FILESDIR=	/usr/share/wscons/fonts

realall:	${FONTS}

.for i in ${FONTS}
$i: ${.CURDIR}/$i.uu
	${_MKTARGET_CREATE}
	${TOOL_UUDECODE} ${.CURDIR}/$i.uu
.endfor

.include <bsd.prog.mk>
