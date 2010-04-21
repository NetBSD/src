#	$NetBSD: Makefile,v 1.4.6.5.4.1 2010/04/21 05:24:34 matt Exp $

# build libraries for xorg

SUBDIR=	expat libfontenc freetype libFS libICE libOSMesa \
	libXau libXdmcp libdrm libpciaccess
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel
.endif
SUBDIR+=pixman xkeyboard-config \
	.WAIT \
	dri fontconfig libSM libX11 liblbxutil \
	.WAIT \
	libXcomposite libXdamage libXext libXfixes libXt \
	libxkbfile \
	.WAIT \
	libXi libXp libXxf86vm \
	.WAIT \
	libGL libXScrnSaver libXTrap libXevie libXfont libXfontcache \
	libXinerama libXmu libXmuu libXpm libXprintUtil libXrender \
	libXres libXtst libXvMC libXv libXxf86dga libXxf86misc libxkbui \
	.WAIT \
	libGLU libGLw libXaw6 libXaw7 libXaw libXcursor libXprintAppUtil \
	libXrandr \
	.WAIT \
	libXft libglut
	
.include <bsd.subdir.mk>
