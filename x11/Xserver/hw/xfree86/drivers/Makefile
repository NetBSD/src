#	$NetBSD: Makefile,v 1.7.2.1 2004/08/12 04:08:33 jmc Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga nv r128 radeon \
	s3 s3virge savage sis tdfx trident vga

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

.include <bsd.subdir.mk>
