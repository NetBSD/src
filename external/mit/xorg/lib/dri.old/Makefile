# $NetBSD: Makefile,v 1.7 2023/07/15 21:24:46 rjs Exp $

# Link the mesa_dri_drivers mega driver.

# Uses 'assert(cond)' in a comma expression, leading to a syntax error because
# in lint mode, 'assert(cond)' expands to an empty string, and '(, expr)' is
# not an expression.
NOLINT=

LINTFLAGS+=	-X 213		# void function '%s' cannot return value
LINTFLAGS+=	-Ac23		# allow C23, for empty initializer braces

# When build_id.c expands the macro invocation 'ElfW(Nhdr)', the result is
# 'Elf64 /* MD native binary size */ _Nhdr'. The comment prevents the two
# identifiers from being concatenated.
build_id.ln:
	touch $@

.include <bsd.own.mk>

.include "../mesa-which.mk"

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE} == "evbarm"

LIBISMODULE=	yes
LIBISCXX=     yes

SHLIB_MAJOR=	0

LIB=		mesa_dri_drivers
DRIDIR=		${X11USRLIBDIR}/modules/dri
DRIDEBUGDIR=	${DEBUGDIR}${X11USRLIBDIR}/modules/dri

LDFLAGS+=	-Wl,--build-id=sha1

#	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/${MODULE}/server \

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/src/egl/main \
	-I${X11SRCDIR.Mesa}/src/egl/drivers/dri \
	-I${X11SRCDIR.Mesa}/../src/mesa/drivers/dri/common \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${X11SRCDIR.Mesa}/../src/util

.if ${MACHINE_ARCH} == "i386"
CPPFLAGS.brw_disk_cache.c+=	-march=i586
.endif

#CPPFLAGS+=	-D_NETBSD_SOURCE -DPTHREADS

# We don't actually build this on non-x86/non-evbarm at all, currently.
# The following if statements are not effective since we only
# get here for x86 and evbarm
.if ${MACHINE_ARCH} == "alpha"
DRIVERS=	r200 radeon
.elif ${MACHINE} == "macppc" || ${MACHINE} == "ofppc"
DRIVERS=	r200 radeon
.elif ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "sparc"
DRIVERS=	r200 radeon
.elif ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
DRIVERS=	i915 i965 r200 radeon
.elif ${MACHINE} == "prep" || ${MACHINE} == "bebox"
DRIVERS=	r200 radeon
.elif ${MACHINE} == "evbarm"
DRIVERS=	r200 radeon
.endif

DRI_SUBDIRS= ${DRIVERS}

DRI_SOURCES.i915+= \
	i830_context.c \
	i830_state.c \
	i830_texblend.c \
	i830_texstate.c \
	i830_vtbl.c \
	i915_context.c \
	i915_debug_fp.c \
	i915_fragprog.c \
	i915_program.c \
	i915_state.c \
	i915_texstate.c \
	i915_vtbl.c \
	i915_tex_layout.c

I915_INTEL_FILES = \
	intel_batchbuffer.c \
	intel_blit.c \
	intel_buffer_objects.c \
	intel_buffers.c \
	intel_clear.c \
	intel_context.c \
	intel_extensions.c \
	intel_fbo.c \
	intel_mipmap_tree.c \
	intel_pixel.c \
	intel_pixel_bitmap.c \
	intel_pixel_copy.c \
	intel_pixel_draw.c \
	intel_pixel_read.c \
	intel_regions.c \
	intel_render.c \
	intel_screen.c \
	intel_state.c \
	intel_syncobj.c \
	intel_tex.c \
	intel_tex_copy.c \
	intel_tex_image.c \
	intel_tex_layout.c \
	intel_tex_subimage.c \
	intel_tex_validate.c \
	intel_tris.c

.for _f in ${I915_INTEL_FILES}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i915/${_f} i915_${_f}
DRI_SOURCES.i915+=	i915_${_f}
CPPFLAGS.i915_${_f}+=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i915
.endfor

