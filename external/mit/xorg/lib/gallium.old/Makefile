# $NetBSD: Makefile,v 1.6 2023/07/15 21:24:46 rjs Exp $

# Transparent struct/union broken
NOLINT=yes

# Link the gallium mega driver.

LIBISMODULE=	yes
LIBISCXX=     yes

.include <bsd.own.mk>

.include "../mesa-which.mk"

SHLIB_MAJOR=    0

LIB=		gallium_dri
DRIDIR=		${X11USRLIBDIR}/modules/dri
DRIDEBUGDIR=	${DEBUGDIR}${X11USRLIBDIR}/modules/dri

LLVM_INCLUDE_OBJDIR!=	cd ${NETBSDSRCDIR}/external/apache2/llvm/include && ${PRINTOBJDIR}

CWARNFLAGS.clang += -Wno-atomic-alignment -Wno-unknown-warning-option -Wno-implicit-int-float-conversion

CXXFLAGS+=	-std=c++14
CFLAGS+=	-std=gnu11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${NETBSDSRCDIR}/external/apache2/llvm/dist/llvm/include \
		-I${LLVM_INCLUDE_OBJDIR} -I${NETBSDSRCDIR}/external/apache2/llvm/config

CFLAGS+=	-pthread
LDFLAGS+=	-pthread

# Reduce debugging for these extremely large objects.
.if ${MKDEBUG:Uno} != "no"
CXXFLAGS+=	-g1
CFLAGS+=	-g1
.endif

GALLIUM_SUBDIRS= \
	auxiliary \
	auxiliary/cso_cache \
	auxiliary/draw \
	auxiliary/driver_ddebug \
	auxiliary/driver_noop \
	auxiliary/driver_rbug \
	auxiliary/driver_trace \
	auxiliary/hud \
	auxiliary/indices \
	auxiliary/nir \
	auxiliary/os \
	auxiliary/pipebuffer \
	auxiliary/pipe-loader \
	auxiliary/postprocess \
	auxiliary/rbug \
	auxiliary/renderonly \
	auxiliary/rtasm \
	auxiliary/tgsi \
	auxiliary/translate \
	auxiliary/util \
	auxiliary/vl \
	drivers/noop \
	drivers/rbug \
	drivers/softpipe \
	winsys/sw/dri \
	winsys/sw/kms-dri \
	winsys/sw/null \
	winsys/sw/wrapper \
	state_trackers/dri

GALLIUM_SUBDIRS_VDPAU=	\
	state_trackers/vdpau

GALLIUM_SUBDIRS_LLVMPIPE= \
	auxiliary/gallivm \
	drivers/llvmpipe

GALLIUM_SUBDIRS_ATI= \
	drivers/radeon \
	drivers/radeonsi \
	drivers/r300 \
	drivers/r300/compiler \
	drivers/r600 \
	drivers/r600/sb \
	winsys/radeon/drm \
	winsys/amdgpu/drm

GALLIUM_SUBDIRS_I915= \
	drivers/i915 \
	winsys/i915/drm

GALLIUM_SUBDIRS_NOUVEAU= \
	drivers/nouveau \
	drivers/nouveau/nv30 \
	drivers/nouveau/nv50 \
	drivers/nouveau/codegen \
	drivers/nouveau/nvc0 \
	winsys/nouveau/drm

BUILD_RADEON=0
BUILD_NOUVEAU=0
BUILD_VDPAU=0
BUILD_LLVMPIPE=0
BUILD_I915=0
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
BUILD_I915=1
.endif
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE} == "evbarm"
BUILD_NOUVEAU=1
. if ${MKLLVMRT} != "no"
BUILD_RADEON=1
. endif
BUILD_VDPAU=1
.endif

.if ${MACHINE_ARCH} == "i386" || \
    ${MACHINE_ARCH} == "x86_64" || \
    ${MACHINE_ARCH} == "powerpc" || \
    ${MACHINE_ARCH} == "powerpc64" || \
    !empty(MACHINE_ARCH:Maarch64*) || \
    !empty(MACHINE_ARCH:Mmips*) || \
    !empty(MACHINE_ARCH:Mearm*) || \
    !empty(MACHINE_ARCH:Marm*)
. if ${MKLLVMRT} != "no"
BUILD_LLVMPIPE=1
. endif
.endif

DRIVERS=		kms_swrast swrast
.if ${BUILD_RADEON} == 1
DRIVERS+=		r600 r300 radeonsi
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_ATI}
.endif
.if ${BUILD_NOUVEAU} == 1
DRIVERS+=		nouveau
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_NOUVEAU}
.endif
.if ${BUILD_VDPAU} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_VDPAU}
.endif
.if ${BUILD_I915} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_I915}
.endif
.if ${BUILD_LLVMPIPE} == 1
GALLIUM_SUBDIRS+=	${GALLIUM_SUBDIRS_LLVMPIPE}
.endif

GALLIUM_SOURCES.auxiliary/cso_cache= \
	cso_cache.c \
	cso_context.c \
	cso_hash.c

