#	$NetBSD: Makefile,v 1.5 2004/03/06 03:31:06 rtr Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga newport nv r128 radeon \
	s3 s3virge savage sis tdfx trident vga

.if ${MACHINE} != "macppc"
SUBDIR+=	apm ark cirrus cyrix dummy i128 i740 i810 neomagic nsc \
		rendition siliconmotion tga tseng vesa vmware via
.endif

.include <bsd.subdir.mk>
