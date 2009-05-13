#	$NetBSD: Makefile,v 1.22.8.1 2009/05/13 19:20:47 jym Exp $

SUBDIR=	ati atimisc chips fbdev glint imstt mga nv r128 radeon \
	s3 s3virge savage sis tdfx trident wsfb

.if ${MACHINE_ARCH} != "sparc64" && ${MACHINE_ARCH} != "sparc"
SUBDIR+=	vga
.endif

.if ${MACHINE_ARCH} == "i386"
SUBDIR+=	i740 i810 neomagic via vmware
.endif

.if ${MACHINE} != "macppc" && ${MACHINE} != "sgimips" && ${MACHINE} != "ofppc"
SUBDIR+=	apm ark cirrus cyrix dummy i128 nsc \
		rendition siliconmotion tga tseng vesa
.endif

.if ${MACHINE} == "netwinder"
SUBDIR=	dummy fbdev vesa vga wsfb
.endif

.if ${MACHINE} == "sgimips"
SUBDIR=	newport wsfb
.endif

.if ${MACHINE_ARCH} == "sparc64"
SUBDIR= sunffb sunleo suncg6 suncg3 suncg14 suntcx sunbw2 ati atimisc \
	r128 radeon glint wsfb
.endif

.if ${MACHINE_ARCH} == "sparc"
SUBDIR= sunleo suncg6 suncg3 suncg14 suntcx sunbw2 pnozz ati atimisc \
	r128 radeon wsfb sunffb ag10e
.endif

.if ${MACHINE} == "shark"
SUBDIR= chips wsfb
.endif

.include <bsd.subdir.mk>
