#	$NetBSD: Makefile,v 1.29.2.2 2015/03/03 08:28:12 snj Exp $

# build libraries for xorg

SUBDIR=	libfontenc freetype libFS libICE libglapi \
	libXau libXdmcp libdrm libpciaccess libpthread-stubs .WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel
.endif
SUBDIR+=libdrm_radeon pixman xkeyboard-config libxcb \
	.WAIT
SUBDIR+=dri7 dri gallium fontconfig libSM libX11 \
	.WAIT \
	libXcomposite libXdamage libXext libXfixes libXt \
	libxkbfile \
	.WAIT \
	libXi libXxf86vm \
	.WAIT \
	libGL libXScrnSaver libXTrap libXevie libXfont libXfontcache \
	libXinerama libXmu libXmuu libXpm libXrender \
	libXres libXtst libXvMC libXvMCW libXv libXxf86dga libXxf86misc \
	libxkbui \
	.WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libI810XvMC
SUBDIR+=libIntelXvMC
SUBDIR+=libchromeXvMC
SUBDIR+=libchromeXvMCPro
.endif
SUBDIR+=libGLU libGLw libXaw6 libXaw libXcursor \
	libXrandr \
	.WAIT \
	libXft libglut \
	.WAIT \
	libXdmGreet \
	xcb-util

.include <bsd.subdir.mk>
