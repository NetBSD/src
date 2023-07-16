#	$NetBSD: libglsl.mk,v 1.7 2023/07/16 22:20:54 rjs Exp $

LIBGLSL_GENERATED_CXX_FILES = \
	glsl_lexer.cpp \
	glsl_parser.cpp 

#COPTS.glsl_lexer.cpp+=	-Wno-deprecated-register
COPTS.vtn_glsl450.c+=	${${ACTIVE_CC} == "clang":? -Wno-error=enum-conversion :}

CPPFLAGS+=	-I${X11SRCDIR.Mesa}/src/compiler \
		-I${X11SRCDIR.Mesa}/../src/compiler \
		-I${X11SRCDIR.Mesa}/src/compiler/nir \
		-I${X11SRCDIR.Mesa}/../src/compiler/nir \
		-I${X11SRCDIR.Mesa}/src/compiler/glsl \
		-I${X11SRCDIR.Mesa}/../src/compiler/glsl \
		-I${X11SRCDIR.Mesa}/src/compiler/glsl/glcpp \
		-I${X11SRCDIR.Mesa}/../src/compiler/glsl/glcpp \
		-I${X11SRCDIR.Mesa}/src/compiler/spirv \
		-I${X11SRCDIR.Mesa}/../src/compiler/spirv

LIBGLSL_FILES = \
	ast_array_index.cpp \
	ast_expr.cpp \
	ast_function.cpp \
	ast_to_hir.cpp \
	ast_type.cpp \
	builtin_functions.cpp \
	builtin_types.cpp \
	builtin_variables.cpp \
	generate_ir.cpp \
	gl_nir_lower_atomics.c \
	gl_nir_link_atomics.c \
	gl_nir_link_uniform_blocks.c \
	gl_nir_link_uniform_initializers.c \
	gl_nir_link_uniforms.c \
	gl_nir_link_xfb.c \
	gl_nir_linker.c \
	gl_nir_lower_buffers.c \
	gl_nir_lower_images.c \
	gl_nir_lower_samplers.c \
	gl_nir_lower_samplers_as_deref.c \
	glsl_parser_extras.cpp \
	glsl_symbol_table.cpp \
	glsl_to_nir.cpp \
	hir_field_selection.cpp \
	ir.cpp \
	ir_array_refcount.cpp \
	ir_basic_block.cpp \
	ir_builder.cpp \
	ir_clone.cpp \
	ir_constant_expression.cpp \
	ir_equals.cpp \
	ir_expression_flattening.cpp \
	ir_function.cpp \
	ir_function_can_inline.cpp \
	ir_function_detect_recursion.cpp \
	ir_hierarchical_visitor.cpp \
	ir_hv_accept.cpp \
	ir_print_visitor.cpp \
	ir_reader.cpp \
	ir_rvalue_visitor.cpp \
	ir_set_program_inouts.cpp \
	ir_validate.cpp \
	ir_variable_refcount.cpp \
	linker.cpp \
	linker_util.cpp \
	link_atomics.cpp \
	link_functions.cpp \
	link_interface_blocks.cpp \
	link_uniforms.cpp \
	link_uniform_initializers.cpp \
	link_uniform_block_active_visitor.cpp \
	link_uniform_block_active_visitor.h \
	link_uniform_blocks.cpp \
	link_varyings.cpp \
	loop_analysis.cpp \
	loop_unroll.cpp \
	lower_blend_equation_advanced.cpp \
	lower_buffer_access.cpp \
	lower_builtins.cpp \
	lower_const_arrays_to_uniforms.cpp \
	lower_cs_derived.cpp \
	lower_discard.cpp \
	lower_discard_flow.cpp \
	lower_distance.cpp \
	lower_if_to_cond_assign.cpp \
	lower_instructions.cpp \
	lower_int64.cpp \
	lower_jumps.cpp \
	lower_mat_op_to_vec.cpp \
	lower_offset_array.cpp \
	lower_packed_varyings.cpp \
	lower_precision.cpp \
	lower_named_interface_blocks.cpp \
	lower_packing_builtins.cpp \
	lower_subroutine.cpp \
	lower_tess_level.cpp \
	lower_variable_index_to_cond_assign.cpp \
	lower_vec_index_to_cond_assign.cpp \
	lower_vec_index_to_swizzle.cpp \
	lower_vector.cpp \
	lower_vector_derefs.cpp \
	lower_vector_insert.cpp \
	lower_vertex_id.cpp \
	lower_output_reads.cpp \
	lower_shared_reference.cpp \
	lower_ubo_reference.cpp \
	lower_xfb_varying.cpp \
	opt_algebraic.cpp \
	opt_array_splitting.cpp \
	opt_conditional_discard.cpp \
	opt_constant_folding.cpp \
	opt_constant_propagation.cpp \
	opt_constant_variable.cpp \
	opt_copy_propagation_elements.cpp \
	opt_dead_builtin_variables.cpp \
	opt_dead_builtin_varyings.cpp \
	opt_dead_code.cpp \
	opt_dead_code_local.cpp \
	opt_dead_functions.cpp \
	opt_flatten_nested_if_blocks.cpp \
	opt_flip_matrices.cpp \
	opt_function_inlining.cpp \
	opt_if_simplification.cpp \
	opt_minmax.cpp \
	opt_rebalance_tree.cpp \
	opt_redundant_jumps.cpp \
	opt_structure_splitting.cpp \
	opt_swizzle.cpp \
	opt_tree_grafting.cpp \
	opt_vectorize.cpp \
	propagate_invariance.cpp \
	s_expression.cpp \
	string_to_uint_map.cpp \
	serialize.cpp \
	shader_cache.cpp \
	blob.c \
	glsl_types.cpp \
	nir_types.cpp \
	shader_enums.c

