/* Target-dependent code for GDB, the GNU debugger.

   Copyright 2001, 2002 Free Software Foundation, Inc.

   Contributed by D.J. Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
   for IBM Deutschland Entwicklung GmbH, IBM Corporation.

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#define S390_TDEP		/* for special macros in tm-s390.h */
#include <defs.h>
#include "arch-utils.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "tm.h"
#include "../bfd/bfd.h"
#include "floatformat.h"
#include "regcache.h"
#include "value.h"
#include "gdb_assert.h"




/* Number of bytes of storage in the actual machine representation
   for register N.  */
int
s390_register_raw_size (int reg_nr)
{
  if (S390_FP0_REGNUM <= reg_nr
      && reg_nr < S390_FP0_REGNUM + S390_NUM_FPRS)
    return S390_FPR_SIZE;
  else
    return 4;
}

int
s390x_register_raw_size (int reg_nr)
{
  return (reg_nr == S390_FPC_REGNUM)
    || (reg_nr >= S390_FIRST_ACR && reg_nr <= S390_LAST_ACR) ? 4 : 8;
}

int
s390_cannot_fetch_register (int regno)
{
  return (regno >= S390_FIRST_CR && regno < (S390_FIRST_CR + 9)) ||
    (regno >= (S390_FIRST_CR + 12) && regno <= S390_LAST_CR);
}

int
s390_register_byte (int reg_nr)
{
  if (reg_nr <= S390_GP_LAST_REGNUM)
    return reg_nr * S390_GPR_SIZE;
  if (reg_nr <= S390_LAST_ACR)
    return S390_ACR0_OFFSET + (((reg_nr) - S390_FIRST_ACR) * S390_ACR_SIZE);
  if (reg_nr <= S390_LAST_CR)
    return S390_CR0_OFFSET + (((reg_nr) - S390_FIRST_CR) * S390_CR_SIZE);
  if (reg_nr == S390_FPC_REGNUM)
    return S390_FPC_OFFSET;
  else
    return S390_FP0_OFFSET + (((reg_nr) - S390_FP0_REGNUM) * S390_FPR_SIZE);
}

#ifndef GDBSERVER
#define S390_MAX_INSTR_SIZE (6)
#define S390_SYSCALL_OPCODE (0x0a)
#define S390_SYSCALL_SIZE   (2)
#define S390_SIGCONTEXT_SREGS_OFFSET (8)
#define S390X_SIGCONTEXT_SREGS_OFFSET (8)
#define S390_SIGREGS_FP0_OFFSET       (144)
#define S390X_SIGREGS_FP0_OFFSET      (216)
#define S390_UC_MCONTEXT_OFFSET    (256)
#define S390X_UC_MCONTEXT_OFFSET   (344)
#define S390_STACK_FRAME_OVERHEAD  (GDB_TARGET_IS_ESAME ? 160:96)
#define S390_SIGNAL_FRAMESIZE  (GDB_TARGET_IS_ESAME ? 160:96)
#define s390_NR_sigreturn          119
#define s390_NR_rt_sigreturn       173



struct frame_extra_info
{
  int initialised;
  int good_prologue;
  CORE_ADDR function_start;
  CORE_ADDR skip_prologue_function_start;
  CORE_ADDR saved_pc_valid;
  CORE_ADDR saved_pc;
  CORE_ADDR sig_fixed_saved_pc_valid;
  CORE_ADDR sig_fixed_saved_pc;
  CORE_ADDR frame_pointer_saved_pc;	/* frame pointer needed for alloca */
  CORE_ADDR stack_bought;	/* amount we decrement the stack pointer by */
  CORE_ADDR sigcontext;
};


static CORE_ADDR s390_frame_saved_pc_nofix (struct frame_info *fi);

int
s390_readinstruction (bfd_byte instr[], CORE_ADDR at,
		      struct disassemble_info *info)
{
  int instrlen;

  static int s390_instrlen[] = {
    2,
    4,
    4,
    6
  };
  if ((*info->read_memory_func) (at, &instr[0], 2, info))
    return -1;
  instrlen = s390_instrlen[instr[0] >> 6];
  if (instrlen > 2)
    {
      if ((*info->read_memory_func) (at + 2, &instr[2], instrlen - 2, info))
        return -1;
    }
  return instrlen;
}

static void
s390_memset_extra_info (struct frame_extra_info *fextra_info)
{
  memset (fextra_info, 0, sizeof (struct frame_extra_info));
}



const char *
s390_register_name (int reg_nr)
{
  static char *register_names[] = {
    "pswm", "pswa",
    "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
    "acr0", "acr1", "acr2", "acr3", "acr4", "acr5", "acr6", "acr7",
    "acr8", "acr9", "acr10", "acr11", "acr12", "acr13", "acr14", "acr15",
    "cr0", "cr1", "cr2", "cr3", "cr4", "cr5", "cr6", "cr7",
    "cr8", "cr9", "cr10", "cr11", "cr12", "cr13", "cr14", "cr15",
    "fpc",
    "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
    "f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15"
  };

  if (reg_nr <= S390_LAST_REGNUM)
    return register_names[reg_nr];
  else
    return NULL;
}




int
s390_stab_reg_to_regnum (int regno)
{
  return regno >= 64 ? S390_PSWM_REGNUM - 64 :
    regno >= 48 ? S390_FIRST_ACR - 48 :
    regno >= 32 ? S390_FIRST_CR - 32 :
    regno <= 15 ? (regno + 2) :
    S390_FP0_REGNUM + ((regno - 16) & 8) + (((regno - 16) & 3) << 1) +
    (((regno - 16) & 4) >> 2);
}


/* Return true if REGIDX is the number of a register used to pass
     arguments, false otherwise.  */
static int
is_arg_reg (int regidx)
{
  return 2 <= regidx && regidx <= 6;
}


/* s390_get_frame_info based on Hartmuts
   prologue definition in
   gcc-2.8.1/config/l390/linux.c 

   It reads one instruction at a time & based on whether
   it looks like prologue code or not it makes a decision on
   whether the prologue is over, there are various state machines
   in the code to determine if the prologue code is possilby valid.
   
   This is done to hopefully allow the code survive minor revs of
   calling conventions.

 */

int
s390_get_frame_info (CORE_ADDR pc, struct frame_extra_info *fextra_info,
		     struct frame_info *fi, int init_extra_info)
{
#define CONST_POOL_REGIDX 13
#define GOT_REGIDX        12
  bfd_byte instr[S390_MAX_INSTR_SIZE];
  CORE_ADDR test_pc = pc, test_pc2;
  CORE_ADDR orig_sp = 0, save_reg_addr = 0, *saved_regs = NULL;
  int valid_prologue, good_prologue = 0;
  int gprs_saved[S390_NUM_GPRS];
  int fprs_saved[S390_NUM_FPRS];
  int regidx, instrlen;
  int const_pool_state;
  int varargs_state;
  int loop_cnt, gdb_gpr_store, gdb_fpr_store;
  int offset, expected_offset;
  int err = 0;
  disassemble_info info;

  /* Have we seen an instruction initializing the frame pointer yet?
     If we've seen an `lr %r11, %r15', then frame_pointer_found is
     non-zero, and frame_pointer_regidx == 11.  Otherwise,
     frame_pointer_found is zero and frame_pointer_regidx is 15,
     indicating that we're using the stack pointer as our frame
     pointer.  */
  int frame_pointer_found = 0;
  int frame_pointer_regidx = 0xf;

  /* What we've seen so far regarding saving the back chain link:
     0 -- nothing yet; sp still has the same value it had at the entry
          point.  Since not all functions allocate frames, this is a
          valid state for the prologue to finish in.
     1 -- We've saved the original sp in some register other than the
          frame pointer (hard-coded to be %r11, yuck).
          save_link_regidx is the register we saved it in.
     2 -- We've seen the initial `bras' instruction of the sequence for
          reserving more than 32k of stack:
                bras %rX, .+8
                .long N
                s %r15, 0(%rX)
          where %rX is not the constant pool register.
          subtract_sp_regidx is %rX, and fextra_info->stack_bought is N.
     3 -- We've reserved space for a new stack frame.  This means we
          either saw a simple `ahi %r15,-N' in state 1, or the final
          `s %r15, ...' in state 2.
     4 -- The frame and link are now fully initialized.  We've
          reserved space for the new stack frame, and stored the old
          stack pointer captured in the back chain pointer field.  */
  int save_link_state = 0;
  int save_link_regidx, subtract_sp_regidx;