.PATH: ${X11SRCDIR.Mesa}/src/intel/blorp
.PATH: ${X11SRCDIR.Mesa}/src/intel/common
.PATH: ${X11SRCDIR.Mesa}/src/intel/compiler
.PATH: ${X11SRCDIR.Mesa}/src/intel/dev
.PATH: ${X11SRCDIR.Mesa}/src/intel/isl
.PATH: ${X11SRCDIR.Mesa}/src/intel/perf
.PATH: ${X11SRCDIR.Mesa}/../src/intel/
.PATH: ${X11SRCDIR.Mesa}/../src/intel/perf

DRI_SOURCES.i965+= \
	blorp.c \
	blorp_blit.c \
	blorp_clear.c \
	gen_batch_decoder.c \
	gen_debug.c \
	gen_decoder.c \
	gen_device_info.c \
	gen_disasm.c \
	gen_l3_config.c \
	gen_perf.c \
	gen_perf_mdapi.c \
	gen_perf_metrics.c \
	gen_urb_config.c \
	intel_log.c \
	brw_binding_tables.c \
	brw_blorp.c \
	brw_bufmgr.c \
	brw_cfg.cpp \
	brw_clear.c \
	brw_clip.c \
	brw_clip_line.c \
	brw_clip_point.c \
	brw_clip_tri.c \
	brw_clip_unfilled.c \
	brw_clip_util.c \
	brw_compile_clip.c \
	brw_compile_sf.c \
	brw_compiler.c \
	brw_compute.c \
	brw_conditional_render.c \
	brw_context.c \
	brw_cs.c \
	brw_curbe.c \
	brw_dead_control_flow.cpp \
	brw_debug_recompile.c \
	brw_disasm.c \
	brw_disasm_info.c \
	brw_disk_cache.c \
	brw_draw.c \
	brw_draw_upload.c \
	brw_eu.c \
	brw_eu_compact.c \
	brw_eu_emit.c \
	brw_eu_util.c \
	brw_eu_validate.c \
	brw_ff_gs.c \
	brw_ff_gs_emit.c \
	brw_formatquery.c \
	brw_fs.cpp \
	brw_fs_bank_conflicts.cpp \
	brw_fs_cmod_propagation.cpp \
	brw_fs_combine_constants.cpp \
	brw_fs_copy_propagation.cpp \
	brw_fs_cse.cpp \
	brw_fs_dead_code_eliminate.cpp \
	brw_fs_generator.cpp \
	brw_fs_live_variables.cpp \
	brw_fs_lower_pack.cpp \
	brw_fs_lower_regioning.cpp \
	brw_fs_nir.cpp \
	brw_fs_reg_allocate.cpp \
	brw_fs_register_coalesce.cpp \
	brw_fs_saturate_propagation.cpp \
	brw_fs_sel_peephole.cpp \
	brw_fs_validate.cpp \
	brw_fs_visitor.cpp \
	brw_generate_mipmap.c \
	brw_gs.c \
	brw_gs_surface_state.c \
	brw_interpolation_map.c \
	brw_link.cpp \
	brw_meta_util.c \
	brw_misc_state.c \
	brw_nir.c \
	brw_nir_analyze_boolean_resolves.c \
	brw_nir_analyze_ubo_ranges.c \
	brw_nir_attribute_workarounds.c \
	brw_nir_lower_conversions.c \
	brw_nir_lower_cs_intrinsics.c \
	brw_nir_lower_image_load_store.c \
	brw_nir_lower_mem_access_bit_sizes.c \
	brw_nir_opt_peephole_ffma.c \
	brw_nir_tcs_workarounds.c \
	brw_nir_trig_workarounds.c \
	brw_nir_uniforms.cpp \
	brw_object_purgeable.c \
	brw_packed_float.c \
	brw_performance_query.c \
	brw_performance_query_mdapi.c \
	brw_pipe_control.c \
	brw_predicated_break.cpp \
	brw_primitive_restart.c \
	brw_program.c \
	brw_program_binary.c \
	brw_program_cache.c \
	brw_queryobj.c \
	brw_reg_type.c \
	brw_reset.c \
	brw_schedule_instructions.cpp \
	brw_sf.c \
	brw_shader.cpp \
	brw_state_upload.c \
	brw_surface_formats.c \
	brw_sync.c \
	brw_tcs.c \
	brw_tcs_surface_state.c \
	brw_tes.c \
	brw_tes_surface_state.c \
	brw_urb.c \
	brw_util.c \
	brw_vec4.cpp \
	brw_vec4_cmod_propagation.cpp \
	brw_vec4_copy_propagation.cpp \
	brw_vec4_cse.cpp \
	brw_vec4_dead_code_eliminate.cpp \
	brw_vec4_generator.cpp \
	brw_vec4_gs_nir.cpp \
	brw_vec4_gs_visitor.cpp \
	brw_vec4_live_variables.cpp \
	brw_vec4_nir.cpp \
	brw_vec4_reg_allocate.cpp \
	brw_vec4_surface_builder.cpp \
	brw_vec4_tcs.cpp \
	brw_vec4_tes.cpp \
	brw_vec4_visitor.cpp \
	brw_vec4_vs_visitor.cpp \
	brw_vs.c \
	brw_vs_surface_state.c \
	brw_vue_map.c \
	brw_wm.c \
	brw_wm_iz.cpp \
	brw_wm_surface_state.c \
	gen6_clip_state.c \
	gen6_constant_state.c \
	gen6_gs_visitor.cpp \
	gen6_multisample_state.c \
	gen6_queryobj.c \
	gen6_sampler_state.c \
	gen6_sol.c \
	gen6_urb.c \
	gen7_l3_state.c \
	gen7_sol_state.c \
	gen7_urb.c \
	gen8_depth_state.c \
	gen8_multisample_state.c \
	hsw_queryobj.c \
	hsw_sol.c \
	isl.c \
	isl_drm.c \
	isl_format.c \
	isl_format_layout.c \
	isl_gen4.c \
	isl_gen6.c \
	isl_gen7.c \
	isl_gen8.c \
	isl_gen9.c \
	isl_storage_image.c \
	isl_tiled_memcpy.c \
	isl_tiled_memcpy_normal.c \
	isl_tiled_memcpy_sse41.c

