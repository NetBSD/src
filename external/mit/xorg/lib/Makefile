#	$NetBSD: Makefile,v 1.15 2009/06/11 05:15:06 mrg Exp $

# build libraries for xorg

SUBDIR=	expat libfontenc freetype libFS libICE libOSMesa \
	libXau libXdmcp libdrm 
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel libpciaccess
.endif
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