# XXX
.if ${MACHINE} == "vax"
COPTS.ir_constant_expression.cpp+=	-O0
COPTS.ir.cpp+=	-O0
COPTS.nir_constant_expressions.c+=	-O0
.endif

LIBGLCPP_GENERATED_FILES = \
	glcpp-lex.c \
	glcpp-parse.c

LIBGLCPP_FILES = \
	pp.c

NIR_GENERATED_FILES = \
	nir_constant_expressions.c \
	nir_intrinsics.c \
	nir_opcodes.c \
	nir_opt_algebraic.c

#BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/compiler/nir/nir.c nir_nir.c

NIR_FILES = \
	nir.c \
	nir_builtin_builder.c \
	nir_clone.c \
	nir_control_flow.c \
	nir_deref.c \
	nir_divergence_analysis.c \
	nir_dominance.c \
	nir_from_ssa.c \
	nir_gather_info.c \
	nir_gather_ssa_types.c \
	nir_gather_xfb_info.c \
	nir_gs_count_vertices.c \
	nir_inline_functions.c \
	nir_inline_uniforms.c \
	nir_instr_set.c \
	nir_linking_helpers.c \
	nir_liveness.c \
	nir_loop_analyze.c \
	nir_lower_alpha_test.c \
	nir_lower_alu.c \
	nir_lower_alu_to_scalar.c \
	nir_lower_array_deref_of_vec.c \
	nir_lower_atomics_to_ssbo.c \
	nir_lower_bit_size.c \
	nir_lower_bitmap.c \
	nir_lower_bool_to_float.c \
	nir_lower_bool_to_int32.c \
	nir_lower_clamp_color_outputs.c \
	nir_lower_clip.c \
	nir_lower_clip_cull_distance_arrays.c \
	nir_lower_clip_disable.c \
	nir_lower_double_ops.c \
	nir_lower_discard_or_demote.c \
	nir_lower_drawpixels.c \
	nir_lower_flatshade.c \
	nir_lower_flrp.c \
	nir_lower_fp16_conv.c \
	nir_lower_fragcoord_wtrans.c \
	nir_lower_frexp.c \
	nir_lower_global_vars_to_local.c \
	nir_lower_goto_ifs.c \
	nir_lower_gs_intrinsics.c \
	nir_lower_idiv.c \
	nir_lower_image.c \
	nir_lower_indirect_derefs.c \
	nir_lower_int64.c \
	nir_lower_int_to_float.c \
	nir_lower_io.c \
	nir_lower_io_arrays_to_elements.c \
	nir_lower_io_to_scalar.c \
	nir_lower_io_to_vector.c \
	nir_lower_io_to_temporaries.c \
	nir_lower_load_const_to_scalar.c \
	nir_lower_locals_to_regs.c \
	nir_lower_mediump.c \
	nir_lower_packing.c \
	nir_lower_passthrough_edgeflags.c \
	nir_lower_patch_vertices.c \
	nir_lower_phis_to_scalar.c \
	nir_lower_pntc_ytransform.c \
	nir_lower_point_size.c \
	nir_lower_point_size_mov.c \
	nir_lower_regs_to_ssa.c \
	nir_lower_returns.c \
	nir_lower_samplers.c \
	nir_lower_scratch.c \
	nir_lower_subgroups.c \
	nir_lower_system_values.c \
	nir_lower_sysvals_to_varyings.c \
	nir_lower_tex.c \
	nir_lower_texcoord_replace.c \
	nir_lower_to_source_mods.c \
	nir_lower_two_sided_color.c \
	nir_lower_ubo_vec4.c \
	nir_lower_uniforms_to_ubo.c \
	nir_lower_var_copies.c \
	nir_lower_variable_initializers.c \
	nir_lower_vars_to_ssa.c \
	nir_lower_vec_to_movs.c \
	nir_lower_viewport_transform.c \
	nir_lower_wpos_center.c \
	nir_lower_wpos_ytransform.c \
	nir_metadata.c \
	nir_move_vec_src_uses_to_dest.c \
	nir_normalize_cubemap_coords.c \
	nir_opt_access.c \
	nir_opt_barriers.c \
	nir_opt_combine_stores.c \
	nir_opt_comparison_pre.c \
	nir_opt_conditional_discard.c \
	nir_opt_constant_folding.c \
	nir_opt_copy_prop_vars.c \
	nir_opt_copy_propagate.c \
	nir_opt_cse.c \
	nir_opt_dce.c \
	nir_opt_dead_cf.c \
	nir_opt_dead_write_vars.c \
	nir_opt_find_array_copies.c \
	nir_opt_fragdepth.c \
	nir_opt_gcm.c \
	nir_opt_idiv_const.c \
	nir_opt_if.c \
	nir_opt_intrinsics.c \
	nir_opt_large_constants.c \
	nir_opt_load_store_vectorize.c \
	nir_opt_loop_unroll.c \
	nir_opt_memcpy.c \
	nir_opt_move.c \
	nir_opt_move_discards_to_top.c \
	nir_opt_offsets.c \
	nir_opt_peephole_select.c \
	nir_opt_phi_precision.c \
	nir_opt_rematerialize_compares.c \
	nir_opt_remove_phis.c \
	nir_opt_shrink_vectors.c \
	nir_opt_sink.c \
	nir_opt_trivial_continues.c \
	nir_opt_undef.c \
	nir_opt_uniform_atomics.c \
	nir_opt_vectorize.c \
	nir_phi_builder.c \
	nir_print.c \
	nir_propagate_invariant.c \
	nir_range_analysis.c \
	nir_remove_dead_variables.c \
	nir_repair_ssa.c \
	nir_schedule.c \
	nir_search.c \
	nir_serialize.c \
	nir_split_per_member_structs.c \
	nir_split_var_copies.c \
	nir_split_vars.c \
	nir_sweep.c \
	nir_to_lcssa.c \
	nir_validate.c \
	nir_worklist.c