I965_INTEL_FILES = \
	intel_batchbuffer.c \
	intel_blit.c \
	intel_buffer_objects.c \
	intel_buffers.c \
	intel_copy_image.c \
	intel_extensions.c \
	intel_fbo.c \
	intel_mipmap_tree.c \
	intel_pixel.c \
	intel_pixel_bitmap.c \
	intel_pixel_copy.c \
	intel_pixel_draw.c \
	intel_pixel_read.c \
	intel_screen.c \
	intel_state.c \
	intel_tex.c \
	intel_tex_copy.c \
	intel_tex_image.c \
	intel_tex_validate.c \
	intel_upload.c


INTEL_GENS_BLORP=	40 45 50 60 70 75 80 90 100 110

.for _gen in ${INTEL_GENS_BLORP}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i965/genX_state_upload.c ${_gen}_state_upload.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i965/genX_blorp_exec.c ${_gen}_blorp_exec.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i965/genX_pipe_control.c ${_gen}_pipe_control.c
DRI_SOURCES.i965+=	${_gen}_state_upload.c ${_gen}_blorp_exec.c ${_gen}_pipe_control.c

CPPFLAGS.${_gen}_state_upload.c+=	-DGEN_VERSIONx10=${_gen}
CPPFLAGS.${_gen}_blorp_exec.c+=		-DGEN_VERSIONx10=${_gen}
CPPFLAGS.${_gen}_pipe_control.c+=	-DGEN_VERSIONx10=${_gen}
.endfor

