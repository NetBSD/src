/* Target-dependent code for Mitsubishi D10V, for GDB.
   Copyright (C) 1996, 1997, 2000 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/*  Contributed by Martin Hunt, hunt@cygnus.com */

#include "defs.h"
#include "frame.h"
#include "obstack.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdb_string.h"
#include "value.h"
#include "inferior.h"
#include "dis-asm.h"
#include "symfile.h"
#include "objfiles.h"
#include "language.h"

#include "sim-d10v.h"

#undef XMALLOC
#define XMALLOC(TYPE) ((TYPE*) xmalloc (sizeof (TYPE)))

struct frame_extra_info
  {
    CORE_ADDR return_pc;
    int frameless;
    int size;
  };

struct gdbarch_tdep
  {
    int a0_regnum;
    int nr_dmap_regs;
    unsigned long (*dmap_register) (int nr);
    unsigned long (*imap_register) (int nr);
    int (*register_sim_regno) (int nr);
  };

/* These are the addresses the D10V-EVA board maps data and
   instruction memory to. */

#define DMEM_START	0x2000000
#define IMEM_START	0x1000000
#define STACK_START	0x0007ffe

/* d10v register names. */

enum
  {
    R0_REGNUM = 0,
    LR_REGNUM = 13,
    PSW_REGNUM = 16,
    NR_IMAP_REGS = 2,
    NR_A_REGS = 2
  };
#define NR_DMAP_REGS (gdbarch_tdep (current_gdbarch)->nr_dmap_regs)
#define A0_REGNUM (gdbarch_tdep (current_gdbarch)->a0_regnum)

/* d10v calling convention. */

#define ARG1_REGNUM R0_REGNUM
#define ARGN_REGNUM 3
#define RET1_REGNUM R0_REGNUM

/* Local functions */

extern void _initialize_d10v_tdep PARAMS ((void));

static void d10v_eva_prepare_to_trace PARAMS ((void));

static void d10v_eva_get_trace_data PARAMS ((void));

static int prologue_find_regs PARAMS ((unsigned short op, struct frame_info * fi, CORE_ADDR addr));

extern void d10v_frame_init_saved_regs PARAMS ((struct frame_info *));

static void do_d10v_pop_frame PARAMS ((struct frame_info * fi));

int
d10v_frame_chain_valid (chain, frame)
     CORE_ADDR chain;
     struct frame_info *frame;	/* not used here */
{
  return ((chain) != 0 && (frame) != 0 && (frame)->pc > IMEM_START);
}

CORE_ADDR
d10v_stack_align (CORE_ADDR len)
{
  return (len + 1) & ~1;
}

/* Should we use EXTRACT_STRUCT_VALUE_ADDRESS instead of
   EXTRACT_RETURN_VALUE?  GCC_P is true if compiled with gcc
   and TYPE is the type (which is known to be struct, union or array).

   The d10v returns anything less than 8 bytes in size in
   registers. */

int
d10v_use_struct_convention (gcc_p, type)
     int gcc_p;
     struct type *type;
{
  return (TYPE_LENGTH (type) > 8);
}


unsigned char *
d10v_breakpoint_from_pc (pcptr, lenptr)
     CORE_ADDR *pcptr;
     int *lenptr;
{
  static unsigned char breakpoint[] =
  {0x2f, 0x90, 0x5e, 0x00};
  *lenptr = sizeof (breakpoint);
  return breakpoint;
}

/* Map the REG_NR onto an ascii name.  Return NULL or an empty string
   when the reg_nr isn't valid. */

enum ts2_regnums
  {
    TS2_IMAP0_REGNUM = 32,
    TS2_DMAP_REGNUM = 34,
    TS2_NR_DMAP_REGS = 1,
    TS2_A0_REGNUM = 35
  };

static char *
d10v_ts2_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "psw", "bpsw", "pc", "bpc", "cr4", "cr5", "cr6", "rpt_c",
    "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "cr15",
    "imap0", "imap1", "dmap", "a0", "a1"
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

enum ts3_regnums
  {
    TS3_IMAP0_REGNUM = 36,
    TS3_DMAP0_REGNUM = 38,
    TS3_NR_DMAP_REGS = 4,
    TS3_A0_REGNUM = 32
  };

static char *
d10v_ts3_register_name (int reg_nr)
{
  static char *register_names[] =
  {
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "psw", "bpsw", "pc", "bpc", "cr4", "cr5", "cr6", "rpt_c",
    "rpt_s", "rpt_e", "mod_s", "mod_e", "cr12", "cr13", "iba", "cr15",
    "a0", "a1",
    "spi", "spu",
    "imap0", "imap1",
    "dmap0", "dmap1", "dmap2", "dmap3"
  };
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= (sizeof (register_names) / sizeof (*register_names)))
    return NULL;
  return register_names[reg_nr];
}

/* Access the DMAP/IMAP registers in a target independant way. */

static unsigned long
d10v_ts2_dmap_register (int reg_nr)
{
  switch (reg_nr)
    {
    case 0:
    case 1:
      return 0x2000;
    case 2:
      return read_register (TS2_DMAP_REGNUM);
    default:
      return 0;
    }
}

static unsigned long
d10v_ts3_dmap_register (int reg_nr)
{
  return read_register (TS3_DMAP0_REGNUM + reg_nr);
}

static unsigned long
d10v_dmap_register (int reg_nr)
{
  return gdbarch_tdep (current_gdbarch)->dmap_register (reg_nr);
}

static unsigned long
d10v_ts2_imap_register (int reg_nr)
{
  return read_register (TS2_IMAP0_REGNUM + reg_nr);
}

static unsigned long
d10v_ts3_imap_register (int reg_nr)
{
  return read_register (TS3_IMAP0_REGNUM + reg_nr);
}

static unsigned long
d10v_imap_register (int reg_nr)
{
  return gdbarch_tdep (current_gdbarch)->imap_register (reg_nr);
}

/* MAP GDB's internal register numbering (determined by the layout fo
   the REGISTER_BYTE array) onto the simulator's register
   numbering. */

static int
d10v_ts2_register_sim_regno (int nr)
{
  if (nr >= TS2_IMAP0_REGNUM
      && nr < TS2_IMAP0_REGNUM + NR_IMAP_REGS)
    return nr - TS2_IMAP0_REGNUM + SIM_D10V_IMAP0_REGNUM;
  if (nr == TS2_DMAP_REGNUM)
    return nr - TS2_DMAP_REGNUM + SIM_D10V_TS2_DMAP_REGNUM;
  if (nr >= TS2_A0_REGNUM
      && nr < TS2_A0_REGNUM + NR_A_REGS)
    return nr - TS2_A0_REGNUM + SIM_D10V_A0_REGNUM;
  return nr;
}