SPIRV_GENERATED_FILES = \
	spirv_info.c \
	vtn_gather_types.c

SPIRV_FILES = \
	gl_spirv.c \
	spirv_to_nir.c \
	vtn_alu.c \
	vtn_amd.c \
	vtn_cfg.c \
	vtn_glsl450.c \
	vtn_opencl.c \
	vtn_subgroup.c \
	vtn_variables.c


.PATH:	${X11SRCDIR.Mesa}/src/compiler
.PATH:	${X11SRCDIR.Mesa}/src/compiler/glsl
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/glsl
.PATH:	${X11SRCDIR.Mesa}/src/compiler/glsl/glcpp
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/glsl/glcpp
.PATH:	${X11SRCDIR.Mesa}/src/compiler/nir
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/nir
.PATH:	${X11SRCDIR.Mesa}/src/compiler/spirv
.PATH:	${X11SRCDIR.Mesa}/../src/compiler/spirv

SRCS+=	${LIBGLSL_GENERATED_CXX_FILES} \
	${LIBGLSL_FILES} \
	${LIBGLCPP_GENERATED_FILES} \
	${LIBGLCPP_FILES} \
	${NIR_GENERATED_FILES} \
	${NIR_FILES} \
	${SPIRV_GENERATED_FILES} \
	${SPIRV_FILES}
