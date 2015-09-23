/* Definitions of target machine for GNU compiler, OR1K cpu.

   Copyright (C) 2010 Embecosm Limited

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#ifndef GCC_OR1K_PROTOS_H
#define GCC_OR1K_PROTOS_H

/* The following are for general support. */
extern int         or1k_trampoline_code_size (void);

/* The following are only needed when handling the machine definition. */
#ifdef RTX_CODE
extern void        or1k_init_expanders (void);
extern void        or1k_expand_prologue (void);
extern void        or1k_expand_epilogue (void);
extern bool        or1k_expand_move (enum machine_mode mode, rtx operands[]);
extern const char *or1k_output_move_double (rtx *operands);
extern void        or1k_expand_conditional_branch (rtx               *operands,
						   enum machine_mode  mode);
extern int         or1k_emit_cmove (rtx  dest,
				    rtx  op,
				    rtx  true_cond,
				    rtx  false_cond);
extern enum machine_mode or1k_select_cc_mode (enum rtx_code op);
extern const char *or1k_output_bf (rtx * operands);
extern const char *or1k_output_cmov (rtx * operands);
extern void        or1k_emit_set_const32 (rtx  op0,
                                          rtx  op1);
extern bool        or1k_expand_symbol_ref (enum machine_mode mode,
                                           rtx operands[]);
extern void        or1k_expand_cmpxchg_qihi (rtx bval, rtx retval,
                        rtx mem, rtx oldval, rtx newval, int is_weak,
                        enum memmodel success_mode, enum memmodel failure_mode);
extern void  or1k_expand_fetch_op_qihi (rtx oldval, rtx mem, rtx operand,
                                        rtx newval, rtx (*generator)(rtx, rtx, rtx, rtx, rtx));
#endif

#endif
extern int or1k_struct_alignment (tree);
extern int or1k_data_alignment (tree, int);

extern int or1k_initial_elimination_offset (int, int);
extern bool or1k_save_reg_p_cached (int regno);
extern void or1k_print_jump_restore (rtx jump_address);
extern rtx or1k_eh_return_handler_rtx (void);
extern rtx or1k_return_addr_rtx (int, rtx);

extern int or1k_legitimate_pic_operand_p (rtx x);

/* For RETURN_ADDR_RTX */
extern rtx get_hard_reg_initial_val (enum machine_mode, unsigned int);
