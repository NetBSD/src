/* Target-dependent code for the Fujitsu FR-V, for GDB, the GNU Debugger.
   Copyright 2002 Free Software Foundation, Inc.

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

#include "defs.h"
#include "inferior.h"
#include "symfile.h"		/* for entry_point_address */
#include "gdbcore.h"
#include "arch-utils.h"
#include "regcache.h"

extern void _initialize_frv_tdep (void);

static gdbarch_init_ftype frv_gdbarch_init;

static gdbarch_register_name_ftype frv_register_name;
static gdbarch_register_raw_size_ftype frv_register_raw_size;
static gdbarch_register_virtual_size_ftype frv_register_virtual_size;
static gdbarch_register_virtual_type_ftype frv_register_virtual_type;
static gdbarch_register_byte_ftype frv_register_byte;
static gdbarch_breakpoint_from_pc_ftype frv_breakpoint_from_pc;
static gdbarch_frame_chain_ftype frv_frame_chain;
static gdbarch_frame_saved_pc_ftype frv_frame_saved_pc;
static gdbarch_skip_prologue_ftype frv_skip_prologue;
static gdbarch_frame_init_saved_regs_ftype frv_frame_init_saved_regs;
static gdbarch_deprecated_extract_return_value_ftype frv_extract_return_value;
static gdbarch_deprecated_extract_struct_value_address_ftype frv_extract_struct_value_address;
static gdbarch_use_struct_convention_ftype frv_use_struct_convention;
static gdbarch_frameless_function_invocation_ftype frv_frameless_function_invocation;
static gdbarch_init_extra_frame_info_ftype stupid_useless_init_extra_frame_info;
static gdbarch_store_struct_return_ftype frv_store_struct_return;
static gdbarch_push_arguments_ftype frv_push_arguments;
static gdbarch_push_return_address_ftype frv_push_return_address;
static gdbarch_pop_frame_ftype frv_pop_frame;
static gdbarch_saved_pc_after_call_ftype frv_saved_pc_after_call;

static void frv_pop_frame_regular (struct frame_info *frame);

/* Register numbers.  You can change these as needed, but don't forget
   to update the simulator accordingly.  */
enum {
  /* The total number of registers we know exist.  */
  frv_num_regs = 147,

  /* Register numbers 0 -- 63 are always reserved for general-purpose
     registers.  The chip at hand may have less.  */
  first_gpr_regnum = 0,
  sp_regnum = 1,
  fp_regnum = 2,
  struct_return_regnum = 3,
  last_gpr_regnum = 63,

  /* Register numbers 64 -- 127 are always reserved for floating-point
     registers.  The chip at hand may have less.  */
  first_fpr_regnum = 64,
  last_fpr_regnum = 127,

  /* Register numbers 128 on up are always reserved for special-purpose
     registers.  */
  first_spr_regnum = 128,
  pc_regnum = 128,
  psr_regnum = 129,
  ccr_regnum = 130,
  cccr_regnum = 131,
  tbr_regnum = 135,
  brr_regnum = 136,
  dbar0_regnum = 137,
  dbar1_regnum = 138,
  dbar2_regnum = 139,
  dbar3_regnum = 140,
  lr_regnum = 145,
  lcr_regnum = 146,
  last_spr_regnum = 146
};

static LONGEST frv_call_dummy_words[] =
{0};


/* The contents of this structure can only be trusted after we've
   frv_frame_init_saved_regs on the frame.  */
struct frame_extra_info
  {
    /* The offset from our frame pointer to our caller's stack
       pointer.  */
    int fp_to_callers_sp_offset;

    /* Non-zero if we've saved our return address on the stack yet.
       Zero if it's still sitting in the link register.  */
    int lr_saved_on_stack;
  };


/* A structure describing a particular variant of the FRV.
   We allocate and initialize one of these structures when we create
   the gdbarch object for a variant.

   At the moment, all the FR variants we support differ only in which
   registers are present; the portable code of GDB knows that
   registers whose names are the empty string don't exist, so the
   `register_names' array captures all the per-variant information we
   need.

   in the future, if we need to have per-variant maps for raw size,
   virtual type, etc., we should replace register_names with an array
   of structures, each of which gives all the necessary info for one
   register.  Don't stick parallel arrays in here --- that's so
   Fortran.  */
struct gdbarch_tdep
{
  /* How many general-purpose registers does this variant have?  */
  int num_gprs;

  /* How many floating-point registers does this variant have?  */
  int num_fprs;

  /* How many hardware watchpoints can it support?  */
  int num_hw_watchpoints;

