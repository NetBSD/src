# $NetBSD: Makefile,v 1.15 2022/06/08 19:27:37 uwe Exp $

FONTS=	vt220l.814 vt220h.814 vt220l.808 vt220h.808 \
	vt220l.816 vt220h.816 vt220l.810 vt220h.810 \
	latin2.808 latin2.814 latin2.816 \
	donna.816 flori.816 orator.816 \
	spleen-5x8.fnt spleen-6x12.fnt spleen-8x16.fnt \
	spleen-12x24.fnt spleen-16x32.fnt spleen-32x64.fnt


# Terminus Font by Dimitar Toshkov Zhekov
# The naming of the files follows upstream conventions:
#
#   i - IBM CP437
#   1 - ISO8859-1
#
#   n - normal
#   b - bold
#   v - CRT VGA Bold (8 columns, rendered in 9)

FONTS += ter-i12n.wsf
FONTS += ter-i16b.wsf
FONTS += ter-i16n.wsf
FONTS += ter-i16v.wsf
FONTS += ter-i20n.wsf
FONTS += ter-i24n.wsf
FONTS += ter-i28n.wsf
FONTS += ter-i32n.wsf

FONTS += ter-112n.wsf
FONTS += ter-116b.wsf
FONTS += ter-116n.wsf
FONTS += ter-116v.wsf
FONTS += ter-120n.wsf
FONTS += ter-124n.wsf
FONTS += ter-128n.wsf
FONTS += ter-132n.wsf


FILES=${FONTS}
UUDECODE_FILES=${FONTS}

FILESDIR=	/usr/share/wscons/fonts

realall:	${FONTS}

.include <bsd.files.mk>
.include <bsd.prog.mk>
