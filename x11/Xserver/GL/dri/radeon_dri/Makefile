#	$NetBSD: Makefile,v 1.2 2005/01/21 05:15:36 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

NOLINT=	1

.include <bsd.own.mk>

LIB=	radeon_dri

# dri
.PATH:		${X11SRCDIR.xc}/lib/GL/dri
SRCS.dri=	dri_util.c
CPPFLAGS.dri_util.c=	-I${DESTDIR}${X11INCDIR}/X11 \
			-I${DESTDIR}${X11INCDIR}/X11/extensions \
			-I${X11SRCDIR.xc}/include/GL \
			-I${X11SRCDIR.xc}/programs/Xserver/include \
			-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
			-I${X11SRCDIR.xc}/lib/GL/include \
			-I${X11SRCDIR.xc}/extras/Mesa/include \
			-I${X11SRCDIR.xc}/extras/Mesa/src \
			-I${X11SRCDIR.xc}/lib/GL/glx \
			-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support
CPPFLAGS.dri_util.c+=	-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules\"


# drm
.PATH:		${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/linux/drm
SRCS.drm=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c
.for fn in ${SRCS.drm}
CPPFLAGS.${fn}=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/shared/drm/kernel
#		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bsd/drm
.endfor


# mesa
.PATH:		${X11SRCDIR.xc}/extras/Mesa/src
SRCS.mesa=	accum.c api_arrayelt.c api_eval.c api_loopback.c \
		api_noop.c api_validate.c attrib.c blend.c buffers.c \
		clip.c colortab.c context.c convolve.c debug.c depth.c \
		dispatch.c dlist.c drawpix.c enable.c enums.c \
		eval.c extensions.c feedback.c fog.c get.c \
		glthread.c hash.c hint.c histogram.c image.c \
		imports.c light.c lines.c matrix.c mmath.c pixel.c \
		points.c polygon.c rastpos.c state.c stencil.c \
		texcompress.c texformat.c teximage.c texobj.c texstate.c \
		texstore.c texutil.c varray.c vtxfmt.c vpexec.c \
		vpparse.c  vpstate.c
.for fn in ${SRCS.mesa}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/extras/Mesa/src/array_cache \
		-I${X11SRCDIR.xc}/extras/Mesa/src/math \
		-I${X11SRCDIR.xc}/extras/Mesa/src/swrast \
		-I${X11SRCDIR.xc}/extras/Mesa/src/swrast_setup \
		-I${X11SRCDIR.xc}/extras/Mesa/src/tnl \
		-I${X11SRCDIR.xc}/extras/Mesa/src/tnl_dd \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src \
		-I${X11SRCDIR.xc}/include
.endfor

# XXX MESA_ASM_OBJS

.include <bsd.endian.mk>

.if ${TARGET_ENDIANNESS} == "1234"
X_BYTE_ORDER=X_LITTLE_ENDIAN
.elif ${TARGET_ENDIANNESS} == "4321"
X_BYTE_ORDER=X_BIG_ENDIAN
.else
X_BYTE_ORDER=0
.endif

# common
.PATH:		${X11SRCDIR.xc}/lib/GL/mesa/src/drv/common
SRCS.common=	hwlog.c mm.c utils.c vblank.c texmem.c
.for fn in ${SRCS.common}
CPPFLAGS.${fn}=	-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/glx \
		-I${X11SRCDIR.xc}/include/GL \
		-I${X11SRCDIR.xc}/include \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/lib/GL/dri/drm \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}/extras/Mesa/src
CPPFLAGS.${fn}+=-DX_BYTE_ORDER=${X_BYTE_ORDER}
.endfor

# XXX currently i386 specific
# radeon
.PATH:		${X11SRCDIR.xc}/lib/GL/mesa/src/drv/radeon
SRCS.radeon=	radeon_context.c radeon_ioctl.c radeon_lock.c \
		radeon_maos.c radeon_sanity.c radeon_screen.c \
		radeon_span.c radeon_state.c radeon_state_init.c \
		radeon_swtcl.c radeon_tcl.c radeon_tex.c \
		radeon_texmem.c radeon_texstate.c radeon_vtxfmt.c \
		radeon_vtxfmt_x86.c radeon_vtxtmp_x86.S \
		radeon_vtxfmt_sse.c radeon_vtxfmt_c.c

.for fn in ${SRCS.radeon}
#CPPFLAGS.${fn}= -I${DESTDIR}${X11INCDIR}/X11 \
#		-I${DESTDIR}${X11INCDIR}/X11/extensions

CPPFLAGS.${fn}= -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS.${fn}+=-I${X11SRCDIR.xc}/extras/Mesa/src \
		-I${X11SRCDIR.xc}/lib/GL/mesa/src/drv/common \
		-I${X11SRCDIR.xc}/lib/GL/mesa/src/drv/radeon \
		-I${X11SRCDIR.xc}/lib/GL/dri \
		-I${X11SRCDIR.xc}/lib/GL/glx \
		-I${X11SRCDIR.xc}/include \
		-I${X11SRCDIR.xc}/include/GL \
		-I${X11SRCDIR.xc}/programs/Xserver/GL/dri \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/ati \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/lib/GL/dri/drm \
		-I${X11SRCDIR.xc}/lib/GL/include \
		-I${X11SRCDIR.xc}
CPPFLAGS.${fn}+=-DX_BYTE_ORDER=${X_BYTE_ORDER}
.endfor


SRCS=	${SRCS.dri} ${SRCS.drm} ${SRCS.mesa} ${SRCS.common} ${SRCS.radeon}

CPPFLAGS+=	${X11FLAGS.DRI} ${X11FLAGS.EXTENSION} ${X11FLAGS.THREADS} # -DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM

LDADD=	-lm
DPADD=	${LIBM}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
