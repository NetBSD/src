/* Definitions of target machine for GNU compiler, for the pdp-11
   Copyright (C) 2000-2018 Free Software Foundation, Inc.
   Contributed by Michael K. Gschwind (mike@vlsivie.tuwien.ac.at).

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

/* declarations */
#ifdef RTX_CODE
extern int simple_memory_operand (rtx, machine_mode);

extern int legitimate_const_double_p (rtx);
extern void notice_update_cc_on_set (rtx, rtx);
extern void output_addr_const_pdp11 (FILE *, rtx);
extern const char *output_move_multiple (rtx *);
extern const char *output_block_move (rtx *);
extern const char *output_jump (enum rtx_code, int, int);
extern void print_operand_address (FILE *, rtx);
typedef enum { no_action, dec_before, inc_after } pdp11_action;
typedef enum { little, either, big } pdp11_partorder;
extern bool pdp11_expand_operands (rtx *, rtx [][2], int, 
				   pdp11_action *, pdp11_partorder);
extern int pdp11_sp_frame_offset (void);
extern int pdp11_initial_elimination_offset (int, int);
extern enum reg_class pdp11_regno_reg_class (int);

#endif /* RTX_CODE */

extern void output_ascii (FILE *, const char *, int);
extern void pdp11_asm_output_var (FILE *, const char *, int, int, bool);
extern void pdp11_expand_prologue (void);
extern void pdp11_expand_epilogue (void);
extern int pdp11_branch_cost (void);
extern poly_int64 pdp11_push_rounding (poly_int64);