  /* How many hardware breakpoints can it support?  */
  int num_hw_breakpoints;

  /* Register names.  */
  char **register_names;
};

#define CURRENT_VARIANT (gdbarch_tdep (current_gdbarch))


/* Allocate a new variant structure, and set up default values for all
   the fields.  */
static struct gdbarch_tdep *
new_variant (void)
{
  struct gdbarch_tdep *var;
  int r;
  char buf[20];

  var = xmalloc (sizeof (*var));
  memset (var, 0, sizeof (*var));
  
  var->num_gprs = 64;
  var->num_fprs = 64;
  var->num_hw_watchpoints = 0;
  var->num_hw_breakpoints = 0;

  /* By default, don't supply any general-purpose or floating-point
     register names.  */
  var->register_names = (char **) xmalloc (frv_num_regs * sizeof (char *));
  for (r = 0; r < frv_num_regs; r++)
    var->register_names[r] = "";

  /* Do, however, supply default names for the special-purpose
     registers.  */
  for (r = first_spr_regnum; r <= last_spr_regnum; ++r)
    {
      sprintf (buf, "x%d", r);
      var->register_names[r] = xstrdup (buf);
    }

  var->register_names[pc_regnum] = "pc";
  var->register_names[lr_regnum] = "lr";
  var->register_names[lcr_regnum] = "lcr";
     
  var->register_names[psr_regnum] = "psr";
  var->register_names[ccr_regnum] = "ccr";
  var->register_names[cccr_regnum] = "cccr";
  var->register_names[tbr_regnum] = "tbr";

  /* Debug registers.  */
  var->register_names[brr_regnum] = "brr";
  var->register_names[dbar0_regnum] = "dbar0";
  var->register_names[dbar1_regnum] = "dbar1";
  var->register_names[dbar2_regnum] = "dbar2";
  var->register_names[dbar3_regnum] = "dbar3";

  return var;
}


/* Indicate that the variant VAR has NUM_GPRS general-purpose
   registers, and fill in the names array appropriately.  */
static void
set_variant_num_gprs (struct gdbarch_tdep *var, int num_gprs)
{
  int r;

  var->num_gprs = num_gprs;

  for (r = 0; r < num_gprs; ++r)
    {
      char buf[20];

      sprintf (buf, "gr%d", r);
      var->register_names[first_gpr_regnum + r] = xstrdup (buf);
    }
}


/* Indicate that the variant VAR has NUM_FPRS floating-point
   registers, and fill in the names array appropriately.  */
static void
set_variant_num_fprs (struct gdbarch_tdep *var, int num_fprs)
{
  int r;

  var->num_fprs = num_fprs;

  for (r = 0; r < num_fprs; ++r)
    {
      char buf[20];

      sprintf (buf, "fr%d", r);
      var->register_names[first_fpr_regnum + r] = xstrdup (buf);
    }
}


static const char *
frv_register_name (int reg)
{
  if (reg < 0)
    return "?toosmall?";
  if (reg >= frv_num_regs)
    return "?toolarge?";

  return CURRENT_VARIANT->register_names[reg];
}


static int
frv_register_raw_size (int reg)
{
  return 4;
}

static int
frv_register_virtual_size (int reg)
{
  return 4;
}

static struct type *
frv_register_virtual_type (int reg)
{
  if (reg >= 64 && reg <= 127)
    return builtin_type_float;
  else
    return builtin_type_int;
}

static int
frv_register_byte (int reg)
{
  return (reg * 4);
}

static const unsigned char *
frv_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenp)
{
  static unsigned char breakpoint[] = {0xc0, 0x70, 0x00, 0x01};
  *lenp = sizeof (breakpoint);
  return breakpoint;
}

static CORE_ADDR
frv_frame_chain (struct frame_info *frame)
{
  CORE_ADDR saved_fp_addr;

  if (frame->saved_regs && frame->saved_regs[fp_regnum] != 0)
    saved_fp_addr = frame->saved_regs[fp_regnum];
  else
    /* Just assume it was saved in the usual place.  */
    saved_fp_addr = frame->frame;

  return read_memory_integer (saved_fp_addr, 4);
}

