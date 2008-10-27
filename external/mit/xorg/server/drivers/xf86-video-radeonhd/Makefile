#	$NetBSD: Makefile,v 1.3 2008/10/27 19:42:50 veego Exp $

.include <bsd.own.mk>

DRIVER=		xf86-video-radeonhd
DRIVER_NAME=	radeonhd_drv

.PATH:	${X11SRCDIR.${DRIVER}}/src/AtomBios
SRCS.ATOMBIOS=	CD_Operations.c Decoder.c hwserv_drv.c

.if ${X11DRI} != "no"
SRCS.DRI=	rhd_dri.c
CPPFLAGS+=	-DUSE_DRI
.endif

SRCS.EXA=	r5xx_exa.c radeon_exa_render.c

SRCS=		rhd_atombios.c rhd_i2c.c rhd_crtc.c rhd_connector.c
SRCS+=		rhd_cursor.c rhd_dac.c rhd_driver.c rhd_edid.c
SRCS+=		rhd_helper.c rhd_id.c  rhd_lut.c rhd_lvtma.c rhd_modes.c
SRCS+=		rhd_monitor.c rhd_output.c rhd_pll.c rhd_tmds.c
SRCS+=		rhd_vga.c rhd_mc.c rhd_shadow.c rhd_randr.c rhd_dig.c
SRCS+=		rhd_ddia.c rhd_atomout.c rhd_atompll.c rhd_atomcrtc.c
SRCS+=		rhd_biosscratch.c
SRCS+=		rhd_cs.c r5xx_accel.c r5xx_xaa.c radeon_3d.c
SRCS+=		rhd_video.c radeon_textured_videofuncs.c
SRCS+=		$(SRCS.EXA) $(SRCS.DRI) rhd_atomwrapper.c $(SRCS.ATOMBIOS)
MAN=		radeonhd.4

CPPFLAGS+=	-DDRIVER_PARSER -DDISABLE_EASF -DENABLE_ALL_SERVICE_FUNCTIONS

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/AtomBios/includes
CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

# XXX
COPTS=	-Wno-error

.include "../Makefile.xf86-driver"
