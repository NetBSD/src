/* Machine description for AArch64 architecture.
   Copyright (C) 2009-2016 Free Software Foundation, Inc.
   Contributed by ARM Ltd.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */


#ifndef GCC_AARCH64_PROTOS_H
#define GCC_AARCH64_PROTOS_H

#include "input.h"

/* SYMBOL_SMALL_ABSOLUTE: Generate symbol accesses through
   high and lo relocs that calculate the base address using a PC
   relative reloc.
   So to get the address of foo, we generate
   adrp x0, foo
   add  x0, x0, :lo12:foo

   To load or store something to foo, we could use the corresponding
   load store variants that generate an
   ldr x0, [x0,:lo12:foo]
   or
   str x1, [x0, :lo12:foo]

   This corresponds to the small code model of the compiler.

   SYMBOL_SMALL_GOT_4G: Similar to the one above but this
   gives us the GOT entry of the symbol being referred to :
   Thus calculating the GOT entry for foo is done using the
   following sequence of instructions.  The ADRP instruction
   gets us to the page containing the GOT entry of the symbol
   and the got_lo12 gets us the actual offset in it, together
   the base and offset, we can address 4G size GOT table.

   adrp  x0, :got:foo
   ldr   x0, [x0, :gotoff_lo12:foo]

   This corresponds to the small PIC model of the compiler.

   SYMBOL_SMALL_GOT_28K: Similar to SYMBOL_SMALL_GOT_4G, but used for symbol
   restricted within 28K GOT table size.

   ldr reg, [gp, #:gotpage_lo15:sym]

   This corresponds to -fpic model for small memory model of the compiler.

   SYMBOL_SMALL_TLSGD
   SYMBOL_SMALL_TLSDESC
   SYMBOL_SMALL_TLSIE
   SYMBOL_TINY_TLSIE
   SYMBOL_TLSLE12
   SYMBOL_TLSLE24
   SYMBOL_TLSLE32
   SYMBOL_TLSLE48
   Each of these represents a thread-local symbol, and corresponds to the
   thread local storage relocation operator for the symbol being referred to.

   SYMBOL_TINY_ABSOLUTE

   Generate symbol accesses as a PC relative address using a single
   instruction.  To compute the address of symbol foo, we generate:

   ADR x0, foo

   SYMBOL_TINY_GOT

   Generate symbol accesses via the GOT using a single PC relative
   instruction.  To compute the address of symbol foo, we generate:

   ldr t0, :got:foo

   The value of foo can subsequently read using:

   ldrb    t0, [t0]

   SYMBOL_FORCE_TO_MEM : Global variables are addressed using
   constant pool.  All variable addresses are spilled into constant
   pools.  The constant pools themselves are addressed using PC
   relative accesses.  This only works for the large code model.
 */
enum aarch64_symbol_type
{
  SYMBOL_SMALL_ABSOLUTE,
  SYMBOL_SMALL_GOT_28K,
  SYMBOL_SMALL_GOT_4G,
  SYMBOL_SMALL_TLSGD,
  SYMBOL_SMALL_TLSDESC,
  SYMBOL_SMALL_TLSIE,
  SYMBOL_TINY_ABSOLUTE,
  SYMBOL_TINY_GOT,
  SYMBOL_TINY_TLSIE,
  SYMBOL_TLSLE12,
  SYMBOL_TLSLE24,
  SYMBOL_TLSLE32,
  SYMBOL_TLSLE48,
  SYMBOL_FORCE_TO_MEM
};

/* A set of tuning parameters contains references to size and time
   cost models and vectors for address cost calculations, register
   move costs and memory move costs.  */

/* Scaled addressing modes can vary cost depending on the mode of the
   value to be loaded/stored.  QImode values cannot use scaled
   addressing modes.  */

struct scale_addr_mode_cost
{
  const int hi;
  const int si;
  const int di;
  const int ti;
};