static CORE_ADDR
frv_frame_saved_pc (struct frame_info *frame)
{
  frv_frame_init_saved_regs (frame);

  /* Perhaps the prologue analyzer recorded where it was stored.
     (As of 14 Oct 2001, it never does.)  */
  if (frame->saved_regs && frame->saved_regs[pc_regnum] != 0)
    return read_memory_integer (frame->saved_regs[pc_regnum], 4);

  /* If the prologue analyzer tells us the link register was saved on
     the stack, get it from there.  */
  if (frame->extra_info->lr_saved_on_stack)
    return read_memory_integer (frame->frame + 8, 4);

  /* Otherwise, it's still in LR.
     However, if FRAME isn't the youngest frame, this is kind of
     suspicious --- if this frame called somebody else, then its LR
     has certainly been overwritten.  */
  if (! frame->next)
    return read_register (lr_regnum);

  /* By default, assume it's saved in the standard place, relative to
     the frame pointer.  */
  return read_memory_integer (frame->frame + 8, 4);
}


/* Return true if REG is a caller-saves ("scratch") register,
   false otherwise.  */
static int
is_caller_saves_reg (int reg)
{
  return ((4 <= reg && reg <= 7)
          || (14 <= reg && reg <= 15)
          || (32 <= reg && reg <= 47));
}


/* Return true if REG is a callee-saves register, false otherwise.  */
static int
is_callee_saves_reg (int reg)
{
  return ((16 <= reg && reg <= 31)
          || (48 <= reg && reg <= 63));
}


/* Return true if REG is an argument register, false otherwise.  */
static int
is_argument_reg (int reg)
{
  return (8 <= reg && reg <= 13);
}


/* Scan an FR-V prologue, starting at PC, until frame->PC.
   If FRAME is non-zero, fill in its saved_regs with appropriate addresses.
   We assume FRAME's saved_regs array has already been allocated and cleared.
   Return the first PC value after the prologue.

   Note that, for unoptimized code, we almost don't need this function
   at all; all arguments and locals live on the stack, so we just need
   the FP to find everything.  The catch: structures passed by value
   have their addresses living in registers; they're never spilled to
   the stack.  So if you ever want to be able to get to these
   arguments in any frame but the top, you'll need to do this serious
   prologue analysis.  */
