/* Target-dependent code for Motorola 68HC11 & 68HC12
   Copyright 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
   Contributed by Stephane Carrez, stcarrez@nerim.fr

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#include "defs.h"
#include "frame.h"
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
#include "arch-utils.h"
#include "regcache.h"

#include "target.h"
#include "opcode/m68hc11.h"
#include "elf/m68hc11.h"
#include "elf-bfd.h"

/* Macros for setting and testing a bit in a minimal symbol.
   For 68HC11/68HC12 we have two flags that tell which return
   type the function is using.  This is used for prologue and frame
   analysis to compute correct stack frame layout.
   
   The MSB of the minimal symbol's "info" field is used for this purpose.
   This field is already being used to store the symbol size, so the
   assumption is that the symbol size cannot exceed 2^30.

   MSYMBOL_SET_RTC	Actually sets the "RTC" bit.
   MSYMBOL_SET_RTI	Actually sets the "RTI" bit.
   MSYMBOL_IS_RTC       Tests the "RTC" bit in a minimal symbol.
   MSYMBOL_IS_RTI       Tests the "RTC" bit in a minimal symbol.
   MSYMBOL_SIZE         Returns the size of the minimal symbol,
   			i.e. the "info" field with the "special" bit
   			masked out.  */

#define MSYMBOL_SET_RTC(msym)                                           \
        MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym))	\
					| 0x80000000)

#define MSYMBOL_SET_RTI(msym)                                           \
        MSYMBOL_INFO (msym) = (char *) (((long) MSYMBOL_INFO (msym))	\
					| 0x40000000)

#define MSYMBOL_IS_RTC(msym)				\
	(((long) MSYMBOL_INFO (msym) & 0x80000000) != 0)

#define MSYMBOL_IS_RTI(msym)				\
	(((long) MSYMBOL_INFO (msym) & 0x40000000) != 0)

#define MSYMBOL_SIZE(msym)				\
	((long) MSYMBOL_INFO (msym) & 0x3fffffff)

enum insn_return_kind {
  RETURN_RTS,
  RETURN_RTC,
  RETURN_RTI
};

  
/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define HARD_X_REGNUM 	0
#define HARD_D_REGNUM	1
#define HARD_Y_REGNUM   2
#define HARD_SP_REGNUM 	3
#define HARD_PC_REGNUM 	4

#define HARD_A_REGNUM   5
#define HARD_B_REGNUM   6
#define HARD_CCR_REGNUM 7

/* 68HC12 page number register.
   Note: to keep a compatibility with gcc register naming, we must
   not have to rename FP and other soft registers.  The page register
   is a real hard register and must therefore be counted by NUM_REGS.
   For this it has the same number as Z register (which is not used).  */
#define HARD_PAGE_REGNUM 8
#define M68HC11_LAST_HARD_REG (HARD_PAGE_REGNUM)

/* Z is replaced by X or Y by gcc during machine reorg.
   ??? There is no way to get it and even know whether
   it's in X or Y or in ZS.  */
#define SOFT_Z_REGNUM        8

/* Soft registers.  These registers are special.  There are treated
   like normal hard registers by gcc and gdb (ie, within dwarf2 info).
   They are physically located in memory.  */
#define SOFT_FP_REGNUM       9
#define SOFT_TMP_REGNUM     10
#define SOFT_ZS_REGNUM      11
#define SOFT_XY_REGNUM      12
#define SOFT_UNUSED_REGNUM  13
#define SOFT_D1_REGNUM      14
#define SOFT_D32_REGNUM     (SOFT_D1_REGNUM+31)
#define M68HC11_MAX_SOFT_REGS 32

#define M68HC11_NUM_REGS        (8)
#define M68HC11_NUM_PSEUDO_REGS (M68HC11_MAX_SOFT_REGS+5)
#define M68HC11_ALL_REGS        (M68HC11_NUM_REGS+M68HC11_NUM_PSEUDO_REGS)

#define M68HC11_REG_SIZE    (2)

#define M68HC12_NUM_REGS        (9)
#define M68HC12_NUM_PSEUDO_REGS ((M68HC11_MAX_SOFT_REGS+5)+1-1)
#define M68HC12_HARD_PC_REGNUM  (SOFT_D32_REGNUM+1)

struct insn_sequence;
struct gdbarch_tdep
  {
    /* Stack pointer correction value.  For 68hc11, the stack pointer points
       to the next push location.  An offset of 1 must be applied to obtain
       the address where the last value is saved.  For 68hc12, the stack
       pointer points to the last value pushed.  No offset is necessary.  */
    int stack_correction;

    /* Description of instructions in the prologue.  */
    struct insn_sequence *prologue;

    /* True if the page memory bank register is available
       and must be used.  */
    int use_page_register;

    /* ELF flags for ABI.  */
    int elf_flags;
  };

#define M6811_TDEP gdbarch_tdep (current_gdbarch)
#define STACK_CORRECTION (M6811_TDEP->stack_correction)
#define USE_PAGE_REGISTER (M6811_TDEP->use_page_register)

struct frame_extra_info
{
  CORE_ADDR return_pc;
  int frameless;
  int size;
  enum insn_return_kind return_kind;
};

/* Table of registers for 68HC11.  This includes the hard registers
   and the soft registers used by GCC.  */
static char *
m68hc11_register_names[] =
{
  "x",    "d",    "y",    "sp",   "pc",   "a",    "b",
  "ccr",  "page", "frame","tmp",  "zs",   "xy",   0,
  "d1",   "d2",   "d3",   "d4",   "d5",   "d6",   "d7",
  "d8",   "d9",   "d10",  "d11",  "d12",  "d13",  "d14",
  "d15",  "d16",  "d17",  "d18",  "d19",  "d20",  "d21",
  "d22",  "d23",  "d24",  "d25",  "d26",  "d27",  "d28",
  "d29",  "d30",  "d31",  "d32"
};