GALLIUM_SOURCES.auxiliary/draw= \
	draw_context.c \
	draw_fs.c \
	draw_gs.c \
	draw_pipe_aaline.c \
	draw_pipe_aapoint.c \
	draw_pipe.c \
	draw_pipe_clip.c \
	draw_pipe_cull.c \
	draw_pipe_flatshade.c \
	draw_pipe_offset.c \
	draw_pipe_pstipple.c \
	draw_pipe_stipple.c \
	draw_pipe_twoside.c \
	draw_pipe_unfilled.c \
	draw_pipe_util.c \
	draw_pipe_validate.c \
	draw_pipe_vbuf.c \
	draw_pipe_wide_line.c \
	draw_pipe_wide_point.c \
	draw_prim_assembler.c \
	draw_pt.c \
	draw_pt_emit.c \
	draw_pt_fetch.c \
	draw_pt_fetch_emit.c \
	draw_pt_fetch_shade_emit.c \
	draw_pt_fetch_shade_pipeline.c \
	draw_pt_post_vs.c \
	draw_pt_so_emit.c \
	draw_pt_util.c \
	draw_pt_vsplit.c \
	draw_vertex.c \
	draw_vs.c \
	draw_vs_exec.c \
	draw_vs_variant.c

.if ${MKLLVMRT} != "no"
GALLIUM_SOURCES.auxiliary/draw+= \
	draw_llvm.c \
	draw_llvm_sample.c \
	draw_pt_fetch_shade_pipeline_llvm.c \
	draw_vs_llvm.c
.endif

GALLIUM_SOURCES.auxiliary/hud= \
	font.c \
	hud_context.c \
	hud_cpu.c \
	hud_nic.c \
	hud_cpufreq.c \
	hud_diskstat.c \
	hud_sensors_temp.c \
	hud_fps.c \
	hud_driver_query.c
GALLIUM_SOURCES.auxiliary/indices= \
	u_primconvert.c
GALLIUM_SOURCES.auxiliary/os= \
	os_process.c
GALLIUM_SOURCES.auxiliary/pipe-loader= \
	pipe_loader.c \
	pipe_loader_drm.c \
	pipe_loader_sw.c

.for _f in ${GALLIUM_SOURCES.auxiliary/pipe-loader}
CPPFLAGS.${_f} +=	-DHAVE_PIPE_LOADER_KMS=1 \
			-DHAVE_PIPE_LOADER_DRI=1 \
			-DGALLIUM_STATIC_TARGETS=1 \
			-I${X11SRCDIR.Mesa}/src/gallium/winsys \
			-I${X11SRCDIR.Mesa}/src/loader \
			-I${X11SRCDIR.Mesa}/../src/util
.endfor

GALLIUM_SOURCES.auxiliary/pipebuffer= \
	pb_buffer_fenced.c \
	pb_bufmgr_cache.c \
	pb_bufmgr_debug.c \
	pb_bufmgr_mm.c \
	pb_bufmgr_slab.c \
	pb_cache.c \
	pb_slab.c \
	pb_validate.c
GALLIUM_SOURCES.auxiliary/postprocess= \
	pp_celshade.c \
	pp_colors.c \
	pp_init.c \
	pp_mlaa.c \
	pp_run.c \
	pp_program.c
GALLIUM_SOURCES.auxiliary/rbug= \
	rbug_connection.c \
	RBUGrbug_context.c \
	RBUGrbug_core.c \
	rbug_demarshal.c \
	rbug_texture.c \
	rbug_shader.c
GALLIUM_SOURCES.auxiliary/rtasm= \
	rtasm_cpu.c \
	rtasm_execmem.c \
	rtasm_x86sse.c
GALLIUM_SOURCES.auxiliary/tgsi= \
	tgsi_aa_point.c \
	tgsi_build.c \
	tgsi_dump.c \
	tgsi_exec.c \
	tgsi_emulate.c \
	tgsi_from_mesa.c \
	tgsi_info.c \
	tgsi_iterate.c \
	tgsi_lowering.c \
	tgsi_parse.c \
	tgsi_point_sprite.c \
	tgsi_sanity.c \
	tgsi_scan.c \
	tgsi_strings.c \
	tgsi_text.c \
	tgsi_transform.c \
	tgsi_two_side.c \
	tgsi_ureg.c \
	tgsi_util.c
GALLIUM_SOURCES.auxiliary/translate= \
	translate.c \
	translate_cache.c \
	translate_generic.c \
	translate_sse.c
GALLIUM_SOURCES.auxiliary/util= \
	os_misc.c \
	u_async_debug.c \
	u_bitmask.c \
	u_blit.c \
	u_blitter.c \
	u_cache.c \
	u_debug.c \
	u_debug_describe.c \
	u_debug_flush.c \
	u_debug_gallium.c \
	u_debug_image.c \
	u_debug_memory.c \
	u_debug_refcnt.c \
	u_debug_stack.c \
	u_debug_symbol.c \
	u_dl.c \
	u_draw.c \
	u_draw_quad.c \
	u_dump_defines.c \
	u_dump_state.c \
	u_format.c \
	u_format_bptc.c \
	u_format_etc.c \
	u_format_latc.c \
	u_format_other.c \
	u_format_rgtc.c \
	u_format_s3tc.c \
	u_format_tests.c \
	u_format_yuv.c \
	u_format_zs.c \
	u_framebuffer.c \
	u_gen_mipmap.c \
	u_handle_table.c \
	u_hash_table.c \
	u_helpers.c \
	u_idalloc.c \
	u_index_modify.c \
	u_linear.c \
	u_log.c \
	u_mm.c \
	u_network.c \
	u_prim.c \
	u_prim_restart.c \
	u_pstipple.c \
	u_resource.c \
	u_ringbuffer.c \
	u_sampler.c \
	u_screen.c \
	u_simple_shaders.c \
	u_suballoc.c \
	u_surface.c \
	u_surfaces.c \
	u_tests.c \
	u_texture.c \
	u_tile.c \
	u_transfer.c \
	u_transfer_helper.c \
	u_threaded_context.c \
	u_upload_mgr.c \
	u_vbuf.c
