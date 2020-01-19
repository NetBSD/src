#	$NetBSD: Makefile,v 1.22 2020/01/19 00:58:15 jmcneill Exp $

.include <bsd.own.mk>

DRIVER=		xf86-video-ati
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE} == "evbarm"
DRIVER_NAME=	radeon_drv_old
.else
DRIVER_NAME=	radeon_drv
.endif

.PATH:  ${X11SRCDIR.${DRIVER}}/src/AtomBios
SRCS.ATOMBIOS= \
        CD_Operations.c \
        Decoder.c \
        hwserv_drv.c

.if ${X11DRI} != "no"
SRCS.DRI=	radeon_dri.c
CPPFLAGS+=	${X11FLAGS.DRI}
.endif

# XXX
.if ${XORG_SERVER_SUBDIR} == "xorg-server.old"
CPPFLAGS+=	-DUSE_XAA
.endif

.if ${X11DRI} != "no"
# XXX
. if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
     ${MACHINE} == "evbarm"
SRCS.KMS=	radeon_dri2.c radeon_kms.c drmmode_display.c radeon_vbo.c
CPPFLAGS+=	-DXF86DRM_MODE
LIBDPLIBS+=	drm_radeon ${.CURDIR}/../../../lib/libdrm_radeon
. endif
.endif

SRCS.EXA=	radeon_exa.c r600_exa.c r6xx_accel.c \
		r600_textured_videofuncs.c r600_shader.c \
		radeon_exa_shared.c \
		evergreen_exa.c evergreen_accel.c evergreen_shader.c \
		evergreen_textured_videofuncs.c \
		cayman_accel.c cayman_shader.c

SRCS=	radeon_accel.c radeon_cursor.c \
	radeon_driver.c radeon_video.c radeon_bios.c \
	radeon_mm_i2c.c radeon_vip.c radeon_misc.c radeon_probe.c \
	legacy_crtc.c legacy_output.c radeon_textured_video.c \
	radeon_crtc.c radeon_output.c radeon_modes.c radeon_tv.c \
	radeon_legacy_memory.c radeon_pm.c radeon_xvmc.c \
	$(SRCS.ATOMBIOS) radeon_atombios.c radeon_atomwrapper.c \
	$(SRCS.DRI) $(SRCS.EXA) atombios_output.c atombios_crtc.c \
	$(SRCS.KMS)
MAN=	radeon.4

CPPFLAGS+=	-DDISABLE_EASF -DENABLE_ALL_SERVICE_FUNCTIONS \
		-DATOM_BIOS -DATOM_BIOS_PARSER -DDRIVER_PARSER \
		-DHAVE_CONFIG_H -DHAVE_XF86_CURSOR_RESET_CURSOR
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/AtomBios/includes \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/dri \
		-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.${DRIVER}}/../include \
		-I${X11SRCDIR.${DRIVER}}/../xorg-server-copy

.if ${XORG_SERVER_SUBDIR} != "xorg-server.old"
CPPFLAGS+=	-DHAVE_XORG_LIST
# XXX These have been copied from xorg-server 1.10.6.
SRCS+=		fi1236.c msp3430.c tda9885.c uda1380.c
.PATH:		${X11SRCDIR.${DRIVER}}/../xorg-server-copy
.endif

.include "../Makefile.xf86-driver"