struct m68hc11_soft_reg 
{
  const char *name;
  CORE_ADDR   addr;
};

static struct m68hc11_soft_reg soft_regs[M68HC11_ALL_REGS];

#define M68HC11_FP_ADDR soft_regs[SOFT_FP_REGNUM].addr

static int soft_min_addr;
static int soft_max_addr;
static int soft_reg_initialized = 0;

/* Look in the symbol table for the address of a pseudo register
   in memory.  If we don't find it, pretend the register is not used
   and not available.  */
static void
m68hc11_get_register_info (struct m68hc11_soft_reg *reg, const char *name)
{
  struct minimal_symbol *msymbol;

  msymbol = lookup_minimal_symbol (name, NULL, NULL);
  if (msymbol)
    {
      reg->addr = SYMBOL_VALUE_ADDRESS (msymbol);
      reg->name = xstrdup (name);

      /* Keep track of the address range for soft registers.  */
      if (reg->addr < (CORE_ADDR) soft_min_addr)
        soft_min_addr = reg->addr;
      if (reg->addr > (CORE_ADDR) soft_max_addr)
        soft_max_addr = reg->addr;
    }
  else
    {
      reg->name = 0;
      reg->addr = 0;
    }
}

/* Initialize the table of soft register addresses according
   to the symbol table.  */
  static void
m68hc11_initialize_register_info (void)
{
  int i;

  if (soft_reg_initialized)
    return;
  
  soft_min_addr = INT_MAX;
  soft_max_addr = 0;
  for (i = 0; i < M68HC11_ALL_REGS; i++)
    {
      soft_regs[i].name = 0;
    }
  
  m68hc11_get_register_info (&soft_regs[SOFT_FP_REGNUM], "_.frame");
  m68hc11_get_register_info (&soft_regs[SOFT_TMP_REGNUM], "_.tmp");
  m68hc11_get_register_info (&soft_regs[SOFT_ZS_REGNUM], "_.z");
  soft_regs[SOFT_Z_REGNUM] = soft_regs[SOFT_ZS_REGNUM];
  m68hc11_get_register_info (&soft_regs[SOFT_XY_REGNUM], "_.xy");

  for (i = SOFT_D1_REGNUM; i < M68HC11_MAX_SOFT_REGS; i++)
    {
      char buf[10];

      sprintf (buf, "_.d%d", i - SOFT_D1_REGNUM + 1);
      m68hc11_get_register_info (&soft_regs[i], buf);
    }

  if (soft_regs[SOFT_FP_REGNUM].name == 0)
    {
      warning ("No frame soft register found in the symbol table.\n");
      warning ("Stack backtrace will not work.\n");
    }
  soft_reg_initialized = 1;
}

/* Given an address in memory, return the soft register number if
   that address corresponds to a soft register.  Returns -1 if not.  */
static int
m68hc11_which_soft_register (CORE_ADDR addr)
{
  int i;
  
  if (addr < soft_min_addr || addr > soft_max_addr)
    return -1;
  
  for (i = SOFT_FP_REGNUM; i < M68HC11_ALL_REGS; i++)
    {
      if (soft_regs[i].name && soft_regs[i].addr == addr)
        return i;
    }
  return -1;
}

/* Fetch a pseudo register.  The 68hc11 soft registers are treated like
   pseudo registers.  They are located in memory.  Translate the register
   fetch into a memory read.  */
static void
m68hc11_pseudo_register_read (struct gdbarch *gdbarch,
			      struct regcache *regcache,
			      int regno, void *buf)
{
  /* The PC is a pseudo reg only for 68HC12 with the memory bank
     addressing mode.  */
  if (regno == M68HC12_HARD_PC_REGNUM)
    {
      const int regsize = TYPE_LENGTH (builtin_type_uint32);
      CORE_ADDR pc = read_register (HARD_PC_REGNUM);
      int page = read_register (HARD_PAGE_REGNUM);

      if (pc >= 0x8000 && pc < 0xc000)
        {
          pc -= 0x8000;
          pc += (page << 14);
          pc += 0x1000000;
        }
      store_unsigned_integer (buf, regsize, pc);
      return;
    }

  m68hc11_initialize_register_info ();
  
  /* Fetch a soft register: translate into a memory read.  */
  if (soft_regs[regno].name)
    {
      target_read_memory (soft_regs[regno].addr, buf, 2);
    }
  else
    {
      memset (buf, 0, 2);
    }
}

/* Store a pseudo register.  Translate the register store
   into a memory write.  */
static void
m68hc11_pseudo_register_write (struct gdbarch *gdbarch,
			       struct regcache *regcache,
			       int regno, const void *buf)
{
  /* The PC is a pseudo reg only for 68HC12 with the memory bank
     addressing mode.  */
  if (regno == M68HC12_HARD_PC_REGNUM)
    {
      const int regsize = TYPE_LENGTH (builtin_type_uint32);
      char *tmp = alloca (regsize);
      CORE_ADDR pc;

      memcpy (tmp, buf, regsize);
      pc = extract_unsigned_integer (tmp, regsize);
      if (pc >= 0x1000000)
        {
          pc -= 0x1000000;
          write_register (HARD_PAGE_REGNUM, (pc >> 14) & 0x0ff);
          pc &= 0x03fff;
          write_register (HARD_PC_REGNUM, pc + 0x8000);
        }
      else
        write_register (HARD_PC_REGNUM, pc);
      return;
    }
  
  m68hc11_initialize_register_info ();

  /* Store a soft register: translate into a memory write.  */
  if (soft_regs[regno].name)
    {
      const int regsize = 2;
      char *tmp = alloca (regsize);
      memcpy (tmp, buf, regsize);
      target_write_memory (soft_regs[regno].addr, tmp, regsize);
    }
}

