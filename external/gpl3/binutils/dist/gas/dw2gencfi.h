/* dw2gencfi.h - Support for generating Dwarf2 CFI information.
   Copyright 2003, 2004, 2005, 2007, 2009 Free Software Foundation, Inc.
   Contributed by Michal Ludvig <mludvig@suse.cz>

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#ifndef DW2GENCFI_H
#define DW2GENCFI_H

#include "dwarf2.h"

struct symbol;

extern const pseudo_typeS cfi_pseudo_table[];

/* cfi_finish() is called at the end of file. It will complain if
   the last CFI wasn't properly closed by .cfi_endproc.  */
extern void cfi_finish (void);

/* Entry points for backends to add unwind information.  */
extern void cfi_new_fde (struct symbol *);
extern void cfi_end_fde (struct symbol *);
extern void cfi_set_return_column (unsigned);
extern void cfi_add_advance_loc (struct symbol *);

extern void cfi_add_CFA_offset (unsigned, offsetT);
extern void cfi_add_CFA_def_cfa (unsigned, offsetT);
extern void cfi_add_CFA_register (unsigned, unsigned);
extern void cfi_add_CFA_def_cfa_register (unsigned);
extern void cfi_add_CFA_def_cfa_offset (offsetT);
extern void cfi_add_CFA_restore (unsigned);
extern void cfi_add_CFA_undefined (unsigned);
extern void cfi_add_CFA_same_value (unsigned);
extern void cfi_add_CFA_remember_state (void);
extern void cfi_add_CFA_restore_state (void);

/* Structures for md_cfi_end.  */

#if defined (TE_PE) || defined (TE_PEP)
#define SUPPORT_FRAME_LINKONCE 1
#else
#define SUPPORT_FRAME_LINKONCE 0
#endif

struct cfi_insn_data
{
  struct cfi_insn_data *next;
#if SUPPORT_FRAME_LINKONCE
  segT cur_seg;
#endif
  int insn;
  union
  {
    struct
    {
      unsigned reg;
      offsetT offset;
    } ri;

    struct
    {
      unsigned reg1;
      unsigned reg2;
    } rr;

    unsigned r;
    offsetT i;

    struct
    {
      symbolS *lab1;
      symbolS *lab2;
    } ll;

    struct cfi_escape_data *esc;

    struct
    {
      unsigned reg, encoding;
      expressionS exp;
    } ea;
  } u;
};

struct fde_entry
{
  struct fde_entry *next;
#if SUPPORT_FRAME_LINKONCE
  segT cur_seg;
#endif
  symbolS *start_address;
  symbolS *end_address;
  struct cfi_insn_data *data;
  struct cfi_insn_data **last;
  unsigned char per_encoding;
  unsigned char lsda_encoding;
  expressionS personality;
  expressionS lsda;
  unsigned int return_column;
  unsigned int signal_frame;
#if SUPPORT_FRAME_LINKONCE
  int handled;
#endif
};

/* The list of all FDEs that have been collected.  */
extern struct fde_entry *all_fde_data;

/* Fake CFI type; outside the byte range of any real CFI insn.  */
#define CFI_adjust_cfa_offset	0x100
#define CFI_return_column	0x101
#define CFI_rel_offset		0x102
#define CFI_escape		0x103
#define CFI_signal_frame	0x104
#define CFI_val_encoded_addr	0x105

#endif /* DW2GENCFI_H */