static int
d10v_ts3_register_sim_regno (int nr)
{
  if (nr >= TS3_IMAP0_REGNUM
      && nr < TS3_IMAP0_REGNUM + NR_IMAP_REGS)
    return nr - TS3_IMAP0_REGNUM + SIM_D10V_IMAP0_REGNUM;
  if (nr >= TS3_DMAP0_REGNUM
      && nr < TS3_DMAP0_REGNUM + TS3_NR_DMAP_REGS)
    return nr - TS3_DMAP0_REGNUM + SIM_D10V_DMAP0_REGNUM;
  if (nr >= TS3_A0_REGNUM
      && nr < TS3_A0_REGNUM + NR_A_REGS)
    return nr - TS3_A0_REGNUM + SIM_D10V_A0_REGNUM;
  return nr;
}

int
d10v_register_sim_regno (int nr)
{
  return gdbarch_tdep (current_gdbarch)->register_sim_regno (nr);
}

/* Index within `registers' of the first byte of the space for
   register REG_NR.  */

int
d10v_register_byte (reg_nr)
     int reg_nr;
{
  if (reg_nr < A0_REGNUM)
    return (reg_nr * 2);
  else if (reg_nr < (A0_REGNUM + NR_A_REGS))
    return (A0_REGNUM * 2
	    + (reg_nr - A0_REGNUM) * 8);
  else
    return (A0_REGNUM * 2
	    + NR_A_REGS * 8
	    + (reg_nr - A0_REGNUM - NR_A_REGS) * 2);
}

/* Number of bytes of storage in the actual machine representation for
   register REG_NR.  */

int
d10v_register_raw_size (reg_nr)
     int reg_nr;
{
  if (reg_nr < A0_REGNUM)
    return 2;
  else if (reg_nr < (A0_REGNUM + NR_A_REGS))
    return 8;
  else
    return 2;
}

/* Number of bytes of storage in the program's representation
   for register N.  */