static const char *
m68hc11_register_name (int reg_nr)
{
  if (reg_nr == M68HC12_HARD_PC_REGNUM && USE_PAGE_REGISTER)
    return "pc";
  if (reg_nr == HARD_PC_REGNUM && USE_PAGE_REGISTER)
    return "ppc";
  
  if (reg_nr < 0)
    return NULL;
  if (reg_nr >= M68HC11_ALL_REGS)
    return NULL;

  /* If we don't know the address of a soft register, pretend it
     does not exist.  */
  if (reg_nr > M68HC11_LAST_HARD_REG && soft_regs[reg_nr].name == 0)
    return NULL;
  return m68hc11_register_names[reg_nr];
}

static const unsigned char *
m68hc11_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenptr)
{
  static unsigned char breakpoint[] = {0x0};
  
  *lenptr = sizeof (breakpoint);
  return breakpoint;
}

/* Immediately after a function call, return the saved pc before the frame
   is setup.  */

static CORE_ADDR
m68hc11_saved_pc_after_call (struct frame_info *frame)
{
  CORE_ADDR addr;
  
  addr = read_register (HARD_SP_REGNUM) + STACK_CORRECTION;
  addr &= 0x0ffff;
  return read_memory_integer (addr, 2) & 0x0FFFF;
}

static CORE_ADDR
m68hc11_frame_saved_pc (struct frame_info *frame)
{
  return frame->extra_info->return_pc;
}

static CORE_ADDR
m68hc11_frame_args_address (struct frame_info *frame)
{
  CORE_ADDR addr;

  addr = frame->frame + frame->extra_info->size + STACK_CORRECTION + 2;
  if (frame->extra_info->return_kind == RETURN_RTC)
    addr += 1;
  else if (frame->extra_info->return_kind == RETURN_RTI)
    addr += 7;

  return addr;
}

static CORE_ADDR
m68hc11_frame_locals_address (struct frame_info *frame)
{
  return frame->frame;
}

/* Discard from the stack the innermost frame, restoring all saved
   registers.  */

static void
m68hc11_pop_frame (void)
{
  register struct frame_info *frame = get_current_frame ();
  register CORE_ADDR fp, sp;
  register int regnum;

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    generic_pop_dummy_frame ();
  else
    {
      fp = FRAME_FP (frame);
      FRAME_INIT_SAVED_REGS (frame);

      /* Copy regs from where they were saved in the frame.  */
      for (regnum = 0; regnum < M68HC11_ALL_REGS; regnum++)
	if (frame->saved_regs[regnum])
	  write_register (regnum,
                          read_memory_integer (frame->saved_regs[regnum], 2));

      write_register (HARD_PC_REGNUM, frame->extra_info->return_pc);
      sp = (fp + frame->extra_info->size + 2) & 0x0ffff;
      write_register (HARD_SP_REGNUM, sp);
    }
  flush_cached_frames ();
}


/* 68HC11 & 68HC12 prologue analysis.

 */
#define MAX_CODES 12

/* 68HC11 opcodes.  */
#undef M6811_OP_PAGE2
#define M6811_OP_PAGE2 (0x18)
#define M6811_OP_LDX   (0xde)
#define M6811_OP_PSHX  (0x3c)
#define M6811_OP_STS   (0x9f)
#define M6811_OP_TSX   (0x30)
#define M6811_OP_XGDX  (0x8f)
#define M6811_OP_ADDD  (0xc3)
#define M6811_OP_TXS   (0x35)
#define M6811_OP_DES   (0x34)

/* 68HC12 opcodes.  */
#define M6812_OP_PAGE2 (0x18)
#define M6812_OP_MOVW  (0x01)
#define M6812_PB_PSHW  (0xae)
#define M6812_OP_STS   (0x7f)
#define M6812_OP_LEAS  (0x1b)
#define M6812_OP_PSHX  (0x34)
#define M6812_OP_PSHY  (0x35)

/* Operand extraction.  */
#define OP_DIRECT      (0x100) /* 8-byte direct addressing.  */
#define OP_IMM_LOW     (0x200) /* Low part of 16-bit constant/address.  */
#define OP_IMM_HIGH    (0x300) /* High part of 16-bit constant/address.  */
#define OP_PBYTE       (0x400) /* 68HC12 indexed operand.  */

/* Identification of the sequence.  */
enum m6811_seq_type
{
  P_LAST = 0,
  P_SAVE_REG,  /* Save a register on the stack.  */
  P_SET_FRAME, /* Setup the frame pointer.  */
  P_LOCAL_1,   /* Allocate 1 byte for locals.  */
  P_LOCAL_2,   /* Allocate 2 bytes for locals.  */
  P_LOCAL_N    /* Allocate N bytes for locals.  */
};

struct insn_sequence {
  enum m6811_seq_type type;
  unsigned length;
  unsigned short code[MAX_CODES];
};

/* Sequence of instructions in the 68HC11 function prologue.  */
static struct insn_sequence m6811_prologue[] = {
  /* Sequences to save a soft-register.  */
  { P_SAVE_REG, 3, { M6811_OP_LDX, OP_DIRECT,
                     M6811_OP_PSHX } },
  { P_SAVE_REG, 5, { M6811_OP_PAGE2, M6811_OP_LDX, OP_DIRECT,
                     M6811_OP_PAGE2, M6811_OP_PSHX } },

  /* Sequences to allocate local variables.  */
  { P_LOCAL_N,  7, { M6811_OP_TSX,
                     M6811_OP_XGDX,
                     M6811_OP_ADDD, OP_IMM_HIGH, OP_IMM_LOW,
                     M6811_OP_XGDX,
                     M6811_OP_TXS } },
  { P_LOCAL_N, 11, { M6811_OP_PAGE2, M6811_OP_TSX,
                     M6811_OP_PAGE2, M6811_OP_XGDX,
                     M6811_OP_ADDD, OP_IMM_HIGH, OP_IMM_LOW,
                     M6811_OP_PAGE2, M6811_OP_XGDX,
                     M6811_OP_PAGE2, M6811_OP_TXS } },
  { P_LOCAL_1,  1, { M6811_OP_DES } },
  { P_LOCAL_2,  1, { M6811_OP_PSHX } },
  { P_LOCAL_2,  2, { M6811_OP_PAGE2, M6811_OP_PSHX } },

