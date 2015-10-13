#	$NetBSD: Makefile,v 1.36 2015/10/13 04:33:21 mrg Exp $

# build libraries for xorg

.include <bsd.own.mk>

SUBDIR=	libfontenc freetype libFS libICE libglapi \
	libXau libXdmcp libdrm libpciaccess libpthread-stubs .WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel libdrm_nouveau
.endif
SUBDIR+=libdrm_radeon pixman
.if !defined(MLIBDIR)
SUBDIR+=xkeyboard-config
.endif
SUBDIR+=libxcb \
	.WAIT
.if !defined(MLIBDIR)
SUBDIR+=dri7 dri gallium
.endif
SUBDIR+=fontconfig libSM libX11 \
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
	libXpresent libXft libglut \
	.WAIT \
	libXdmGreet \
	xcb-util
SUBDIR+=libgbm

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