  /* What we've seen so far regarding r12 --- the GOT (Global Offset
     Table) pointer.  We expect to see `l %r12, N(%r13)', which loads
     r12 with the offset from the constant pool to the GOT, and then
     an `ar %r12, %r13', which adds the constant pool address,
     yielding the GOT's address.  Here's what got_state means:
     0 -- seen nothing
     1 -- seen `l %r12, N(%r13)', but no `ar'
     2 -- seen load and add, so GOT pointer is totally initialized
     When got_state is 1, then got_load_addr is the address of the
     load instruction, and got_load_len is the length of that
     instruction.  */
  int got_state= 0;
  CORE_ADDR got_load_addr = 0, got_load_len = 0;

  const_pool_state = varargs_state = 0;

  memset (gprs_saved, 0, sizeof (gprs_saved));
  memset (fprs_saved, 0, sizeof (fprs_saved));
  info.read_memory_func = dis_asm_read_memory;

  save_link_regidx = subtract_sp_regidx = 0;
  if (fextra_info)
    {
      if (fi && fi->frame)
	{
          orig_sp = fi->frame;
          if (! init_extra_info && fextra_info->initialised)
            orig_sp += fextra_info->stack_bought;
	  saved_regs = fi->saved_regs;
	}
      if (init_extra_info || !fextra_info->initialised)
	{
	  s390_memset_extra_info (fextra_info);
	  fextra_info->function_start = pc;
	  fextra_info->initialised = 1;
	}
    }
  instrlen = 0;
  do
    {
      valid_prologue = 0;
      test_pc += instrlen;
      /* add the previous instruction len */
      instrlen = s390_readinstruction (instr, test_pc, &info);
      if (instrlen < 0)
	{
	  good_prologue = 0;
	  err = -1;
	  break;
	}
      /* We probably are in a glibc syscall */
      if (instr[0] == S390_SYSCALL_OPCODE && test_pc == pc)
	{
	  good_prologue = 1;
	  if (saved_regs && fextra_info && fi->next && fi->next->extra_info
	      && fi->next->extra_info->sigcontext)
	    {
	      /* We are backtracing from a signal handler */
	      save_reg_addr = fi->next->extra_info->sigcontext +
		REGISTER_BYTE (S390_GP0_REGNUM);
	      for (regidx = 0; regidx < S390_NUM_GPRS; regidx++)
		{
		  saved_regs[S390_GP0_REGNUM + regidx] = save_reg_addr;
		  save_reg_addr += S390_GPR_SIZE;
		}
	      save_reg_addr = fi->next->extra_info->sigcontext +
		(GDB_TARGET_IS_ESAME ? S390X_SIGREGS_FP0_OFFSET :
		 S390_SIGREGS_FP0_OFFSET);
	      for (regidx = 0; regidx < S390_NUM_FPRS; regidx++)
		{
		  saved_regs[S390_FP0_REGNUM + regidx] = save_reg_addr;
		  save_reg_addr += S390_FPR_SIZE;
		}
	    }
	  break;
	}
      if (save_link_state == 0)
	{
	  /* check for a stack relative STMG or STM */
	  if (((GDB_TARGET_IS_ESAME &&
		((instr[0] == 0xeb) && (instr[5] == 0x24))) ||
	       (instr[0] == 0x90)) && ((instr[2] >> 4) == 0xf))
	    {
	      regidx = (instr[1] >> 4);
	      if (regidx < 6)
		varargs_state = 1;
	      offset = ((instr[2] & 0xf) << 8) + instr[3];
	      expected_offset =
		S390_GPR6_STACK_OFFSET + (S390_GPR_SIZE * (regidx - 6));
	      if (offset != expected_offset)
		{
		  good_prologue = 0;
		  break;
		}
	      if (saved_regs)
		save_reg_addr = orig_sp + offset;
	      for (; regidx <= (instr[1] & 0xf); regidx++)
		{
		  if (gprs_saved[regidx])
		    {
		      good_prologue = 0;
		      break;
		    }
		  good_prologue = 1;
		  gprs_saved[regidx] = 1;
		  if (saved_regs)
		    {
		      saved_regs[S390_GP0_REGNUM + regidx] = save_reg_addr;
		      save_reg_addr += S390_GPR_SIZE;
		    }
		}
	      valid_prologue = 1;
	      continue;
	    }
	}
      /* check for a stack relative STG or ST */
      if ((save_link_state == 0 || save_link_state == 3) &&
	  ((GDB_TARGET_IS_ESAME &&
	    ((instr[0] == 0xe3) && (instr[5] == 0x24))) ||
	   (instr[0] == 0x50)) && ((instr[2] >> 4) == 0xf))
	{
	  regidx = instr[1] >> 4;
	  offset = ((instr[2] & 0xf) << 8) + instr[3];
	  if (offset == 0)
	    {
	      if (save_link_state == 3 && regidx == save_link_regidx)
		{
		  save_link_state = 4;
		  valid_prologue = 1;
		  continue;
		}
	      else
		break;
	    }
	  if (regidx < 6)
	    varargs_state = 1;
	  expected_offset =
	    S390_GPR6_STACK_OFFSET + (S390_GPR_SIZE * (regidx - 6));
	  if (offset != expected_offset)
	    {
	      good_prologue = 0;
	      break;
	    }
	  if (gprs_saved[regidx])
	    {
	      good_prologue = 0;
	      break;
	    }
	  good_prologue = 1;
	  gprs_saved[regidx] = 1;
	  if (saved_regs)
	    {
	      save_reg_addr = orig_sp + offset;
	      saved_regs[S390_GP0_REGNUM + regidx] = save_reg_addr;
	    }
	  valid_prologue = 1;
	  continue;
	}

      /* Check for an fp-relative STG, ST, or STM.  This is probably
          spilling an argument from a register out into a stack slot.
          This could be a user instruction, but if we haven't included
          any other suspicious instructions in the prologue, this
          could only be an initializing store, which isn't too bad to
          skip.  The consequences of not including arg-to-stack spills
          are more serious, though --- you don't see the proper values
          of the arguments.  */
      if ((save_link_state == 3 || save_link_state == 4)
          && ((instr[0] == 0x50      /* st %rA, D(%rX,%rB) */
               && (instr[1] & 0xf) == 0 /* %rX is zero, no index reg */
               && is_arg_reg ((instr[1] >> 4) & 0xf)
               && ((instr[2] >> 4) & 0xf) == frame_pointer_regidx)
              || (instr[0] == 0x90 /* stm %rA, %rB, D(%rC) */
                  && is_arg_reg ((instr[1] >> 4) & 0xf)
                  && is_arg_reg (instr[1] & 0xf)
                  && ((instr[2] >> 4) & 0xf) == frame_pointer_regidx)))
        {
          valid_prologue = 1;
          continue;
        }

      /* check for STD */
      if (instr[0] == 0x60 && (instr[2] >> 4) == 0xf)
	{
	  regidx = instr[1] >> 4;
	  if (regidx == 0 || regidx == 2)
	    varargs_state = 1;
	  if (fprs_saved[regidx])
	    {
	      good_prologue = 0;
	      break;
	    }
	  fprs_saved[regidx] = 1;
	  if (saved_regs)
	    {
	      save_reg_addr = orig_sp + (((instr[2] & 0xf) << 8) + instr[3]);
	      saved_regs[S390_FP0_REGNUM + regidx] = save_reg_addr;
	    }
	  valid_prologue = 1;
	  continue;
	}


      if (const_pool_state == 0)
	{

	  if (GDB_TARGET_IS_ESAME)
	    {
	      /* Check for larl CONST_POOL_REGIDX,offset on ESAME */
	      if ((instr[0] == 0xc0)
		  && (instr[1] == (CONST_POOL_REGIDX << 4)))
		{
		  const_pool_state = 2;
		  valid_prologue = 1;
		  continue;
		}
	    }
	  else
	    {
	      /* Check for BASR gpr13,gpr0 used to load constant pool pointer to r13 in old compiler */
	      if (instr[0] == 0xd && (instr[1] & 0xf) == 0
		  && ((instr[1] >> 4) == CONST_POOL_REGIDX))
		{
		  const_pool_state = 1;
		  valid_prologue = 1;
		  continue;
		}
	    }
	  /* Check for new fangled bras %r13,newpc to load new constant pool */
	  /* embedded in code, older pre abi compilers also emitted this stuff.  */
	  if ((instr[0] == 0xa7) && ((instr[1] & 0xf) == 0x5) &&
	      ((instr[1] >> 4) == CONST_POOL_REGIDX)
	      && ((instr[2] & 0x80) == 0))
	    {
	      const_pool_state = 2;
	      test_pc +=
		(((((instr[2] & 0xf) << 8) + instr[3]) << 1) - instrlen);
	      valid_prologue = 1;
	      continue;
	    }
	}
      /* Check for AGHI or AHI CONST_POOL_REGIDX,val */
      if (const_pool_state == 1 && (instr[0] == 0xa7) &&
	  ((GDB_TARGET_IS_ESAME &&
	    (instr[1] == ((CONST_POOL_REGIDX << 4) | 0xb))) ||
	   (instr[1] == ((CONST_POOL_REGIDX << 4) | 0xa))))
	{
	  const_pool_state = 2;
	  valid_prologue = 1;
	  continue;
	}
      /* Check for LGR or LR gprx,15 */
      if ((GDB_TARGET_IS_ESAME &&
	   instr[0] == 0xb9 && instr[1] == 0x04 && (instr[3] & 0xf) == 0xf) ||
	  (instr[0] == 0x18 && (instr[1] & 0xf) == 0xf))
	{
	  if (GDB_TARGET_IS_ESAME)
	    regidx = instr[3] >> 4;
	  else
	    regidx = instr[1] >> 4;
	  if (save_link_state == 0 && regidx != 0xb)
	    {
	      /* Almost defintely code for
	         decrementing the stack pointer 
	         ( i.e. a non leaf function 
	         or else leaf with locals ) */
	      save_link_regidx = regidx;
	      save_link_state = 1;
	      valid_prologue = 1;
	      continue;
	    }
	  /* We use this frame pointer for alloca
	     unfortunately we need to assume its gpr11
	     otherwise we would need a smarter prologue
	     walker. */
	  if (!frame_pointer_found && regidx == 0xb)
	    {
	      frame_pointer_regidx = 0xb;
	      frame_pointer_found = 1;
	      if (fextra_info)
		fextra_info->frame_pointer_saved_pc = test_pc;
	      valid_prologue = 1;
	      continue;
	    }
	}
      /* Check for AHI or AGHI gpr15,val */
      if (save_link_state == 1 && (instr[0] == 0xa7) &&
	  ((GDB_TARGET_IS_ESAME && (instr[1] == 0xfb)) || (instr[1] == 0xfa)))
	{
	  if (fextra_info)
	    fextra_info->stack_bought =
	      -extract_signed_integer (&instr[2], 2);
	  save_link_state = 3;
	  valid_prologue = 1;
	  continue;
	}
      /* Alternatively check for the complex construction for
         buying more than 32k of stack
         BRAS gprx,.+8
         long val
         s    %r15,0(%gprx)  gprx currently r1 */
      if ((save_link_state == 1) && (instr[0] == 0xa7)
	  && ((instr[1] & 0xf) == 0x5) && (instr[2] == 0)
	  && (instr[3] == 0x4) && ((instr[1] >> 4) != CONST_POOL_REGIDX))
	{
	  subtract_sp_regidx = instr[1] >> 4;
	  save_link_state = 2;
	  if (fextra_info)
	    target_read_memory (test_pc + instrlen,
				(char *) &fextra_info->stack_bought,
				sizeof (fextra_info->stack_bought));
	  test_pc += 4;
	  valid_prologue = 1;
	  continue;
	}
      if (save_link_state == 2 && instr[0] == 0x5b
	  && instr[1] == 0xf0 &&
	  instr[2] == (subtract_sp_regidx << 4) && instr[3] == 0)
	{
	  save_link_state = 3;
	  valid_prologue = 1;
	  continue;
	}
      /* check for LA gprx,offset(15) used for varargs */
      if ((instr[0] == 0x41) && ((instr[2] >> 4) == 0xf) &&
	  ((instr[1] & 0xf) == 0))
	{
	  /* some code uses gpr7 to point to outgoing args */
	  if (((instr[1] >> 4) == 7) && (save_link_state == 0) &&
	      ((instr[2] & 0xf) == 0)
	      && (instr[3] == S390_STACK_FRAME_OVERHEAD))
	    {
	      valid_prologue = 1;
	      continue;
	    }
	  if (varargs_state == 1)
	    {
	      varargs_state = 2;
	      valid_prologue = 1;
	      continue;
	    }
	}
      /* Check for a GOT load */

      if (GDB_TARGET_IS_ESAME)
	{
	  /* Check for larl  GOT_REGIDX, on ESAME */
	  if ((got_state == 0) && (instr[0] == 0xc0)
	      && (instr[1] == (GOT_REGIDX << 4)))
	    {
	      got_state = 2;
	      valid_prologue = 1;
	      continue;
	    }
	}
      else
	{
	  /* check for l GOT_REGIDX,x(CONST_POOL_REGIDX) */
	  if (got_state == 0 && const_pool_state == 2 && instr[0] == 0x58
	      && (instr[2] == (CONST_POOL_REGIDX << 4))
	      && ((instr[1] >> 4) == GOT_REGIDX))
	    {
	      got_state = 1;
              got_load_addr = test_pc;
              got_load_len = instrlen;
	      valid_prologue = 1;
	      continue;
	    }
	  /* Check for subsequent ar got_regidx,basr_regidx */
	  if (got_state == 1 && instr[0] == 0x1a &&
	      instr[1] == ((GOT_REGIDX << 4) | CONST_POOL_REGIDX))
	    {
	      got_state = 2;
	      valid_prologue = 1;
	      continue;
	    }
	}
    }
  while (valid_prologue && good_prologue);
  if (good_prologue)
    {
      /* If this function doesn't reference the global offset table,
         then the compiler may use r12 for other things.  If the last
         instruction we saw was a load of r12 from the constant pool,
         with no subsequent add to make the address PC-relative, then
         the load was probably a genuine body instruction; don't treat
         it as part of the prologue.  */
      if (got_state == 1
          && got_load_addr + got_load_len == test_pc)
        {
          test_pc = got_load_addr;
          instrlen = got_load_len;
        }
        
      good_prologue = (((const_pool_state == 0) || (const_pool_state == 2)) &&
		       ((save_link_state == 0) || (save_link_state == 4)) &&
		       ((varargs_state == 0) || (varargs_state == 2)));
    }
  if (fextra_info)
    {
      fextra_info->good_prologue = good_prologue;
      fextra_info->skip_prologue_function_start =
	(good_prologue ? test_pc : pc);
    }
  if (saved_regs)
    /* The SP's element of the saved_regs array holds the old SP,
       not the address at which it is saved.  */
    saved_regs[S390_SP_REGNUM] = orig_sp;
  return err;
}


