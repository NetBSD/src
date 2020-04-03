/* This file is obj-evax.h
   Copyright (C) 1996-2018 Free Software Foundation, Inc.
   Contributed by Klaus K�mpf (kkaempf@progis.de) of
     proGIS Software, Aachen, Germany.

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
   along with GAS; see the file COPYING.  If not, write to
   the Free Software Foundation, 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

/*
 * This file is obj-evax.h and is intended to be a template for
 * object format specific header files.
 */

#include "as.h"

/* define an obj specific macro off which target cpu back ends may key.  */
#define OBJ_EVAX 1

/* include whatever target cpu is appropriate.  */
#include "targ-cpu.h"

#define OUTPUT_FLAVOR bfd_target_evax_flavour

struct fix;

/* Simply linked list of .linkage.  */
struct alpha_linkage_fixups
{
  /* Next entry.  */
  struct alpha_linkage_fixups *next;

  /* Corresponding fixup.  */
  struct fix *fixp;

  /* Label that designates this entry.
     Note that a linkage entry can only be designated by one label.
     Also, s_alpha_linkage force the creation of a label.  */
  symbolS *label;
};

/*
 * SYMBOLS
 */

/*
 * If your object format needs to reorder symbols, define this.  When
 * defined, symbols are kept on a doubly linked list and functions are
 * made available for push, insert, append, and delete.  If not defined,
 * symbols are kept on a singly linked list, only the append and clear
 * facilities are available, and they are macros.
 */

/* #define SYMBOLS_NEED_PACKPOINTERS */

#define OBJ_EMIT_LINENO(a,b,c)	/* must be *something*.  This no-op's it out.  */

#define obj_symbol_new_hook(s)       evax_symbol_new_hook (s)
#define obj_frob_symbol(s,p)         evax_frob_symbol (s, &p)
#define obj_frob_file_before_adjust  evax_frob_file_before_adjust
#define obj_frob_file_before_fix     evax_frob_file_before_fix

#define S_GET_OTHER(S)	0
#define S_GET_TYPE(S)	0
#define S_GET_DESC(S)	0

#define PDSC_S_K_KIND_FP_STACK 9
#define PDSC_S_K_KIND_FP_REGISTER 10
#define PDSC_S_K_KIND_NULL 8

#define PDSC_S_K_MIN_STACK_SIZE 32
#define PDSC_S_K_MIN_REGISTER_SIZE 24
#define PDSC_S_K_NULL_SIZE 16

#define PDSC_S_M_HANDLER_VALID 0x10		/* low byte */
#define PDSC_S_M_HANDLER_DATA_VALID 0x40	/* low byte */
#define PDSC_S_M_BASE_REG_IS_FP 0x80		/* low byte */
#define PDSC_S_M_NATIVE 0x10		/* high byte */
#define PDSC_S_M_NO_JACKET 0x20		/* high byte */

#define LKP_S_K_SIZE 16

extern segT alpha_link_section;
extern struct alpha_linkage_fixups *alpha_linkage_fixup_root;

extern void evax_section (int);
extern void evax_symbol_new_hook (symbolS *);
extern void evax_frob_symbol (symbolS *, int *);
extern void evax_frob_file_before_adjust (void);
extern void evax_frob_file_before_fix (void);
extern char *evax_shorten_name (char *);

/*
 * Local Variables:
 * comment-column: 0
 * fill-column: 131
 * End:
 */