int
d10v_register_virtual_size (reg_nr)
     int reg_nr;
{
  return TYPE_LENGTH (REGISTER_VIRTUAL_TYPE (reg_nr));
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

struct type *
d10v_register_virtual_type (reg_nr)
     int reg_nr;
{
  if (reg_nr >= A0_REGNUM
      && reg_nr < (A0_REGNUM + NR_A_REGS))
    return builtin_type_int64;
  else if (reg_nr == PC_REGNUM
	   || reg_nr == SP_REGNUM)
    return builtin_type_int32;
  else
    return builtin_type_int16;
}

/* convert $pc and $sp to/from virtual addresses */
int
d10v_register_convertible (nr)
     int nr;
{
  return ((nr) == PC_REGNUM || (nr) == SP_REGNUM);
}

void
d10v_register_convert_to_virtual (regnum, type, from, to)
     int regnum;
     struct type *type;
     char *from;
     char *to;
{
  ULONGEST x = extract_unsigned_integer (from, REGISTER_RAW_SIZE (regnum));
  if (regnum == PC_REGNUM)
    x = (x << 2) | IMEM_START;
  else
    x |= DMEM_START;
  store_unsigned_integer (to, TYPE_LENGTH (type), x);
}

void
d10v_register_convert_to_raw (type, regnum, from, to)
     struct type *type;
     int regnum;
     char *from;
     char *to;
{
  ULONGEST x = extract_unsigned_integer (from, TYPE_LENGTH (type));
  x &= 0x3ffff;
  if (regnum == PC_REGNUM)
    x >>= 2;
  store_unsigned_integer (to, 2, x);
}


CORE_ADDR
d10v_make_daddr (x)
     CORE_ADDR x;
{
  return ((x) | DMEM_START);
}

CORE_ADDR
d10v_make_iaddr (x)
     CORE_ADDR x;
{
  return (((x) << 2) | IMEM_START);
}

int
d10v_daddr_p (x)
     CORE_ADDR x;
{
  return (((x) & 0x3000000) == DMEM_START);
}

int
d10v_iaddr_p (x)
     CORE_ADDR x;
{
  return (((x) & 0x3000000) == IMEM_START);
}


CORE_ADDR
d10v_convert_iaddr_to_raw (x)
     CORE_ADDR x;
{
  return (((x) >> 2) & 0xffff);
}

CORE_ADDR
d10v_convert_daddr_to_raw (x)
     CORE_ADDR x;
{
  return ((x) & 0xffff);
}

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. 

   We store structs through a pointer passed in the first Argument
   register. */

void
d10v_store_struct_return (addr, sp)
     CORE_ADDR addr;
     CORE_ADDR sp;
{
  write_register (ARG1_REGNUM, (addr));
}

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  

   Things always get returned in RET1_REGNUM, RET2_REGNUM, ... */

void
d10v_store_return_value (type, valbuf)
     struct type *type;
     char *valbuf;
{
  write_register_bytes (REGISTER_BYTE (RET1_REGNUM),
			valbuf,
			TYPE_LENGTH (type));
}

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */

CORE_ADDR
d10v_extract_struct_value_address (regbuf)
     char *regbuf;
{
  return (extract_address ((regbuf) + REGISTER_BYTE (ARG1_REGNUM),
			   REGISTER_RAW_SIZE (ARG1_REGNUM))
	  | DMEM_START);
}

CORE_ADDR
d10v_frame_saved_pc (frame)
     struct frame_info *frame;
{
  return ((frame)->extra_info->return_pc);
}

CORE_ADDR
d10v_frame_args_address (fi)
     struct frame_info *fi;
{
  return (fi)->frame;
}

CORE_ADDR
d10v_frame_locals_address (fi)
     struct frame_info *fi;
{
  return (fi)->frame;
}

/* Immediately after a function call, return the saved pc.  We can't
   use frame->return_pc beause that is determined by reading R13 off
   the stack and that may not be written yet. */

CORE_ADDR
d10v_saved_pc_after_call (frame)
     struct frame_info *frame;
{
  return ((read_register (LR_REGNUM) << 2)
	  | IMEM_START);
}

/* Discard from the stack the innermost frame, restoring all saved
   registers.  */

void
d10v_pop_frame ()
{
  generic_pop_current_frame (do_d10v_pop_frame);
}

static void
do_d10v_pop_frame (fi)
     struct frame_info *fi;
{
  CORE_ADDR fp;
  int regnum;
  char raw_buffer[8];

  fp = FRAME_FP (fi);
  /* fill out fsr with the address of where each */
  /* register was stored in the frame */
  d10v_frame_init_saved_regs (fi);

  /* now update the current registers with the old values */
  for (regnum = A0_REGNUM; regnum < A0_REGNUM + NR_A_REGS; regnum++)
    {
      if (fi->saved_regs[regnum])
	{
	  read_memory (fi->saved_regs[regnum], raw_buffer, REGISTER_RAW_SIZE (regnum));
	  write_register_bytes (REGISTER_BYTE (regnum), raw_buffer, REGISTER_RAW_SIZE (regnum));
	}
    }
  for (regnum = 0; regnum < SP_REGNUM; regnum++)
    {
      if (fi->saved_regs[regnum])
	{
	  write_register (regnum, read_memory_unsigned_integer (fi->saved_regs[regnum], REGISTER_RAW_SIZE (regnum)));
	}
    }
  if (fi->saved_regs[PSW_REGNUM])
    {
      write_register (PSW_REGNUM, read_memory_unsigned_integer (fi->saved_regs[PSW_REGNUM], REGISTER_RAW_SIZE (PSW_REGNUM)));
    }

  write_register (PC_REGNUM, read_register (LR_REGNUM));
  write_register (SP_REGNUM, fp + fi->extra_info->size);
  target_store_registers (-1);
  flush_cached_frames ();
}

static int
check_prologue (op)
     unsigned short op;
{
  /* st  rn, @-sp */
  if ((op & 0x7E1F) == 0x6C1F)
    return 1;

  /* st2w  rn, @-sp */
  if ((op & 0x7E3F) == 0x6E1F)
    return 1;

  /* subi  sp, n */
  if ((op & 0x7FE1) == 0x01E1)
    return 1;

  /* mv  r11, sp */
  if (op == 0x417E)
    return 1;

  /* nop */
  if (op == 0x5E00)
    return 1;

  /* st  rn, @sp */
  if ((op & 0x7E1F) == 0x681E)
    return 1;

  /* st2w  rn, @sp */
  if ((op & 0x7E3F) == 0x3A1E)
    return 1;

  return 0;
}

CORE_ADDR
d10v_skip_prologue (pc)
     CORE_ADDR pc;
{
  unsigned long op;
  unsigned short op1, op2;
  CORE_ADDR func_addr, func_end;
  struct symtab_and_line sal;

  /* If we have line debugging information, then the end of the */
  /* prologue should the first assembly instruction of  the first source line */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      sal = find_pc_line (func_addr, 0);
      if (sal.end && sal.end < func_end)
	return sal.end;
    }

  if (target_read_memory (pc, (char *) &op, 4))
    return pc;			/* Can't access it -- assume no prologue. */

  while (1)
    {
      op = (unsigned long) read_memory_integer (pc, 4);
      if ((op & 0xC0000000) == 0xC0000000)
	{
	  /* long instruction */
	  if (((op & 0x3FFF0000) != 0x01FF0000) &&	/* add3 sp,sp,n */
	      ((op & 0x3F0F0000) != 0x340F0000) &&	/* st  rn, @(offset,sp) */
	      ((op & 0x3F1F0000) != 0x350F0000))	/* st2w  rn, @(offset,sp) */
	    break;
	}
      else
	{
	  /* short instructions */
	  if ((op & 0xC0000000) == 0x80000000)
	    {
	      op2 = (op & 0x3FFF8000) >> 15;
	      op1 = op & 0x7FFF;
	    }
	  else
	    {
	      op1 = (op & 0x3FFF8000) >> 15;
	      op2 = op & 0x7FFF;
	    }
	  if (check_prologue (op1))
	    {
	      if (!check_prologue (op2))
		{
		  /* if the previous opcode was really part of the prologue */
		  /* and not just a NOP, then we want to break after both instructions */
		  if (op1 != 0x5E00)
		    pc += 4;
		  break;
		}
	    }
	  else
	    break;
	}
      pc += 4;
    }
  return pc;
}

/* Given a GDB frame, determine the address of the calling function's frame.
   This will be used to create a new GDB frame struct, and then
   INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.
 */

CORE_ADDR
d10v_frame_chain (fi)
     struct frame_info *fi;
{
  d10v_frame_init_saved_regs (fi);

  if (fi->extra_info->return_pc == IMEM_START
      || inside_entry_file (fi->extra_info->return_pc))
    return (CORE_ADDR) 0;

  if (!fi->saved_regs[FP_REGNUM])
    {
      if (!fi->saved_regs[SP_REGNUM]
	  || fi->saved_regs[SP_REGNUM] == STACK_START)
	return (CORE_ADDR) 0;

      return fi->saved_regs[SP_REGNUM];
    }

  if (!read_memory_unsigned_integer (fi->saved_regs[FP_REGNUM],
				     REGISTER_RAW_SIZE (FP_REGNUM)))
    return (CORE_ADDR) 0;

  return D10V_MAKE_DADDR (read_memory_unsigned_integer (fi->saved_regs[FP_REGNUM],
					    REGISTER_RAW_SIZE (FP_REGNUM)));
}

static int next_addr, uses_frame;

static int
prologue_find_regs (op, fi, addr)
     unsigned short op;
     struct frame_info *fi;
     CORE_ADDR addr;
{
  int n;

  /* st  rn, @-sp */
  if ((op & 0x7E1F) == 0x6C1F)
    {
      n = (op & 0x1E0) >> 5;
      next_addr -= 2;
      fi->saved_regs[n] = next_addr;
      return 1;
    }

  /* st2w  rn, @-sp */
  else if ((op & 0x7E3F) == 0x6E1F)
    {
      n = (op & 0x1E0) >> 5;
      next_addr -= 4;
      fi->saved_regs[n] = next_addr;
      fi->saved_regs[n + 1] = next_addr + 2;
      return 1;
    }

  /* subi  sp, n */
  if ((op & 0x7FE1) == 0x01E1)
    {
      n = (op & 0x1E) >> 1;
      if (n == 0)
	n = 16;
      next_addr -= n;
      return 1;
    }

  /* mv  r11, sp */
  if (op == 0x417E)
    {
      uses_frame = 1;
      return 1;
    }

  /* nop */
  if (op == 0x5E00)
    return 1;

  /* st  rn, @sp */
  if ((op & 0x7E1F) == 0x681E)
    {
      n = (op & 0x1E0) >> 5;
      fi->saved_regs[n] = next_addr;
      return 1;
    }

  /* st2w  rn, @sp */
  if ((op & 0x7E3F) == 0x3A1E)
    {
      n = (op & 0x1E0) >> 5;
      fi->saved_regs[n] = next_addr;
      fi->saved_regs[n + 1] = next_addr + 2;
      return 1;
    }

  return 0;
}