static CORE_ADDR
frv_analyze_prologue (CORE_ADDR pc, struct frame_info *frame)
{
  /* When writing out instruction bitpatterns, we use the following
     letters to label instruction fields:
     P - The parallel bit.  We don't use this.
     J - The register number of GRj in the instruction description.
     K - The register number of GRk in the instruction description.
     I - The register number of GRi.
     S - a signed imediate offset.
     U - an unsigned immediate offset.

     The dots below the numbers indicate where hex digit boundaries
     fall, to make it easier to check the numbers.  */

  /* Non-zero iff we've seen the instruction that initializes the
     frame pointer for this function's frame.  */
  int fp_set = 0;

  /* If fp_set is non_zero, then this is the distance from
     the stack pointer to frame pointer: fp = sp + fp_offset.  */
  int fp_offset = 0;

  /* Total size of frame prior to any alloca operations. */
  int framesize = 0;

  /* The number of the general-purpose register we saved the return
     address ("link register") in, or -1 if we haven't moved it yet.  */
  int lr_save_reg = -1;

  /* Non-zero iff we've saved the LR onto the stack.  */
  int lr_saved_on_stack = 0;

  /* If gr_saved[i] is non-zero, then we've noticed that general
     register i has been saved at gr_sp_offset[i] from the stack
     pointer.  */
  char gr_saved[64];
  int gr_sp_offset[64];

  memset (gr_saved, 0, sizeof (gr_saved));

  while (! frame || pc < frame->pc)
    {
      LONGEST op = read_memory_integer (pc, 4);

      /* The tests in this chain of ifs should be in order of
	 decreasing selectivity, so that more particular patterns get
	 to fire before less particular patterns.  */

      /* Setting the FP from the SP:
	 ori sp, 0, fp
	 P 000010 0100010 000001 000000000000 = 0x04881000
	 0 111111 1111111 111111 111111111111 = 0x7fffffff
             .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      if ((op & 0x7fffffff) == 0x04881000)
	{
	  fp_set = 1;
	  fp_offset = 0;
	}

      /* Move the link register to the scratch register grJ, before saving:
         movsg lr, grJ
         P 000100 0000011 010000 000111 JJJJJJ = 0x080d01c0
         0 111111 1111111 111111 111111 000000 = 0x7fffffc0
             .    .   .    .   .    .    .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7fffffc0) == 0x080d01c0)
        {
          int gr_j = op & 0x3f;

          /* If we're moving it to a scratch register, that's fine.  */
          if (is_caller_saves_reg (gr_j))
            lr_save_reg = gr_j;
          /* Otherwise it's not a prologue instruction that we
             recognize.  */
          else
            break;
        }

      /* To save multiple callee-saves registers on the stack, at
         offset zero:

	 std grK,@(sp,gr0)
	 P KKKKKK 0000011 000001 000011 000000 = 0x000c10c0
	 0 000000 1111111 111111 111111 111111 = 0x01ffffff

	 stq grK,@(sp,gr0)
	 P KKKKKK 0000011 000001 000100 000000 = 0x000c1100
	 0 000000 1111111 111111 111111 111111 = 0x01ffffff
             .    .   .    .   .    .    .   .
         We treat this as part of the prologue, and record the register's
	 saved address in the frame structure.  */
      else if ((op & 0x01ffffff) == 0x000c10c0
            || (op & 0x01ffffff) == 0x000c1100)
	{
	  int gr_k = ((op >> 25) & 0x3f);
	  int ope  = ((op >> 6)  & 0x3f);
          int count;
	  int i;

          /* Is it an std or an stq?  */
          if (ope == 0x03)
            count = 2;
          else
            count = 4;

	  /* Is it really a callee-saves register?  */
	  if (is_callee_saves_reg (gr_k))
	    {
	      for (i = 0; i < count; i++)
	        {
		  gr_saved[gr_k + i] = 1;
		  gr_sp_offset[gr_k + i] = 4 * i;
		}
	    }
	  else
	    /* It's not a prologue instruction.  */
	    break;
	}

      /* Adjusting the stack pointer.  (The stack pointer is GR1.)
	 addi sp, S, sp
         P 000001 0010000 000001 SSSSSSSSSSSS = 0x02401000
         0 111111 1111111 111111 000000000000 = 0x7ffff000
             .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7ffff000) == 0x02401000)
        {
	  /* Sign-extend the twelve-bit field.
	     (Isn't there a better way to do this?)  */
	  int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

	  framesize -= s;
	}

      /* Setting the FP to a constant distance from the SP:
	 addi sp, S, fp
         P 000010 0010000 000001 SSSSSSSSSSSS = 0x04401000
         0 111111 1111111 111111 000000000000 = 0x7ffff000
             .    .   .    .   .    .   .   .
	 We treat this as part of the prologue.  */
      else if ((op & 0x7ffff000) == 0x04401000)
	{
	  /* Sign-extend the twelve-bit field.
	     (Isn't there a better way to do this?)  */
	  int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;
	  fp_set = 1;
	  fp_offset = s;
	}

      /* To spill an argument register to a scratch register:
	    ori GRi, 0, GRk
	 P KKKKKK 0100010 IIIIII 000000000000 = 0x00880000
	 0 000000 1111111 000000 111111111111 = 0x01fc0fff
	     .    .   .    .   .    .   .   .
	 For the time being, we treat this as a prologue instruction,
	 assuming that GRi is an argument register.  This one's kind
	 of suspicious, because it seems like it could be part of a
	 legitimate body instruction.  But we only come here when the
	 source info wasn't helpful, so we have to do the best we can.
	 Hopefully once GCC and GDB agree on how to emit line number
	 info for prologues, then this code will never come into play.  */
      else if ((op & 0x01fc0fff) == 0x00880000)
	{
	  int gr_i = ((op >> 12) & 0x3f);

          /* If the source isn't an arg register, then this isn't a
             prologue instruction.  */
	  if (! is_argument_reg (gr_i))
	    break;
	}

      /* To spill 16-bit values to the stack:
	     sthi GRk, @(fp, s)
	 P KKKKKK 1010001 000010 SSSSSSSSSSSS = 0x01442000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
             .    .   .    .   .    .   .   . 
         And for 8-bit values, we use STB instructions.
	     stbi GRk, @(fp, s)
	 P KKKKKK 1010000 000010 SSSSSSSSSSSS = 0x01402000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   .
         We check that GRk is really an argument register, and treat
         all such as part of the prologue.  */
      else if (   (op & 0x01fff000) == 0x01442000
	       || (op & 0x01fff000) == 0x01402000)
	{
	  int gr_k = ((op >> 25) & 0x3f);

	  if (! is_argument_reg (gr_k))
	    break;		/* Source isn't an arg register.  */
	}

      /* To save multiple callee-saves register on the stack, at a
         non-zero offset:

	 stdi GRk, @(sp, s)
	 P KKKKKK 1010011 000001 SSSSSSSSSSSS = 0x014c1000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
             .    .   .    .   .    .   .   .
	 stqi GRk, @(sp, s)
	 P KKKKKK 1010100 000001 SSSSSSSSSSSS = 0x01501000
	 0 000000 1111111 111111 000000000000 = 0x01fff000
	     .    .   .    .   .    .   .   .
         We treat this as part of the prologue, and record the register's
	 saved address in the frame structure.  */
      else if ((op & 0x01fff000) == 0x014c1000
            || (op & 0x01fff000) == 0x01501000)
	{
	  int gr_k = ((op >> 25) & 0x3f);
          int count;
	  int i;

          /* Is it a stdi or a stqi?  */
          if ((op & 0x01fff000) == 0x014c1000)
            count = 2;
          else
            count = 4;

	  /* Is it really a callee-saves register?  */
	  if (is_callee_saves_reg (gr_k))
	    {
	      /* Sign-extend the twelve-bit field.
		 (Isn't there a better way to do this?)  */
	      int s = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

	      for (i = 0; i < count; i++)
		{
		  gr_saved[gr_k + i] = 1;
		  gr_sp_offset[gr_k + i] = s + (4 * i);
		}
	    }
	  else
	    /* It's not a prologue instruction.  */
	    break;
	}

      /* Storing any kind of integer register at any constant offset
         from any other register.

	 st GRk, @(GRi, gr0)
         P KKKKKK 0000011 IIIIII 000010 000000 = 0x000c0080
         0 000000 1111111 000000 111111 111111 = 0x01fc0fff
             .    .   .    .   .    .    .   .
	 sti GRk, @(GRi, d12)
	 P KKKKKK 1010010 IIIIII SSSSSSSSSSSS = 0x01480000
	 0 000000 1111111 000000 000000000000 = 0x01fc0000
             .    .   .    .   .    .   .   .
         These could be almost anything, but a lot of prologue
         instructions fall into this pattern, so let's decode the
         instruction once, and then work at a higher level.  */
      else if (((op & 0x01fc0fff) == 0x000c0080)
            || ((op & 0x01fc0000) == 0x01480000))
        {
          int gr_k = ((op >> 25) & 0x3f);
          int gr_i = ((op >> 12) & 0x3f);
          int offset;

          /* Are we storing with gr0 as an offset, or using an
             immediate value?  */
          if ((op & 0x01fc0fff) == 0x000c0080)
            offset = 0;
          else
            offset = (((op & 0xfff) - 0x800) & 0xfff) - 0x800;

          /* If the address isn't relative to the SP or FP, it's not a
             prologue instruction.  */
          if (gr_i != sp_regnum && gr_i != fp_regnum)
            break;

          /* Saving the old FP in the new frame (relative to the SP).  */
          if (gr_k == fp_regnum && gr_i == sp_regnum)
            ;

          /* Saving callee-saves register(s) on the stack, relative to
             the SP.  */
          else if (gr_i == sp_regnum
                   && is_callee_saves_reg (gr_k))
            {
              gr_saved[gr_k] = 1;
              gr_sp_offset[gr_k] = offset;
            }

          /* Saving the scratch register holding the return address.  */
          else if (lr_save_reg != -1
                   && gr_k == lr_save_reg)
            lr_saved_on_stack = 1;

          /* Spilling int-sized arguments to the stack.  */
          else if (is_argument_reg (gr_k))
            ;

          /* It's not a store instruction we recognize, so this must
             be the end of the prologue.  */
          else
            break;
        }

      /* It's not any instruction we recognize, so this must be the end
         of the prologue.  */
      else
	break;

      pc += 4;
    }

  if (frame)
    {
      frame->extra_info->lr_saved_on_stack = lr_saved_on_stack;

      /* If we know the relationship between the stack and frame
         pointers, record the addresses of the registers we noticed.
         Note that we have to do this as a separate step at the end,
         because instructions may save relative to the SP, but we need
         their addresses relative to the FP.  */
      if (fp_set)
        {
          int i;

          for (i = 0; i < 64; i++)
            if (gr_saved[i])
              frame->saved_regs[i] = (frame->frame
                                      - fp_offset + gr_sp_offset[i]);

          frame->extra_info->fp_to_callers_sp_offset = framesize - fp_offset;
        }
    }

  return pc;
}