INTEL_GENS_ISL=	40 50 60 70 75 80 90 100 110

.for _gen in ${INTEL_GENS_ISL}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/intel/isl/isl_emit_depth_stencil.c ${_gen}_isl_emit_depth_stencil.c
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/intel/isl/isl_surface_state.c ${_gen}_isl_surface_state.c
DRI_SOURCES.i965+=	${_gen}_isl_emit_depth_stencil.c ${_gen}_isl_surface_state.c

CPPFLAGS.${_gen}_isl_emit_depth_stencil.c+=	-DGEN_VERSIONx10=${_gen} -I${X11SRCDIR.Mesa}/src/intel/isl/
CPPFLAGS.${_gen}_isl_surface_state.c+=		-DGEN_VERSIONx10=${_gen} -I${X11SRCDIR.Mesa}/src/intel/isl/
.endfor

.for _f in ${I965_INTEL_FILES}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i965/${_f} i965_${_f}
DRI_SOURCES.i965+=	i965_${_f}
.endfor

.for _f in ${DRI_SOURCES.i965}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/i965 \
			-I${X11SRCDIR.Mesa}/src/intel \
			-I${X11SRCDIR.Mesa}/src/intel/compiler \
			-I${X11SRCDIR.Mesa}/../src/intel \
			-I${X11SRCDIR.Mesa}/src/compiler/nir \
			-I${X11SRCDIR.Mesa}/../src/compiler/nir
.endfor

# Needs mfence
CPPFLAGS.brw_bufmgr.c+=	-msse2

DRI_SOURCES.r200 = \
	r200_context.c \
	r200_ioctl.c \
	r200_state.c \
	r200_state_init.c \
	r200_cmdbuf.c \
	r200_tex.c \
	r200_texstate.c \
	r200_tcl.c \
	r200_swtcl.c \
	r200_maos.c \
	r200_sanity.c \
	r200_fragshader.c \
	r200_vertprog.c \
	r200_blit.c

R200_RADEON_FILES= \
	radeon_buffer_objects.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_fog.c \
	radeon_mipmap_tree.c \
	radeon_pixel_read.c \
	radeon_queryobj.c \
	radeon_span.c \
	radeon_texture.c \
	radeon_tex_copy.c \
	radeon_tile.c \
	radeon_screen.c

.for _f in ${R200_RADEON_FILES}
BUILDSYMLINKS+=		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/radeon/${_f} r200_${_f}
DRI_SOURCES.r200+=	r200_${_f}
.endfor

.for _f in ${DRI_SOURCES.r200}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/r200/server \
			-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/r200 \
			-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/radeon/server \
			-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/radeon \
			-DRADEON_R200
.endfor

DRI_SOURCES.radeon = \
	radeon_buffer_objects.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_fog.c \
	radeon_mipmap_tree.c \
	radeon_pixel_read.c \
	radeon_queryobj.c \
	radeon_span.c \
	radeon_texture.c \
	radeon_tex_copy.c \
	radeon_tile.c \
	radeon_context.c \
	radeon_ioctl.c \
	radeon_screen.c \
	radeon_state.c \
	radeon_state_init.c \
	radeon_tex.c \
	radeon_texstate.c \
	radeon_tcl.c \
	radeon_swtcl.c \
	radeon_maos.c \
	radeon_sanity.c \
	radeon_blit.c

.for _f in ${DRI_SOURCES.radeon}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/radeon/server \
			-I${X11SRCDIR.Mesa}/src/mesa/drivers/dri/radeon \
			-DRADEON_R100
.endfor

.for _d in ${DRI_SUBDIRS}
SRCS+=	${DRI_SOURCES.${_d}}
.PATH: ${X11SRCDIR.Mesa}/src/mesa/drivers/dri/${_d}
.endfor