/* Put here the code to store, into fi->saved_regs, the addresses of
   the saved registers of frame described by FRAME_INFO.  This
   includes special registers such as pc and fp saved in special ways
   in the stack frame.  sp is even more special: the address we return
   for it IS the sp for the next frame. */

void
d10v_frame_init_saved_regs (fi)
     struct frame_info *fi;
{
  CORE_ADDR fp, pc;
  unsigned long op;
  unsigned short op1, op2;
  int i;

  fp = fi->frame;
  memset (fi->saved_regs, 0, SIZEOF_FRAME_SAVED_REGS);
  next_addr = 0;

  pc = get_pc_function_start (fi->pc);

  uses_frame = 0;
  while (1)
    {
      op = (unsigned long) read_memory_integer (pc, 4);
      if ((op & 0xC0000000) == 0xC0000000)
	{
	  /* long instruction */
	  if ((op & 0x3FFF0000) == 0x01FF0000)
	    {
	      /* add3 sp,sp,n */
	      short n = op & 0xFFFF;
	      next_addr += n;
	    }
	  else if ((op & 0x3F0F0000) == 0x340F0000)
	    {
	      /* st  rn, @(offset,sp) */
	      short offset = op & 0xFFFF;
	      short n = (op >> 20) & 0xF;
	      fi->saved_regs[n] = next_addr + offset;
	    }
	  else if ((op & 0x3F1F0000) == 0x350F0000)
	    {
	      /* st2w  rn, @(offset,sp) */
	      short offset = op & 0xFFFF;
	      short n = (op >> 20) & 0xF;
	      fi->saved_regs[n] = next_addr + offset;
	      fi->saved_regs[n + 1] = next_addr + offset + 2;
	    }
	  else
	    break;
	}
      else
	{
	  /* short instructions */
	  if ((op & 0xC0000000) == 0x80000000)
	    {
	      op2 = (op & 0x3FFF8000) >> 15;
	      op1 = op & 0x7FFF;
	    }
	  else
	    {
	      op1 = (op & 0x3FFF8000) >> 15;
	      op2 = op & 0x7FFF;
	    }
	  if (!prologue_find_regs (op1, fi, pc) || !prologue_find_regs (op2, fi, pc))
	    break;
	}
      pc += 4;
    }

  fi->extra_info->size = -next_addr;

  if (!(fp & 0xffff))
    fp = D10V_MAKE_DADDR (read_register (SP_REGNUM));

  for (i = 0; i < NUM_REGS - 1; i++)
    if (fi->saved_regs[i])
      {
	fi->saved_regs[i] = fp - (next_addr - fi->saved_regs[i]);
      }

  if (fi->saved_regs[LR_REGNUM])
    {
      CORE_ADDR return_pc = read_memory_unsigned_integer (fi->saved_regs[LR_REGNUM], REGISTER_RAW_SIZE (LR_REGNUM));
      fi->extra_info->return_pc = D10V_MAKE_IADDR (return_pc);
    }
  else
    {
      fi->extra_info->return_pc = D10V_MAKE_IADDR (read_register (LR_REGNUM));
    }

  /* th SP is not normally (ever?) saved, but check anyway */
  if (!fi->saved_regs[SP_REGNUM])
    {
      /* if the FP was saved, that means the current FP is valid, */
      /* otherwise, it isn't being used, so we use the SP instead */
      if (uses_frame)
	fi->saved_regs[SP_REGNUM] = read_register (FP_REGNUM) + fi->extra_info->size;
      else
	{
	  fi->saved_regs[SP_REGNUM] = fp + fi->extra_info->size;
	  fi->extra_info->frameless = 1;
	  fi->saved_regs[FP_REGNUM] = 0;
	}
    }
}

void
d10v_init_extra_frame_info (fromleaf, fi)
     int fromleaf;
     struct frame_info *fi;
{
  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));
  frame_saved_regs_zalloc (fi);

  fi->extra_info->frameless = 0;
  fi->extra_info->size = 0;
  fi->extra_info->return_pc = 0;

  /* The call dummy doesn't save any registers on the stack, so we can
     return now.  */
  if (PC_IN_CALL_DUMMY (fi->pc, fi->frame, fi->frame))
    {
      return;
    }
  else
    {
      d10v_frame_init_saved_regs (fi);
    }
}

static void
show_regs (args, from_tty)
     char *args;
     int from_tty;
{
  int a;
  printf_filtered ("PC=%04lx (0x%lx) PSW=%04lx RPT_S=%04lx RPT_E=%04lx RPT_C=%04lx\n",
		   (long) read_register (PC_REGNUM),
		   (long) D10V_MAKE_IADDR (read_register (PC_REGNUM)),
		   (long) read_register (PSW_REGNUM),
		   (long) read_register (24),
		   (long) read_register (25),
		   (long) read_register (23));
  printf_filtered ("R0-R7  %04lx %04lx %04lx %04lx %04lx %04lx %04lx %04lx\n",
		   (long) read_register (0),
		   (long) read_register (1),
		   (long) read_register (2),
		   (long) read_register (3),
		   (long) read_register (4),
		   (long) read_register (5),
		   (long) read_register (6),
		   (long) read_register (7));
  printf_filtered ("R8-R15 %04lx %04lx %04lx %04lx %04lx %04lx %04lx %04lx\n",
		   (long) read_register (8),
		   (long) read_register (9),
		   (long) read_register (10),
		   (long) read_register (11),
		   (long) read_register (12),
		   (long) read_register (13),
		   (long) read_register (14),
		   (long) read_register (15));
  for (a = 0; a < NR_IMAP_REGS; a++)
    {
      if (a > 0)
	printf_filtered ("    ");
      printf_filtered ("IMAP%d %04lx", a, d10v_imap_register (a));
    }
  if (NR_DMAP_REGS == 1)
    printf_filtered ("    DMAP %04lx\n", d10v_dmap_register (2));
  else
    {
      for (a = 0; a < NR_DMAP_REGS; a++)
	{
	  printf_filtered ("    DMAP%d %04lx", a, d10v_dmap_register (a));
	}
      printf_filtered ("\n");
    }
  printf_filtered ("A0-A%d", NR_A_REGS - 1);
  for (a = A0_REGNUM; a < A0_REGNUM + NR_A_REGS; a++)
    {
      char num[MAX_REGISTER_RAW_SIZE];
      int i;
      printf_filtered ("  ");
      read_register_gen (a, (char *) &num);
      for (i = 0; i < MAX_REGISTER_RAW_SIZE; i++)
	{
	  printf_filtered ("%02x", (num[i] & 0xff));
	}
    }
  printf_filtered ("\n");
}

