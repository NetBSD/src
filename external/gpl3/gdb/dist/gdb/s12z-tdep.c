/* Target-dependent code for the S12Z, for the GDB.
   Copyright (C) 2018-2019 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Much of this file is shamelessly copied from or1k-tdep.c and others.  */

#include "defs.h"

#include "arch-utils.h"
#include "dwarf2-frame.h"
#include "common/errors.h"
#include "frame-unwind.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "inferior.h"
#include "opcode/s12z.h"
#include "trad-frame.h"
#include "remote.h"

/* Two of the registers included in S12Z_N_REGISTERS are
   the CCH and CCL "registers" which are just views into
   the CCW register.  */
#define N_PHYSICAL_REGISTERS (S12Z_N_REGISTERS - 2)


/*  A permutation of all the physical registers.   Indexing this array
    with an integer from gdb's internal representation will return the
    register enum.  */
static const int reg_perm[N_PHYSICAL_REGISTERS] =
  {
   REG_D0,
   REG_D1,
   REG_D2,
   REG_D3,
   REG_D4,
   REG_D5,
   REG_D6,
   REG_D7,
   REG_X,
   REG_Y,
   REG_S,
   REG_P,
   REG_CCW
  };

/*  The inverse of the above permutation.  Indexing this
    array with a register enum (e.g. REG_D2) will return the register
    number in gdb's internal representation.  */
static const int inv_reg_perm[N_PHYSICAL_REGISTERS] =
  {
   2, 3, 4, 5,      /* d2, d3, d4, d5 */
   0, 1,            /* d0, d1 */
   6, 7,            /* d6, d7 */
   8, 9, 10, 11, 12 /* x, y, s, p, ccw */
  };

/*  Return the name of the register REGNUM.  */
static const char *
s12z_register_name (struct gdbarch *gdbarch, int regnum)
{
  /*  Registers is declared in opcodes/s12z.h.   */
  return registers[reg_perm[regnum]].name;
}

static CORE_ADDR
s12z_skip_prologue (struct gdbarch *gdbarch, CORE_ADDR pc)
{
  CORE_ADDR start_pc = 0;

  if (find_pc_partial_function (pc, NULL, &start_pc, NULL))
    {
      CORE_ADDR prologue_end = skip_prologue_using_sal (gdbarch, pc);

      if (prologue_end != 0)
        return prologue_end;
    }

  warning (_("%s Failed to find end of prologue PC = %08x\n"),
                      __FUNCTION__, (unsigned int) pc);

  return pc;
}

/* Implement the unwind_pc gdbarch method.  */
static CORE_ADDR
s12z_unwind_pc (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, REG_P);
}

/* Implement the unwind_sp gdbarch method.  */
static CORE_ADDR
s12z_unwind_sp (struct gdbarch *gdbarch, struct frame_info *next_frame)
{
  return frame_unwind_register_unsigned (next_frame, REG_S);
}

static struct type *
s12z_register_type (struct gdbarch *gdbarch, int reg_nr)
{
  switch (registers[reg_perm[reg_nr]].bytes)
    {
    case 1:
      return builtin_type (gdbarch)->builtin_uint8;
    case 2:
      return builtin_type (gdbarch)->builtin_uint16;
    case 3:
      return builtin_type (gdbarch)->builtin_uint24;
    case 4:
      return builtin_type (gdbarch)->builtin_uint32;
    default:
      return builtin_type (gdbarch)->builtin_uint32;
    }
  return builtin_type (gdbarch)->builtin_int0;
}


static int
s12z_dwarf_reg_to_regnum (struct gdbarch *gdbarch, int num)
{
  switch (num)
    {
    case 15:      return REG_S;
    case 7:       return REG_X;
    case 8:       return REG_Y;
    case 42:      return REG_D0;
    case 43:      return REG_D1;
    case 44:      return REG_D2;
    case 45:      return REG_D3;
    case 46:      return REG_D4;
    case 47:      return REG_D5;
    case 48:      return REG_D6;
    case 49:      return REG_D7;
    }
  return -1;
}


/* Support functions for frame handling.  */

/* Copy of gdb_buffered_insn_length_fprintf from disasm.c.  */

static int ATTRIBUTE_PRINTF (2, 3)
s12z_fprintf_disasm (void *stream, const char *format, ...)
{
  return 0;
}

struct disassemble_info
s12z_disassemble_info (struct gdbarch *gdbarch)
{
  struct disassemble_info di;
  init_disassemble_info (&di, &null_stream, s12z_fprintf_disasm);
  di.arch = gdbarch_bfd_arch_info (gdbarch)->arch;
  di.mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  di.endian = gdbarch_byte_order (gdbarch);
  di.read_memory_func = [](bfd_vma memaddr, gdb_byte *myaddr,
			   unsigned int len, struct disassemble_info *info)
    {
      return target_read_code (memaddr, myaddr, len);
    };
  return di;
}

