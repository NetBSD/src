/* Native-dependent code for BSD Unix running on i386's, for GDB.
   Copyright  1997 Free Software Foundation, Inc.
   Contributed by Jonathan Stone(jonathan@dsg.stanford.edu) at Stanford

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
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: mipsnbsd-nat.c,v 1.1 1997/10/19 20:52:57 jonathan Exp $
*/

#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/user.h>
#include <sys/ptrace.h>
#include <machine/reg.h>
#include <setjmp.h>

#include "defs.h"
#include "inferior.h"
#include "gdbcore.h"

#define JB_ELEMENT_SIZE 4

void
fetch_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  bzero(&inferior_registers, sizeof(inferior_registers));
  ptrace (PT_GETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  memcpy (&registers[REGISTER_BYTE (0)], &inferior_registers, 4*71);

  bzero(&inferior_fp_registers, sizeof(inferior_fp_registers));
  ptrace (PT_GETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);

  memcpy (&registers[REGISTER_BYTE (18)], &inferior_fp_registers, 8*12+4*3);

  registers_fetched ();
}

void
store_inferior_registers (regno)
     int regno;
{
  struct reg inferior_registers;
  struct fpreg inferior_fp_registers;

  memcpy (&inferior_registers, &registers[REGISTER_BYTE (0)], 4*71);

  ptrace (PT_SETREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_registers, 0);

  memcpy (&inferior_fp_registers, &registers[REGISTER_BYTE (18)], 8*12+4*3);

  ptrace (PT_SETFPREGS, inferior_pid,
	  (PTRACE_ARG3_TYPE) &inferior_fp_registers, 0);
}


/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into PC.
   This routine returns true on success. */

int
get_longjmp_target(pc)
     CORE_ADDR *pc;
{
  CORE_ADDR jb_addr;
  char buf[TARGET_PTR_BIT / TARGET_CHAR_BIT];

  jb_addr = read_register (A0_REGNUM);

  if (target_read_memory (jb_addr + JB_PC * JB_ELEMENT_SIZE, buf,
			  TARGET_PTR_BIT / TARGET_CHAR_BIT))
    return 0;

  *pc = extract_address (buf, TARGET_PTR_BIT / TARGET_CHAR_BIT);

  return 1;
}

/* XXX - Add this to machine/regs.h instead? */
struct md_core {
	struct reg intreg;
	struct fpreg freg;
};


/* Extract the register values out of the core file and store
   them where `read_register' will find them.

   CORE_REG_SECT points to the register values themselves, read into memory.
   CORE_REG_SIZE is the size of that area.
   WHICH says which set of registers we are handling (0 = int, 2 = float
         on machines where they are discontiguous).
   REG_ADDR is the offset from u.u_ar0 to the register values relative to
            core_reg_sect.  This is used with old-fashioned core files to
	    locate the registers in a large upage-plus-stack ".reg" section.
	    Original upage address X is at location core_reg_sect+x+reg_addr.
 */
void
fetch_core_registers (core_reg_sect, core_reg_size, which, ignore)
  char *core_reg_sect;
  unsigned core_reg_size;
  int which;
  unsigned int ignore;	/* reg addr, unused in this version */
{
  struct md_core *core_reg;

  core_reg = (struct md_core *)core_reg_sect;

  if (which == 0) {
    /* Integer registers */
    memcpy(&registers[REGISTER_BYTE (0)],
	   &core_reg->intreg, sizeof(struct reg));
  }

  else if (which == 2) {
    /* Floating point registers */
    memcpy(&registers[REGISTER_BYTE (FP0_REGNUM)],
	   &core_reg->freg, sizeof(struct fpreg));
  }
}


/* Get registers from a kernel crash dump. 
   FIXME: NetBSD 1.3 does not produce kernel crashdumps.  */
void
fetch_kcore_registers(pcb)
struct pcb *pcb;
{
  int i, *ip, tmp=0;
  u_long sp;

#if 0
  supply_register(SP_REGNUM, (char *)&pcb->pcb_sp);
  supply_register(PC_REGNUM, (char *)&pcb->pcb_pc);
#endif

  /* The kernel does not use the FPU, so ignore it. */
  registers_fetched ();
}

#ifdef __NetBSD__
/* Non-zero if we just simulated a single-step ptrace call.  This is
   needed because we cannot remove the breakpoints in the inferior
   process until after the `wait' in `wait_for_inferior'.  Used for
   4.4bsd for mips, where the kernel does not emulate single-step. */