CORE_ADDR
d10v_read_pc (pid)
     int pid;
{
  int save_pid;
  CORE_ADDR pc;
  CORE_ADDR retval;

  save_pid = inferior_pid;
  inferior_pid = pid;
  pc = (int) read_register (PC_REGNUM);
  inferior_pid = save_pid;
  retval = D10V_MAKE_IADDR (pc);
  return retval;
}

void
d10v_write_pc (val, pid)
     CORE_ADDR val;
     int pid;
{
  int save_pid;

  save_pid = inferior_pid;
  inferior_pid = pid;
  write_register (PC_REGNUM, D10V_CONVERT_IADDR_TO_RAW (val));
  inferior_pid = save_pid;
}

CORE_ADDR
d10v_read_sp ()
{
  return (D10V_MAKE_DADDR (read_register (SP_REGNUM)));
}

void
d10v_write_sp (val)
     CORE_ADDR val;
{
  write_register (SP_REGNUM, D10V_CONVERT_DADDR_TO_RAW (val));
}

void
d10v_write_fp (val)
     CORE_ADDR val;
{
  write_register (FP_REGNUM, D10V_CONVERT_DADDR_TO_RAW (val));
}

CORE_ADDR
d10v_read_fp ()
{
  return (D10V_MAKE_DADDR (read_register (FP_REGNUM)));
}

/* Function: push_return_address (pc)
   Set up the return address for the inferior function call.
   Needed for targets where we don't actually execute a JSR/BSR instruction */

CORE_ADDR
d10v_push_return_address (pc, sp)
     CORE_ADDR pc;
     CORE_ADDR sp;
{
  write_register (LR_REGNUM, D10V_CONVERT_IADDR_TO_RAW (CALL_DUMMY_ADDRESS ()));
  return sp;
}


/* When arguments must be pushed onto the stack, they go on in reverse
   order.  The below implements a FILO (stack) to do this. */

struct stack_item
{
  int len;
  struct stack_item *prev;
  void *data;
};

static struct stack_item *push_stack_item PARAMS ((struct stack_item * prev, void *contents, int len));
static struct stack_item *
push_stack_item (prev, contents, len)
     struct stack_item *prev;
     void *contents;
     int len;
{
  struct stack_item *si;
  si = xmalloc (sizeof (struct stack_item));
  si->data = xmalloc (len);
  si->len = len;
  si->prev = prev;
  memcpy (si->data, contents, len);
  return si;
}

static struct stack_item *pop_stack_item PARAMS ((struct stack_item * si));
static struct stack_item *
pop_stack_item (si)
     struct stack_item *si;
{
  struct stack_item *dead = si;
  si = si->prev;
  free (dead->data);
  free (dead);
  return si;
}


CORE_ADDR
d10v_push_arguments (nargs, args, sp, struct_return, struct_addr)
     int nargs;
     value_ptr *args;
     CORE_ADDR sp;
     int struct_return;
     CORE_ADDR struct_addr;
{
  int i;
  int regnum = ARG1_REGNUM;
  struct stack_item *si = NULL;

  /* Fill in registers and arg lists */
  for (i = 0; i < nargs; i++)
    {
      value_ptr arg = args[i];
      struct type *type = check_typedef (VALUE_TYPE (arg));
      char *contents = VALUE_CONTENTS (arg);
      int len = TYPE_LENGTH (type);
      /* printf ("push: type=%d len=%d\n", type->code, len); */
      if (TYPE_CODE (type) == TYPE_CODE_PTR)
	{
	  /* pointers require special handling - first convert and
	     then store */
	  long val = extract_signed_integer (contents, len);
	  len = 2;
	  if (TYPE_TARGET_TYPE (type)
	      && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC))
	    {
	      /* function pointer */
	      val = D10V_CONVERT_IADDR_TO_RAW (val);
	    }
	  else if (D10V_IADDR_P (val))
	    {
	      /* also function pointer! */
	      val = D10V_CONVERT_DADDR_TO_RAW (val);
	    }
	  else
	    {
	      /* data pointer */
	      val &= 0xFFFF;
	    }
	  if (regnum <= ARGN_REGNUM)
	    write_register (regnum++, val & 0xffff);
	  else
	    {
	      char ptr[2];
	      /* arg will go onto stack */
	      store_address (ptr, 2, val & 0xffff);
	      si = push_stack_item (si, ptr, 2);
	    }
	}
      else
	{
	  int aligned_regnum = (regnum + 1) & ~1;
	  if (len <= 2 && regnum <= ARGN_REGNUM)
	    /* fits in a single register, do not align */
	    {
	      long val = extract_unsigned_integer (contents, len);
	      write_register (regnum++, val);
	    }
	  else if (len <= (ARGN_REGNUM - aligned_regnum + 1) * 2)
	    /* value fits in remaining registers, store keeping left
	       aligned */
	    {
	      int b;
	      regnum = aligned_regnum;
	      for (b = 0; b < (len & ~1); b += 2)
		{
		  long val = extract_unsigned_integer (&contents[b], 2);
		  write_register (regnum++, val);
		}
	      if (b < len)
		{
		  long val = extract_unsigned_integer (&contents[b], 1);
		  write_register (regnum++, (val << 8));
		}
	    }
	  else
	    {
	      /* arg will go onto stack */
	      regnum = ARGN_REGNUM + 1;
	      si = push_stack_item (si, contents, len);
	    }
	}
    }

  while (si)
    {
      sp = (sp - si->len) & ~1;
      write_memory (sp, si->data, si->len);
      si = pop_stack_item (si);
    }

  return sp;
}