int
s390_check_function_end (CORE_ADDR pc)
{
  bfd_byte instr[S390_MAX_INSTR_SIZE];
  disassemble_info info;
  int regidx, instrlen;

  info.read_memory_func = dis_asm_read_memory;
  instrlen = s390_readinstruction (instr, pc, &info);
  if (instrlen < 0)
    return -1;
  /* check for BR */
  if (instrlen != 2 || instr[0] != 07 || (instr[1] >> 4) != 0xf)
    return 0;
  regidx = instr[1] & 0xf;
  /* Check for LMG or LG */
  instrlen =
    s390_readinstruction (instr, pc - (GDB_TARGET_IS_ESAME ? 6 : 4), &info);
  if (instrlen < 0)
    return -1;
  if (GDB_TARGET_IS_ESAME)
    {

      if (instrlen != 6 || instr[0] != 0xeb || instr[5] != 0x4)
	return 0;
    }
  else if (instrlen != 4 || instr[0] != 0x98)
    {
      return 0;
    }
  if ((instr[2] >> 4) != 0xf)
    return 0;
  if (regidx == 14)
    return 1;
  instrlen = s390_readinstruction (instr, pc - (GDB_TARGET_IS_ESAME ? 12 : 8),
				   &info);
  if (instrlen < 0)
    return -1;
  if (GDB_TARGET_IS_ESAME)
    {
      /* Check for LG */
      if (instrlen != 6 || instr[0] != 0xe3 || instr[5] != 0x4)
	return 0;
    }
  else
    {
      /* Check for L */
      if (instrlen != 4 || instr[0] != 0x58)
	return 0;
    }
  if (instr[2] >> 4 != 0xf)
    return 0;
  if (instr[1] >> 4 != regidx)
    return 0;
  return 1;
}