/* Initialize a prologue cache.  */

static struct trad_frame_cache *
s12z_frame_cache (struct frame_info *this_frame, void **prologue_cache)
{
  struct trad_frame_cache *info;

  CORE_ADDR this_sp;
  CORE_ADDR this_sp_for_id;

  CORE_ADDR start_addr;
  CORE_ADDR end_addr;

  /* Nothing to do if we already have this info.  */
  if (NULL != *prologue_cache)
    return (struct trad_frame_cache *) *prologue_cache;

  /* Get a new prologue cache and populate it with default values.  */
  info = trad_frame_cache_zalloc (this_frame);
  *prologue_cache = info;

  /* Find the start address of this function (which is a normal frame, even
     if the next frame is the sentinel frame) and the end of its prologue.  */
  CORE_ADDR this_pc = get_frame_pc (this_frame);
  struct gdbarch *gdbarch = get_frame_arch (this_frame);
  find_pc_partial_function (this_pc, NULL, &start_addr, NULL);

  /* Get the stack pointer if we have one (if there's no process executing
     yet we won't have a frame.  */
  this_sp = (NULL == this_frame) ? 0 :
    get_frame_register_unsigned (this_frame, REG_S);

  /* Return early if GDB couldn't find the function.  */
  if (start_addr == 0)
    {
      warning (_("Couldn't find function including address %s SP is %s\n"),
               paddress (gdbarch, this_pc),
               paddress (gdbarch, this_sp));

      /* JPB: 28-Apr-11.  This is a temporary patch, to get round GDB
	 crashing right at the beginning.  Build the frame ID as best we
	 can.  */
      trad_frame_set_id (info, frame_id_build (this_sp, this_pc));

      return info;
    }

  /* The default frame base of this frame (for ID purposes only - frame
     base is an overloaded term) is its stack pointer.  For now we use the
     value of the SP register in this frame.  However if the PC is in the
     prologue of this frame, before the SP has been set up, then the value
     will actually be that of the prev frame, and we'll need to adjust it
     later.  */
  trad_frame_set_this_base (info, this_sp);
  this_sp_for_id = this_sp;

  /* We should only examine code that is in the prologue.  This is all code
     up to (but not including) end_addr.  We should only populate the cache
     while the address is up to (but not including) the PC or end_addr,
     whichever is first.  */
  end_addr = s12z_skip_prologue (gdbarch, start_addr);

  /* All the following analysis only occurs if we are in the prologue and
     have executed the code.  Check we have a sane prologue size, and if
     zero we are frameless and can give up here.  */
  if (end_addr < start_addr)
    error (_("end addr %s is less than start addr %s"),
	   paddress (gdbarch, end_addr), paddress (gdbarch, start_addr));

  CORE_ADDR addr = start_addr; /* Where we have got to?  */
  int frame_size = 0;
  int saved_frame_size = 0;
  while (this_pc > addr)
    {
      struct disassemble_info di = s12z_disassemble_info (gdbarch);

      /* No instruction can be more than 11 bytes long, I think.  */
      gdb_byte buf[11];

      int nb = print_insn_s12z (addr, &di);
      gdb_assert (nb <= 11);

      if (0 != target_read_code (addr, buf, nb))
        memory_error (TARGET_XFER_E_IO, addr);

      if (buf[0] == 0x05)        /* RTS */
        {
          frame_size = saved_frame_size;
        }
      /* Conditional Branches.   If any of these are encountered, then
         it is likely that a RTS will terminate it.  So we need to save
         the frame size so it can be restored.  */
      else if ( (buf[0] == 0x02)      /* BRSET */
                || (buf[0] == 0x0B)   /* DBcc / TBcc */
                || (buf[0] == 0x03))  /* BRCLR */
        {
          saved_frame_size = frame_size;
        }
      else if (buf[0] == 0x04)        /* PUL/ PSH .. */
        {
          bool pull = buf[1] & 0x80;
          int stack_adjustment = 0;
          if (buf[1] & 0x40)
            {
              if (buf[1] & 0x01) stack_adjustment += 3;  /* Y */
              if (buf[1] & 0x02) stack_adjustment += 3;  /* X */
              if (buf[1] & 0x04) stack_adjustment += 4;  /* D7 */
              if (buf[1] & 0x08) stack_adjustment += 4;  /* D6 */
              if (buf[1] & 0x10) stack_adjustment += 2;  /* D5 */
              if (buf[1] & 0x20) stack_adjustment += 2;  /* D4 */
            }
          else
            {
              if (buf[1] & 0x01) stack_adjustment += 2;  /* D3 */
              if (buf[1] & 0x02) stack_adjustment += 2;  /* D2 */
              if (buf[1] & 0x04) stack_adjustment += 1;  /* D1 */
              if (buf[1] & 0x08) stack_adjustment += 1;  /* D0 */
              if (buf[1] & 0x10) stack_adjustment += 1;  /* CCL */
              if (buf[1] & 0x20) stack_adjustment += 1;  /* CCH */
            }

          if (!pull)
            stack_adjustment = -stack_adjustment;
          frame_size -= stack_adjustment;
        }
      else if (buf[0] == 0x0a)   /* LEA S, (xxx, S) */
        {
          if (0x06 == (buf[1] >> 4))
            {
              int simm = (signed char) (buf[1] & 0x0F);
              frame_size -= simm;
            }
        }
      else if (buf[0] == 0x1a)   /* LEA S, (S, xxxx) */
        {
	  int simm = (signed char) buf[1];
	  frame_size -= simm;
        }
      addr += nb;
    }

  /* If the PC has not actually got to this point, then the frame
     base will be wrong, and we adjust it. */
  if (this_pc < addr)
    {
      /* Only do if executing.  */
      if (0 != this_sp)
        {
          this_sp_for_id = this_sp - frame_size;
          trad_frame_set_this_base (info, this_sp_for_id);
        }
      trad_frame_set_reg_value (info, REG_S, this_sp + 3);
      trad_frame_set_reg_addr (info, REG_P, this_sp);
    }
  else
    {
      gdb_assert (this_sp == this_sp_for_id);
      /* The stack pointer of the prev frame is frame_size greater
         than the stack pointer of this frame plus one address
         size (caused by the JSR or BSR).  */
      trad_frame_set_reg_value (info, REG_S,
                                this_sp + frame_size + 3);
      trad_frame_set_reg_addr (info, REG_P, this_sp + frame_size);
    }


  /* Build the frame ID.  */
  trad_frame_set_id (info, frame_id_build (this_sp_for_id, start_addr));

  return info;
}