  /* Initialize the frame pointer.  */
  { P_SET_FRAME, 2, { M6811_OP_STS, OP_DIRECT } },
  { P_LAST, 0, { 0 } }
};


/* Sequence of instructions in the 68HC12 function prologue.  */
static struct insn_sequence m6812_prologue[] = {  
  { P_SAVE_REG,  5, { M6812_OP_PAGE2, M6812_OP_MOVW, M6812_PB_PSHW,
                      OP_IMM_HIGH, OP_IMM_LOW } },
  { P_SET_FRAME, 3, { M6812_OP_STS, OP_IMM_HIGH, OP_IMM_LOW } },
  { P_LOCAL_N,   2, { M6812_OP_LEAS, OP_PBYTE } },
  { P_LOCAL_2,   1, { M6812_OP_PSHX } },
  { P_LOCAL_2,   1, { M6812_OP_PSHY } },
  { P_LAST, 0 }
};


/* Analyze the sequence of instructions starting at the given address.
   Returns a pointer to the sequence when it is recognized and
   the optional value (constant/address) associated with it.
   Advance the pc for the next sequence.  */
static struct insn_sequence *
m68hc11_analyze_instruction (struct insn_sequence *seq, CORE_ADDR *pc,
                             CORE_ADDR *val)
{
  unsigned char buffer[MAX_CODES];
  unsigned bufsize;
  unsigned j;
  CORE_ADDR cur_val;
  short v = 0;

  bufsize = 0;
  for (; seq->type != P_LAST; seq++)
    {
      cur_val = 0;
      for (j = 0; j < seq->length; j++)
        {
          if (bufsize < j + 1)
            {
              buffer[bufsize] = read_memory_unsigned_integer (*pc + bufsize,
                                                              1);
              bufsize++;
            }
          /* Continue while we match the opcode.  */
          if (seq->code[j] == buffer[j])
            continue;
          
          if ((seq->code[j] & 0xf00) == 0)
            break;
          
          /* Extract a sequence parameter (address or constant).  */
          switch (seq->code[j])
            {
            case OP_DIRECT:
              cur_val = (CORE_ADDR) buffer[j];
              break;

            case OP_IMM_HIGH:
              cur_val = cur_val & 0x0ff;
              cur_val |= (buffer[j] << 8);
              break;

            case OP_IMM_LOW:
              cur_val &= 0x0ff00;
              cur_val |= buffer[j];
              break;

            case OP_PBYTE:
              if ((buffer[j] & 0xE0) == 0x80)
                {
                  v = buffer[j] & 0x1f;
                  if (v & 0x10)
                    v |= 0xfff0;
                }
              else if ((buffer[j] & 0xfe) == 0xf0)
                {
                  v = read_memory_unsigned_integer (*pc + j + 1, 1);
                  if (buffer[j] & 1)
                    v |= 0xff00;
                  *pc = *pc + 1;
                }
              else if (buffer[j] == 0xf2)
                {
                  v = read_memory_unsigned_integer (*pc + j + 1, 2);
                  *pc = *pc + 2;
                }
              cur_val = v;
              break;
            }
        }

      /* We have a full match.  */
      if (j == seq->length)
        {
          *val = cur_val;
          *pc = *pc + j;
          return seq;
        }
    }
  return 0;
}

/* Return the instruction that the function at the PC is using.  */
static enum insn_return_kind
m68hc11_get_return_insn (CORE_ADDR pc)
{
  struct minimal_symbol *sym;

  /* A flag indicating that this is a STO_M68HC12_FAR or STO_M68HC12_INTERRUPT
     function is stored by elfread.c in the high bit of the info field.
     Use this to decide which instruction the function uses to return.  */
  sym = lookup_minimal_symbol_by_pc (pc);
  if (sym == 0)
    return RETURN_RTS;

  if (MSYMBOL_IS_RTC (sym))
    return RETURN_RTC;
  else if (MSYMBOL_IS_RTI (sym))
    return RETURN_RTI;
  else
    return RETURN_RTS;
}


/* Analyze the function prologue to find some information
   about the function:
    - the PC of the first line (for m68hc11_skip_prologue)
    - the offset of the previous frame saved address (from current frame)
    - the soft registers which are pushed.  */
