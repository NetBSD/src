#	$NetBSD: Makefile,v 1.47 2019/03/10 11:18:02 mrg Exp $

# build libraries for xorg

.include <bsd.own.mk>

.include "mesa-which.mk"

SUBDIR=	libfontenc freetype libFS libICE libglapi${OLD_PREFIX} \
	libXau libXdmcp libdrm libpciaccess libpthread-stubs libxshmfence .WAIT
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=libdrm_intel libdrm_amdgpu
.endif
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "evbarm"
SUBDIR+=libdrm_nouveau libvdpau
.endif
SUBDIR+=libdrm_radeon pixman
.if !defined(MLIBDIR)
SUBDIR+=xkeyboard-config
.endif
SUBDIR+=libxcb \
	.WAIT
.if !defined(MLIBDIR)
SUBDIR+=${EXTRA_DRI_DIRS} dri${OLD_PREFIX} gallium${OLD_PREFIX}
.endif
SUBDIR+=fontconfig libSM libX11 \
	.WAIT \
	libXcomposite libXdamage libXext libXfixes libXt \
	libxkbfile libepoxy \
	.WAIT \
	libXi libXxf86vm \
	.WAIT \
	libGL${OLD_PREFIX} libXScrnSaver libXTrap libXevie libXfont libXfont2 \
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
SUBDIR+=libgbm${OLD_PREFIX}

.include <bsd.buildinstall.mk>
.include <bsd.subdir.mk>