static CORE_ADDR
frv_skip_prologue (CORE_ADDR pc)
{
  CORE_ADDR func_addr, func_end, new_pc;

  new_pc = pc;

  /* If the line table has entry for a line *within* the function
     (i.e., not in the prologue, and not past the end), then that's
     our location.  */
  if (find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      struct symtab_and_line sal;

      sal = find_pc_line (func_addr, 0);

      if (sal.line != 0 && sal.end < func_end)
	{
	  new_pc = sal.end;
	}
    }

  /* The FR-V prologue is at least five instructions long (twenty bytes).
     If we didn't find a real source location past that, then
     do a full analysis of the prologue.  */
  if (new_pc < pc + 20)
    new_pc = frv_analyze_prologue (pc, 0);

  return new_pc;
}

static void
frv_frame_init_saved_regs (struct frame_info *frame)
{
  if (frame->saved_regs)
    return;

  frame_saved_regs_zalloc (frame);
  frame->saved_regs[fp_regnum] = frame->frame;

  /* Find the beginning of this function, so we can analyze its
     prologue.  */     
  {
    CORE_ADDR func_addr, func_end;

    if (find_pc_partial_function (frame->pc, NULL, &func_addr, &func_end))
      frv_analyze_prologue (func_addr, frame);
  }
}