/* Additional cost for addresses.  */
struct cpu_addrcost_table
{
  const struct scale_addr_mode_cost addr_scale_costs;
  const int pre_modify;
  const int post_modify;
  const int register_offset;
  const int register_sextend;
  const int register_zextend;
  const int imm_offset;
};

/* Additional costs for register copies.  Cost is for one register.  */
struct cpu_regmove_cost
{
  const int GP2GP;
  const int GP2FP;
  const int FP2GP;
  const int FP2FP;
};

/* Cost for vector insn classes.  */
struct cpu_vector_cost
{
  const int scalar_stmt_cost;		 /* Cost of any scalar operation,
					    excluding load and store.  */
  const int scalar_load_cost;		 /* Cost of scalar load.  */
  const int scalar_store_cost;		 /* Cost of scalar store.  */
  const int vec_stmt_cost;		 /* Cost of any vector operation,
					    excluding load, store, permute,
					    vector-to-scalar and
					    scalar-to-vector operation.  */
  const int vec_permute_cost;		 /* Cost of permute operation.  */
  const int vec_to_scalar_cost;		 /* Cost of vec-to-scalar operation.  */
  const int scalar_to_vec_cost;		 /* Cost of scalar-to-vector
					    operation.  */
  const int vec_align_load_cost;	 /* Cost of aligned vector load.  */
  const int vec_unalign_load_cost;	 /* Cost of unaligned vector load.  */
  const int vec_unalign_store_cost;	 /* Cost of unaligned vector store.  */
  const int vec_store_cost;		 /* Cost of vector store.  */
  const int cond_taken_branch_cost;	 /* Cost of taken branch.  */
  const int cond_not_taken_branch_cost;  /* Cost of not taken branch.  */
};

/* Branch costs.  */
struct cpu_branch_cost
{
  const int predictable;    /* Predictable branch or optimizing for size.  */
  const int unpredictable;  /* Unpredictable branch or optimizing for speed.  */
};

struct tune_params
{
  const struct cpu_cost_table *insn_extra_cost;
  const struct cpu_addrcost_table *addr_cost;
  const struct cpu_regmove_cost *regmove_cost;
  const struct cpu_vector_cost *vec_costs;
  const struct cpu_branch_cost *branch_costs;
  int memmov_cost;
  int issue_rate;
  unsigned int fusible_ops;
  int function_align;
  int jump_align;
  int loop_align;
  int int_reassoc_width;
  int fp_reassoc_width;
  int vec_reassoc_width;
  int min_div_recip_mul_sf;
  int min_div_recip_mul_df;
  /* Value for aarch64_case_values_threshold; or 0 for the default.  */
  unsigned int max_case_values;
  /* Value for PARAM_L1_CACHE_LINE_SIZE; or 0 to use the default.  */
  unsigned int cache_line_size;

/* An enum specifying how to take into account CPU autoprefetch capabilities
   during instruction scheduling:
   - AUTOPREFETCHER_OFF: Do not take autoprefetch capabilities into account.
   - AUTOPREFETCHER_WEAK: Attempt to sort sequences of loads/store in order of
   offsets but allow the pipeline hazard recognizer to alter that order to
   maximize multi-issue opportunities.
   - AUTOPREFETCHER_STRONG: Attempt to sort sequences of loads/store in order of
   offsets and prefer this even if it restricts multi-issue opportunities.  */

  enum aarch64_autoprefetch_model
  {
    AUTOPREFETCHER_OFF,
    AUTOPREFETCHER_WEAK,
    AUTOPREFETCHER_STRONG
  } autoprefetcher_model;

  unsigned int extra_tuning_flags;
};

#define AARCH64_FUSION_PAIR(x, name) \
  AARCH64_FUSE_##name##_index, 
/* Supported fusion operations.  */
enum aarch64_fusion_pairs_index
{
#include "aarch64-fusion-pairs.def"
  AARCH64_FUSE_index_END
};
#undef AARCH64_FUSION_PAIR

#define AARCH64_FUSION_PAIR(x, name) \
  AARCH64_FUSE_##name = (1u << AARCH64_FUSE_##name##_index),