static CORE_ADDR
s390_sniff_pc_function_start (CORE_ADDR pc, struct frame_info *fi)
{
  CORE_ADDR function_start, test_function_start;
  int loop_cnt, err, function_end;
  struct frame_extra_info fextra_info;
  function_start = get_pc_function_start (pc);

  if (function_start == 0)
    {
      test_function_start = pc;
      if (test_function_start & 1)
	return 0;		/* This has to be bogus */
      loop_cnt = 0;
      do
	{

	  err =
	    s390_get_frame_info (test_function_start, &fextra_info, fi, 1);
	  loop_cnt++;
	  test_function_start -= 2;
	  function_end = s390_check_function_end (test_function_start);
	}
      while (!(function_end == 1 || err || loop_cnt >= 4096 ||
	       (fextra_info.good_prologue)));
      if (fextra_info.good_prologue)
	function_start = fextra_info.function_start;
      else if (function_end == 1)
	function_start = test_function_start;
    }
  return function_start;
}



CORE_ADDR
s390_function_start (struct frame_info *fi)
{
  CORE_ADDR function_start = 0;

  if (fi->extra_info && fi->extra_info->initialised)
    function_start = fi->extra_info->function_start;
  else if (fi->pc)
    function_start = get_pc_function_start (fi->pc);
  return function_start;
}




int
s390_frameless_function_invocation (struct frame_info *fi)
{
  struct frame_extra_info fextra_info, *fextra_info_ptr;
  int frameless = 0;

  if (fi->next == NULL)		/* no may be frameless */
    {
      if (fi->extra_info)
	fextra_info_ptr = fi->extra_info;
      else
	{
	  fextra_info_ptr = &fextra_info;
	  s390_get_frame_info (s390_sniff_pc_function_start (fi->pc, fi),
			       fextra_info_ptr, fi, 1);
	}
      frameless = ((fextra_info_ptr->stack_bought == 0));
    }
  return frameless;

}


static int
s390_is_sigreturn (CORE_ADDR pc, struct frame_info *sighandler_fi,
		   CORE_ADDR *sregs, CORE_ADDR *sigcaller_pc)
{
  bfd_byte instr[S390_MAX_INSTR_SIZE];
  disassemble_info info;
  int instrlen;
  CORE_ADDR scontext;
  int retval = 0;
  CORE_ADDR orig_sp;
  CORE_ADDR temp_sregs;

  scontext = temp_sregs = 0;

  info.read_memory_func = dis_asm_read_memory;
  instrlen = s390_readinstruction (instr, pc, &info);
  if (sigcaller_pc)
    *sigcaller_pc = 0;
  if (((instrlen == S390_SYSCALL_SIZE) &&
       (instr[0] == S390_SYSCALL_OPCODE)) &&
      ((instr[1] == s390_NR_sigreturn) || (instr[1] == s390_NR_rt_sigreturn)))
    {
      if (sighandler_fi)
	{
	  if (s390_frameless_function_invocation (sighandler_fi))
	    orig_sp = sighandler_fi->frame;
	  else
	    orig_sp = ADDR_BITS_REMOVE ((CORE_ADDR)
					read_memory_integer (sighandler_fi->
							     frame,
							     S390_GPR_SIZE));
	  if (orig_sp && sigcaller_pc)
	    {
	      scontext = orig_sp + S390_SIGNAL_FRAMESIZE;
	      if (pc == scontext && instr[1] == s390_NR_rt_sigreturn)
		{
		  /* We got a new style rt_signal */
		  /* get address of read ucontext->uc_mcontext */
		  temp_sregs = orig_sp + (GDB_TARGET_IS_ESAME ?
					  S390X_UC_MCONTEXT_OFFSET :
					  S390_UC_MCONTEXT_OFFSET);
		}
	      else
		{
		  /* read sigcontext->sregs */
		  temp_sregs = ADDR_BITS_REMOVE ((CORE_ADDR)
						 read_memory_integer (scontext
								      +
								      (GDB_TARGET_IS_ESAME
								       ?
								       S390X_SIGCONTEXT_SREGS_OFFSET
								       :
								       S390_SIGCONTEXT_SREGS_OFFSET),
								      S390_GPR_SIZE));

		}
	      /* read sigregs->psw.addr */
	      *sigcaller_pc =
		ADDR_BITS_REMOVE ((CORE_ADDR)
				  read_memory_integer (temp_sregs +
						       REGISTER_BYTE
						       (S390_PC_REGNUM),
						       S390_PSW_ADDR_SIZE));
	    }
	}
      retval = 1;
    }
  if (sregs)
    *sregs = temp_sregs;
  return retval;
}

/*
  We need to do something better here but this will keep us out of trouble
  for the moment.
  For some reason the blockframe.c calls us with fi->next->fromleaf
  so this seems of little use to us. */
void
s390_init_frame_pc_first (int next_fromleaf, struct frame_info *fi)
{
  CORE_ADDR sigcaller_pc;

  fi->pc = 0;
  if (next_fromleaf)
    {
      fi->pc = ADDR_BITS_REMOVE (read_register (S390_RETADDR_REGNUM));
      /* fix signal handlers */
    }
  else if (fi->next && fi->next->pc)
    fi->pc = s390_frame_saved_pc_nofix (fi->next);
  if (fi->pc && fi->next && fi->next->frame &&
      s390_is_sigreturn (fi->pc, fi->next, NULL, &sigcaller_pc))
    {
      fi->pc = sigcaller_pc;
    }

}

void
s390_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
  fi->extra_info = frame_obstack_alloc (sizeof (struct frame_extra_info));
  if (fi->pc)
    s390_get_frame_info (s390_sniff_pc_function_start (fi->pc, fi),
			 fi->extra_info, fi, 1);
  else
    s390_memset_extra_info (fi->extra_info);
}

/* If saved registers of frame FI are not known yet, read and cache them.
   &FEXTRA_INFOP contains struct frame_extra_info; TDATAP can be NULL,
   in which case the framedata are read.  */

void
s390_frame_init_saved_regs (struct frame_info *fi)
{

  int quick;

  if (fi->saved_regs == NULL)
    {
      /* zalloc memsets the saved regs */
      frame_saved_regs_zalloc (fi);
      if (fi->pc)
	{
	  quick = (fi->extra_info && fi->extra_info->initialised
		   && fi->extra_info->good_prologue);
	  s390_get_frame_info (quick ? fi->extra_info->function_start :
			       s390_sniff_pc_function_start (fi->pc, fi),
			       fi->extra_info, fi, !quick);
	}
    }
}



CORE_ADDR
s390_frame_args_address (struct frame_info *fi)
{

  /* Apparently gdb already knows gdb_args_offset itself */
  return fi->frame;
}


static CORE_ADDR
s390_frame_saved_pc_nofix (struct frame_info *fi)
{
  if (fi->extra_info && fi->extra_info->saved_pc_valid)
    return fi->extra_info->saved_pc;

  if (deprecated_generic_find_dummy_frame (fi->pc, fi->frame))
    return generic_read_register_dummy (fi->pc, fi->frame, S390_PC_REGNUM);

  s390_frame_init_saved_regs (fi);
  if (fi->extra_info)
    {
      fi->extra_info->saved_pc_valid = 1;
      if (fi->extra_info->good_prologue
          && fi->saved_regs[S390_RETADDR_REGNUM])
        fi->extra_info->saved_pc
          = ADDR_BITS_REMOVE (read_memory_integer
                              (fi->saved_regs[S390_RETADDR_REGNUM],
                               S390_GPR_SIZE));
      else
        fi->extra_info->saved_pc
          = ADDR_BITS_REMOVE (read_register (S390_RETADDR_REGNUM));
      return fi->extra_info->saved_pc;
    }
  return 0;
}