/* Given a return value in `regbuf' with a type `valtype', 
   extract and copy its value into `valbuf'.  */

void
d10v_extract_return_value (type, regbuf, valbuf)
     struct type *type;
     char regbuf[REGISTER_BYTES];
     char *valbuf;
{
  int len;
  /*    printf("RET: TYPE=%d len=%d r%d=0x%x\n",type->code, TYPE_LENGTH (type), RET1_REGNUM - R0_REGNUM, (int) extract_unsigned_integer (regbuf + REGISTER_BYTE(RET1_REGNUM), REGISTER_RAW_SIZE (RET1_REGNUM)));  */
  if (TYPE_CODE (type) == TYPE_CODE_PTR
      && TYPE_TARGET_TYPE (type)
      && (TYPE_CODE (TYPE_TARGET_TYPE (type)) == TYPE_CODE_FUNC))
    {
      /* pointer to function */
      int num;
      short snum;
      snum = extract_address (regbuf + REGISTER_BYTE (RET1_REGNUM), REGISTER_RAW_SIZE (RET1_REGNUM));
      store_address (valbuf, 4, D10V_MAKE_IADDR (snum));
    }
  else if (TYPE_CODE (type) == TYPE_CODE_PTR)
    {
      /* pointer to data */
      int num;
      short snum;
      snum = extract_address (regbuf + REGISTER_BYTE (RET1_REGNUM), REGISTER_RAW_SIZE (RET1_REGNUM));
      store_address (valbuf, 4, D10V_MAKE_DADDR (snum));
    }
  else
    {
      len = TYPE_LENGTH (type);
      if (len == 1)
	{
	  unsigned short c = extract_unsigned_integer (regbuf + REGISTER_BYTE (RET1_REGNUM), REGISTER_RAW_SIZE (RET1_REGNUM));
	  store_unsigned_integer (valbuf, 1, c);
	}
      else if ((len & 1) == 0)
	memcpy (valbuf, regbuf + REGISTER_BYTE (RET1_REGNUM), len);
      else
	{
	  /* For return values of odd size, the first byte is in the
	     least significant part of the first register.  The
	     remaining bytes in remaining registers. Interestingly,
	     when such values are passed in, the last byte is in the
	     most significant byte of that same register - wierd. */
	  memcpy (valbuf, regbuf + REGISTER_BYTE (RET1_REGNUM) + 1, len);
	}
    }
}

/* Translate a GDB virtual ADDR/LEN into a format the remote target
   understands.  Returns number of bytes that can be transfered
   starting at TARG_ADDR.  Return ZERO if no bytes can be transfered
   (segmentation fault).  Since the simulator knows all about how the
   VM system works, we just call that to do the translation. */

static void
remote_d10v_translate_xfer_address (CORE_ADDR memaddr, int nr_bytes,
				    CORE_ADDR *targ_addr, int *targ_len)
{
  long out_addr;
  long out_len;
  out_len = sim_d10v_translate_addr (memaddr, nr_bytes,
				     &out_addr,
				     d10v_dmap_register,
				     d10v_imap_register);
  *targ_addr = out_addr;
  *targ_len = out_len;
}


/* The following code implements access to, and display of, the D10V's
   instruction trace buffer.  The buffer consists of 64K or more
   4-byte words of data, of which each words includes an 8-bit count,
   an 8-bit segment number, and a 16-bit instruction address.

   In theory, the trace buffer is continuously capturing instruction
   data that the CPU presents on its "debug bus", but in practice, the
   ROMified GDB stub only enables tracing when it continues or steps
   the program, and stops tracing when the program stops; so it
   actually works for GDB to read the buffer counter out of memory and
   then read each trace word.  The counter records where the tracing
   stops, but there is no record of where it started, so we remember
   the PC when we resumed and then search backwards in the trace
   buffer for a word that includes that address.  This is not perfect,
   because you will miss trace data if the resumption PC is the target
   of a branch.  (The value of the buffer counter is semi-random, any
   trace data from a previous program stop is gone.)  */

/* The address of the last word recorded in the trace buffer.  */

#define DBBC_ADDR (0xd80000)

/* The base of the trace buffer, at least for the "Board_0".  */

#define TRACE_BUFFER_BASE (0xf40000)

static void trace_command PARAMS ((char *, int));

static void untrace_command PARAMS ((char *, int));

static void trace_info PARAMS ((char *, int));

static void tdisassemble_command PARAMS ((char *, int));

static void display_trace PARAMS ((int, int));

/* True when instruction traces are being collected.  */

static int tracing;

/* Remembered PC.  */

static CORE_ADDR last_pc;

/* True when trace output should be displayed whenever program stops.  */

static int trace_display;

/* True when trace listing should include source lines.  */

static int default_trace_show_source = 1;

struct trace_buffer
  {
    int size;
    short *counts;
    CORE_ADDR *addrs;
  }
trace_data;

static void
trace_command (args, from_tty)
     char *args;
     int from_tty;
{
  /* Clear the host-side trace buffer, allocating space if needed.  */
  trace_data.size = 0;
  if (trace_data.counts == NULL)
    trace_data.counts = (short *) xmalloc (65536 * sizeof (short));
  if (trace_data.addrs == NULL)
    trace_data.addrs = (CORE_ADDR *) xmalloc (65536 * sizeof (CORE_ADDR));

  tracing = 1;

  printf_filtered ("Tracing is now on.\n");
}

static void
untrace_command (args, from_tty)
     char *args;
     int from_tty;
{
  tracing = 0;

  printf_filtered ("Tracing is now off.\n");
}

static void
trace_info (args, from_tty)
     char *args;
     int from_tty;
{
  int i;

  if (trace_data.size)
    {
      printf_filtered ("%d entries in trace buffer:\n", trace_data.size);

      for (i = 0; i < trace_data.size; ++i)
	{
	  printf_filtered ("%d: %d instruction%s at 0x%s\n",
			   i,
			   trace_data.counts[i],
			   (trace_data.counts[i] == 1 ? "" : "s"),
			   paddr_nz (trace_data.addrs[i]));
	}
    }
  else
    printf_filtered ("No entries in trace buffer.\n");

  printf_filtered ("Tracing is currently %s.\n", (tracing ? "on" : "off"));
}

/* Print the instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns length of the instruction, in bytes.  */