LIBDPLIBS+=	expat		${NETBSDSRCDIR}/external/mit/expat/lib/libexpat
LIBDPLIBS+=	m		${NETBSDSRCDIR}/lib/libm
LIBDPLIBS+=	pthread		${NETBSDSRCDIR}/lib/libpthread
LIBDPLIBS+= 	glapi		${.CURDIR}/../libglapi${OLD_SUFFIX}
LIBDPLIBS+= 	drm		${.CURDIR}/../libdrm
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
LIBDPLIBS+= 	drm_intel	${.CURDIR}/../libdrm_intel
.endif
LIBDPLIBS+= 	drm_radeon	${.CURDIR}/../libdrm_radeon

MESA_SRC_MODULES=  main math math_xform vbo tnl swrast ss common asm_c program asm_s
.include "../libmesa${OLD_SUFFIX}.mk"
.include "../libglsl${OLD_SUFFIX}.mk"

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SRCS+=	streaming-load-memcpy.c
CPPFLAGS.streaming-load-memcpy.c+=	-msse4.1
CPPFLAGS.isl_tiled_memcpy_sse41.c+=	-msse4.1
.endif

CFLAGS+= ${${ACTIVE_CC} == "clang":? -Wno-error=atomic-alignment :}

.include "../driver${OLD_SUFFIX}.mk"

.for _d in ${DRIVERS}
SYMLINKS+= mesa_dri_drivers.so.${SHLIB_MAJOR} ${DRIDIR}/${_d}_dri.so.${SHLIB_MAJOR} 
SYMLINKS+= ${_d}_dri.so.${SHLIB_MAJOR} ${DRIDIR}/${_d}_dri.so
.if ${MKDEBUG} != "no"
SYMLINKS+= mesa_dri_drivers.so.${SHLIB_MAJOR}.debug ${DRIDEBUGDIR}/${_d}_dri.so.${SHLIB_MAJOR}.debug
.endif
.endfor

.endif

PKGCONFIG=	dri
PKGDIST.dri=	${X11SRCDIR.Mesa}/../src/pkgconfig
.include "${.CURDIR}/../libGL${OLD_SUFFIX}/mesa-ver.mk"
PKGCONFIG_VERSION.dri=	${MESA_VER}

# XXX remove these from bsd.x11.mk
PKGCONFIG_SED_FLAGS= \
	-e "s,@DRI_DRIVER_INSTALL_DIR@,${X11USRLIBDIR}/modules/dri,; \
	    s,@DRI_PC_REQ_PRIV@,,"

.PATH:          ${X11SRCDIR.Mesa}/src/util

FILESDIR=	/etc
BUILDSYMLINKS+=	00-mesa-defaults.conf drirc
FILES=		drirc

.PATH:          ${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common

.include <bsd.x11.mk>
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE} == "evbarm"
LIBDIR=		${X11USRLIBDIR}/modules/dri

CWARNFLAGS.clang+=	-Wno-error=initializer-overrides -Wno-error=switch \
			-Wno-error=tautological-constant-out-of-range-compare \
			-Wno-error=enum-conversion \
			-Wno-error=implicit-int-float-conversion \
			-Wno-error=tautological-constant-compare \
			-Wno-c99-designator -Wno-xor-used-as-pow

COPTS+= -Wno-error=stack-protector

COPTS.u_atomic.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-builtin-declaration-mismatch :}

COPTS.brw_eu_compact.c+=	-Wno-error=stack-protector
COPTS.brw_fs_copy_propagation.cpp+=	-Wno-error=stack-protector
COPTS.brw_fs.cpp+=	-Wno-error=stack-protector
COPTS.brw_fs_reg_allocate.cpp+=	-Wno-error=stack-protector

.include <bsd.lib.mk>
.else
.include <bsd.inc.mk>
.endif
# Don't re-build .c files when .y files change
.y.c:
