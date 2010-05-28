#	$NetBSD: Makefile,v 1.18 2010/05/28 06:26:55 mrg Exp $

# build libraries for xorg

SUBDIR=	expat libfontenc freetype libFS libICE libOSMesa \
	libXau libXdmcp libdrm libpciaccess
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel libchromeXvMC
.endif
SUBDIR+=libdrm_radeon
SUBDIR+=pixman xkeyboard-config \
	.WAIT \
	dri fontconfig libSM libX11 \
	.WAIT \
	libXcomposite libXdamage libXext libXfixes libXt \
	libxkbfile \
	.WAIT \
	libXi libXxf86vm \
	.WAIT \
	libGL libXScrnSaver libXTrap libXevie libXfont libXfontcache \
	libXinerama libXmu libXmuu libXpm libXrender \
	libXres libXtst libXvMC libXv libXxf86dga libXxf86misc libxkbui \
	.WAIT \
	libGLU libGLw libXaw6 libXaw libXcursor \
	libXrandr \
	.WAIT \
	libXft libglut
	
.include <bsd.subdir.mk>
