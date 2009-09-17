#	$NetBSD: Makefile,v 1.5.6.1 2009/09/17 04:24:31 snj Exp $

.include <bsd.own.mk>

LIB=		GL

# glx
.PATH:		${X11SRCDIR.MesaLib}/src/glx/x11
SRCS.glx=	glcontextmodes.c clientattrib.c compsize.c eval.c glxcmds.c \
		glxext.c glxextensions.c indirect.c indirect_init.c \
		indirect_size.c indirect_window_pos.c \
		indirect_transpose_matrix.c indirect_vertex_array.c \
		indirect_vertex_program.c pixel.c pixelstore.c render2.c \
		renderpix.c single2.c singlepix.c vertarr.c xfont.c \
		glx_pbuffer.c glx_query.c \
		glxcurrent.c indirect_texture_compression.c

.PATH:		${X11SRCDIR.MesaLib}/src/mesa/glapi
SRCS.glx+=	glapi.c glapi_getproc.c glthread.c

# XXX see TODO
#.if ${MACHINE_ARCH} == "i386"
#.PATH:		${X11SRCDIR.MesaLib}/src/mesa/x86
#SRCS.glx+=	glapi_x86.S
#CPPFLAGS+=	-DUSE_X86_ASM -I${X11SRCDIR.MesaLib}/src/mesa/x86
## -DUSE_X86_ASM $(MMX_DEFS) $(3DNOW_DEFS) $(SSE_DEFS)
#.endif


.if ${X11DRI} != "no"
# dri
SRCS.dri=	XF86dri.c dri_glx.c drisw_glx.c dri_common.c \
		glxhash.c dri2_glx.c dri2.c
.for fn in ${SRCS.dri}
CPPFLAGS.${fn}=-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\"
.endfor
.endif


# mesa
.PATH:		${X11SRCDIR.MesaLib}/src/mesa/main
SRCS.mesa=	dispatch.c


.PATH:		${X11SRCDIR.MesaLib}/src/glw
.PATH:		${X11SRCDIR.MesaLib}/include/GL
.PATH:		${X11SRCDIR.MesaLib}/src/mesa/drivers/x11
INCS=	GLwDrawA.h GLwDrawAP.h GLwMDrawA.h GLwMDrawAP.h gl.h gl_mangle.h \
	glext.h glx.h glx_mangle.h glxext.h xmesa.h xmesa_x.h \
	xmesa_xf86.h
INCSDIR=${X11INCDIR}/GL

SRCS+=	${SRCS.dri} ${SRCS.mesa} ${SRCS.glx}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/GL \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${DESTDIR}${X11INCDIR}/X11/drm \
		-I${X11SRCDIR.MesaLib}/include \
		-I${X11SRCDIR.MesaLib}/include/GL/internal \
		-I${X11SRCDIR.MesaLib}/src/mesa \
		-I${X11SRCDIR.MesaLib}/src/mesa/glapi \
		-I${X11SRCDIR.MesaLib}/src/mesa \
		${X11FLAGS.THREADLIB}

#		-I${X11SRCDIR.dri2proto}/src/mesa \

CFLAGS+=	-fno-strict-aliasing -fvisibility=hidden -pthread
CPPFLAGS+=	-DUSE_SSE_SYSCTL_DETECTION \
		-D__GLX_ALIGN64 \
		-DMESA_EXECMEM_MMAP \
		-DEXEC_HEAP_SIZE=10485760 \
		-D_NETBSD_SOURCE \
		-DUSE_NATIVE_LIBM_FUNCS \
		-DPTHREADS \
		-DUSE_EXTERNAL_DXTN_LIB=1 \
		-DIN_DRI_DRIVER \
		-DGLX_DIRECT_RENDERING \
		-DGLX_INDIRECT_RENDERING \
		-DHZ=100 \
		-DHAVE_ALIAS \
		-DXF86VIDMODE \
		-UIN_DRI_DRIVER

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic \
		Xxf86vm	${.CURDIR}/../libXxf86vm \
		Xfixes	${.CURDIR}/../libXfixes \
		Xdamage	${.CURDIR}/../libXdamage \
		drm	${.CURDIR}/../libdrm \
		m	${.CURDIR}/../../../../../lib/libm

# XXX XXX
COPTS.dri_glx.c=			-Wno-error
COPTS.glxext.c=				-Wno-error
COPTS.indirect_vertex_program.c=	-Wno-error
MKLINT=no

PKGCONFIG=	gl
PKGDIST.gl=	${X11SRCDIR.MesaLib}/src/mesa
PKGCONFIG_VERSION.gl=	7.4.2

.include <bsd.x11.mk>
.include <bsd.lib.mk>