int one_stepped;
CORE_ADDR target_addr;		/* Branch target offset, if we have a
				   breakpoint there... */
CORE_ADDR step_addr;		/* Offset of instruction after instruction
				   to be stepped, if we have a breakpoint
				   there. */
long step_cache [3];		/* Cache for instructions wiped out by
				   step breakpoint(s)... */

/* single_step() is called just before we want to resume the inferior,
   if we want to single-step it but there is no hardware or kernel single-step
   support (as on 4.4bsd for pmax).  We find all the possible targets of the
   coming instruction and breakpoint them.

   single_step is also called just after the inferior stops.  If we had
   set up a simulated single-step, we undo our damage. */

/* thoughts:

   For the current instruction, check to see if we're in a delay slot.
   If we are, the next instruction executed will either be the target of
   the branch or jump instruction preceding the current instruction, or
   it will be the instruction following the current instruction.   If
   we are not, then the next instruction executed will either be the
   instruction following the current instruction, or the instruction
   following that (if the current instruction is a branch likely instruction
   and the branch is not taken).

   So, if we are in a delay slot then we set a breakpoint for the target
   of the preceding instruction.   Unless the preceding instruction was
   a jump instruction (only jumps are unconditional), we also set a break-
   point at the instruction following the current one and the instruction
   following that.   Setting two breakpoints after the current instruction
   is cheaper and easier than figuring out whether the current instruction
   is a branch likely instruction. */

