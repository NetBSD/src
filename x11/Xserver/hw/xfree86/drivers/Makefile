#	$NetBSD: Makefile,v 1.10 2005/03/23 11:09:58 martin Exp $

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

.if ${MACHINE} == "sparc64"
SUBDIR= sunffb sunleo suncg6 suncg3 suncg14 suntcx sunbw2  ati vga
.endif

.include <bsd.subdir.mk>