static int
print_insn (memaddr, stream)
     CORE_ADDR memaddr;
     struct ui_file *stream;
{
  /* If there's no disassembler, something is very wrong.  */
  if (tm_print_insn == NULL)
    internal_error ("print_insn: no disassembler");

  if (TARGET_BYTE_ORDER == BIG_ENDIAN)
    tm_print_insn_info.endian = BFD_ENDIAN_BIG;
  else
    tm_print_insn_info.endian = BFD_ENDIAN_LITTLE;
  return (*tm_print_insn) (memaddr, &tm_print_insn_info);
}

static void
d10v_eva_prepare_to_trace ()
{
  if (!tracing)
    return;

  last_pc = read_register (PC_REGNUM);
}

/* Collect trace data from the target board and format it into a form
   more useful for display.  */

static void
d10v_eva_get_trace_data ()
{
  int count, i, j, oldsize;
  int trace_addr, trace_seg, trace_cnt, next_cnt;
  unsigned int last_trace, trace_word, next_word;
  unsigned int *tmpspace;

  if (!tracing)
    return;

  tmpspace = xmalloc (65536 * sizeof (unsigned int));

  last_trace = read_memory_unsigned_integer (DBBC_ADDR, 2) << 2;

  /* Collect buffer contents from the target, stopping when we reach
     the word recorded when execution resumed.  */

  count = 0;
  while (last_trace > 0)
    {
      QUIT;
      trace_word =
	read_memory_unsigned_integer (TRACE_BUFFER_BASE + last_trace, 4);
      trace_addr = trace_word & 0xffff;
      last_trace -= 4;
      /* Ignore an apparently nonsensical entry.  */
      if (trace_addr == 0xffd5)
	continue;
      tmpspace[count++] = trace_word;
      if (trace_addr == last_pc)
	break;
      if (count > 65535)
	break;
    }

  /* Move the data to the host-side trace buffer, adjusting counts to
     include the last instruction executed and transforming the address
     into something that GDB likes.  */

  for (i = 0; i < count; ++i)
    {
      trace_word = tmpspace[i];
      next_word = ((i == 0) ? 0 : tmpspace[i - 1]);
      trace_addr = trace_word & 0xffff;
      next_cnt = (next_word >> 24) & 0xff;
      j = trace_data.size + count - i - 1;
      trace_data.addrs[j] = (trace_addr << 2) + 0x1000000;
      trace_data.counts[j] = next_cnt + 1;
    }

  oldsize = trace_data.size;
  trace_data.size += count;

  free (tmpspace);

  if (trace_display)
    display_trace (oldsize, trace_data.size);
}

static void
tdisassemble_command (arg, from_tty)
     char *arg;
     int from_tty;
{
  int i, count;
  CORE_ADDR low, high;
  char *space_index;

  if (!arg)
    {
      low = 0;
      high = trace_data.size;
    }
  else if (!(space_index = (char *) strchr (arg, ' ')))
    {
      low = parse_and_eval_address (arg);
      high = low + 5;
    }
  else
    {
      /* Two arguments.  */
      *space_index = '\0';
      low = parse_and_eval_address (arg);
      high = parse_and_eval_address (space_index + 1);
      if (high < low)
	high = low;
    }

  printf_filtered ("Dump of trace from %s to %s:\n", paddr_u (low), paddr_u (high));

  display_trace (low, high);

  printf_filtered ("End of trace dump.\n");
  gdb_flush (gdb_stdout);
}

static void
display_trace (low, high)
     int low, high;
{
  int i, count, trace_show_source, first, suppress;
  CORE_ADDR next_address;

  trace_show_source = default_trace_show_source;
  if (!have_full_symbols () && !have_partial_symbols ())
    {
      trace_show_source = 0;
      printf_filtered ("No symbol table is loaded.  Use the \"file\" command.\n");
      printf_filtered ("Trace will not display any source.\n");
    }

  first = 1;
  suppress = 0;
  for (i = low; i < high; ++i)
    {
      next_address = trace_data.addrs[i];
      count = trace_data.counts[i];
      while (count-- > 0)
	{
	  QUIT;
	  if (trace_show_source)
	    {
	      struct symtab_and_line sal, sal_prev;

	      sal_prev = find_pc_line (next_address - 4, 0);
	      sal = find_pc_line (next_address, 0);

	      if (sal.symtab)
		{
		  if (first || sal.line != sal_prev.line)
		    print_source_lines (sal.symtab, sal.line, sal.line + 1, 0);
		  suppress = 0;
		}
	      else
		{
		  if (!suppress)
		    /* FIXME-32x64--assumes sal.pc fits in long.  */
		    printf_filtered ("No source file for address %s.\n",
				 local_hex_string ((unsigned long) sal.pc));
		  suppress = 1;
		}
	    }
	  first = 0;
	  print_address (next_address, gdb_stdout);
	  printf_filtered (":");
	  printf_filtered ("\t");
	  wrap_here ("    ");
	  next_address = next_address + print_insn (next_address, gdb_stdout);
	  printf_filtered ("\n");
	  gdb_flush (gdb_stdout);
	}
    }
}


static gdbarch_init_ftype d10v_gdbarch_init;

static struct gdbarch *
d10v_gdbarch_init (info, arches)
     struct gdbarch_info info;
     struct gdbarch_list *arches;
{
  static LONGEST d10v_call_dummy_words[] =
  {0};
  struct gdbarch *gdbarch;
  int d10v_num_regs;
  struct gdbarch_tdep *tdep;
  gdbarch_register_name_ftype *d10v_register_name;