GALLIUM_SOURCES.auxiliary/nir = \
	tgsi_to_nir.c
GALLIUM_SOURCES.auxiliary/vl= \
	vl_bicubic_filter.c \
	vl_compositor.c \
	vl_compositor_cs.c \
	vl_compositor_gfx.c \
	vl_csc.c \
	vl_decoder.c \
	vl_deint_filter.c \
	vl_idct.c \
	vl_matrix_filter.c \
	vl_mc.c \
	vl_median_filter.c \
	vl_mpeg12_bitstream.c \
	vl_mpeg12_decoder.c \
	vl_vertex_buffers.c \
	vl_video_buffer.c \
	vl_winsys_dri.c \
	vl_zscan.c

# Generated
.PATH: ${X11SRCDIR.Mesa}/../src/gallium/auxiliary
GALLIUM_SOURCES.auxiliary= \
	u_indices_gen.c \
	u_unfilled_gen.c \
	u_format_table.c
.for _f in ${GALLIUM_SOURCES.auxiliary/vl}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/loader
.endfor
.for _f in ${GALLIUM_SOURCES.auxiliary}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/util
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/util
.endfor

GALLIUM_SOURCES.auxiliary/renderonly= \
	renderonly.c

GALLIUM_SOURCES.auxiliary/gallivm= \
	lp_bld_arit.c \
	lp_bld_arit_overflow.c \
	lp_bld_assert.c \
	lp_bld_bitarit.c \
	lp_bld_const.c \
	lp_bld_conv.c \
	lp_bld_flow.c \
	lp_bld_format_aos_array.c \
	lp_bld_format_aos.c \
	lp_bld_format_float.c \
	lp_bld_format.c \
	lp_bld_format_soa.c \
	lp_bld_format_s3tc.c \
	lp_bld_format_srgb.c \
	lp_bld_format_yuv.c \
	lp_bld_gather.c \
	lp_bld_init.c \
	lp_bld_intr.c \
	lp_bld_logic.c \
	lp_bld_misc.cpp \
	lp_bld_pack.c \
	lp_bld_printf.c \
	lp_bld_quad.c \
	lp_bld_sample_aos.c \
	lp_bld_sample.c \
	lp_bld_sample_soa.c \
	lp_bld_struct.c \
	lp_bld_swizzle.c \
	lp_bld_tgsi_action.c \
	lp_bld_tgsi_aos.c \
	lp_bld_tgsi.c \
	lp_bld_tgsi_info.c \
	lp_bld_tgsi_soa.c \
	lp_bld_type.c
	#lp_bld_debug.cpp