/* Should we use EXTRACT_STRUCT_VALUE_ADDRESS instead of
   EXTRACT_RETURN_VALUE?  GCC_P is true if compiled with gcc
   and TYPE is the type (which is known to be struct, union or array).

   The frv returns all structs in memory.  */

static int
frv_use_struct_convention (int gcc_p, struct type *type)
{
  return 1;
}

static void
frv_extract_return_value (struct type *type, char *regbuf, char *valbuf)
{
  memcpy (valbuf, (regbuf
		   + frv_register_byte (8)
		   + (TYPE_LENGTH (type) < 4 ? 4 - TYPE_LENGTH (type) : 0)),
		   TYPE_LENGTH (type));
}

static CORE_ADDR
frv_extract_struct_value_address (char *regbuf)
{
  return extract_address (regbuf + frv_register_byte (struct_return_regnum),
			  4);
}

static void
frv_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  write_register (struct_return_regnum, addr);
}

static int
frv_frameless_function_invocation (struct frame_info *frame)
{
  return frameless_look_for_prologue (frame);
}

static CORE_ADDR
frv_saved_pc_after_call (struct frame_info *frame)
{
  return read_register (lr_regnum);
}

static void
frv_init_extra_frame_info (int fromleaf, struct frame_info *frame)
{
  frame->extra_info = (struct frame_extra_info *)
    frame_obstack_alloc (sizeof (struct frame_extra_info));
  frame->extra_info->fp_to_callers_sp_offset = 0;
  frame->extra_info->lr_saved_on_stack = 0;
}

#define ROUND_UP(n,a) (((n)+(a)-1) & ~((a)-1))
#define ROUND_DOWN(n,a) ((n) & ~((a)-1))

static CORE_ADDR
frv_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
		    int struct_return, CORE_ADDR struct_addr)
{
  int argreg;
  int argnum;
  char *val;
  char valbuf[4];
  struct value *arg;
  struct type *arg_type;
  int len;
  enum type_code typecode;
  CORE_ADDR regval;
  int stack_space;
  int stack_offset;

#if 0
  printf("Push %d args at sp = %x, struct_return=%d (%x)\n",
	 nargs, (int) sp, struct_return, struct_addr);
#endif

  stack_space = 0;
  for (argnum = 0; argnum < nargs; ++argnum)
    stack_space += ROUND_UP (TYPE_LENGTH (VALUE_TYPE (args[argnum])), 4);

  stack_space -= (6 * 4);
  if (stack_space > 0)
    sp -= stack_space;

  /* Make sure stack is dword aligned. */
  sp = ROUND_DOWN (sp, 8);

  stack_offset = 0;

  argreg = 8;

  if (struct_return)
    write_register (struct_return_regnum, struct_addr);

  for (argnum = 0; argnum < nargs; ++argnum)
    {
      arg = args[argnum];
      arg_type = check_typedef (VALUE_TYPE (arg));
      len = TYPE_LENGTH (arg_type);
      typecode = TYPE_CODE (arg_type);

      if (typecode == TYPE_CODE_STRUCT || typecode == TYPE_CODE_UNION)
	{
	  store_address (valbuf, 4, VALUE_ADDRESS (arg));
	  typecode = TYPE_CODE_PTR;
	  len = 4;
	  val = valbuf;
	}
      else
	{
	  val = (char *) VALUE_CONTENTS (arg);
	}

      while (len > 0)
	{
	  int partial_len = (len < 4 ? len : 4);

	  if (argreg < 14)
	    {
	      regval = extract_address (val, partial_len);
#if 0
	      printf("  Argnum %d data %x -> reg %d\n",
		     argnum, (int) regval, argreg);
#endif
	      write_register (argreg, regval);
	      ++argreg;
	    }
	  else
	    {
#if 0
	      printf("  Argnum %d data %x -> offset %d (%x)\n",
		     argnum, *((int *)val), stack_offset, (int) (sp + stack_offset));
#endif
	      write_memory (sp + stack_offset, val, partial_len);
	      stack_offset += ROUND_UP(partial_len, 4);
	    }
	  len -= partial_len;
	  val += partial_len;
	}
    }
  return sp;
}