/* Supported fusion operations.  */
enum aarch64_fusion_pairs
{
  AARCH64_FUSE_NOTHING = 0,
#include "aarch64-fusion-pairs.def"
  AARCH64_FUSE_ALL = (1u << AARCH64_FUSE_index_END) - 1
};
#undef AARCH64_FUSION_PAIR

#define AARCH64_EXTRA_TUNING_OPTION(x, name) \
  AARCH64_EXTRA_TUNE_##name##_index,
/* Supported tuning flags indexes.  */
enum aarch64_extra_tuning_flags_index
{
#include "aarch64-tuning-flags.def"
  AARCH64_EXTRA_TUNE_index_END
};
#undef AARCH64_EXTRA_TUNING_OPTION


#define AARCH64_EXTRA_TUNING_OPTION(x, name) \
  AARCH64_EXTRA_TUNE_##name = (1u << AARCH64_EXTRA_TUNE_##name##_index),
/* Supported tuning flags.  */
enum aarch64_extra_tuning_flags
{
  AARCH64_EXTRA_TUNE_NONE = 0,
#include "aarch64-tuning-flags.def"
  AARCH64_EXTRA_TUNE_ALL = (1u << AARCH64_EXTRA_TUNE_index_END) - 1
};
#undef AARCH64_EXTRA_TUNING_OPTION

/* Enum describing the various ways that the
   aarch64_parse_{arch,tune,cpu,extension} functions can fail.
   This way their callers can choose what kind of error to give.  */

enum aarch64_parse_opt_result
{
  AARCH64_PARSE_OK,			/* Parsing was successful.  */
  AARCH64_PARSE_MISSING_ARG,		/* Missing argument.  */
  AARCH64_PARSE_INVALID_FEATURE,	/* Invalid feature modifier.  */
  AARCH64_PARSE_INVALID_ARG		/* Invalid arch, tune, cpu arg.  */
};

extern struct tune_params aarch64_tune_params;

HOST_WIDE_INT aarch64_initial_elimination_offset (unsigned, unsigned);
int aarch64_get_condition_code (rtx);
bool aarch64_bitmask_imm (HOST_WIDE_INT val, machine_mode);
int aarch64_branch_cost (bool, bool);
enum aarch64_symbol_type aarch64_classify_symbolic_expression (rtx);
bool aarch64_cannot_change_mode_class (machine_mode,
				       machine_mode,
				       enum reg_class);
bool aarch64_const_vec_all_same_int_p (rtx, HOST_WIDE_INT);
bool aarch64_constant_address_p (rtx);
bool aarch64_expand_movmem (rtx *);
bool aarch64_float_const_zero_rtx_p (rtx);
bool aarch64_function_arg_regno_p (unsigned);
bool aarch64_gen_movmemqi (rtx *);
bool aarch64_gimple_fold_builtin (gimple_stmt_iterator *);
bool aarch64_is_extend_from_extract (machine_mode, rtx, rtx);
bool aarch64_is_long_call_p (rtx);
bool aarch64_is_noplt_call_p (rtx);
bool aarch64_label_mentioned_p (rtx);
void aarch64_declare_function_name (FILE *, const char*, tree);
bool aarch64_legitimate_pic_operand_p (rtx);
bool aarch64_modes_tieable_p (machine_mode mode1,
			      machine_mode mode2);
bool aarch64_zero_extend_const_eq (machine_mode, rtx, machine_mode, rtx);
bool aarch64_move_imm (HOST_WIDE_INT, machine_mode);
bool aarch64_mov_operand_p (rtx, machine_mode);
int aarch64_simd_attr_length_rglist (enum machine_mode);
rtx aarch64_reverse_mask (enum machine_mode);
bool aarch64_offset_7bit_signed_scaled_p (machine_mode, HOST_WIDE_INT);
char *aarch64_output_scalar_simd_mov_immediate (rtx, machine_mode);
char *aarch64_output_simd_mov_immediate (rtx, machine_mode, unsigned);
bool aarch64_pad_arg_upward (machine_mode, const_tree);
bool aarch64_pad_reg_upward (machine_mode, const_tree, bool);
bool aarch64_regno_ok_for_base_p (int, bool);
bool aarch64_regno_ok_for_index_p (int, bool);
bool aarch64_simd_check_vect_par_cnst_half (rtx op, machine_mode mode,
					    bool high);
