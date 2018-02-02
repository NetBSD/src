#	$NetBSD: Makefile,v 1.13 2018/02/02 01:02:40 mrg Exp $

DRIVER=		xf86-video-openchrome
DRIVER_NAME=	openchrome_drv

SRCS=	\
	via_3d.c \
	via_analog.c \
	via_ch7xxx.c \
	via_display.c \
	via_driver.c \
	via_exa.c \
	via_exa_h2.c \
	via_exa_h6.c \
	via_fp.c \
	via_i2c.c \
	via_id.c \
	via_memcpy.c \
	via_memmgr.c \
	via_outputs.c \
	via_ums.c \
	via_sii164.c \
	via_tmds.c \
	via_tv.c \
	via_vgahw.c \
	via_vt162x.c \
	via_xv_overlay.c \
	via_xv.c

# DRI
SRCS+=	\
	via_dri.c \
	via_kms.c \
	via_xvmc.c

MAN=	openchrome.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src
CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"

# XXX
COPTS.via_dri.c+= -Wno-error
COPTS.via_display.c+= -Wno-error=stack-protector
COPTS.via_driver.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}
COPTS.via_lvds.c+=	${${ACTIVE_CC} == "gcc":? -Wno-error=discarded-qualifiers :}

CWARNFLAGS.clang+=	-Wno-tautological-compare