.for _f in ${GALLIUM_SOURCES.auxiliary/gallivm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/util
.endfor


GALLIUM_SOURCES.auxiliary/driver_ddebug = \
        dd_context.c \
	dd_draw.c \
	dd_screen.c

GALLIUM_SOURCES.auxiliary/driver_noop = \
        noop_pipe.c \
        noop_state.c

GALLIUM_SOURCES.auxiliary/driver_trace = \
        tr_context.c \
        tr_dump.c \
        tr_dump_state.c \
        tr_screen.c \
        tr_texture.c

GALLIUM_SOURCES.auxiliary/driver_rbug = \
        DRIVERrbug_core.c \
        DRIVERrbug_context.c \
        rbug_objects.c \
        rbug_screen.c
# Conflicts with auxiliary/rbug/ files
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/auxiliary/rbug/rbug_core.c		RBUGrbug_core.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/auxiliary/rbug/rbug_context.c		RBUGrbug_context.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/auxiliary/driver_rbug/rbug_core.c	DRIVERrbug_core.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/auxiliary/driver_rbug/rbug_context.c	DRIVERrbug_context.c
CPPFLAGS.DRIVERrbug_core.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary
CPPFLAGS.DRIVERrbug_core.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/driver_rbug
CPPFLAGS.DRIVERrbug_core.c+=	-I${X11SRCDIR.Mesa}/src/gallium/drivers
CPPFLAGS.DRIVERrbug_context.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary
CPPFLAGS.DRIVERrbug_context.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary/driver_rbug
CPPFLAGS.DRIVERrbug_context.c+=	-I${X11SRCDIR.Mesa}/src/gallium/drivers

GALLIUM_SOURCES.drivers/i915= \
	i915_blit.c \
	i915_clear.c \
	i915_context.c \
	i915_debug.c \
	i915_debug_fp.c \
	i915_flush.c \
	i915_fpc_emit.c \
	i915_fpc_optimize.c \
	i915_fpc_translate.c \
	i915_prim_emit.c \
	i915_prim_vbuf.c \
	i915_query.c \
	i915_resource_buffer.c \
	i915_resource.c \
	i915_resource_texture.c \
	i915_screen.c \
	i915_state.c \
	i915_state_derived.c \
	i915_state_dynamic.c \
	i915_state_emit.c \
	i915_state_fpc.c \
	i915_state_immediate.c \
	i915_state_sampler.c \
	i915_state_static.c \
	i915_surface.c

GALLIUM_SOURCES.winsys/i915/drm= \
	i915_drm_batchbuffer.c \
	i915_drm_buffer.c \
	i915_drm_fence.c \
	i915_drm_winsys.c

# Conflicts with r600/radeon_video.c and radeon/radeon_video.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/r600/radeon_video.c r600_radeon_video.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/radeon/radeon_video.c radeon_radeon_video.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/r600/radeon_vce.c r600_radeon_vce.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/radeon/radeon_vce.c radeon_radeon_vce.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/r600/radeon_uvd.c r600_radeon_uvd.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/gallium/drivers/radeon/radeon_uvd.c radeon_radeon_uvd.c

GALLIUM_SOURCES.drivers/radeon = \
        cayman_msaa.c \
        r600_buffer_common.c \
        r600_pipe_common.c \
        r600_query.c \
        r600_streamout.c \
        r600_texture.c \
        radeon_radeon_uvd.c \
	radeon_uvd_enc.c \
	radeon_uvd_enc_1_1.c \
        radeon_vce_40_2_2.c \
	radeon_vce_50.c \
	radeon_vce_52.c \
	radeon_vcn_dec.c \
	radeon_vcn_dec_jpeg.c \
	radeon_vcn_enc.c \
	radeon_vcn_enc_1_2.c \
        radeon_radeon_vce.c \
        radeon_radeon_video.c \

GALLIUM_SOURCES.drivers/r600 = \
	eg_debug.c \
	r600_gpu_load.c \
	r600_perfcounter.c \
	r600_radeon_vce.c \
	r600_radeon_video.c \
        compute_memory_pool.c \
	r600_viewport.c \
        eg_asm.c \
        evergreen_compute.c \
        evergreen_hw_context.c \
        evergreen_state.c \
        r600_asm.c \
        r600_blit.c \
        r600_hw_context.c \
        r600_isa.c \
        r600_pipe.c \
        r600_shader.c \
        r600_state.c \
        r600_state_common.c \
        r600_uvd.c \
        r600_radeon_uvd.c \
	r600_test_dma.c \
        r700_asm.c
GALLIUM_SOURCES.drivers/r600/sb = \
        sb_bc_builder.cpp \
        sb_bc_decoder.cpp \
        sb_bc_dump.cpp \
        sb_bc_finalize.cpp \
        sb_bc_parser.cpp \
        sb_context.cpp \
        sb_core.cpp \
        sb_dce_cleanup.cpp \
        sb_def_use.cpp \
        sb_dump.cpp \
        sb_expr.cpp \
        sb_gcm.cpp \
        sb_gvn.cpp \
        sb_if_conversion.cpp \
        sb_ir.cpp \
        sb_liveness.cpp \
        sb_pass.cpp \
        sb_peephole.cpp \
        sb_psi_ops.cpp \
        sb_ra_checker.cpp \
        sb_ra_coalesce.cpp \
        sb_ra_init.cpp \
        sb_sched.cpp \
        sb_shader.cpp \
        sb_ssa_builder.cpp \
        sb_valtable.cpp

GALLIUM_SOURCES.drivers/r300 = \
	r300_blit.c \
	r300_chipset.c \
	r300_context.c \
	r300_debug.c \
	r300_emit.c \
	r300_flush.c \
	r300_fs.c \
	r300_hyperz.c \
	r300_query.c \
	r300_render.c \
	r300_render_stencilref.c \
	r300_render_translate.c \
	r300_resource.c \
	r300_screen_buffer.c \
	r300_screen.c \
	r300_state.c \
	r300_state_derived.c \
	r300_texture.c \
	r300_texture_desc.c \
	r300_tgsi_to_rc.c \
	r300_transfer.c \
	r300_vs.c \
	r300_vs_draw.c

GALLIUM_SOURCES.drivers/r300/compiler = \
	memory_pool.c \
	r300_fragprog.c \
	r300_fragprog_emit.c \
	r300_fragprog_swizzle.c \
	r3xx_fragprog.c \
	r3xx_vertprog.c \
	r3xx_vertprog_dump.c \
	r500_fragprog.c \
	r500_fragprog_emit.c \
	radeon_code.c \
	radeon_compiler.c \
	radeon_compiler_util.c \
	radeon_dataflow.c \
	radeon_dataflow_deadcode.c \
	radeon_dataflow_swizzles.c \
	radeon_emulate_branches.c \
	radeon_emulate_loops.c \
	radeon_inline_literals.c \
	radeon_list.c \
	radeon_opcodes.c \
	radeon_optimize.c \
	radeon_pair_dead_sources.c \
	radeon_pair_regalloc.c \
	radeon_pair_schedule.c \
	radeon_pair_translate.c \
	radeon_program_alu.c \
	radeon_program.c \
	radeon_program_pair.c \
	radeon_program_print.c \
	radeon_program_tex.c \
	radeon_remove_constants.c \
	radeon_rename_regs.c \
	radeon_variable.c \
	radeon_vert_fc.c

GALLIUM_SOURCES.drivers/radeonsi = \
	cik_sdma.c \
	si_blit.c \
	si_buffer.c \
	si_clear.c \
	si_compute.c \
	si_compute_blit.c \
	si_cp_dma.c \
	si_debug.c \
	si_descriptors.c \
	si_dma.c \
	si_dma_cs.c \
	si_fence.c \
	si_get.c \
	si_gfx_cs.c \
	si_gpu_load.c \
	si_pipe.c \
	si_pm4.c \
	si_perfcounter.c \
	si_query.c \
	si_shader.c \
	si_shader_nir.c \
	si_shader_tgsi_alu.c \
	si_shader_tgsi_mem.c \
	si_shader_tgsi_setup.c \
	si_shaderlib_tgsi.c \
	si_state.c \
	si_state_binning.c \
	si_state_draw.c \
	si_state_msaa.c \
	si_state_shaders.c \
	si_state_streamout.c \
	si_state_viewport.c \
	si_test_dma.c \
	si_test_dma_perf.c \
	si_texture.c \
	si_uvd.c

.for _f in ${GALLIUM_SOURCES.drivers/radeon}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/radeon
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/r300} ${GALLIUM_SOURCES.drivers/r300/compiler}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/r300 \
			-I${X11SRCDIR.Mesa}/src/gallium/drivers/r300/compiler
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/r600} ${GALLIUM_SOURCES.drivers/r600/sb} ${GALLIUM_SOURCES.drivers/radeonsi}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/r600
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/gallium/drivers/r600
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/common
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/amd/common
.endfor