static void
m68hc11_guess_from_prologue (CORE_ADDR pc, CORE_ADDR fp,
                             CORE_ADDR *first_line,
                             int *frame_offset, CORE_ADDR *pushed_regs)
{
  CORE_ADDR save_addr;
  CORE_ADDR func_end;
  int size;
  int found_frame_point;
  int saved_reg;
  CORE_ADDR first_pc;
  int done = 0;
  struct insn_sequence *seq_table;
  
  first_pc = get_pc_function_start (pc);
  size = 0;

  m68hc11_initialize_register_info ();
  if (first_pc == 0)
    {
      *frame_offset = 0;
      *first_line   = pc;
      return;
    }

  seq_table = gdbarch_tdep (current_gdbarch)->prologue;
  
  /* The 68hc11 stack is as follows:


     |           |
     +-----------+
     |           |
     | args      |
     |           |
     +-----------+
     | PC-return |
     +-----------+
     | Old frame |
     +-----------+
     |           |
     | Locals    |
     |           |
     +-----------+ <--- current frame
     |           |

     With most processors (like 68K) the previous frame can be computed
     easily because it is always at a fixed offset (see link/unlink).
     That is, locals are accessed with negative offsets, arguments are
     accessed with positive ones.  Since 68hc11 only supports offsets
     in the range [0..255], the frame is defined at the bottom of
     locals (see picture).

     The purpose of the analysis made here is to find out the size
     of locals in this function.  An alternative to this is to use
     DWARF2 info.  This would be better but I don't know how to
     access dwarf2 debug from this function.
     
     Walk from the function entry point to the point where we save
     the frame.  While walking instructions, compute the size of bytes
     which are pushed.  This gives us the index to access the previous
     frame.

     We limit the search to 128 bytes so that the algorithm is bounded
     in case of random and wrong code.  We also stop and abort if
     we find an instruction which is not supposed to appear in the
     prologue (as generated by gcc 2.95, 2.96).
  */
  pc = first_pc;
  func_end = pc + 128;
  found_frame_point = 0;
  *frame_offset = 0;
  save_addr = fp + STACK_CORRECTION;
  while (!done && pc + 2 < func_end)
    {
      struct insn_sequence *seq;
      CORE_ADDR val;
      
      seq = m68hc11_analyze_instruction (seq_table, &pc, &val);
      if (seq == 0)
        break;

      if (seq->type == P_SAVE_REG)
        {
          if (found_frame_point)
            {
              saved_reg = m68hc11_which_soft_register (val);
              if (saved_reg < 0)
                break;

              save_addr -= 2;
              if (pushed_regs)
                pushed_regs[saved_reg] = save_addr;
            }
          else
            {
              size += 2;
            }
        }
      else if (seq->type == P_SET_FRAME)
        {
          found_frame_point = 1;
          *frame_offset = size;
        }
      else if (seq->type == P_LOCAL_1)
        {
          size += 1;
        }
      else if (seq->type == P_LOCAL_2)
        {
          size += 2;
        }
      else if (seq->type == P_LOCAL_N)
        {
          /* Stack pointer is decremented for the allocation.  */
          if (val & 0x8000)
            size -= (int) (val) | 0xffff0000;
          else
            size -= val;
        }
    }
  *first_line  = pc;
}

static CORE_ADDR
m68hc11_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end;
  struct symtab_and_line sal;
  int frame_offset;

  /* If we have line debugging information, then the end of the
     prologue should be the first assembly instruction of the
     first source line.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      sal = find_pc_line (func_addr, 0);
      if (sal.end && sal.end < func_end)
	return sal.end;
    }

  m68hc11_guess_from_prologue (pc, 0, &pc, &frame_offset, 0);
  return pc;
}

/* Given a GDB frame, determine the address of the calling function's frame.
   This will be used to create a new GDB frame struct, and then
   INIT_EXTRA_FRAME_INFO and INIT_FRAME_PC will be called for the new frame.
*/

static CORE_ADDR
m68hc11_frame_chain (struct frame_info *frame)
{
  CORE_ADDR addr;

  if (PC_IN_CALL_DUMMY (frame->pc, frame->frame, frame->frame))
    return frame->frame;	/* dummy frame same as caller's frame */

  if (frame->extra_info->return_pc == 0
      || inside_entry_file (frame->extra_info->return_pc))
    return (CORE_ADDR) 0;

  if (frame->frame == 0)
    {
      return (CORE_ADDR) 0;
    }

  addr = frame->frame + frame->extra_info->size + STACK_CORRECTION - 2;
  addr = read_memory_unsigned_integer (addr, 2) & 0x0FFFF;
  return addr;
}  

/* Put here the code to store, into a struct frame_saved_regs, the
   addresses of the saved registers of frame described by FRAME_INFO.
   This includes special registers such as pc and fp saved in special
   ways in the stack frame.   sp is even more special: the address we
   return for it IS the sp for the next frame.  */
static void
m68hc11_frame_init_saved_regs (struct frame_info *fi)
{
  CORE_ADDR pc;
  CORE_ADDR addr;

  if (fi->saved_regs == NULL)
    frame_saved_regs_zalloc (fi);
  else
    memset (fi->saved_regs, 0, sizeof (fi->saved_regs));

  pc = fi->pc;
  fi->extra_info->return_kind = m68hc11_get_return_insn (pc);
  m68hc11_guess_from_prologue (pc, fi->frame, &pc, &fi->extra_info->size,
                               fi->saved_regs);

  addr = fi->frame + fi->extra_info->size + STACK_CORRECTION;
  if (soft_regs[SOFT_FP_REGNUM].name)
    fi->saved_regs[SOFT_FP_REGNUM] = addr - 2;

  /* Take into account how the function was called/returns.  */
  if (fi->extra_info->return_kind == RETURN_RTC)
    {
      fi->saved_regs[HARD_PAGE_REGNUM] = addr;
      addr++;
    }
  else if (fi->extra_info->return_kind == RETURN_RTI)
    {
      fi->saved_regs[HARD_CCR_REGNUM] = addr;
      fi->saved_regs[HARD_D_REGNUM] = addr + 1;
      fi->saved_regs[HARD_X_REGNUM] = addr + 3;
      fi->saved_regs[HARD_Y_REGNUM] = addr + 5;
      addr += 7;
    }
  fi->saved_regs[HARD_SP_REGNUM] = addr;
  fi->saved_regs[HARD_PC_REGNUM] = fi->saved_regs[HARD_SP_REGNUM];
}

static void
m68hc11_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
  CORE_ADDR addr;

  fi->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));
  
  if (fi->next)
    fi->pc = FRAME_SAVED_PC (fi->next);
  
  m68hc11_frame_init_saved_regs (fi);

  if (fromleaf)
    {
      fi->extra_info->return_kind = m68hc11_get_return_insn (fi->pc);
      fi->extra_info->return_pc = m68hc11_saved_pc_after_call (fi);
    }
  else
    {
      addr = fi->saved_regs[HARD_PC_REGNUM];
      addr = read_memory_unsigned_integer (addr, 2) & 0x0ffff;

      /* Take into account the 68HC12 specific call (PC + page).  */
      if (fi->extra_info->return_kind == RETURN_RTC
          && addr >= 0x08000 && addr < 0x0c000
          && USE_PAGE_REGISTER)
        {
          CORE_ADDR page_addr = fi->saved_regs[HARD_PAGE_REGNUM];

          unsigned page = read_memory_unsigned_integer (page_addr, 1);
          addr -= 0x08000;
          addr += ((page & 0x0ff) << 14);
          addr += 0x1000000;
        }
      fi->extra_info->return_pc = addr;
    }
}

