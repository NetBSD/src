#	$NetBSD: Makefile,v 1.9.2.1 2004/11/15 09:12:39 rtr Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga nv r128 radeon \
	s3 s3virge savage sis tdfx trident vga wsfb

.if ${MACHINE} == "i386"
SUBDIR+=	i740 i810 neomagic via vmware
.endif

.if ${MACHINE} != "macppc" && ${MACHINE} != "sgimips"
SUBDIR+=	apm ark cirrus cyrix dummy i128 nsc \
		rendition siliconmotion tga tseng vesa
.endif

.if ${MACHINE} == "sgimips"
SUBDIR=	newport
.endif

.include "../../../Makefile.serverlib"

.if ${X11LOADABLE} != "no"
.include <bsd.subdir.mk>
.endif
