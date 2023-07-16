#	$NetBSD: Makefile,v 1.34 2023/07/16 22:20:54 rjs Exp $

.include <bsd.own.mk>

.include "../mesa-which.mk"

LIB=		GL
LIBISCXX=	yes

SUBDIR=		internal

# glx
.PATH:		${X11SRCDIR.Mesa}/src/glx
SRCS.glx=	\
	clientattrib.c \
	clientinfo.c \
	compsize.c \
	create_context.c \
	eval.c \
	glx_error.c \
	glx_pbuffer.c \
	glx_query.c \
	glxcmds.c \
	glxconfig.c \
	MESAindirect.c \
	MESAindirect_init.c \
	MESAindirect_size.c \
	MESAindirect_window_pos.c \
	glxcurrent.c \
	glxext.c \
	glxextensions.c \
	glxhash.c \
	indirect_glx.c \
	indirect_texture_compression.c \
	indirect_transpose_matrix.c \
	indirect_vertex_array.c \
	indirect_vertex_program.c \
	pixel.c \
	pixelstore.c \
	query_renderer.c \
	render2.c \
	renderpix.c \
	single2.c \
	singlepix.c \
	vertarr.c \

SRCS+=	UTILdebug.c
CPPFLAGS.UTILdebug.c+=	-I${X11SRCDIR.Mesa}/src/util \
			-I${X11SRCDIR.Mesa}/src/mesa \
			-I${X11SRCDIR.Mesa}/src \
			-I${X11SRCDIR.Mesa}/src/gallium/include


BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mapi/glapi/gen/indirect_init.c MESAindirect_init.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/glx/indirect_window_pos.c MESAindirect_window_pos.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mapi/glapi/gen/indirect.c MESAindirect.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/../src/mapi/glapi/gen/indirect_size.c MESAindirect_size.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/util/debug.c UTILdebug.c

# glapi
.PATH:		${X11SRCDIR.Mesa}/src/mapi
.PATH:		${X11SRCDIR.Mesa}/src/mapi/glapi
#SRCS.glapi=	glapi_gentable.c

.PATH:		${X11SRCDIR.Mesa}/src/mesa/x86
.PATH:		${X11SRCDIR.Mesa}/src/mesa/x86-64
.PATH:		${X11SRCDIR.Mesa}/src/mesa/sparc

SRCS.i386.glx=	common_x86_asm.S \
		x86_xform2.S	\
		x86_xform3.S	\
		x86_xform4.S	\
		x86_cliptest.S	\
		mmx_blend.S	\
		3dnow_xform1.S	\
		3dnow_xform2.S	\
		3dnow_xform3.S	\
		3dnow_xform4.S	\
		sse_xform1.S	\
		sse_xform2.S	\
		sse_xform3.S	\
		sse_xform4.S	\
		sse_normal.S	\
		read_rgba_span_x86.S
SRCS.x86_64.glx=	xform4.S
SRCS.sparc.glx=		xform.S norm.S sparc_clip.S

.if !empty(SRCS.${MACHINE_ARCH}.glx)
SRCS.glx += ${SRCS.${XORG_MACHINE_ARCH:U${MACHINE_ARCH}}.glx}
.endif
SRCS.glapi+=			entry.c

.for _f in ${SRCS.glx}
CPPFLAGS.${_f}= -I${X11SRCDIR.Mesa}/../src/mapi/glapi/gen \
		-I${X11SRCDIR.Mesa}/src \
		-I${X11SRCDIR.Mesa}/src/mesa \
		-I${X11SRCDIR.Mesa}/../src/mesa \
		-DGLX_SHARED_GLAPI
.endfor

.for _f in ${SRCS.glapi}
CPPFLAGS.${_f}= -DMAPI_MODE_BRIDGE \
		-DMAPI_ABI_HEADER=\"glapi/gen/glapi_mapi_tmp.h\" \
		-I${X11SRCDIR.Mesa}/src \
		-I${X11SRCDIR.Mesa}/../src/mapi
.endfor


.if ${X11DRI} != "no"
# dri
SRCS.dri=	\
	dri_common.c \
	dri_common_query_renderer.c \
	dri_common_interop.c \
	xfont.c \
	drisw_glx.c \
	dri2.c \
	dri2_glx.c

.for fn in ${SRCS.dri}
CPPFLAGS.${fn}= \
	-I${X11SRCDIR.Mesa}/src \
	-DGL_LIB_NAME="\"libGL.so.${SHLIB_MAJOR}\""
.endfor
.endif

SRCS.util+= \
	ralloc.c \
	os_file.c \
	mesa-sha1.c \
	u_printf.cpp \
	u_process.c 

CPPFLAGS.ralloc.c += -I${X11SRCDIR.Mesa}/src
CPPFLAGS.u_printf.cpp += -I${X11SRCDIR.Mesa}/src

.include "../libloader.mk"

.PATH:		${X11SRCDIR.Mesa}/src/glw
.PATH:		${X11SRCDIR.Mesa}/include/GL
.PATH:		${X11SRCDIR.Mesa}/src/mesa/drivers/x11
INCS=	gl.h glcorearb.h glext.h \
	glx.h glxext.h \
	osmesa.h wglext.h
INCSDIR=${X11INCDIR}/GL