.for _f in ${GALLIUM_SOURCES.drivers/radeon}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/radeonsi
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd/common
.endfor

GALLIUM_SOURCES.drivers/nouveau = \
	nouveau_buffer.c \
	nouveau_fence.c \
	nouveau_heap.c \
	nouveau_mm.c \
	nouveau_screen.c \
	nouveau_video.c \
	nouveau_vp3_video_bsp.c \
	nouveau_vp3_video.c \
	nouveau_vp3_video_vp.c

GALLIUM_SOURCES.drivers/nouveau/nv30 = \
	nv30_clear.c \
	nv30_context.c \
	nv30_draw.c \
	nv30_format.c \
	nv30_fragprog.c \
	nv30_fragtex.c \
	nv30_miptree.c \
	nv30_push.c \
	nv30_query.c \
	nv30_resource.c \
	nv30_screen.c \
	nv30_state.c \
	nv30_state_validate.c \
	nv30_texture.c \
	nv30_transfer.c \
	nv30_vbo.c \
	nv30_vertprog.c \
	nv40_verttex.c \
	nvfx_fragprog.c \
	nvfx_vertprog.c

GALLIUM_SOURCES.drivers/nouveau/nv50 = \
	nv50_compute.c \
	nv50_context.c \
	nv50_formats.c \
	nv50_miptree.c \
	nv50_program.c \
	nv50_push.c \
	nv50_query.c \
	nv50_query_hw.c \
	nv50_query_hw_metric.c \
	nv50_query_hw_sm.c \
	nv50_resource.c \
	nv50_screen.c \
	nv50_shader_state.c \
	nv50_state.c \
	nv50_state_validate.c \
	nv50_surface.c \
	nv50_tex.c \
	nv50_transfer.c \
	nv50_vbo.c \
	nv84_video_bsp.c \
	nv84_video.c \
	nv84_video_vp.c \
	nv98_video_bsp.c \
	nv98_video.c \
	nv98_video_ppp.c \
	nv98_video_vp.c

GALLIUM_SOURCES.drivers/nouveau/codegen = \
	nv50_ir.cpp \
	nv50_ir_bb.cpp \
	nv50_ir_build_util.cpp \
	nv50_ir_emit_gk110.cpp \
	nv50_ir_emit_gm107.cpp \
	nv50_ir_emit_nv50.cpp \
	nv50_ir_emit_nvc0.cpp \
	nv50_ir_from_common.cpp \
	nv50_ir_from_nir.cpp \
	nv50_ir_from_tgsi.cpp \
	nv50_ir_graph.cpp \
	nv50_ir_lowering_gm107.cpp \
	nv50_ir_lowering_helper.cpp \
	nv50_ir_lowering_nv50.cpp \
	nv50_ir_lowering_nvc0.cpp \
	nv50_ir_peephole.cpp \
	nv50_ir_print.cpp \
	nv50_ir_ra.cpp \
	nv50_ir_ssa.cpp \
	nv50_ir_target.cpp \
	nv50_ir_target_nv50.cpp \
	nv50_ir_util.cpp \
	nv50_ir_target_gm107.cpp \
	nv50_ir_target_nvc0.cpp