bool aarch64_simd_imm_scalar_p (rtx x, machine_mode mode);
bool aarch64_simd_imm_zero_p (rtx, machine_mode);
bool aarch64_simd_scalar_immediate_valid_for_move (rtx, machine_mode);
bool aarch64_simd_shift_imm_p (rtx, machine_mode, bool);
bool aarch64_simd_valid_immediate (rtx, machine_mode, bool,
				   struct simd_immediate_info *);
bool aarch64_symbolic_address_p (rtx);
bool aarch64_uimm12_shift (HOST_WIDE_INT);
bool aarch64_use_return_insn_p (void);
const char *aarch64_mangle_builtin_type (const_tree);
const char *aarch64_output_casesi (rtx *);

enum aarch64_symbol_type aarch64_classify_symbol (rtx, rtx);
enum aarch64_symbol_type aarch64_classify_tls_symbol (rtx);
enum reg_class aarch64_regno_regclass (unsigned);
int aarch64_asm_preferred_eh_data_format (int, int);
int aarch64_fpconst_pow_of_2 (rtx);
machine_mode aarch64_hard_regno_caller_save_mode (unsigned, unsigned,
						       machine_mode);
int aarch64_hard_regno_mode_ok (unsigned, machine_mode);
int aarch64_hard_regno_nregs (unsigned, machine_mode);
int aarch64_simd_attr_length_move (rtx_insn *);
int aarch64_uxt_size (int, HOST_WIDE_INT);
int aarch64_vec_fpconst_pow_of_2 (rtx);
rtx aarch64_eh_return_handler_rtx (void);
rtx aarch64_legitimize_reload_address (rtx *, machine_mode, int, int, int);
rtx aarch64_mask_from_zextract_ops (rtx, rtx);
const char *aarch64_output_move_struct (rtx *operands);
rtx aarch64_return_addr (int, rtx);
rtx aarch64_simd_gen_const_vector_dup (machine_mode, int);
bool aarch64_simd_mem_operand_p (rtx);
rtx aarch64_simd_vect_par_cnst_half (machine_mode, bool);
rtx aarch64_tls_get_addr (void);
tree aarch64_fold_builtin (tree, int, tree *, bool);
unsigned aarch64_dbx_register_number (unsigned);
unsigned aarch64_trampoline_size (void);
void aarch64_asm_output_labelref (FILE *, const char *);
void aarch64_cpu_cpp_builtins (cpp_reader *);
void aarch64_elf_asm_named_section (const char *, unsigned, tree);
const char * aarch64_gen_far_branch (rtx *, int, const char *, const char *);
const char * aarch64_output_probe_stack_range (rtx, rtx);
void aarch64_err_no_fpadvsimd (machine_mode, const char *);
void aarch64_expand_epilogue (bool);
void aarch64_expand_mov_immediate (rtx, rtx);
void aarch64_expand_prologue (void);
void aarch64_expand_vector_init (rtx, rtx);
void aarch64_init_cumulative_args (CUMULATIVE_ARGS *, const_tree, rtx,
				   const_tree, unsigned);
void aarch64_init_expanders (void);
void aarch64_init_simd_builtins (void);
void aarch64_emit_call_insn (rtx);
void aarch64_register_pragmas (void);
void aarch64_relayout_simd_types (void);
void aarch64_reset_previous_fndecl (void);
void aarch64_save_restore_target_globals (tree);
void aarch64_emit_approx_rsqrt (rtx, rtx);

/* Initialize builtins for SIMD intrinsics.  */
void init_aarch64_simd_builtins (void);