static CORE_ADDR
frv_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  write_register (lr_regnum, CALL_DUMMY_ADDRESS ());
  return sp;
}

static void
frv_store_return_value (struct type *type, char *valbuf)
{
  int length = TYPE_LENGTH (type);
  int reg8_offset = frv_register_byte (8);

  if (length <= 4)
    write_register_bytes (reg8_offset + (4 - length), valbuf, length);
  else if (length == 8)
    write_register_bytes (reg8_offset, valbuf, length);
  else
    internal_error (__FILE__, __LINE__,
                    "Don't know how to return a %d-byte value.", length);
}

static void
frv_pop_frame (void)
{
  generic_pop_current_frame (frv_pop_frame_regular);
}

static void
frv_pop_frame_regular (struct frame_info *frame)
{
  CORE_ADDR fp;
  int regno;

  fp = frame->frame;

  frv_frame_init_saved_regs (frame);

  write_register (pc_regnum, frv_frame_saved_pc (frame));
  for (regno = 0; regno < frv_num_regs; ++regno)
    {
      if (frame->saved_regs[regno]
	  && regno != pc_regnum
	  && regno != sp_regnum)
	{
	  write_register (regno,
			  read_memory_integer (frame->saved_regs[regno], 4));
	}
    }
  write_register (sp_regnum, fp + frame->extra_info->fp_to_callers_sp_offset);
  flush_cached_frames ();
}


static void
frv_remote_translate_xfer_address (CORE_ADDR memaddr, int nr_bytes,
				   CORE_ADDR *targ_addr, int *targ_len)
{
  *targ_addr = memaddr;
  *targ_len  = nr_bytes;
}


/* Hardware watchpoint / breakpoint support for the FR500
   and FR400.  */

int
frv_check_watch_resources (int type, int cnt, int ot)
{
  struct gdbarch_tdep *var = CURRENT_VARIANT;

  /* Watchpoints not supported on simulator.  */
  if (strcmp (target_shortname, "sim") == 0)
    return 0;

  if (type == bp_hardware_breakpoint)
    {
      if (var->num_hw_breakpoints == 0)
	return 0;
      else if (cnt <= var->num_hw_breakpoints)
	return 1;
    }
  else
    {
      if (var->num_hw_watchpoints == 0)
	return 0;
      else if (ot)
	return -1;
      else if (cnt <= var->num_hw_watchpoints)
	return 1;
    }
  return -1;
}


CORE_ADDR
frv_stopped_data_address (void)
{
  CORE_ADDR brr, dbar0, dbar1, dbar2, dbar3;

  brr = read_register (brr_regnum);
  dbar0 = read_register (dbar0_regnum);
  dbar1 = read_register (dbar1_regnum);
  dbar2 = read_register (dbar2_regnum);
  dbar3 = read_register (dbar3_regnum);

  if (brr & (1<<11))
    return dbar0;
  else if (brr & (1<<10))
    return dbar1;
  else if (brr & (1<<9))
    return dbar2;
  else if (brr & (1<<8))
    return dbar3;
  else
    return 0;
}