/* Same as 'info reg' but prints the registers in a different way.  */
static void
show_regs (char *args, int from_tty)
{
  int ccr = read_register (HARD_CCR_REGNUM);
  int i;
  int nr;
  
  printf_filtered ("PC=%04x SP=%04x FP=%04x CCR=%02x %c%c%c%c%c%c%c%c\n",
		   (int) read_register (HARD_PC_REGNUM),
		   (int) read_register (HARD_SP_REGNUM),
		   (int) read_register (SOFT_FP_REGNUM),
		   ccr,
		   ccr & M6811_S_BIT ? 'S' : '-',
		   ccr & M6811_X_BIT ? 'X' : '-',
		   ccr & M6811_H_BIT ? 'H' : '-',
		   ccr & M6811_I_BIT ? 'I' : '-',
		   ccr & M6811_N_BIT ? 'N' : '-',
		   ccr & M6811_Z_BIT ? 'Z' : '-',
		   ccr & M6811_V_BIT ? 'V' : '-',
		   ccr & M6811_C_BIT ? 'C' : '-');

  printf_filtered ("D=%04x IX=%04x IY=%04x",
		   (int) read_register (HARD_D_REGNUM),
		   (int) read_register (HARD_X_REGNUM),
		   (int) read_register (HARD_Y_REGNUM));

  if (USE_PAGE_REGISTER)
    {
      printf_filtered (" Page=%02x",
                       (int) read_register (HARD_PAGE_REGNUM));
    }
  printf_filtered ("\n");

  nr = 0;
  for (i = SOFT_D1_REGNUM; i < M68HC11_ALL_REGS; i++)
    {
      /* Skip registers which are not defined in the symbol table.  */
      if (soft_regs[i].name == 0)
        continue;
      
      printf_filtered ("D%d=%04x",
                       i - SOFT_D1_REGNUM + 1,
                       (int) read_register (i));
      nr++;
      if ((nr % 8) == 7)
        printf_filtered ("\n");
      else
        printf_filtered (" ");
    }
  if (nr && (nr % 8) != 7)
    printf_filtered ("\n");
}

static CORE_ADDR
m68hc11_stack_align (CORE_ADDR addr)
{
  return ((addr + 1) & -2);
}

static CORE_ADDR
m68hc11_push_arguments (int nargs,
                        struct value **args,
                        CORE_ADDR sp,
                        int struct_return,
                        CORE_ADDR struct_addr)
{
  int stack_alloc;
  int argnum;
  int first_stack_argnum;
  int stack_offset;
  struct type *type;
  char *val;
  int len;
  
  stack_alloc = 0;
  first_stack_argnum = 0;
  if (struct_return)
    {
      /* The struct is allocated on the stack and gdb used the stack
         pointer for the address of that struct.  We must apply the
         stack offset on the address.  */
      write_register (HARD_D_REGNUM, struct_addr + STACK_CORRECTION);
    }
  else if (nargs > 0)
    {
      type = VALUE_TYPE (args[0]);
      len = TYPE_LENGTH (type);
      
      /* First argument is passed in D and X registers.  */
      if (len <= 4)
        {
          LONGEST v = extract_unsigned_integer (VALUE_CONTENTS (args[0]), len);
          first_stack_argnum = 1;
          write_register (HARD_D_REGNUM, v);
          if (len > 2)
            {
              v >>= 16;
              write_register (HARD_X_REGNUM, v);
            }
        }
    }
  for (argnum = first_stack_argnum; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      stack_alloc += (TYPE_LENGTH (type) + 1) & -2;
    }
  sp -= stack_alloc;

  stack_offset = STACK_CORRECTION;
  for (argnum = first_stack_argnum; argnum < nargs; argnum++)
    {
      type = VALUE_TYPE (args[argnum]);
      len = TYPE_LENGTH (type);

      val = (char*) VALUE_CONTENTS (args[argnum]);
      write_memory (sp + stack_offset, val, len);
      stack_offset += len;
      if (len & 1)
        {
          static char zero = 0;

          write_memory (sp + stack_offset, &zero, 1);
          stack_offset++;
        }
    }
  return sp;
}


/* Return a location where we can set a breakpoint that will be hit
   when an inferior function call returns.  */
CORE_ADDR
m68hc11_call_dummy_address (void)
{
  return entry_point_address ();
}

static struct type *
m68hc11_register_virtual_type (int reg_nr)
{
  switch (reg_nr)
    {
    case HARD_PAGE_REGNUM:
    case HARD_A_REGNUM:
    case HARD_B_REGNUM:
    case HARD_CCR_REGNUM:
      return builtin_type_uint8;

    case M68HC12_HARD_PC_REGNUM:
      return builtin_type_uint32;

    default:
      return builtin_type_uint16;
    }
}

static void
m68hc11_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  /* The struct address computed by gdb is on the stack.
     It uses the stack pointer so we must apply the stack
     correction offset.  */
  write_register (HARD_D_REGNUM, addr + STACK_CORRECTION);
}

static void
m68hc11_store_return_value (struct type *type, char *valbuf)
{
  int len;

  len = TYPE_LENGTH (type);

  /* First argument is passed in D and X registers.  */
  if (len <= 4)
    {
      LONGEST v = extract_unsigned_integer (valbuf, len);

      write_register (HARD_D_REGNUM, v);
      if (len > 2)
        {
          v >>= 16;
          write_register (HARD_X_REGNUM, v);
        }
    }
  else
    error ("return of value > 4 is not supported.");
}


/* Given a return value in `regbuf' with a type `type', 
   extract and copy its value into `valbuf'.  */

