#	$NetBSD: Makefile,v 1.5.8.1 2009/05/13 18:53:26 jym Exp $

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
		glx_pbuffer.c glx_query.c glx_texture_compression.c

.PATH:		${X11SRCDIR.MesaLib}/src/mesa/glapi
SRCS.glx+=	glapi.c glthread.c

# XXX see TODO
#.if ${MACHINE_ARCH} == "i386"
#.PATH:		${X11SRCDIR.MesaLib}/src/mesa/x86
#SRCS.glx+=	glapi_x86.S
#CPPFLAGS+=	-DUSE_X86_ASM -I${X11SRCDIR.MesaLib}/src/mesa/x86
## -DUSE_X86_ASM $(MMX_DEFS) $(3DNOW_DEFS) $(SSE_DEFS)
#.endif


.if ${X11DRI} != "no"
# dri
SRCS.dri=	XF86dri.c dri_glx.c
.for fn in ${SRCS.dri}
CPPFLAGS.${fn}=-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\"
.endfor
.endif


# mesa
.PATH:		${X11SRCDIR.MesaLib}/src/mesa/main
SRCS.mesa=	dispatch.c


.PATH:		${X11SRCDIR.MesaLib}/src/glw
.PATH:		${X11SRCDIR.MesaLib}/include/GL
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
		-I${X11SRCDIR.MesaLib}/src/mesa/main \
		-I${X11SRCDIR.MesaLib}/src/mesa/glapi \
		-I${X11SRCDIR.MesaLib}/src/mesa \
		${X11FLAGS.THREADLIB}

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
PKGCONFIG_VERSION.gl=	7.0.3

.include <bsd.x11.mk>
.include <bsd.lib.mk>
