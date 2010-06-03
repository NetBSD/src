#	$NetBSD: Makefile,v 1.19 2010/06/03 09:49:45 veego Exp $

# build libraries for xorg

SUBDIR=	expat libfontenc freetype libFS libICE libOSMesa \
	libXau libXdmcp libdrm libpciaccess .WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel
.endif
SUBDIR+=libdrm_radeon pixman xkeyboard-config \
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
	.WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libchromeXvMC
.endif
SUBDIR+=libGLU libGLw libXaw6 libXaw libXcursor \
	libXrandr \
	.WAIT \
	libXft libglut
	
.include <bsd.subdir.mk>