static void
m68hc11_extract_return_value (struct type *type,
                              char *regbuf,
                              char *valbuf)
{
  int len = TYPE_LENGTH (type);
  
  switch (len)
    {
    case 1:
      memcpy (valbuf, &regbuf[HARD_D_REGNUM * 2 + 1], len);
      break;
  
    case 2:
      memcpy (valbuf, &regbuf[HARD_D_REGNUM * 2], len);
      break;
      
    case 3:
      memcpy (&valbuf[0], &regbuf[HARD_X_REGNUM * 2 + 1], 1);
      memcpy (&valbuf[1], &regbuf[HARD_D_REGNUM * 2], 2);
      break;
      
    case 4:
      memcpy (&valbuf[0], &regbuf[HARD_X_REGNUM * 2], 2);
      memcpy (&valbuf[2], &regbuf[HARD_D_REGNUM * 2], 2);
      break;

    default:
      error ("bad size for return value");
    }
}

/* Should call_function allocate stack space for a struct return?  */
static int
m68hc11_use_struct_convention (int gcc_p, struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
          || TYPE_CODE (type) == TYPE_CODE_UNION
          || TYPE_LENGTH (type) > 4);
}

static int
m68hc11_return_value_on_stack (struct type *type)
{
  return TYPE_LENGTH (type) > 4;
}

/* Extract from an array REGBUF containing the (raw) register state
   the address in which a function should return its structure value,
   as a CORE_ADDR (or an expression that can be used as one).  */
static CORE_ADDR
m68hc11_extract_struct_value_address (char *regbuf)
{
  return extract_address (&regbuf[HARD_D_REGNUM * 2],
                          REGISTER_RAW_SIZE (HARD_D_REGNUM));
}

/* Function: push_return_address (pc)
   Set up the return address for the inferior function call.
   Needed for targets where we don't actually execute a JSR/BSR instruction */

static CORE_ADDR
m68hc11_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  char valbuf[2];
  
  pc = CALL_DUMMY_ADDRESS ();
  sp -= 2;
  store_unsigned_integer (valbuf, 2, pc);
  write_memory (sp + STACK_CORRECTION, valbuf, 2);
  return sp;
}

/* Index within `registers' of the first byte of the space for
   register N.  */
static int
m68hc11_register_byte (int reg_nr)
{
  return (reg_nr * M68HC11_REG_SIZE);
}

static int
m68hc11_register_raw_size (int reg_nr)
{
  switch (reg_nr)
    {
    case HARD_PAGE_REGNUM:
    case HARD_A_REGNUM:
    case HARD_B_REGNUM:
    case HARD_CCR_REGNUM:
      return 1;

    case M68HC12_HARD_PC_REGNUM:
      return 4;

    default:
      return M68HC11_REG_SIZE;
    }
}

/* Test whether the ELF symbol corresponds to a function using rtc or
   rti to return.  */
   
static void
m68hc11_elf_make_msymbol_special (asymbol *sym, struct minimal_symbol *msym)
{
  unsigned char flags;

  flags = ((elf_symbol_type *)sym)->internal_elf_sym.st_other;
  if (flags & STO_M68HC12_FAR)
    MSYMBOL_SET_RTC (msym);
  if (flags & STO_M68HC12_INTERRUPT)
    MSYMBOL_SET_RTI (msym);
}

static int
gdb_print_insn_m68hc11 (bfd_vma memaddr, disassemble_info *info)
{
  if (TARGET_ARCHITECTURE->arch == bfd_arch_m68hc11)
    return print_insn_m68hc11 (memaddr, info);
  else
    return print_insn_m68hc12 (memaddr, info);
}

static struct gdbarch *
m68hc11_gdbarch_init (struct gdbarch_info info,
                      struct gdbarch_list *arches)
{
  static LONGEST m68hc11_call_dummy_words[] =
  {0};
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int elf_flags;

  soft_reg_initialized = 0;

  /* Extract the elf_flags if available.  */
  if (info.abfd != NULL
      && bfd_get_flavour (info.abfd) == bfd_target_elf_flavour)
    elf_flags = elf_elfheader (info.abfd)->e_flags;
  else
    elf_flags = 0;

  /* try to find a pre-existing architecture */
  for (arches = gdbarch_list_lookup_by_info (arches, &info);
       arches != NULL;
       arches = gdbarch_list_lookup_by_info (arches->next, &info))
    {
      if (gdbarch_tdep (arches->gdbarch)->elf_flags != elf_flags)
	continue;

      return arches->gdbarch;
    }

  /* Need a new architecture. Fill in a target specific vector.  */
  tdep = (struct gdbarch_tdep *) xmalloc (sizeof (struct gdbarch_tdep));
  gdbarch = gdbarch_alloc (&info, tdep);
  tdep->elf_flags = elf_flags;

  switch (info.bfd_arch_info->arch)
    {
    case bfd_arch_m68hc11:
      tdep->stack_correction = 1;
      tdep->use_page_register = 0;
      tdep->prologue = m6811_prologue;
      set_gdbarch_addr_bit (gdbarch, 16);
      set_gdbarch_num_pseudo_regs (gdbarch, M68HC11_NUM_PSEUDO_REGS);
      set_gdbarch_pc_regnum (gdbarch, HARD_PC_REGNUM);
      set_gdbarch_num_regs (gdbarch, M68HC11_NUM_REGS);
      break;

    case bfd_arch_m68hc12:
      tdep->stack_correction = 0;
      tdep->use_page_register = elf_flags & E_M68HC12_BANKS;
      tdep->prologue = m6812_prologue;
      set_gdbarch_addr_bit (gdbarch, elf_flags & E_M68HC12_BANKS ? 32 : 16);
      set_gdbarch_num_pseudo_regs (gdbarch,
                                   elf_flags & E_M68HC12_BANKS
                                   ? M68HC12_NUM_PSEUDO_REGS
                                   : M68HC11_NUM_PSEUDO_REGS);
      set_gdbarch_pc_regnum (gdbarch, elf_flags & E_M68HC12_BANKS
                             ? M68HC12_HARD_PC_REGNUM : HARD_PC_REGNUM);
      set_gdbarch_num_regs (gdbarch, elf_flags & E_M68HC12_BANKS
                            ? M68HC12_NUM_REGS : M68HC11_NUM_REGS);
      break;

    default:
      break;
    }