CORE_ADDR
s390_frame_saved_pc (struct frame_info *fi)
{
  CORE_ADDR saved_pc = 0, sig_pc;

  if (fi->extra_info && fi->extra_info->sig_fixed_saved_pc_valid)
    return fi->extra_info->sig_fixed_saved_pc;
  saved_pc = s390_frame_saved_pc_nofix (fi);

  if (fi->extra_info)
    {
      fi->extra_info->sig_fixed_saved_pc_valid = 1;
      if (saved_pc)
	{
	  if (s390_is_sigreturn (saved_pc, fi, NULL, &sig_pc))
	    saved_pc = sig_pc;
	}
      fi->extra_info->sig_fixed_saved_pc = saved_pc;
    }
  return saved_pc;
}




/* We want backtraces out of signal handlers so we don't
   set thisframe->signal_handler_caller to 1 */

CORE_ADDR
s390_frame_chain (struct frame_info *thisframe)
{
  CORE_ADDR prev_fp = 0;

  if (deprecated_generic_find_dummy_frame (thisframe->pc, thisframe->frame))
    return generic_read_register_dummy (thisframe->pc, thisframe->frame,
                                        S390_SP_REGNUM);
  else
    {
      int sigreturn = 0;
      CORE_ADDR sregs = 0;
      struct frame_extra_info prev_fextra_info;

      memset (&prev_fextra_info, 0, sizeof (prev_fextra_info));
      if (thisframe->pc)
	{
	  CORE_ADDR saved_pc, sig_pc;

	  saved_pc = s390_frame_saved_pc_nofix (thisframe);
	  if (saved_pc)
	    {
	      if ((sigreturn =
		   s390_is_sigreturn (saved_pc, thisframe, &sregs, &sig_pc)))
		saved_pc = sig_pc;
	      s390_get_frame_info (s390_sniff_pc_function_start
				   (saved_pc, NULL), &prev_fextra_info, NULL,
				   1);
	    }
	}
      if (sigreturn)
	{
	  /* read sigregs,regs.gprs[11 or 15] */
	  prev_fp = read_memory_integer (sregs +
					 REGISTER_BYTE (S390_GP0_REGNUM +
							(prev_fextra_info.
							 frame_pointer_saved_pc
							 ? 11 : 15)),
					 S390_GPR_SIZE);
	  thisframe->extra_info->sigcontext = sregs;
	}
      else
	{
	  if (thisframe->saved_regs)
	    {
	      int regno;

              if (prev_fextra_info.frame_pointer_saved_pc
                  && thisframe->saved_regs[S390_FRAME_REGNUM])
                regno = S390_FRAME_REGNUM;
              else
                regno = S390_SP_REGNUM;

	      if (thisframe->saved_regs[regno])
                {
                  /* The SP's entry of `saved_regs' is special.  */
                  if (regno == S390_SP_REGNUM)
                    prev_fp = thisframe->saved_regs[regno];
                  else
                    prev_fp =
                      read_memory_integer (thisframe->saved_regs[regno],
                                           S390_GPR_SIZE);
                }
	    }
	}
    }
  return ADDR_BITS_REMOVE (prev_fp);
}

/*
  Whether struct frame_extra_info is actually needed I'll have to figure
  out as our frames are similar to rs6000 there is a possibility
  i386 dosen't need it. */



/* a given return value in `regbuf' with a type `valtype', extract and copy its
   value into `valbuf' */
void
s390_extract_return_value (struct type *valtype, char *regbuf, char *valbuf)
{
  /* floats and doubles are returned in fpr0. fpr's have a size of 8 bytes.
     We need to truncate the return value into float size (4 byte) if
     necessary. */
  int len = TYPE_LENGTH (valtype);

  if (TYPE_CODE (valtype) == TYPE_CODE_FLT)
    memcpy (valbuf, &regbuf[REGISTER_BYTE (S390_FP0_REGNUM)], len);
  else
    {
      int offset = 0;
      /* return value is copied starting from r2. */
      if (TYPE_LENGTH (valtype) < S390_GPR_SIZE)
	offset = S390_GPR_SIZE - TYPE_LENGTH (valtype);
      memcpy (valbuf,
	      regbuf + REGISTER_BYTE (S390_GP0_REGNUM + 2) + offset,
	      TYPE_LENGTH (valtype));
    }
}


static char *
s390_promote_integer_argument (struct type *valtype, char *valbuf,
			       char *reg_buff, int *arglen)
{
  char *value = valbuf;
  int len = TYPE_LENGTH (valtype);

  if (len < S390_GPR_SIZE)
    {
      /* We need to upgrade this value to a register to pass it correctly */
      int idx, diff = S390_GPR_SIZE - len, negative =
	(!TYPE_UNSIGNED (valtype) && value[0] & 0x80);
      for (idx = 0; idx < S390_GPR_SIZE; idx++)
	{
	  reg_buff[idx] = (idx < diff ? (negative ? 0xff : 0x0) :
			   value[idx - diff]);
	}
      value = reg_buff;
      *arglen = S390_GPR_SIZE;
    }
  else
    {
      if (len & (S390_GPR_SIZE - 1))
	{
	  fprintf_unfiltered (gdb_stderr,
			      "s390_promote_integer_argument detected an argument not "
			      "a multiple of S390_GPR_SIZE & greater than S390_GPR_SIZE "
			      "we might not deal with this correctly.\n");
	}
      *arglen = len;
    }

  return (value);
}

void
s390_store_return_value (struct type *valtype, char *valbuf)
{
  int arglen;
  char *reg_buff = alloca (max (S390_FPR_SIZE, REGISTER_SIZE)), *value;

  if (TYPE_CODE (valtype) == TYPE_CODE_FLT)
    {
      if (TYPE_LENGTH (valtype) == 4
          || TYPE_LENGTH (valtype) == 8)
        write_register_bytes (REGISTER_BYTE (S390_FP0_REGNUM), valbuf,
                              TYPE_LENGTH (valtype));
      else
        error ("GDB is unable to return `long double' values "
               "on this architecture.");
    }
  else
    {
      value =
	s390_promote_integer_argument (valtype, valbuf, reg_buff, &arglen);
      /* Everything else is returned in GPR2 and up. */
      write_register_bytes (REGISTER_BYTE (S390_GP0_REGNUM + 2), value,
			    arglen);
    }
}
static int
gdb_print_insn_s390 (bfd_vma memaddr, disassemble_info * info)
{
  bfd_byte instrbuff[S390_MAX_INSTR_SIZE];
  int instrlen, cnt;

  instrlen = s390_readinstruction (instrbuff, (CORE_ADDR) memaddr, info);
  if (instrlen < 0)
    {
      (*info->memory_error_func) (instrlen, memaddr, info);
      return -1;
    }
  for (cnt = 0; cnt < instrlen; cnt++)
    info->fprintf_func (info->stream, "%02X ", instrbuff[cnt]);
  for (cnt = instrlen; cnt < S390_MAX_INSTR_SIZE; cnt++)
    info->fprintf_func (info->stream, "   ");
  instrlen = print_insn_s390 (memaddr, info);
  return instrlen;
}



/* Not the most efficent code in the world */
int
s390_fp_regnum (void)
{
  int regno = S390_SP_REGNUM;
  struct frame_extra_info fextra_info;

  CORE_ADDR pc = ADDR_BITS_REMOVE (read_register (S390_PC_REGNUM));

  s390_get_frame_info (s390_sniff_pc_function_start (pc, NULL), &fextra_info,
		       NULL, 1);
  if (fextra_info.frame_pointer_saved_pc)
    regno = S390_FRAME_REGNUM;
  return regno;
}

CORE_ADDR
s390_read_fp (void)
{
  return read_register (s390_fp_regnum ());
}