void aarch64_simd_emit_reg_reg_move (rtx *, enum machine_mode, unsigned int);

/* Expand builtins for SIMD intrinsics.  */
rtx aarch64_simd_expand_builtin (int, tree, rtx);

void aarch64_simd_lane_bounds (rtx, HOST_WIDE_INT, HOST_WIDE_INT, const_tree);

void aarch64_split_128bit_move (rtx, rtx);

bool aarch64_split_128bit_move_p (rtx, rtx);

void aarch64_split_simd_combine (rtx, rtx, rtx);

void aarch64_split_simd_move (rtx, rtx);

/* Check for a legitimate floating point constant for FMOV.  */
bool aarch64_float_const_representable_p (rtx);

#if defined (RTX_CODE)

bool aarch64_legitimate_address_p (machine_mode, rtx, RTX_CODE, bool);
machine_mode aarch64_select_cc_mode (RTX_CODE, rtx, rtx);
rtx aarch64_gen_compare_reg (RTX_CODE, rtx, rtx);
rtx aarch64_load_tp (rtx);

void aarch64_expand_compare_and_swap (rtx op[]);
void aarch64_split_compare_and_swap (rtx op[]);
void aarch64_gen_atomic_cas (rtx, rtx, rtx, rtx, rtx);

bool aarch64_atomic_ldop_supported_p (enum rtx_code);
void aarch64_gen_atomic_ldop (enum rtx_code, rtx, rtx, rtx, rtx, rtx);
void aarch64_split_atomic_op (enum rtx_code, rtx, rtx, rtx, rtx, rtx, rtx);

bool aarch64_gen_adjusted_ldpstp (rtx *, bool, enum machine_mode, RTX_CODE);
#endif /* RTX_CODE */

void aarch64_init_builtins (void);

bool aarch64_process_target_attr (tree, const char*);
void aarch64_override_options_internal (struct gcc_options *);

rtx aarch64_expand_builtin (tree exp,
			    rtx target,
			    rtx subtarget ATTRIBUTE_UNUSED,
			    machine_mode mode ATTRIBUTE_UNUSED,
			    int ignore ATTRIBUTE_UNUSED);
tree aarch64_builtin_decl (unsigned, bool ATTRIBUTE_UNUSED);
tree aarch64_builtin_rsqrt (unsigned int);
tree aarch64_builtin_vectorized_function (unsigned int, tree, tree);

extern void aarch64_split_combinev16qi (rtx operands[3]);
extern void aarch64_expand_vec_perm (rtx target, rtx op0, rtx op1, rtx sel);
extern bool aarch64_madd_needs_nop (rtx_insn *);
extern void aarch64_final_prescan_insn (rtx_insn *);
extern bool
aarch64_expand_vec_perm_const (rtx target, rtx op0, rtx op1, rtx sel);
void aarch64_atomic_assign_expand_fenv (tree *, tree *, tree *);
int aarch64_ccmp_mode_to_code (enum machine_mode mode);

bool extract_base_offset_in_addr (rtx mem, rtx *base, rtx *offset);
bool aarch64_operands_ok_for_ldpstp (rtx *, bool, enum machine_mode);
bool aarch64_operands_adjust_ok_for_ldpstp (rtx *, bool, enum machine_mode);
extern bool aarch64_nopcrelative_literal_loads;

extern void aarch64_asm_output_pool_epilogue (FILE *, const char *,
					      tree, HOST_WIDE_INT);

/* Defined in common/config/aarch64-common.c.  */
bool aarch64_handle_option (struct gcc_options *, struct gcc_options *,
			     const struct cl_decoded_option *, location_t);
const char *aarch64_rewrite_selected_cpu (const char *name);
enum aarch64_parse_opt_result aarch64_parse_extension (const char *,
						       unsigned long *);
std::string aarch64_get_extension_string_for_isa_flags (unsigned long,
							unsigned long);

#endif /* GCC_AARCH64_PROTOS_H */
