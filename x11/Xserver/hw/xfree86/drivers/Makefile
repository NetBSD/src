#	$NetBSD: Makefile,v 1.4 2004/01/10 13:02:13 rtr Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga newport nv r128 radeon \
	s3 s3virge savage sis tdfx trident vga

.if ${MACHINE} != "macppc"
SUBDIR+=	apm ark cirrus cyrix dummy i128 i740 i810 neomagic nsc \
		rendition siliconmotion tga tseng vesa vmware
.endif

.include <bsd.subdir.mk>