/* Implement the this_id function for the stub unwinder.  */
static void
s12z_frame_this_id (struct frame_info *this_frame,
		    void **prologue_cache, struct frame_id *this_id)
{
  struct trad_frame_cache *info = s12z_frame_cache (this_frame,
						    prologue_cache);

  trad_frame_get_id (info, this_id);
}


/* Implement the prev_register function for the stub unwinder.  */
static struct value *
s12z_frame_prev_register (struct frame_info *this_frame,
			  void **prologue_cache, int regnum)
{
  struct trad_frame_cache *info = s12z_frame_cache (this_frame,
						    prologue_cache);

  return trad_frame_get_register (info, this_frame, regnum);
}

/* Data structures for the normal prologue-analysis-based unwinder.  */
static const struct frame_unwind s12z_frame_unwind = {
  NORMAL_FRAME,
  default_frame_unwind_stop_reason,
  s12z_frame_this_id,
  s12z_frame_prev_register,
  NULL,
  default_frame_sniffer,
  NULL,
};


constexpr gdb_byte s12z_break_insn[] = {0x00};

typedef BP_MANIPULATION (s12z_break_insn) s12z_breakpoint;

struct gdbarch_tdep
{
};

/*  A vector of human readable characters representing the
    bits in the CCW register.  Unused bits are represented as '-'.
    Lowest significant bit comes first.  */
static const char ccw_bits[] =
  {
   'C',  /* Carry  */
   'V',  /* Two's Complement Overflow  */
   'Z',  /* Zero  */
   'N',  /* Negative  */
   'I',  /* Interrupt  */
   '-',
   'X',  /* Non-Maskable Interrupt  */
   'S',  /* STOP Disable  */
   '0',  /*  Interrupt priority level */
   '0',  /*  ditto  */
   '0',  /*  ditto  */
   '-',
   '-',
   '-',
   '-',
   'U'   /* User/Supervisor State.  */
  };

/* Print a human readable representation of the CCW register.
   For example: "u----000SX-Inzvc" corresponds to the value
   0xD0.  */
static void
s12z_print_ccw_info (struct gdbarch *gdbarch,
                     struct ui_file *file,
                     struct frame_info *frame,
                     int reg)
{
  struct value *v = value_of_register (reg, frame);
  const char *name = gdbarch_register_name (gdbarch, reg);
  uint32_t ccw = value_as_long (v);
  fputs_filtered (name, file);
  size_t len = strlen (name);
  const int stop_1 = 15;
  const int stop_2 = 17;
  for (int i = 0; i < stop_1 - len; ++i)
    fputc_filtered (' ', file);
  fprintf_filtered (file, "0x%04x", ccw);
  for (int i = 0; i < stop_2 - len; ++i)
    fputc_filtered (' ', file);
  for (int b = 15; b >= 0; --b)
    {
      if (ccw & (0x1u << b))
        {
          if (ccw_bits[b] == 0)
            fputc_filtered ('1', file);
          else
            fputc_filtered (ccw_bits[b], file);
        }
      else
        fputc_filtered (tolower (ccw_bits[b]), file);
    }
  fputc_filtered ('\n', file);
}