void
single_step (ignore)
     int ignore; /* pid, but we don't need it */
{
  CORE_ADDR pc;
  CORE_ADDR epc;
  CORE_ADDR next;
  unsigned long cause;
  unsigned long delay_instruction;

  if (!one_stepped)
    {
      epc = read_register (PC_REGNUM);
      cause = read_register (CAUSE_REGNUM);
      pc = epc;
#define CAUSE_BD	0x80000000UL
      if (cause & CAUSE_BD)
	pc += 4;
      next = pc + 4;
      target_addr = 0;
      step_addr = next;

      if (cause & CAUSE_BD)
	{
          delay_instruction =
	      read_memory_integer (epc, sizeof(delay_instruction));

/* Main instruction opcode mask - top six bits... */
#define MI_IMASK	0xfc000000UL
/* Opcode for ``special'' instructions.... */
#define MI_SPECIAL	0x0
/* Opcode for ``regimm'' instructions... */
#define MI_REGIMM	0x04000000
/* Mask for special instruction opcodes... */
#if 0
#  define MS_IMASK	0x37	/* XXX Mellon's original, broken? */
#else
#  define MS_IMASK	0x3f	/* XXX jrs attempt at fix */
#endif

/* JALR and JR are special instructions... */
#define MS_JALR		9
#define MS_JR		8
/* Target register for jump... */
#define MS_JR_REG(x)	(((x) & 0x03e00000) >> 21)
/* J and JAL are regular instructions... */
#define MI_J		0x08000000
#define	MI_JAL		0x0c000000
/* All the bits following the opcode are used in J and JAL instructions... */
#define MI_JMASK	(~MI_IMASK)
#define MI_JSIGN	0x20000000
/* Coprocessor instructions use the bottom two bits of the opcode as a
   coprocessor id, so we only test the top four bits to see if this is a
   COPz instruction... */
#define MC_MASK		0xf0000000UL
#define MC_COPz		0x40000000
/* The coprocessor subopcode is in the five bits following the regular
   instruction opcode... */
#define	MCB_MASK	0x03e00000
/* All coprocessor branches have the following subopcode... */
#define MCB_BC		0x01000000
/* Offsets for coprocessor branches are contained in the bottom 16 bis. */
#define	MCB_OFFSET	0xffff
/* The following are all the branch instructions with regular opcodes: */
#define MI_BEQ		0x10000000
#define MI_BEQL		0x50000000
#define MI_BGTZ		0x1c000000
#define MI_BGTZL	0x5c000000
#define MI_BLEZ		0x18000000
#define MI_BLEZL	0x58000000
#define	MI_BNE		0x14000000
#define MI_BNEL		0x54000000
/* They also use the lower sixteen bits for their target offset... */
#define	BRANCH_OFFSET	0xffff
/* REGIMM instructions have a second opcode in bits 20 through 16... */
#define REGIMM_MASK	0x001f0000
/* The following are all the ``regimm'' branch instructions: */
#define	MRI_BGEZ	0x00010000
#define	MRI_BGEZAL	0x00110000
#define	MRI_BGEZALL	0x00130000
#define	MRI_BGEZL	0x00030000
#define	MRI_BLTZ	0x00000000
#define	MRI_BLTZAL	0x00100000
#define	MRI_BLTZALL	0x00120000
#define	MRI_BLTZL	0x00020000

		/* Decode the instruction sufficiently to determine
		   the branch target... */
	  switch (delay_instruction & MI_IMASK)
	    {
	    case MI_J:
	    case MI_JAL:
	      target_addr = delay_instruction & MI_JMASK;
		/* Sign extend... */
	      if (target_addr & MI_JSIGN)
	        target_addr |= ~(CORE_ADDR)MI_JMASK;
	      target_addr += (pc >> 2) & ~MI_JMASK;
	      target_addr <<= 2;
	      step_addr = 0;
	      break;
	    case MI_SPECIAL:
	      switch (delay_instruction & MS_IMASK)
	        {
	        case MS_JALR:
	        case MS_JR:
	          target_addr +=
		    read_register (MS_JR_REG (delay_instruction));
		  step_addr = 0;
	          break;
		default:
		bad_delay:
		  error ("In delay slot of non-branch instruction: %x\n",
			 delay_instruction);
		  break;
	        }
	      break;
	    case MI_REGIMM:
	      switch (delay_instruction & REGIMM_MASK)
		{
		case MRI_BGEZ:
		case MRI_BGEZAL:
		case MRI_BGEZALL:
		case MRI_BGEZL:
		case MRI_BLTZ:
		case MRI_BLTZAL:
		case MRI_BLTZALL:
		case MRI_BLTZL:
			/* Compiler should sign extend this... */
		  target_addr = (short)delay_instruction;
		  target_addr <<= 2;
		  target_addr += pc;
		  break;
		default:
		  error ("In delay  slot, register-immediate insn: %x\n",
			 delay_instruction);
		  goto bad_delay;
		}
	      break;
	    case MI_BEQ:
	    case MI_BEQL:
	    case MI_BGTZ:
	    case MI_BGTZL:
	    case MI_BLEZ:
	    case MI_BLEZL:
	    case MI_BNE:
	    case MI_BNEL:
	      target_addr = (short)delay_instruction;
	      target_addr <<= 2;
	      target_addr += pc;
	      break;
	    default:
	      error ("In delay  slot, unrecognised instruction: %x\n",
			 delay_instruction);
	      goto bad_delay;
	    }
	}

	/* Don't try to put down two breakpoints in the same spot... */
      if (step_addr == target_addr)
	target_addr = 0;

      if (step_addr)
	{
	  target_insert_breakpoint (step_addr, (char *)&step_cache [0]);
	  if (step_addr + 4 != target_addr)
	    target_insert_breakpoint (step_addr + 4, (char *)&step_cache [1]);
        }
      if (target_addr)
        {
	  target_insert_breakpoint (target_addr, (char *)&step_cache [2]);
	}

	/* If the breakpoint occurred in a branch instruction,
	   re-run the branch (the breakpoint instruction should
	   be gone by now)... */
      if (epc != pc)
	{
	  write_register (PC_REGNUM, epc);
	}
      one_stepped = 1;
      return;
    }
  else
    {
      epc = read_register (PC_REGNUM);
      cause = read_register (CAUSE_REGNUM);
      pc = epc;
#define CAUSE_BD	0x80000000UL
      if (cause & CAUSE_BD)
	pc += 4;
      /* Remove step breakpoints */
      if (step_addr)
	{
	  target_remove_breakpoint (step_addr, (char *)&step_cache [0]);
	  if (step_addr + 4 != target_addr)
	    target_remove_breakpoint (step_addr + 4, (char *)&step_cache [1]);
	}

      if (target_addr)
	{
          target_remove_breakpoint (target_addr, (char *)&step_cache [2]);
	  target_addr = 0;
	}

      one_stepped = 0;
    }
}



/* Register that we are able to handle core file formats.
   FIXME: is this really bfd_target_unknown_flavour? */

static struct core_fns netbsd_core_fns =
{
  bfd_target_unknown_flavour,
  fetch_core_registers,
  NULL
};

void
_initialize_mipsbsd_nat ()
{
  add_core_fns (&netbsd_core_fns);
}
#endif /* __NetBSD__ */
