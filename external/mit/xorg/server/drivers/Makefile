#	$NetBSD: Makefile,v 1.35 2009/11/28 16:48:25 mbalmer Exp $

SUBDIR= \
	xf86-input-keyboard \
	xf86-input-mouse \
	xf86-input-void \
	xf86-input-ws

.if ${MACHINE_ARCH} == "i386"
SUBDIR+= \
	xf86-input-elographics
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64"
SUBDIR+= \
	xf86-input-vmmouse \
	xf86-video-vmware \
	xf86-video-vesa \
	xf86-video-wsfb
.endif

.if ${MACHINE} == "macppc"
SUBDIR+= \
	xf86-video-ati \
	xf86-video-chips \
	xf86-video-mach64 \
	xf86-video-mga \
	xf86-video-nv \
	xf86-video-radeon \
	xf86-video-r128 \
	xf86-video-tdfx \
	xf86-video-wsfb
#	xf86-video-imstt
.elif ${MACHINE} == "netwinder"
SUBDIR+= \
	xf86-video-wsfb
.elif ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
SUBDIR+= \
	xf86-video-ag10e \
	xf86-video-ati \
	xf86-video-glint \
	xf86-video-mach64 \
	xf86-video-mga \
	xf86-video-r128 \
	xf86-video-radeon \
	xf86-video-suncg6 \
	xf86-video-sunffb \
	xf86-video-sunleo \
	xf86-video-wsfb
.if ${MACHINE} == "sparc"
SUBDIR+= \
	xf86-video-pnozz \
	xf86-video-suntcx
.endif
.elif ${MACHINE} == "shark"
SUBDIR+= \
	xf86-video-chips \
        xf86-video-igs \
        xf86-video-wsfb
.elif ${MACHINE} == "sgimips"
SUBDIR+= \
	xf86-video-crime \
	xf86-video-wsfb \
	xf86-video-newport
.elif ${MACHINE} == "alpha"
SUBDIR+= \
	xf86-video-apm \
	xf86-video-ark \
	xf86-video-ast \
	xf86-video-ati \
	xf86-video-chips \
	xf86-video-cirrus \
	xf86-video-cirrus_alpine \
	xf86-video-cirrus_laguna \
	xf86-video-glint \
	xf86-video-i128 \
	xf86-video-i740 \
	xf86-video-mach64 \
	xf86-video-mga \
	xf86-video-nv \
	xf86-video-r128 \
	xf86-video-radeon \
	xf86-video-radeonhd \
	xf86-video-s3 \
	xf86-video-s3virge \
	xf86-video-savage \
	xf86-video-siliconmotion \
	xf86-video-sis \
	xf86-video-tdfx \
	xf86-video-tga \
	xf86-video-trident \
	xf86-video-tseng \
	xf86-video-wsfb
#	xf86-video-imstt
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+= \
	xf86-video-apm \
	xf86-video-ark \
	xf86-video-ast \
	xf86-video-ati \
	xf86-video-chips \
	xf86-video-cirrus \
	xf86-video-cirrus_alpine \
	xf86-video-cirrus_laguna \

# replace with xf86-video-geode
#	xf86-video-cyrix \

SUBDIR+= \
	xf86-video-glint \
	xf86-video-i128 \
	xf86-video-i740 \

# needs porting for xorg-server 1.6
#	xf86-video-imstt \

SUBDIR+= \
	xf86-video-intel \
	xf86-video-mach64 \
	xf86-video-mga \
	xf86-video-neomagic \
	xf86-video-nv \
	xf86-video-r128 \
	xf86-video-radeon \
	xf86-video-radeonhd \
	xf86-video-s3 \
	xf86-video-s3virge \
	xf86-video-savage \
	xf86-video-siliconmotion \
	xf86-video-sis \
	xf86-video-tdfx \
	xf86-video-tga \
	xf86-video-trident \
	xf86-video-tseng \

# replace with xf86-video-openchrome
#	xf86-video-via
.endif

# broken with xorg-server 1.6
# x86
#	xf86-video-nsc \
#	xf86-video-vga \
# alpha
#	xf86-video-vga \

#xf86-video-rendition

.include <bsd.subdir.mk>