static void
s390_pop_frame_regular (struct frame_info *frame)
{
  int regnum;

  write_register (S390_PC_REGNUM, FRAME_SAVED_PC (frame));

  /* Restore any saved registers.  */
  if (frame->saved_regs)
    {
      for (regnum = 0; regnum < NUM_REGS; regnum++)
        if (frame->saved_regs[regnum] != 0)
          {
            ULONGEST value;
            
            value = read_memory_unsigned_integer (frame->saved_regs[regnum],
                                                  REGISTER_RAW_SIZE (regnum));
            write_register (regnum, value);
          }

      /* Actually cut back the stack.  Remember that the SP's element of
         saved_regs is the old SP itself, not the address at which it is
         saved.  */
      write_register (S390_SP_REGNUM, frame->saved_regs[S390_SP_REGNUM]);
    }

  /* Throw away any cached frame information.  */
  flush_cached_frames ();
}


/* Destroy the innermost (Top-Of-Stack) stack frame, restoring the 
   machine state that was in effect before the frame was created. 
   Used in the contexts of the "return" command, and of 
   target function calls from the debugger.  */
void
s390_pop_frame (void)
{
  /* This function checks for and handles generic dummy frames, and
     calls back to our function for ordinary frames.  */
  generic_pop_current_frame (s390_pop_frame_regular);
}


/* Return non-zero if TYPE is an integer-like type, zero otherwise.
   "Integer-like" types are those that should be passed the way
   integers are: integers, enums, ranges, characters, and booleans.  */
static int
is_integer_like (struct type *type)
{
  enum type_code code = TYPE_CODE (type);

  return (code == TYPE_CODE_INT
          || code == TYPE_CODE_ENUM
          || code == TYPE_CODE_RANGE
          || code == TYPE_CODE_CHAR
          || code == TYPE_CODE_BOOL);
}


/* Return non-zero if TYPE is a pointer-like type, zero otherwise.
   "Pointer-like" types are those that should be passed the way
   pointers are: pointers and references.  */
static int
is_pointer_like (struct type *type)
{
  enum type_code code = TYPE_CODE (type);

  return (code == TYPE_CODE_PTR
          || code == TYPE_CODE_REF);
}


/* Return non-zero if TYPE is a `float singleton' or `double
   singleton', zero otherwise.

   A `T singleton' is a struct type with one member, whose type is
   either T or a `T singleton'.  So, the following are all float
   singletons:

   struct { float x };
   struct { struct { float x; } x; };
   struct { struct { struct { float x; } x; } x; };

   ... and so on.

   WHY THE HECK DO WE CARE ABOUT THIS???  Well, it turns out that GCC
   passes all float singletons and double singletons as if they were
   simply floats or doubles.  This is *not* what the ABI says it
   should do.  */
static int
is_float_singleton (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_STRUCT
          && TYPE_NFIELDS (type) == 1
          && (TYPE_CODE (TYPE_FIELD_TYPE (type, 0)) == TYPE_CODE_FLT
              || is_float_singleton (TYPE_FIELD_TYPE (type, 0))));
}


/* Return non-zero if TYPE is a struct-like type, zero otherwise.
   "Struct-like" types are those that should be passed as structs are:
   structs and unions.

   As an odd quirk, not mentioned in the ABI, GCC passes float and
   double singletons as if they were a plain float, double, etc.  (The
   corresponding union types are handled normally.)  So we exclude
   those types here.  *shrug* */
static int
is_struct_like (struct type *type)
{
  enum type_code code = TYPE_CODE (type);

  return (code == TYPE_CODE_UNION
          || (code == TYPE_CODE_STRUCT && ! is_float_singleton (type)));
}


/* Return non-zero if TYPE is a float-like type, zero otherwise.
   "Float-like" types are those that should be passed as
   floating-point values are.

   You'd think this would just be floats, doubles, long doubles, etc.
   But as an odd quirk, not mentioned in the ABI, GCC passes float and
   double singletons as if they were a plain float, double, etc.  (The
   corresponding union types are handled normally.)  So we exclude
   those types here.  *shrug* */
static int
is_float_like (struct type *type)
{
  return (TYPE_CODE (type) == TYPE_CODE_FLT
          || is_float_singleton (type));
}


/* Return non-zero if TYPE is considered a `DOUBLE_OR_FLOAT', as
   defined by the parameter passing conventions described in the
   "GNU/Linux for S/390 ELF Application Binary Interface Supplement".
   Otherwise, return zero.  */
static int
is_double_or_float (struct type *type)
{
  return (is_float_like (type)
          && (TYPE_LENGTH (type) == 4
              || TYPE_LENGTH (type) == 8));
}


/* Return non-zero if TYPE is considered a `SIMPLE_ARG', as defined by
   the parameter passing conventions described in the "GNU/Linux for
   S/390 ELF Application Binary Interface Supplement".  Return zero
   otherwise.  */
static int
is_simple_arg (struct type *type)
{
  unsigned length = TYPE_LENGTH (type);

  /* This is almost a direct translation of the ABI's language, except
     that we have to exclude 8-byte structs; those are DOUBLE_ARGs.  */
  return ((is_integer_like (type) && length <= 4)
          || is_pointer_like (type)
          || (is_struct_like (type) && length != 8)
          || (is_float_like (type) && length == 16));
}


/* Return non-zero if TYPE should be passed as a pointer to a copy,
   zero otherwise.  TYPE must be a SIMPLE_ARG, as recognized by
   `is_simple_arg'.  */
static int
pass_by_copy_ref (struct type *type)
{
  unsigned length = TYPE_LENGTH (type);

  return ((is_struct_like (type) && length != 1 && length != 2 && length != 4)
          || (is_float_like (type) && length == 16));
}


/* Return ARG, a `SIMPLE_ARG', sign-extended or zero-extended to a full
   word as required for the ABI.  */
static LONGEST
extend_simple_arg (struct value *arg)
{
  struct type *type = VALUE_TYPE (arg);

  /* Even structs get passed in the least significant bits of the
     register / memory word.  It's not really right to extract them as
     an integer, but it does take care of the extension.  */
  if (TYPE_UNSIGNED (type))
    return extract_unsigned_integer (VALUE_CONTENTS (arg),
                                     TYPE_LENGTH (type));
  else
    return extract_signed_integer (VALUE_CONTENTS (arg),
                                   TYPE_LENGTH (type));
}


/* Return non-zero if TYPE is a `DOUBLE_ARG', as defined by the
   parameter passing conventions described in the "GNU/Linux for S/390
   ELF Application Binary Interface Supplement".  Return zero
   otherwise.  */
static int
is_double_arg (struct type *type)
{
  unsigned length = TYPE_LENGTH (type);

  return ((is_integer_like (type)
           || is_struct_like (type))
          && length == 8);
}


/* Round ADDR up to the next N-byte boundary.  N must be a power of
   two.  */
static CORE_ADDR
round_up (CORE_ADDR addr, int n)
{
  /* Check that N is really a power of two.  */
  gdb_assert (n && (n & (n-1)) == 0);
  return ((addr + n - 1) & -n);
}


/* Round ADDR down to the next N-byte boundary.  N must be a power of
   two.  */
static CORE_ADDR
round_down (CORE_ADDR addr, int n)
{
  /* Check that N is really a power of two.  */
  gdb_assert (n && (n & (n-1)) == 0);
  return (addr & -n);
}


/* Return the alignment required by TYPE.  */
static int
alignment_of (struct type *type)
{
  int alignment;

  if (is_integer_like (type)
      || is_pointer_like (type)
      || TYPE_CODE (type) == TYPE_CODE_FLT)
    alignment = TYPE_LENGTH (type);
  else if (TYPE_CODE (type) == TYPE_CODE_STRUCT
           || TYPE_CODE (type) == TYPE_CODE_UNION)
    {
      int i;

      alignment = 1;
      for (i = 0; i < TYPE_NFIELDS (type); i++)
        {
          int field_alignment = alignment_of (TYPE_FIELD_TYPE (type, i));

          if (field_alignment > alignment)
            alignment = field_alignment;
        }
    }
  else
    alignment = 1;

  /* Check that everything we ever return is a power of two.  Lots of
     code doesn't want to deal with aligning things to arbitrary
     boundaries.  */
  gdb_assert ((alignment & (alignment - 1)) == 0);

  return alignment;
}


