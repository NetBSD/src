/* Definitions of target machine for GNU compiler for Hitachi Super-H.
   Copyright (C) 1993, 1994, 1995, 1996, 1997, 1998, 1999, 2000
   Free Software Foundation, Inc.
   Contributed by Steve Chamberlain (sac@cygnus.com).
   Improved by Jim Wilson (wilson@cygnus.com).

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef GCC_SH_PROTOS_H
#define GCC_SH_PROTOS_H

extern rtx get_fpscr_rtx PROTO ((void));
extern void emit_sf_insn PROTO ((rtx));
extern void emit_df_insn PROTO ((rtx));
extern int fp_zero_operand PROTO ((rtx));
extern int fp_one_operand PROTO ((rtx));
extern int expand_block_move PROTO ((rtx *));
extern int prepare_move_operands PROTO ((rtx[], enum machine_mode));
extern rtx prepare_scc_operands PROTO ((enum rtx_code));
extern void from_compare PROTO ((rtx *, int));
extern char *output_movedouble PROTO ((rtx, rtx[], enum machine_mode));
extern void print_operand PROTO ((FILE *, rtx, int));
extern char *output_far_jump PROTO ((rtx, rtx));
extern char *output_branch PROTO ((int, rtx, rtx *));
extern char *output_branchy_insn PROTO ((enum rtx_code, char *, rtx, rtx *));
extern char *output_ieee_ccmpeq PROTO ((rtx, rtx *));
extern void output_file_start PROTO ((FILE *));
extern int shift_insns_rtx PROTO ((rtx));
extern int shiftcosts PROTO ((rtx));
extern int andcosts PROTO ((rtx));
extern int multcosts PROTO ((rtx));
extern void print_operand_address PROTO ((FILE *, rtx));
extern void gen_shifty_op PROTO ((int, rtx *));
extern void gen_shifty_hi_op PROTO ((int, rtx *));
extern int expand_ashiftrt PROTO ((rtx *));
extern int sh_dynamicalize_shift_p PROTO ((rtx));
extern int shl_and_kind PROTO ((rtx, rtx, int *));
extern int shl_and_length PROTO ((rtx));
extern int shl_and_scr_length PROTO ((rtx));
extern int gen_shl_and PROTO ((rtx, rtx, rtx, rtx));
extern int shl_sext_kind PROTO ((rtx, rtx, int *));
extern int shl_sext_length PROTO ((rtx));
extern int gen_shl_sext PROTO ((rtx, rtx, rtx, rtx));
extern rtx sfunc_uses_reg PROTO ((rtx));
extern int sh_loop_align PROTO ((rtx));
extern void remove_dead_before_cse PROTO ((void));
extern void machine_dependent_reorg PROTO ((rtx));
extern int barrier_align PROTO ((rtx));
extern void final_prescan_insn PROTO ((rtx, rtx *, int));
extern char *output_jump_label_table PROTO ((void));
extern void sh_expand_prologue PROTO ((void));
extern void sh_expand_epilogue PROTO ((void));
extern void function_epilogue PROTO ((FILE *, int));
extern rtx sh_builtin_saveregs PROTO ((tree));
extern int initial_elimination_offset PROTO ((int, int));
extern int sh_handle_pragma PROTO ((int (*) PROTO((void)),
				    void (*) PROTO((int)), char *));
extern void sh_pragma_insert_attributes PROTO ((tree, tree *, tree *));
extern int sh_valid_machine_decl_attribute PROTO ((tree, tree, tree, tree));
extern int general_movsrc_operand PROTO ((rtx, enum machine_mode));
extern int general_movdst_operand PROTO ((rtx, enum machine_mode));
extern int arith_reg_operand PROTO ((rtx, enum machine_mode));
extern int system_reg_operand PROTO ((rtx, enum machine_mode));
extern int fp_arith_reg_operand PROTO ((rtx, enum machine_mode));
extern int fp_extended_operand PROTO ((rtx, enum machine_mode));
extern int arith_operand PROTO ((rtx, enum machine_mode));
extern int arith_reg_or_0_operand PROTO ((rtx, enum machine_mode));
extern int logical_operand PROTO ((rtx, enum machine_mode));
extern int fp_zero_operand PROTO ((rtx));
extern int fp_one_operand PROTO ((rtx));
extern int tertiary_reload_operand PROTO ((rtx, enum machine_mode));
extern int fpscr_operand PROTO ((rtx, enum machine_mode));
extern int commutative_float_operator PROTO ((rtx, enum machine_mode));
extern int noncommutative_float_operator PROTO ((rtx, enum machine_mode));
extern int binary_float_operator PROTO ((rtx, enum machine_mode));
extern int reg_unused_after PROTO ((rtx, rtx));
extern void expand_sf_unop PROTO ((rtx (*) PROTO ((rtx, rtx, rtx)), rtx *));
extern void expand_sf_binop PROTO ((rtx (*) PROTO ((rtx, rtx, rtx, rtx)),
				    rtx *));
extern void expand_df_unop PROTO ((rtx (*) PROTO ((rtx, rtx, rtx)), rtx *));
extern void expand_df_binop PROTO ((rtx (*) PROTO ((rtx, rtx, rtx, rtx)),
				    rtx *));
extern void expand_fp_branch PROTO ((rtx (*) PROTO((void)),
				     rtx (*) PROTO ((void))));
extern void emit_fpscr_use PROTO ((void));


#endif /* sh-protos.h */