static struct gdbarch *
frv_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *var;

  /* Check to see if we've already built an appropriate architecture
     object for this executable.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches)
    return arches->gdbarch;

  /* Select the right tdep structure for this variant.  */
  var = new_variant ();
  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_frv:
    case bfd_mach_frvsimple:
    case bfd_mach_fr500:
    case bfd_mach_frvtomcat:
      set_variant_num_gprs (var, 64);
      set_variant_num_fprs (var, 64);
      break;

    case bfd_mach_fr400:
      set_variant_num_gprs (var, 32);
      set_variant_num_fprs (var, 32);
      break;

    default:
      /* Never heard of this variant.  */
      return 0;
    }
  
  gdbarch = gdbarch_alloc (&info, var);

  set_gdbarch_short_bit (gdbarch, 16);
  set_gdbarch_int_bit (gdbarch, 32);
  set_gdbarch_long_bit (gdbarch, 32);
  set_gdbarch_long_long_bit (gdbarch, 64);
  set_gdbarch_float_bit (gdbarch, 32);
  set_gdbarch_double_bit (gdbarch, 64);
  set_gdbarch_long_double_bit (gdbarch, 64);
  set_gdbarch_ptr_bit (gdbarch, 32);

  set_gdbarch_num_regs (gdbarch, frv_num_regs);
  set_gdbarch_sp_regnum (gdbarch, sp_regnum);
  set_gdbarch_fp_regnum (gdbarch, fp_regnum);
  set_gdbarch_pc_regnum (gdbarch, pc_regnum);

  set_gdbarch_register_name (gdbarch, frv_register_name);
  set_gdbarch_register_size (gdbarch, 4);
  set_gdbarch_register_bytes (gdbarch, frv_num_regs * 4);
  set_gdbarch_register_byte (gdbarch, frv_register_byte);
  set_gdbarch_register_raw_size (gdbarch, frv_register_raw_size);
  set_gdbarch_max_register_raw_size (gdbarch, 4);
  set_gdbarch_register_virtual_size (gdbarch, frv_register_virtual_size);
  set_gdbarch_max_register_virtual_size (gdbarch, 4);
  set_gdbarch_register_virtual_type (gdbarch, frv_register_virtual_type);

  set_gdbarch_skip_prologue (gdbarch, frv_skip_prologue);
  set_gdbarch_breakpoint_from_pc (gdbarch, frv_breakpoint_from_pc);

  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);
  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_frameless_function_invocation (gdbarch, frv_frameless_function_invocation);

  set_gdbarch_saved_pc_after_call (gdbarch, frv_saved_pc_after_call);

  set_gdbarch_frame_chain (gdbarch, frv_frame_chain);
  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_frame_saved_pc (gdbarch, frv_frame_saved_pc);
  set_gdbarch_frame_args_address (gdbarch, default_frame_address);
  set_gdbarch_frame_locals_address (gdbarch, default_frame_address);

  set_gdbarch_frame_init_saved_regs (gdbarch, frv_frame_init_saved_regs);

  set_gdbarch_use_struct_convention (gdbarch, frv_use_struct_convention);
  set_gdbarch_deprecated_extract_return_value (gdbarch, frv_extract_return_value);

  set_gdbarch_store_struct_return (gdbarch, frv_store_struct_return);
  set_gdbarch_deprecated_store_return_value (gdbarch, frv_store_return_value);
  set_gdbarch_deprecated_extract_struct_value_address (gdbarch, frv_extract_struct_value_address);

  /* Settings for calling functions in the inferior.  */
  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_coerce_float_to_double (gdbarch, 
				      standard_coerce_float_to_double);
  set_gdbarch_push_arguments (gdbarch, frv_push_arguments);
  set_gdbarch_push_return_address (gdbarch, frv_push_return_address);
  set_gdbarch_pop_frame (gdbarch, frv_pop_frame);

  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_call_dummy_words (gdbarch, frv_call_dummy_words);
  set_gdbarch_sizeof_call_dummy_words (gdbarch, sizeof (frv_call_dummy_words));
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_init_extra_frame_info (gdbarch, frv_init_extra_frame_info);

  /* Settings that should be unnecessary.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);

  set_gdbarch_read_pc (gdbarch, generic_target_read_pc);
  set_gdbarch_write_pc (gdbarch, generic_target_write_pc);
  set_gdbarch_read_fp (gdbarch, generic_target_read_fp);
  set_gdbarch_read_sp (gdbarch, generic_target_read_sp);
  set_gdbarch_write_sp (gdbarch, generic_target_write_sp);

  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, entry_point_address);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, pc_in_call_dummy_at_entry_point);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);

  set_gdbarch_get_saved_register (gdbarch, generic_unwind_get_saved_register);

  set_gdbarch_decr_pc_after_break (gdbarch, 0);
  set_gdbarch_function_start_offset (gdbarch, 0);
  set_gdbarch_register_convertible (gdbarch, generic_register_convertible_not);

  set_gdbarch_remote_translate_xfer_address
    (gdbarch, frv_remote_translate_xfer_address);

  /* Hardware watchpoint / breakpoint support.  */
  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_frv:
    case bfd_mach_frvsimple:
    case bfd_mach_fr500:
    case bfd_mach_frvtomcat:
      /* fr500-style hardware debugging support.  */
      var->num_hw_watchpoints = 4;
      var->num_hw_breakpoints = 4;
      break;

    case bfd_mach_fr400:
      /* fr400-style hardware debugging support.  */
      var->num_hw_watchpoints = 2;
      var->num_hw_breakpoints = 4;
      break;

    default:
      /* Otherwise, assume we don't have hardware debugging support.  */
      var->num_hw_watchpoints = 0;
      var->num_hw_breakpoints = 0;
      break;
    }

  return gdbarch;
}

void
_initialize_frv_tdep (void)
{
  register_gdbarch_init (bfd_arch_frv, frv_gdbarch_init);

  tm_print_insn = print_insn_frv;
}


