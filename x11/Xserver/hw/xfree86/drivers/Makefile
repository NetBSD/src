#	$NetBSD: Makefile,v 1.6 2004/03/14 07:26:58 matt Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga newport nv r128 radeon \
	s3 s3virge savage sis tdfx trident vga

.if ${MACHINE} == "i386"
SUBDIR+=	i740 i810 neomagic via vmware
.endif

.if ${MACHINE} != "macppc" && ${MACHINE} != "sgimips"
SUBDIR+=	apm ark cirrus cyrix dummy i128 nsc \
		rendition siliconmotion tga tseng vesa
.endif

.include <bsd.subdir.mk>