/* Put the actual parameter values pointed to by ARGS[0..NARGS-1] in
   place to be passed to a function, as specified by the "GNU/Linux
   for S/390 ELF Application Binary Interface Supplement".

   SP is the current stack pointer.  We must put arguments, links,
   padding, etc. whereever they belong, and return the new stack
   pointer value.
   
   If STRUCT_RETURN is non-zero, then the function we're calling is
   going to return a structure by value; STRUCT_ADDR is the address of
   a block we've allocated for it on the stack.

   Our caller has taken care of any type promotions needed to satisfy
   prototypes or the old K&R argument-passing rules.  */
CORE_ADDR
s390_push_arguments (int nargs, struct value **args, CORE_ADDR sp,
		     int struct_return, CORE_ADDR struct_addr)
{
  int i;
  int pointer_size = (TARGET_PTR_BIT / TARGET_CHAR_BIT);

  /* The number of arguments passed by reference-to-copy.  */
  int num_copies;

  /* If the i'th argument is passed as a reference to a copy, then
     copy_addr[i] is the address of the copy we made.  */
  CORE_ADDR *copy_addr = alloca (nargs * sizeof (CORE_ADDR));

  /* Build the reference-to-copy area.  */
  num_copies = 0;
  for (i = 0; i < nargs; i++)
    {
      struct value *arg = args[i];
      struct type *type = VALUE_TYPE (arg);
      unsigned length = TYPE_LENGTH (type);

      if (is_simple_arg (type)
          && pass_by_copy_ref (type))
        {
          sp -= length;
          sp = round_down (sp, alignment_of (type));
          write_memory (sp, VALUE_CONTENTS (arg), length);
          copy_addr[i] = sp;
          num_copies++;
        }
    }

  /* Reserve space for the parameter area.  As a conservative
     simplification, we assume that everything will be passed on the
     stack.  */
  {
    int i;

    for (i = 0; i < nargs; i++)
      {
        struct value *arg = args[i];
        struct type *type = VALUE_TYPE (arg);
        int length = TYPE_LENGTH (type);
        
        sp = round_down (sp, alignment_of (type));

        /* SIMPLE_ARG values get extended to 32 bits.  Assume every
           argument is.  */
        if (length < 4) length = 4;
        sp -= length;
      }
  }

  /* Include space for any reference-to-copy pointers.  */
  sp = round_down (sp, pointer_size);
  sp -= num_copies * pointer_size;
    
  /* After all that, make sure it's still aligned on an eight-byte
     boundary.  */
  sp = round_down (sp, 8);

  /* Finally, place the actual parameters, working from SP towards
     higher addresses.  The code above is supposed to reserve enough
     space for this.  */
  {
    int fr = 0;
    int gr = 2;
    CORE_ADDR starg = sp;

    for (i = 0; i < nargs; i++)
      {
        struct value *arg = args[i];
        struct type *type = VALUE_TYPE (arg);
        
        if (is_double_or_float (type)
            && fr <= 2)
          {
            /* When we store a single-precision value in an FP register,
               it occupies the leftmost bits.  */
            write_register_bytes (REGISTER_BYTE (S390_FP0_REGNUM + fr),
                                  VALUE_CONTENTS (arg),
                                  TYPE_LENGTH (type));
            fr += 2;
          }
        else if (is_simple_arg (type)
                 && gr <= 6)
          {
            /* Do we need to pass a pointer to our copy of this
               argument?  */
            if (pass_by_copy_ref (type))
              write_register (S390_GP0_REGNUM + gr, copy_addr[i]);
            else
              write_register (S390_GP0_REGNUM + gr, extend_simple_arg (arg));

            gr++;
          }
        else if (is_double_arg (type)
                 && gr <= 5)
          {
            write_register_gen (S390_GP0_REGNUM + gr,
                                VALUE_CONTENTS (arg));
            write_register_gen (S390_GP0_REGNUM + gr + 1,
                                VALUE_CONTENTS (arg) + 4);
            gr += 2;
          }
        else
          {
            /* The `OTHER' case.  */
            enum type_code code = TYPE_CODE (type);
            unsigned length = TYPE_LENGTH (type);
            
            /* If we skipped r6 because we couldn't fit a DOUBLE_ARG
               in it, then don't go back and use it again later.  */
            if (is_double_arg (type) && gr == 6)
              gr = 7;

            if (is_simple_arg (type))
              {
                /* Simple args are always either extended to 32 bits,
                   or pointers.  */
                starg = round_up (starg, 4);

                /* Do we need to pass a pointer to our copy of this
                   argument?  */
                if (pass_by_copy_ref (type))
                  write_memory_signed_integer (starg, pointer_size,
                                               copy_addr[i]);
                else
                  /* Simple args are always extended to 32 bits.  */
                  write_memory_signed_integer (starg, 4,
                                               extend_simple_arg (arg));
                starg += 4;
              }
            else
              {
                /* You'd think we should say:
                   starg = round_up (starg, alignment_of (type));
                   Unfortunately, GCC seems to simply align the stack on
                   a four-byte boundary, even when passing doubles.  */
                starg = round_up (starg, 4);
                write_memory (starg, VALUE_CONTENTS (arg), length);
                starg += length;
              }
          }
      }
  }

  /* Allocate the standard frame areas: the register save area, the
     word reserved for the compiler (which seems kind of meaningless),
     and the back chain pointer.  */
  sp -= 96;

  /* Write the back chain pointer into the first word of the stack
     frame.  This will help us get backtraces from within functions
     called from GDB.  */
  write_memory_unsigned_integer (sp, (TARGET_PTR_BIT / TARGET_CHAR_BIT),
                                 read_fp ());

  return sp;
}


static int
s390_use_struct_convention (int gcc_p, struct type *value_type)
{
  enum type_code code = TYPE_CODE (value_type);

  return (code == TYPE_CODE_STRUCT
          || code == TYPE_CODE_UNION);
}


/* Return the GDB type object for the "standard" data type
   of data in register N.  */
struct type *
s390_register_virtual_type (int regno)
{
  if (S390_FP0_REGNUM <= regno && regno < S390_FP0_REGNUM + S390_NUM_FPRS)
    return builtin_type_double;
  else
    return builtin_type_int;
}


struct type *
s390x_register_virtual_type (int regno)
{
  return (regno == S390_FPC_REGNUM) ||
    (regno >= S390_FIRST_ACR && regno <= S390_LAST_ACR) ? builtin_type_int :
    (regno >= S390_FP0_REGNUM) ? builtin_type_double : builtin_type_long;
}



void
s390_store_struct_return (CORE_ADDR addr, CORE_ADDR sp)
{
  write_register (S390_GP0_REGNUM + 2, addr);
}



const static unsigned char *
s390_breakpoint_from_pc (CORE_ADDR *pcptr, int *lenptr)
{
  static unsigned char breakpoint[] = { 0x0, 0x1 };

  *lenptr = sizeof (breakpoint);
  return breakpoint;
}

/* Advance PC across any function entry prologue instructions to reach some
   "real" code.  */
CORE_ADDR
s390_skip_prologue (CORE_ADDR pc)
{
  struct frame_extra_info fextra_info;

  s390_get_frame_info (pc, &fextra_info, NULL, 1);
  return fextra_info.skip_prologue_function_start;
}

/* Immediately after a function call, return the saved pc.
   Can't go through the frames for this because on some machines
   the new frame is not set up until the new function executes
   some instructions.  */
CORE_ADDR
s390_saved_pc_after_call (struct frame_info *frame)
{
  return ADDR_BITS_REMOVE (read_register (S390_RETADDR_REGNUM));
}

static CORE_ADDR
s390_addr_bits_remove (CORE_ADDR addr)
{
  return (addr) & 0x7fffffff;
}


static CORE_ADDR
s390_push_return_address (CORE_ADDR pc, CORE_ADDR sp)
{
  write_register (S390_RETADDR_REGNUM, CALL_DUMMY_ADDRESS ());
  return sp;
}