SRCS+=	${SRCS.dri} ${SRCS.util} ${SRCS.glx} ${SRCS.glapi}

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/include \
	-I${X11SRCDIR.Mesa}/include/GL/internal \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/src/mapi \
	-I${X11SRCDIR.Mesa}/src/mapi/glapi \
	-I${X11SRCDIR.Mesa}/../src/mapi/glapi \
	-I${X11SRCDIR.Mesa}/src/glx \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${DESTDIR}${X11INCDIR}/GL \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}

#		-D_NETBSD_SOURCE \

# old
CPPFLAGS+=	-D__GLX_ALIGN64 \
		-DXF86VIDMODE \
		-DGLX_SHARED_GLAPI

# new
CPPFLAGS+=	\
	-DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 \
	-DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
	-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_DLFCN_H=1 -DYYTEXT_POINTER=1 \
	-DHAVE___BUILTIN_BSWAP32=1 -DHAVE___BUILTIN_BSWAP64=1 \
	-DHAVE___BUILTIN_CLZ=1 -DHAVE___BUILTIN_CLZLL=1 \
	-DHAVE___BUILTIN_CTZ=1 -DHAVE___BUILTIN_EXPECT=1 \
	-DHAVE___BUILTIN_FFS=1 -DHAVE___BUILTIN_FFSLL=1 \
	-DHAVE___BUILTIN_POPCOUNT=1 -DHAVE___BUILTIN_POPCOUNTLL=1 \
	-DHAVE___BUILTIN_UNREACHABLE=1 -DHAVE_FUNC_ATTRIBUTE_CONST=1 \
	-DHAVE_FUNC_ATTRIBUTE_FLATTEN=1 -DHAVE_FUNC_ATTRIBUTE_FORMAT=1 \
	-DHAVE_FUNC_ATTRIBUTE_MALLOC=1 -DHAVE_FUNC_ATTRIBUTE_PACKED=1 \
	-DHAVE_FUNC_ATTRIBUTE_PURE=1 -DHAVE_FUNC_ATTRIBUTE_RETURNS_NONNULL=1 \
	-DHAVE_FUNC_ATTRIBUTE_UNUSED=1 -DHAVE_FUNC_ATTRIBUTE_VISIBILITY=1 \
	-DHAVE_FUNC_ATTRIBUTE_WARN_UNUSED_RESULT=1 \
	-DHAVE_FUNC_ATTRIBUTE_WEAK=1 -DHAVE_FUNC_ATTRIBUTE_ALIAS=1 \
	-DHAVE_FUNC_ATTRIBUTE_NORETURN=1 -DHAVE_ENDIAN_H=1 -DHAVE_DLADDR=1 \
	-DHAVE_CLOCK_GETTIME=1 -DHAVE_PTHREAD_PRIO_INHERIT=1 \
	-DHAVE_PTHREAD=1 -DENABLE_ST_OMX_BELLAGIO=0 -DENABLE_ST_OMX_TIZONIA=0 \
	-DHAVE_TIMESPEC_GET -DGLX_USE_TLS

.include "../asm.mk"

CPPFLAGS+=	\
	-DHAVE_LIBDRM -DGLX_USE_DRM \
	-DGLX_INDIRECT_RENDERING -DGLX_DIRECT_RENDERING \
	-DHAVE_ALIAS -DMESA_EGL_NO_X11_HEADERS

CPPFLAGS+=	\
	-DUSE_EXTERNAL_DXTN_LIB=1 \
	-DYYTEXT_POINTER=1

CFLAGS+=	-fvisibility=hidden -fno-strict-aliasing -fno-builtin-memcmp
CFLAGS+=	-pthread

LIBDPLIBS=	Xext		${.CURDIR}/../libXext \
		X11		${.CURDIR}/../libX11/dynamic \
		Xxf86vm		${.CURDIR}/../libXxf86vm \
		Xfixes		${.CURDIR}/../libXfixes \
		Xdamage		${.CURDIR}/../libXdamage \
		glapi		${.CURDIR}/../libglapi${OLD_SUFFIX} \
		drm		${.CURDIR}/../libdrm \
		X11-xcb		${.CURDIR}/../libX11/libX11-xcb \
		xcb-dri2	${.CURDIR}/../libxcb/dri2 \
		xcb-glx		${.CURDIR}/../libxcb/glx \
		xcb-shm		${.CURDIR}/../libxcb/shm \
		expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat \
		m		${NETBSDSRCDIR}/lib/libm \
		pthread		${NETBSDSRCDIR}/lib/libpthread

MKLINT=no

PKGCONFIG=	gl
PKGDIST.gl=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL${OLD_SUFFIX}/mesa-ver.mk"
PKGCONFIG_VERSION.gl=	${MESA_VER}

# XXX remove these from bsd.x11.mk

PKGCONFIG_SED_FLAGS= \
	-e "s,@GL_PC_REQ_PRIV@,libdrm x11 xext xdamage xfixes xcb-glx xcb-dri2 xxf86vm,; \
	    s,@GL_PKGCONF_LIB@,GL,; \
	    s,@GL_PC_LIB_PRIV@,-lm -lpthread -pthread,; \
	    s,@GL_PC_CFLAGS@,,; \
	    s,@GLX_TLS@,yes,"


CWARNFLAGS.clang+=	-Wno-tautological-compare -Wno-format -Wno-constant-conversion \
			-Wno-error=incompatible-pointer-types -Wno-error=atomic-alignment
CWARNFLAGS.gcc+=	${CC_WNO_STRINGOP_OVERFLOW}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