  /* Initially set everything according to the ABI.
     Use 16-bit integers since it will be the case for most
     programs.  The size of these types should normally be set
     according to the dwarf2 debug information.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, elf_flags & E_M68HC11_I32 ? 32 : 16);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, elf_flags & E_M68HC11_F64 ? 64 : 32);
  set_gdbarch_long_double_bit (gdbarch, elf_flags & E_M68HC11_F64 ? 64 : 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_ptr_bit (gdbarch, 16);
  set_gdbarch_long_long_bit (gdbarch, 64);

  /* Set register info.  */
  set_gdbarch_fp0_regnum (gdbarch, -1);
  set_gdbarch_max_register_raw_size (gdbarch, 2);
  set_gdbarch_max_register_virtual_size (gdbarch, 2);
  set_gdbarch_register_raw_size (gdbarch, m68hc11_register_raw_size);
  set_gdbarch_register_virtual_size (gdbarch, m68hc11_register_raw_size);
  set_gdbarch_register_byte (gdbarch, m68hc11_register_byte);
  set_gdbarch_frame_init_saved_regs (gdbarch, m68hc11_frame_init_saved_regs);
  set_gdbarch_frame_args_skip (gdbarch, 0);

  set_gdbarch_read_pc (gdbarch, generic_target_read_pc);
  set_gdbarch_write_pc (gdbarch, generic_target_write_pc);
  set_gdbarch_read_fp (gdbarch, generic_target_read_fp);
  set_gdbarch_read_sp (gdbarch, generic_target_read_sp);
  set_gdbarch_write_sp (gdbarch, generic_target_write_sp);

  set_gdbarch_sp_regnum (gdbarch, HARD_SP_REGNUM);
  set_gdbarch_fp_regnum (gdbarch, SOFT_FP_REGNUM);
  set_gdbarch_register_name (gdbarch, m68hc11_register_name);
  set_gdbarch_register_size (gdbarch, 2);
  set_gdbarch_register_bytes (gdbarch, M68HC11_ALL_REGS * 2);
  set_gdbarch_register_virtual_type (gdbarch, m68hc11_register_virtual_type);
  set_gdbarch_pseudo_register_read (gdbarch, m68hc11_pseudo_register_read);
  set_gdbarch_pseudo_register_write (gdbarch, m68hc11_pseudo_register_write);

  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, m68hc11_call_dummy_address);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1); /*???*/
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, generic_pc_in_call_dummy);
  set_gdbarch_call_dummy_words (gdbarch, m68hc11_call_dummy_words);
  set_gdbarch_sizeof_call_dummy_words (gdbarch,
                                       sizeof (m68hc11_call_dummy_words));
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_get_saved_register (gdbarch, generic_get_saved_register);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);
  set_gdbarch_deprecated_extract_return_value (gdbarch, m68hc11_extract_return_value);
  set_gdbarch_push_arguments (gdbarch, m68hc11_push_arguments);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_return_address (gdbarch, m68hc11_push_return_address);
  set_gdbarch_return_value_on_stack (gdbarch, m68hc11_return_value_on_stack);

  set_gdbarch_store_struct_return (gdbarch, m68hc11_store_struct_return);
  set_gdbarch_deprecated_store_return_value (gdbarch, m68hc11_store_return_value);
  set_gdbarch_deprecated_extract_struct_value_address (gdbarch, m68hc11_extract_struct_value_address);
  set_gdbarch_register_convertible (gdbarch, generic_register_convertible_not);


  set_gdbarch_frame_chain (gdbarch, m68hc11_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, generic_file_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, m68hc11_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, m68hc11_frame_args_address);
  set_gdbarch_frame_locals_address (gdbarch, m68hc11_frame_locals_address);
  set_gdbarch_saved_pc_after_call (gdbarch, m68hc11_saved_pc_after_call);
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);

  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_get_saved_register (gdbarch, generic_get_saved_register);

  set_gdbarch_store_struct_return (gdbarch, m68hc11_store_struct_return);
  set_gdbarch_deprecated_store_return_value (gdbarch, m68hc11_store_return_value);
  set_gdbarch_deprecated_extract_struct_value_address
    (gdbarch, m68hc11_extract_struct_value_address);
  set_gdbarch_use_struct_convention (gdbarch, m68hc11_use_struct_convention);
  set_gdbarch_init_extra_frame_info (gdbarch, m68hc11_init_extra_frame_info);
  set_gdbarch_pop_frame (gdbarch, m68hc11_pop_frame);
  set_gdbarch_skip_prologue (gdbarch, m68hc11_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_decr_pc_after_break (gdbarch, 0);
  set_gdbarch_function_start_offset (gdbarch, 0);
  set_gdbarch_breakpoint_from_pc (gdbarch, m68hc11_breakpoint_from_pc);
  set_gdbarch_stack_align (gdbarch, m68hc11_stack_align);
  set_gdbarch_print_insn (gdbarch, gdb_print_insn_m68hc11);

  /* Minsymbol frobbing.  */
  set_gdbarch_elf_make_msymbol_special (gdbarch,
                                        m68hc11_elf_make_msymbol_special);

  set_gdbarch_believe_pcc_promotion (gdbarch, 1);

  return gdbarch;
}

void
_initialize_m68hc11_tdep (void)
{
  register_gdbarch_init (bfd_arch_m68hc11, m68hc11_gdbarch_init);
  register_gdbarch_init (bfd_arch_m68hc12, m68hc11_gdbarch_init);

  add_com ("regs", class_vars, show_regs, "Print all registers");
} 

