#	$NetBSD: Makefile,v 1.1.8.1 2009/09/17 04:24:36 snj Exp $

.include <bsd.own.mk>

DRIVER=		xf86-video-ati
DRIVER_NAME=	radeon_drv

.PATH:  ${X11SRCDIR.${DRIVER}}/src/AtomBios
SRCS.ATOMBIOS= \
        CD_Operations.c \
        Decoder.c \
        hwserv_drv.c

.if ${X11DRI} != "no"
SRCS.DRI=	radeon_dri.c
CPPFLAGS+=	${X11FLAGS.DRI}
.endif

SRCS.EXA=	radeon_exa.c r600_exa.c r6xx_accel.c \
		r600_textured_videofuncs.c r600_shader.c

SRCS=	radeon_accel.c radeon_cursor.c radeon_dga.c \
	radeon_driver.c radeon_video.c radeon_bios.c \
	radeon_mm_i2c.c radeon_vip.c radeon_misc.c radeon_probe.c \
	legacy_crtc.c legacy_output.c radeon_textured_video.c \
	radeon_crtc.c radeon_output.c radeon_modes.c radeon_tv.c \
	radeon_legacy_memory.c \
	$(SRCS.ATOMBIOS) radeon_atombios.c radeon_atomwrapper.c \
	$(SRCS.DRI) $(SRCS.EXA) atombios_output.c atombios_crtc.c
MAN=	radeon.4

CPPFLAGS+=	-DDISABLE_EASF -DENABLE_ALL_SERVICE_FUNCTIONS \
		-DATOM_BIOS -DATOM_BIOS_PARSER -DDRIVER_PARSER \
		-DHAVE_CONFIG_H
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/AtomBios/includes \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/dri \
		-I${DESTDIR}${X11INCDIR}/X11/drm \
		-I${X11SRCDIR.${DRIVER}}/../include

# XXX
COPTS=	-Wno-error

.include "../Makefile.xf86-driver"