GALLIUM_SOURCES.drivers/nouveau/nvc0 = \
	nvc0_compute.c \
	nvc0_context.c \
	nvc0_formats.c \
	nvc0_miptree.c \
	nvc0_resource.c \
	nvc0_screen.c \
	nvc0_state.c \
	nvc0_state_validate.c \
	nvc0_surface.c \
	nvc0_tex.c \
	nvc0_transfer.c \
	nvc0_vbo.c \
	nvc0_vbo_translate.c \
	nvc0_program.c \
	nvc0_shader_state.c \
	nvc0_query.c \
	nvc0_query_hw.c \
	nvc0_query_hw_metric.c \
	nvc0_query_hw_sm.c \
	nvc0_query_sw.c \
	nve4_compute.c \
	nvc0_video.c \
	nvc0_video_bsp.c \
	nvc0_video_vp.c \
	nvc0_video_ppp.c


GALLIUM_SOURCES.winsys/nouveau/drm = \
        nouveau_drm_winsys.c

.for _f in ${GALLIUM_SOURCES.drivers/nouveau} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nv30} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nv50} \
	   ${GALLIUM_SOURCES.drivers/nouveau/codegen} \
	   ${GALLIUM_SOURCES.drivers/nouveau/nvc0} \
	   ${GALLIUM_SOURCES.winsys/nouveau/drm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/drivers/nouveau \
			-I${DESTDIR}${X11INCDIR}/libdrm/nouveau
.endfor

GALLIUM_SOURCES.winsys/radeon/drm = \
        radeon_drm_bo.c \
        radeon_drm_cs.c \
	radeon_drm_surface.c \
        radeon_drm_winsys.c

GALLIUM_SOURCES.winsys/amdgpu/drm = \
	amdgpu_bo.c \
	amdgpu_cs.c \
	amdgpu_surface.c \
	amdgpu_winsys.c

.for _f in ${GALLIUM_SOURCES.winsys/amdgpu/drm}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/amd
.endfor

GALLIUM_SOURCES.state_trackers/vdpau = \
	bitmap.c \
	decode.c \
	device.c \
	ftab.c \
	htab.c \
	mixer.c \
	output.c \
	preemption.c \
	presentation.c \
	query.c \
	surface.c

GALLIUM_SOURCES.drivers/llvmpipe = \
	lp_bld_alpha.c \
	lp_bld_blend_aos.c \
	lp_bld_blend.c \
	lp_bld_blend_logicop.c \
	lp_bld_depth.c \
	lp_bld_interp.c \
	lp_clear.c \
	lp_context.c \
	lp_draw_arrays.c \
	lp_fence.c \
	lp_flush.c \
	lp_jit.c \
	lp_memory.c \
	lp_perf.c \
	lp_query.c \
	lp_rast.c \
	lp_rast_debug.c \
	lp_rast_tri.c \
	lp_scene.c \
	lp_scene_queue.c \
	lp_screen.c \
	lp_setup.c \
	lp_setup_line.c \
	lp_setup_point.c \
	lp_setup_tri.c \
	lp_setup_vbuf.c \
	lp_state_blend.c \
	lp_state_clip.c \
	lp_state_derived.c \
	lp_state_fs.c \
	lp_state_gs.c \
	lp_state_rasterizer.c \
	lp_state_sampler.c \
	lp_state_setup.c \
	lp_state_so.c \
	lp_state_surface.c \
	lp_state_vertex.c \
	lp_state_vs.c \
	lp_surface.c \
	lp_tex_sample.c \
	lp_texture.c

GALLIUM_SOURCES.drivers/softpipe = \
	sp_buffer.c \
	sp_clear.c \
	sp_context.c \
	sp_compute.c \
	sp_draw_arrays.c \
	sp_fence.c \
	sp_flush.c \
	sp_fs_exec.c \
	sp_image.c \
	sp_prim_vbuf.c \
	sp_quad_blend.c \
	sp_quad_depth_test.c \
	sp_quad_fs.c \
	sp_quad_pipe.c \
	sp_quad_stipple.c \
	sp_query.c \
	sp_screen.c \
	sp_setup.c \
	sp_state_blend.c \
	sp_state_clip.c \
	sp_state_derived.c \
	sp_state_image.c \
	sp_state_rasterizer.c \
	sp_state_sampler.c \
	sp_state_shader.c \
	sp_state_so.c \
	sp_state_surface.c \
	sp_state_vertex.c \
	sp_surface.c \
	sp_tex_sample.c \
	sp_tex_tile_cache.c \
	sp_texture.c \
	sp_tile_cache.c

GALLIUM_SOURCES.winsys/sw/null = \
	null_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/wrapper = \
	wrapper_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/dri = \
	dri_sw_winsys.c

GALLIUM_SOURCES.winsys/sw/kms-dri = \
	kms_dri_sw_winsys.c

GALLIUM_SOURCES.state_trackers/dri = \
        dri_context.c \
        dri_drawable.c \
	dri_helpers.c \
        dri_query_renderer.c \
        dri_screen.c \
	drisw.c \
	dri2.c
.for _f in ${GALLIUM_SOURCES.state_trackers/dri}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/gallium/state_trackers/dri/common
.endfor

# missing
CPPFLAGS+=	\
	-DGALLIUM_SOFTPIPE \
	-DGALLIUM_STATIC_TARGETS=1 \
	-DMESA_EGL_NO_X11_HEADERS

.for _d in ${GALLIUM_SUBDIRS}
SRCS+=	${GALLIUM_SOURCES.${_d}}
.PATH: ${X11SRCDIR.Mesa}/src/gallium/${_d}
. for _s in ${GALLIUM_SOURCES.${_d}}
CPPFLAGS.${_s}+= -I${X11SRCDIR.Mesa}/src/gallium/${_d}
. endfor
.endfor

#. if defined(${GALLIUM_SOURCES.${_d}})
#SRCS+=	${GALLIUM_SOURCES.${_d}}
#. endif

.include "../libloader.old.mk"

.if ${BUILD_RADEON} == 1
CFLAGS+=	-pthread
LDFLAGS+=	-pthread
.endif

LIBDPLIBS+=	X11-xcb         ${.CURDIR}/../libX11/libX11-xcb
LIBDPLIBS+=	xcb-dri2        ${.CURDIR}/../libxcb/dri2
LIBDPLIBS+=	xcb		${.CURDIR}/../libxcb/libxcb
.if ${BUILD_RADEON} == 1 || ${BUILD_NOUVEAU} == 1
LIBDPLIBS+= 	drm		${.CURDIR}/../libdrm
.if ${BUILD_I915} == 1
LIBDPLIBS+= 	drm_intel	${.CURDIR}/../libdrm_intel
.endif
.if ${BUILD_RADEON} == 1
LIBDPLIBS+= 	drm_radeon	${.CURDIR}/../libdrm_radeon
LIBDPLIBS+= 	drm_amdgpu	${.CURDIR}/../libdrm_amdgpu
.endif	# ${BUILD_RADEON} == 1
.if ${BUILD_NOUVEAU} == 1
LIBDPLIBS+= 	drm_nouveau	${.CURDIR}/../libdrm_nouveau
.endif	# ${BUILD_NOUVEAU} == 1
.endif	# ${BUILD_RADEON} == 1 || ${BUILD_NOUVEAU} == 1
LIBDPLIBS+= 	glapi		${.CURDIR}/../libglapi${OLD_SUFFIX}
LIBDPLIBS+=	expat		${.CURDIR}/../../../../../external/mit/expat/lib/libexpat
LIBDPLIBS+=	terminfo	${.CURDIR}/../../../../../lib/libterminfo
LIBDPLIBS+=	execinfo	${.CURDIR}/../../../../../lib/libexecinfo

LIBDPLIBS+=	elf		${NETBSDSRCDIR}/external/bsd/elftoolchain/lib/libelf
LIBDPLIBS+=	z		${.CURDIR}/../../../../../lib/libz
LIBDPLIBS+=	m		${.CURDIR}/../../../../../lib/libm

# gallium drivers requiring LLVM
.if ${BUILD_LLVMPIPE} == 1 || ${BUILD_RADEON} == 1

LLVMRT_LIBS=	\
		MCJIT \
		ExecutionEngine \
		ExecutionEngineOrcTargetProcess \
		ExecutionEngineOrcShared \
		Orc \
		RuntimeDyld

.if ${BUILD_RADEON} == 1
LLVMRT_LIBS+=	\
		AMDGPUCodeGen \
		AMDGPUMCTargetDesc \
		AMDGPUTargetInfo \
		AMDGPUAsmParser \
		AMDGPUUtils
.endif

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
# XXX duplicates
LLVMRT_LIBS+=	\
		X86Disassembler \
		X86CodeGen \
		X86MCTargetDesc \
		X86Disassembler \
		X86MCTargetDesc \
		X86TargetInfo
.endif

.if ${MACHINE_CPU} == "aarch64"
LLVMRT_LIBS+=	\
		AArch64AsmParser \
		AArch64CodeGen \
		AArch64MCTargetDesc \
		AArch64Utils \
		AArch64Disassembler \
		AArch64TargetInfo
.endif

.if ${MACHINE_CPU} == "arm"
LLVMRT_LIBS+=	\
		ARMCodeGen \
		ARMDisassembler \
		ARMTargetInfo \
		ARMMCTargetDesc \
		ARMAsmParser \
		ARMUtils
.endif

.if ${MACHINE_CPU} == "mips"
LLVMRT_LIBS+=	\
		MipsCodeGen \
		MipsDisassembler \
		MipsAsmParser \
		MipsMCTargetDesc \
		MipsTargetInfo
.endif

.if ${MACHINE_CPU} == "powerpc"
LLVMRT_LIBS+=	\
		PowerPCCodeGen \
		PowerPCDisassembler \
		PowerPCTargetInfo \
		PowerPCMCTargetDesc \
		PowerPCAsmParser
.endif

LLVMRT_LIBS+=	\
		Passes \
		TransformsCoroutines \
		ObjCARC \
		GlobalISel \
		MCDisassembler \
		SelectionDAG \
		AsmPrinter \
		CodeGen \
		TransformsCFGuard \
		Target \
		InstCombine \
		ScalarOpts \
		DebugInfoCodeView \
		DebugInfoDWARF \
		Object \
		BitReader \
		TextAPI \
		MIRParser \
		MCParser \
		MC \
		ipo \
		BitWriter \
		FrontendOpenMP \
		Instrumentation \
		IRReader \
		AsmParser \
		Vectorize \
		TransformsAggressiveInstCombine \
		ProfileData \
		TransformsUtils \
		Analysis \
		IR \
		Remarks \
		BitstreamReader \
		Linker \
		BinaryFormat \
		Support \
		Demangle

.include "${NETBSDSRCDIR}/external/apache2/llvm/link.mk"

.endif  # ${BUILD_LLVM_PIPE} == 1 || ${BUILD_RADEON} == 1

LDFLAGS+=	-Wl,--version-script=${X11SRCDIR.Mesa}/src/gallium/targets/dri/dri.sym
.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64" || ${MACHINE_CPU} == "aarch64"
LDFLAGS+=	-Wl,-z,defs
.endif

##  build mesagallium parts
MESA_SRC_MODULES=  main math vbo state_tracker program asm_s
.if ${BUILD_RADEON} == 1
MESA_SRC_MODULES+= amd
.endif

.include "../libmesa.old.mk"
.include "../libglsl.old.mk"

# Special addition for just gallium; it misses the rest of asm_c files.
.PATH: ${X11SRCDIR.Mesa}/src/mesa/x86
SRCS+=	common_x86.c

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
SRCS+=	streaming-load-memcpy.c
CPPFLAGS.streaming-load-memcpy.c+= -msse4.1
.endif

# Needs 64bit atomics
CPPFLAGS.i386+= -march=i586
CPPFLAGS+=	${CPPFLAGS.${XORG_MACHINE_ARCH:U${MACHINE_ARCH}}}

CPPFLAGS.r600_pipe_common.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"
CPPFLAGS.si_get.c+=	-DMESA_LLVM_VERSION_STRING=\"${LLVM_VERSION}\"

.include "../driver${OLD_SUFFIX}.mk"


.PATH: ${X11SRCDIR.Mesa}/src/gallium/targets/dri
SRCS+=	target.c

CPPFLAGS+= \
	-I${X11SRCDIR.Mesa}/src/gallium/include \
	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary \
	-I${X11SRCDIR.Mesa}/src/gallium/drivers \
	-I${X11SRCDIR.Mesa}/../src/mesa/drivers/dri/common

CPPFLAGS.target.c += \
	-DDRI_TARGET \
	-DGALLIUM_NOOP \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_SOFTPIPE \
	-I${X11SRCDIR.Mesa}/src/gallium/state_trackers/dri \
	-I${X11SRCDIR.Mesa}/src/loader \
	-I${X11SRCDIR.Mesa}/../src/util \
	-I${X11SRCDIR.Mesa}/../src/gallium/drivers \
	-I${X11SRCDIR.Mesa}/src/gallium/winsys

.if ${BUILD_LLVMPIPE} == 1
CPPFLAGS.target.c += \
	-DGALLIUM_LLVMPIPE
.endif  #${BUILD_LLVMPIPE} == 1

.if ${BUILD_I915} == 1
CPPFLAGS.target.c += \
	-DGALLIUM_I915 \
	-DGALLIUM_ILO
.endif

.if ${BUILD_RADEON} == 1
CPPFLAGS.target.c += \
	-DGALLIUM_R200 \
	-DGALLIUM_R300 \
	-DGALLIUM_R600 \
	-DGALLIUM_RADEONSI
.endif	# ${BUILD_RADEON} == 1

.if ${BUILD_NOUVEAU} == 1
CPPFLAGS.target.c += \
	-DGALLIUM_NOUVEAU
.endif	# ${BUILD_NOUVEAU} == 1

CWARNFLAGS.clang+=	-Wno-error=constant-conversion \
			-Wno-error=tautological-constant-out-of-range-compare \
			-Wno-error=pointer-sign \
			-Wno-error=switch \
			-Wno-error=absolute-value \
			-Wno-error=tautological-compare \
			-Wno-error=static-in-inline \
			-Wno-error=logical-not-parentheses \
			-Wno-error=constant-logical-operand \
			-Wno-error=unknown-warning-option \
			-Wno-error=typedef-redefinition \
			-Wno-error=enum-conversion # https://bugs.freedesktop.org/show_bug.cgi?id=109761

CWARNFLAGS+=		-Wno-error=stack-protector

.include <bsd.x11.mk>
LIBDIR=		${X11USRLIBDIR}/modules/dri

.for _d in ${DRIVERS}
SYMLINKS+= gallium_dri.so ${DRIDIR}/${_d}_dri.so
SYMLINKS+= gallium_dri.so.${SHLIB_MAJOR} ${DRIDIR}/${_d}_dri.so.${SHLIB_MAJOR} 
.if ${MKDEBUG} != "no"
SYMLINKS+= gallium_dri.so.${SHLIB_MAJOR}.debug ${DRIDEBUGDIR}/${_d}_dri.so.${SHLIB_MAJOR}.debug
.endif
.endfor

.if ${MACHINE_ARCH} == "sparc" || ${COMMON_MACHINE_ARCH:U} == "sparc"
COPTS+=	${${ACTIVE_CC} == "clang":? -Wa,-Av8plus  :}
.endif

.if ${MACHINE_ARCH} == "vax"
COPTS.nir.c += -O1
.endif

COPTS.u_atomic.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 10:? -Wno-builtin-declaration-mismatch :}

.include <bsd.lib.mk>
# Don't regenerate c files
.y.c:
