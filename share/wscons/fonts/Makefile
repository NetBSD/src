# $NetBSD: Makefile,v 1.12 2020/07/08 11:24:49 fcambus Exp $

FONTS=	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
	vt220l.816 vt220h.816 vt220l.810 vt220h.810 \
	latin2.808 latin2.814 latin2.816 \
	donna.816 flori.816 orator.816 \
	spleen-5x8.fnt spleen-6x12.fnt spleen-8x16.fnt \
	spleen-12x24.fnt spleen-16x32.fnt spleen-32x64.fnt

FILES=${FONTS}
UUDECODE_FILES=${FONTS}

FILESDIR=	/usr/share/wscons/fonts

realall:	${FONTS}

.include <bsd.files.mk>
.include <bsd.prog.mk>
