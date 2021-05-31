#	$NetBSD: Makefile,v 1.7 2021/05/31 00:04:15 joerg Exp $

DRIVER=		xf86-video-geode
DRIVER_NAME=	geode_drv

SRCS=	\
	geode_driver.c \
	geode_common.c \
	geode_dcon.c \
	geode_ddc.c \
	geode_msr.c \
	gx_driver.c\
	gx_accel.c \
	gx_cursor.c \
	gx_video.c \
	gx_regacc.c \
	gx_rotate.c \
	gx_randr.c \
	durango.c \
	panel.c \
	lx_driver.c \
	lx_cursor.c \
	lx_memory.c \
	lx_panel.c \
	lx_output.c \
	lx_display.c \
	lx_video.c \
	lx_exa.c \
	cimarron.c
NOMAN=	# defined

CPPFLAGS+=	-DHAVE_GX -DHAVE_LX -DAMD_V4L2_VIDEO -DOPT_ACCEL -DPNL_SUP
# This enables multi-mmap support; no idea why it is Linux 2.6 specific
CPPFLAGS+=	-DLINUX_2_6
CPPFLAGS+=	-DHAVE_XF86_CURSOR_RESET_CURSOR

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/cim \
		-I${X11SRCDIR.${DRIVER}}/src/gfx \
		-I${X11SRCDIR.${DRIVER}}/src/panel

# XXX ugh
CPPFLAGS+=	-Doff64_t=off_t -Dlseek64=lseek

COPTS.gx_randr.c+=	-Wno-error	# XXX
COPTS.lx_memory.c+=	-Wno-error	# XXX
COPTS.panel.c+=		-Wno-error	# XXX -- huh? don't get this one
COPTS.geode_driver.c+=	-Wno-error	# XXX
COPTS.gx_accel.c+=	-Wno-error	# XXX
COPTS.gx_cursor.c+=	-Wno-error	# XXX
COPTS.gx_video.c+=	-Wno-error	# XXX
COPTS.lx_video.c+=	-Wno-error	# XXX
COPTS.lx_driver.c+=	-Wno-error	# XXX
COPTS.lx_panel.c+=	-Wno-error	# XXX

CWARNFLAGS.clang+=	-Wno-empty-body

COPTS+=		-fcommon

.include "../Makefile.xf86-driver"

XORG_PKG_VER_PROG=	${DRIVER}
.include "../../../xorg-pkg-ver.mk"