static void
s12z_print_registers_info (struct gdbarch *gdbarch,
			    struct ui_file *file,
			    struct frame_info *frame,
			    int regnum, int print_all)
{
  const int numregs = (gdbarch_num_regs (gdbarch)
		       + gdbarch_num_pseudo_regs (gdbarch));

  if (regnum == -1)
    {
      for (int reg = 0; reg < numregs; reg++)
        {
          if (REG_CCW == reg_perm[reg])
            {
              s12z_print_ccw_info (gdbarch, file, frame, reg);
              continue;
            }
          default_print_registers_info (gdbarch, file, frame, reg, print_all);
        }
    }
  else if (REG_CCW == reg_perm[regnum])
    s12z_print_ccw_info (gdbarch, file, frame, regnum);
  else
    default_print_registers_info (gdbarch, file, frame, regnum, print_all);
}




static void
s12z_extract_return_value (struct type *type, struct regcache *regcache,
                              void *valbuf)
{
  int reg = -1;

  switch (TYPE_LENGTH (type))
    {
    case 0:   /* Nothing to do */
      return;

    case 1:
      reg = REG_D0;
      break;

    case 2:
      reg = REG_D2;
      break;

    case 3:
      reg = REG_X;
      break;

    case 4:
      reg = REG_D6;
      break;

    default:
      error (_("bad size for return value"));
      return;
    }

  regcache->cooked_read (inv_reg_perm[reg], (gdb_byte *) valbuf);
}

static enum return_value_convention
s12z_return_value (struct gdbarch *gdbarch, struct value *function,
                   struct type *type, struct regcache *regcache,
                   gdb_byte *readbuf, const gdb_byte *writebuf)
{
  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION
      || TYPE_CODE (type) == TYPE_CODE_ARRAY
      || TYPE_LENGTH (type) > 4)
    return RETURN_VALUE_STRUCT_CONVENTION;

  if (readbuf)
    s12z_extract_return_value (type, regcache, readbuf);

  return RETURN_VALUE_REGISTER_CONVENTION;
}


static void
show_bdccsr_command (const char *args, int from_tty)
{
  struct string_file output;
  target_rcmd ("bdccsr", &output);

  printf_unfiltered ("The current BDCCSR value is %s\n", output.string().c_str());
}

static struct gdbarch *
s12z_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch_tdep *tdep = XNEW (struct gdbarch_tdep);
  struct gdbarch *gdbarch = gdbarch_alloc (&info, tdep);

  add_cmd ("bdccsr", class_support, show_bdccsr_command,
	   _("Show the current value of the microcontroller's BDCCSR."),
           &maintenanceinfolist);

  /* Target data types.  */
  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 16);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 32);
  set_gdbarch_ptr_bit (gdbarch, 24);
  set_gdbarch_addr_bit (gdbarch, 24);
  set_gdbarch_char_signed (gdbarch, 0);

  set_gdbarch_ps_regnum (gdbarch, REG_CCW);
  set_gdbarch_pc_regnum (gdbarch, REG_P);
  set_gdbarch_sp_regnum (gdbarch, REG_S);


  set_gdbarch_print_registers_info (gdbarch, s12z_print_registers_info);

  set_gdbarch_breakpoint_kind_from_pc (gdbarch,
				       s12z_breakpoint::kind_from_pc);
  set_gdbarch_sw_breakpoint_from_kind (gdbarch,
				       s12z_breakpoint::bp_from_kind);

  set_gdbarch_num_regs (gdbarch, N_PHYSICAL_REGISTERS);
  set_gdbarch_register_name (gdbarch, s12z_register_name);
  set_gdbarch_skip_prologue (gdbarch, s12z_skip_prologue);
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, s12z_dwarf_reg_to_regnum);

  set_gdbarch_register_type (gdbarch, s12z_register_type);

  /* Functions to access frame data.  */
  set_gdbarch_unwind_pc (gdbarch, s12z_unwind_pc);
  set_gdbarch_unwind_sp (gdbarch, s12z_unwind_sp);

  frame_unwind_append_unwinder (gdbarch, &s12z_frame_unwind);
  /* Currently, the only known producer for this archtecture, produces buggy
     dwarf CFI.   So don't append a dwarf unwinder until the situation is
     better understood.  */

  set_gdbarch_return_value (gdbarch, s12z_return_value);

  return gdbarch;
}

void
_initialize_s12z_tdep (void)
{
  gdbarch_register (bfd_arch_s12z, s12z_gdbarch_init, NULL);
}