  /* Find a candidate among the list of pre-declared architectures. */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found, create a new architecture from the information
     provided. */
  tdep = XMALLOC (struct gdbarch_tdep);
  gdbarch = gdbarch_alloc (&info, tdep);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_d10v_ts2:
      d10v_num_regs = 37;
      d10v_register_name = d10v_ts2_register_name;
      tdep->a0_regnum = TS2_A0_REGNUM;
      tdep->nr_dmap_regs = TS2_NR_DMAP_REGS;
      tdep->register_sim_regno = d10v_ts2_register_sim_regno;
      tdep->dmap_register = d10v_ts2_dmap_register;
      tdep->imap_register = d10v_ts2_imap_register;
      break;
    default:
    case bfd_mach_d10v_ts3:
      d10v_num_regs = 42;
      d10v_register_name = d10v_ts3_register_name;
      tdep->a0_regnum = TS3_A0_REGNUM;
      tdep->nr_dmap_regs = TS3_NR_DMAP_REGS;
      tdep->register_sim_regno = d10v_ts3_register_sim_regno;
      tdep->dmap_register = d10v_ts3_dmap_register;
      tdep->imap_register = d10v_ts3_imap_register;
      break;
    }

  set_gdbarch_read_pc (gdbarch, d10v_read_pc);
  set_gdbarch_write_pc (gdbarch, d10v_write_pc);
  set_gdbarch_read_fp (gdbarch, d10v_read_fp);
  set_gdbarch_write_fp (gdbarch, d10v_write_fp);
  set_gdbarch_read_sp (gdbarch, d10v_read_sp);
  set_gdbarch_write_sp (gdbarch, d10v_write_sp);

  set_gdbarch_num_regs (gdbarch, d10v_num_regs);
  set_gdbarch_sp_regnum (gdbarch, 15);
  set_gdbarch_fp_regnum (gdbarch, 11);
  set_gdbarch_pc_regnum (gdbarch, 18);
  set_gdbarch_register_name (gdbarch, d10v_register_name);
  set_gdbarch_register_size (gdbarch, 2);
  set_gdbarch_register_bytes (gdbarch, (d10v_num_regs - 2) * 2 + 16);
  set_gdbarch_register_byte (gdbarch, d10v_register_byte);
  set_gdbarch_register_raw_size (gdbarch, d10v_register_raw_size);
  set_gdbarch_max_register_raw_size (gdbarch, 8);
  set_gdbarch_register_virtual_size (gdbarch, d10v_register_virtual_size);
  set_gdbarch_max_register_virtual_size (gdbarch, 8);
  set_gdbarch_register_virtual_type (gdbarch, d10v_register_virtual_type);

  set_gdbarch_ptr_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_short_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_int_bit (gdbarch, 2 * TARGET_CHAR_BIT);
  set_gdbarch_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_long_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_float_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_double_bit (gdbarch, 4 * TARGET_CHAR_BIT);
  set_gdbarch_long_double_bit (gdbarch, 8 * TARGET_CHAR_BIT);

  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, entry_point_address);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, generic_pc_in_call_dummy);
  set_gdbarch_call_dummy_words (gdbarch, d10v_call_dummy_words);
  set_gdbarch_sizeof_call_dummy_words (gdbarch, sizeof (d10v_call_dummy_words));
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_get_saved_register (gdbarch, generic_get_saved_register);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);

  set_gdbarch_register_convertible (gdbarch, d10v_register_convertible);
  set_gdbarch_register_convert_to_virtual (gdbarch, d10v_register_convert_to_virtual);
  set_gdbarch_register_convert_to_raw (gdbarch, d10v_register_convert_to_raw);

  set_gdbarch_extract_return_value (gdbarch, d10v_extract_return_value);
  set_gdbarch_push_arguments (gdbarch, d10v_push_arguments);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_return_address (gdbarch, d10v_push_return_address);

  set_gdbarch_d10v_make_daddr (gdbarch, d10v_make_daddr);
  set_gdbarch_d10v_make_iaddr (gdbarch, d10v_make_iaddr);
  set_gdbarch_d10v_daddr_p (gdbarch, d10v_daddr_p);
  set_gdbarch_d10v_iaddr_p (gdbarch, d10v_iaddr_p);
  set_gdbarch_d10v_convert_daddr_to_raw (gdbarch, d10v_convert_daddr_to_raw);
  set_gdbarch_d10v_convert_iaddr_to_raw (gdbarch, d10v_convert_iaddr_to_raw);

  set_gdbarch_store_struct_return (gdbarch, d10v_store_struct_return);
  set_gdbarch_store_return_value (gdbarch, d10v_store_return_value);
  set_gdbarch_extract_struct_value_address (gdbarch, d10v_extract_struct_value_address);
  set_gdbarch_use_struct_convention (gdbarch, d10v_use_struct_convention);

  set_gdbarch_frame_init_saved_regs (gdbarch, d10v_frame_init_saved_regs);
  set_gdbarch_init_extra_frame_info (gdbarch, d10v_init_extra_frame_info);

  set_gdbarch_pop_frame (gdbarch, d10v_pop_frame);

  set_gdbarch_skip_prologue (gdbarch, d10v_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 4);
  set_gdbarch_function_start_offset (gdbarch, 0);
  set_gdbarch_breakpoint_from_pc (gdbarch, d10v_breakpoint_from_pc);

  set_gdbarch_remote_translate_xfer_address (gdbarch, remote_d10v_translate_xfer_address);

  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_frameless_function_invocation (gdbarch, frameless_look_for_prologue);
  set_gdbarch_frame_chain (gdbarch, d10v_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, d10v_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, d10v_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, d10v_frame_args_address);
  set_gdbarch_frame_locals_address (gdbarch, d10v_frame_locals_address);
  set_gdbarch_saved_pc_after_call (gdbarch, d10v_saved_pc_after_call);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);

  return gdbarch;
}


extern void (*target_resume_hook) PARAMS ((void));
extern void (*target_wait_loop_hook) PARAMS ((void));

void
_initialize_d10v_tdep ()
{
  register_gdbarch_init (bfd_arch_d10v, d10v_gdbarch_init);

  tm_print_insn = print_insn_d10v;

  target_resume_hook = d10v_eva_prepare_to_trace;
  target_wait_loop_hook = d10v_eva_get_trace_data;

  add_com ("regs", class_vars, show_regs, "Print all registers");

  add_com ("itrace", class_support, trace_command,
	   "Enable tracing of instruction execution.");

  add_com ("iuntrace", class_support, untrace_command,
	   "Disable tracing of instruction execution.");

  add_com ("itdisassemble", class_vars, tdisassemble_command,
	   "Disassemble the trace buffer.\n\
Two optional arguments specify a range of trace buffer entries\n\
as reported by info trace (NOT addresses!).");

  add_info ("itrace", trace_info,
	    "Display info about the trace data buffer.");

  add_show_from_set (add_set_cmd ("itracedisplay", no_class,
				  var_integer, (char *) &trace_display,
			     "Set automatic display of trace.\n", &setlist),
		     &showlist);
  add_show_from_set (add_set_cmd ("itracesource", no_class,
			   var_integer, (char *) &default_trace_show_source,
		      "Set display of source code with trace.\n", &setlist),
		     &showlist);

}