struct gdbarch *
s390_gdbarch_init (struct gdbarch_info info, struct gdbarch_list *arches)
{
  static LONGEST s390_call_dummy_words[] = { 0 };
  struct gdbarch *gdbarch;
  struct gdbarch_tdep *tdep;
  int elf_flags;

  /* First see if there is already a gdbarch that can satisfy the request.  */
  arches = gdbarch_list_lookup_by_info (arches, &info);
  if (arches != NULL)
    return arches->gdbarch;

  /* None found: is the request for a s390 architecture? */
  if (info.bfd_arch_info->arch != bfd_arch_s390)
    return NULL;		/* No; then it's not for us.  */

  /* Yes: create a new gdbarch for the specified machine type.  */
  gdbarch = gdbarch_alloc (&info, NULL);

  set_gdbarch_believe_pcc_promotion (gdbarch, 0);
  set_gdbarch_char_signed (gdbarch, 0);

  set_gdbarch_frame_args_skip (gdbarch, 0);
  set_gdbarch_frame_args_address (gdbarch, s390_frame_args_address);
  set_gdbarch_frame_chain (gdbarch, s390_frame_chain);
  set_gdbarch_frame_init_saved_regs (gdbarch, s390_frame_init_saved_regs);
  set_gdbarch_frame_locals_address (gdbarch, s390_frame_args_address);
  /* We can't do this */
  set_gdbarch_frame_num_args (gdbarch, frame_num_args_unknown);
  set_gdbarch_store_struct_return (gdbarch, s390_store_struct_return);
  set_gdbarch_deprecated_extract_return_value (gdbarch, s390_extract_return_value);
  set_gdbarch_deprecated_store_return_value (gdbarch, s390_store_return_value);
  /* Amount PC must be decremented by after a breakpoint.
     This is often the number of bytes in BREAKPOINT
     but not always.  */
  set_gdbarch_decr_pc_after_break (gdbarch, 2);
  set_gdbarch_pop_frame (gdbarch, s390_pop_frame);
  /* Stack grows downward.  */
  set_gdbarch_inner_than (gdbarch, core_addr_lessthan);
  /* Offset from address of function to start of its code.
     Zero on most machines.  */
  set_gdbarch_function_start_offset (gdbarch, 0);
  set_gdbarch_max_register_raw_size (gdbarch, 8);
  set_gdbarch_max_register_virtual_size (gdbarch, 8);
  set_gdbarch_breakpoint_from_pc (gdbarch, s390_breakpoint_from_pc);
  set_gdbarch_skip_prologue (gdbarch, s390_skip_prologue);
  set_gdbarch_init_extra_frame_info (gdbarch, s390_init_extra_frame_info);
  set_gdbarch_init_frame_pc_first (gdbarch, s390_init_frame_pc_first);
  set_gdbarch_read_fp (gdbarch, s390_read_fp);
  /* This function that tells us whether the function invocation represented
     by FI does not have a frame on the stack associated with it.  If it
     does not, FRAMELESS is set to 1, else 0.  */
  set_gdbarch_frameless_function_invocation (gdbarch,
					     s390_frameless_function_invocation);
  /* Return saved PC from a frame */
  set_gdbarch_frame_saved_pc (gdbarch, s390_frame_saved_pc);
  /* FRAME_CHAIN takes a frame's nominal address
     and produces the frame's chain-pointer. */
  set_gdbarch_frame_chain (gdbarch, s390_frame_chain);
  set_gdbarch_saved_pc_after_call (gdbarch, s390_saved_pc_after_call);
  set_gdbarch_register_byte (gdbarch, s390_register_byte);
  set_gdbarch_pc_regnum (gdbarch, S390_PC_REGNUM);
  set_gdbarch_sp_regnum (gdbarch, S390_SP_REGNUM);
  set_gdbarch_fp_regnum (gdbarch, S390_FP_REGNUM);
  set_gdbarch_fp0_regnum (gdbarch, S390_FP0_REGNUM);
  set_gdbarch_num_regs (gdbarch, S390_NUM_REGS);
  set_gdbarch_cannot_fetch_register (gdbarch, s390_cannot_fetch_register);
  set_gdbarch_cannot_store_register (gdbarch, s390_cannot_fetch_register);
  set_gdbarch_get_saved_register (gdbarch, generic_unwind_get_saved_register);
  set_gdbarch_use_struct_convention (gdbarch, s390_use_struct_convention);
  set_gdbarch_frame_chain_valid (gdbarch, func_frame_chain_valid);
  set_gdbarch_register_name (gdbarch, s390_register_name);
  set_gdbarch_stab_reg_to_regnum (gdbarch, s390_stab_reg_to_regnum);
  set_gdbarch_dwarf_reg_to_regnum (gdbarch, s390_stab_reg_to_regnum);
  set_gdbarch_dwarf2_reg_to_regnum (gdbarch, s390_stab_reg_to_regnum);
  set_gdbarch_deprecated_extract_struct_value_address
    (gdbarch, generic_cannot_extract_struct_value_address);

  /* Parameters for inferior function calls.  */
  set_gdbarch_call_dummy_p (gdbarch, 1);
  set_gdbarch_use_generic_dummy_frames (gdbarch, 1);
  set_gdbarch_call_dummy_length (gdbarch, 0);
  set_gdbarch_call_dummy_location (gdbarch, AT_ENTRY_POINT);
  set_gdbarch_call_dummy_address (gdbarch, entry_point_address);
  set_gdbarch_call_dummy_start_offset (gdbarch, 0);
  set_gdbarch_pc_in_call_dummy (gdbarch, pc_in_call_dummy_at_entry_point);
  set_gdbarch_push_dummy_frame (gdbarch, generic_push_dummy_frame);
  set_gdbarch_push_arguments (gdbarch, s390_push_arguments);
  set_gdbarch_save_dummy_frame_tos (gdbarch, generic_save_dummy_frame_tos);
  set_gdbarch_call_dummy_breakpoint_offset_p (gdbarch, 1);
  set_gdbarch_call_dummy_breakpoint_offset (gdbarch, 0);
  set_gdbarch_call_dummy_stack_adjust_p (gdbarch, 0);
  set_gdbarch_fix_call_dummy (gdbarch, generic_fix_call_dummy);
  set_gdbarch_push_return_address (gdbarch, s390_push_return_address);
  set_gdbarch_sizeof_call_dummy_words (gdbarch,
                                       sizeof (s390_call_dummy_words));
  set_gdbarch_call_dummy_words (gdbarch, s390_call_dummy_words);
  set_gdbarch_coerce_float_to_double (gdbarch,
                                      standard_coerce_float_to_double);

  switch (info.bfd_arch_info->mach)
    {
    case bfd_mach_s390_31:
      set_gdbarch_register_size (gdbarch, 4);
      set_gdbarch_register_raw_size (gdbarch, s390_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, s390_register_raw_size);
      set_gdbarch_register_virtual_type (gdbarch, s390_register_virtual_type);

      set_gdbarch_addr_bits_remove (gdbarch, s390_addr_bits_remove);
      set_gdbarch_register_bytes (gdbarch, S390_REGISTER_BYTES);
      break;
    case bfd_mach_s390_64:
      set_gdbarch_register_size (gdbarch, 8);
      set_gdbarch_register_raw_size (gdbarch, s390x_register_raw_size);
      set_gdbarch_register_virtual_size (gdbarch, s390x_register_raw_size);
      set_gdbarch_register_virtual_type (gdbarch,
					 s390x_register_virtual_type);

      set_gdbarch_long_bit (gdbarch, 64);
      set_gdbarch_long_long_bit (gdbarch, 64);
      set_gdbarch_ptr_bit (gdbarch, 64);
      set_gdbarch_register_bytes (gdbarch, S390X_REGISTER_BYTES);
      break;
    }

  return gdbarch;
}



void
_initialize_s390_tdep (void)
{

  /* Hook us into the gdbarch mechanism.  */
  register_gdbarch_init (bfd_arch_s390, s390_gdbarch_init);
  if (!tm_print_insn)		/* Someone may have already set it */
    tm_print_insn = gdb_print_insn_s390;
}

#endif /* GDBSERVER */
