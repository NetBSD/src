/* Target-dependent code for GDB, the GNU debugger.

   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1995, 1996,
   1997, 2000, 2001, 2002 Free Software Foundation, Inc.

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
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "symfile.h"
#include "objfiles.h"
#include "regcache.h"
#include "value.h"

#include "solib-svr4.h"
#include "ppc-tdep.h"

/* The following two instructions are used in the signal trampoline
   code on GNU/Linux PPC.  */
#define INSTR_LI_R0_0x7777	0x38007777
#define INSTR_SC		0x44000002

/* Since the *-tdep.c files are platform independent (i.e, they may be
   used to build cross platform debuggers), we can't include system
   headers.  Therefore, details concerning the sigcontext structure
   must be painstakingly rerecorded.  What's worse, if these details
   ever change in the header files, they'll have to be changed here
   as well. */

/* __SIGNAL_FRAMESIZE from <asm/ptrace.h> */
#define PPC_LINUX_SIGNAL_FRAMESIZE 64

/* From <asm/sigcontext.h>, offsetof(struct sigcontext_struct, regs) == 0x1c */
#define PPC_LINUX_REGS_PTR_OFFSET (PPC_LINUX_SIGNAL_FRAMESIZE + 0x1c)

/* From <asm/sigcontext.h>, 
   offsetof(struct sigcontext_struct, handler) == 0x14 */
#define PPC_LINUX_HANDLER_PTR_OFFSET (PPC_LINUX_SIGNAL_FRAMESIZE + 0x14)

/* From <asm/ptrace.h>, values for PT_NIP, PT_R1, and PT_LNK */
#define PPC_LINUX_PT_R0		0
#define PPC_LINUX_PT_R1		1
#define PPC_LINUX_PT_R2		2
#define PPC_LINUX_PT_R3		3
#define PPC_LINUX_PT_R4		4
#define PPC_LINUX_PT_R5		5
#define PPC_LINUX_PT_R6		6
#define PPC_LINUX_PT_R7		7
#define PPC_LINUX_PT_R8		8
#define PPC_LINUX_PT_R9		9
#define PPC_LINUX_PT_R10	10
#define PPC_LINUX_PT_R11	11
#define PPC_LINUX_PT_R12	12
#define PPC_LINUX_PT_R13	13
#define PPC_LINUX_PT_R14	14
#define PPC_LINUX_PT_R15	15
#define PPC_LINUX_PT_R16	16
#define PPC_LINUX_PT_R17	17
#define PPC_LINUX_PT_R18	18
#define PPC_LINUX_PT_R19	19
#define PPC_LINUX_PT_R20	20
#define PPC_LINUX_PT_R21	21
#define PPC_LINUX_PT_R22	22
#define PPC_LINUX_PT_R23	23
#define PPC_LINUX_PT_R24	24
#define PPC_LINUX_PT_R25	25
#define PPC_LINUX_PT_R26	26
#define PPC_LINUX_PT_R27	27
#define PPC_LINUX_PT_R28	28
#define PPC_LINUX_PT_R29	29
#define PPC_LINUX_PT_R30	30
#define PPC_LINUX_PT_R31	31
#define PPC_LINUX_PT_NIP	32
#define PPC_LINUX_PT_MSR	33
#define PPC_LINUX_PT_CTR	35
#define PPC_LINUX_PT_LNK	36
#define PPC_LINUX_PT_XER	37
#define PPC_LINUX_PT_CCR	38
#define PPC_LINUX_PT_MQ		39
#define PPC_LINUX_PT_FPR0	48	/* each FP reg occupies 2 slots in this space */
#define PPC_LINUX_PT_FPR31 (PPC_LINUX_PT_FPR0 + 2*31)
#define PPC_LINUX_PT_FPSCR (PPC_LINUX_PT_FPR0 + 2*32 + 1)

static int ppc_linux_at_sigtramp_return_path (CORE_ADDR pc);

/* Determine if pc is in a signal trampoline...

   Ha!  That's not what this does at all.  wait_for_inferior in
   infrun.c calls PC_IN_SIGTRAMP in order to detect entry into a
   signal trampoline just after delivery of a signal.  But on
   GNU/Linux, signal trampolines are used for the return path only.
   The kernel sets things up so that the signal handler is called
   directly.

   If we use in_sigtramp2() in place of in_sigtramp() (see below)
   we'll (often) end up with stop_pc in the trampoline and prev_pc in
   the (now exited) handler.  The code there will cause a temporary
   breakpoint to be set on prev_pc which is not very likely to get hit
   again.

   If this is confusing, think of it this way...  the code in
   wait_for_inferior() needs to be able to detect entry into a signal
   trampoline just after a signal is delivered, not after the handler
   has been run.

   So, we define in_sigtramp() below to return 1 if the following is
   true:

   1) The previous frame is a real signal trampoline.

   - and -

   2) pc is at the first or second instruction of the corresponding
   handler.

   Why the second instruction?  It seems that wait_for_inferior()
   never sees the first instruction when single stepping.  When a
   signal is delivered while stepping, the next instruction that
   would've been stepped over isn't, instead a signal is delivered and
   the first instruction of the handler is stepped over instead.  That
   puts us on the second instruction.  (I added the test for the
   first instruction long after the fact, just in case the observed
   behavior is ever fixed.)

   PC_IN_SIGTRAMP is called from blockframe.c as well in order to set
   the signal_handler_caller flag.  Because of our strange definition
   of in_sigtramp below, we can't rely on signal_handler_caller
   getting set correctly from within blockframe.c.  This is why we
   take pains to set it in init_extra_frame_info().  */

int
ppc_linux_in_sigtramp (CORE_ADDR pc, char *func_name)
{
  CORE_ADDR lr;
  CORE_ADDR sp;
  CORE_ADDR tramp_sp;
  char buf[4];
  CORE_ADDR handler;

  lr = read_register (gdbarch_tdep (current_gdbarch)->ppc_lr_regnum);
  if (!ppc_linux_at_sigtramp_return_path (lr))
    return 0;

  sp = read_register (SP_REGNUM);

  if (target_read_memory (sp, buf, sizeof (buf)) != 0)
    return 0;

  tramp_sp = extract_unsigned_integer (buf, 4);

  if (target_read_memory (tramp_sp + PPC_LINUX_HANDLER_PTR_OFFSET, buf,
			  sizeof (buf)) != 0)
    return 0;

  handler = extract_unsigned_integer (buf, 4);

  return (pc == handler || pc == handler + 4);
}

/*
 * The signal handler trampoline is on the stack and consists of exactly
 * two instructions.  The easiest and most accurate way of determining
 * whether the pc is in one of these trampolines is by inspecting the
 * instructions.  It'd be faster though if we could find a way to do this
 * via some simple address comparisons.
 */
static int
ppc_linux_at_sigtramp_return_path (CORE_ADDR pc)
{
  char buf[12];
  unsigned long pcinsn;
  if (target_read_memory (pc - 4, buf, sizeof (buf)) != 0)
    return 0;

  /* extract the instruction at the pc */
  pcinsn = extract_unsigned_integer (buf + 4, 4);

  return (
	   (pcinsn == INSTR_LI_R0_0x7777
	    && extract_unsigned_integer (buf + 8, 4) == INSTR_SC)
	   ||
	   (pcinsn == INSTR_SC
	    && extract_unsigned_integer (buf, 4) == INSTR_LI_R0_0x7777));
}

CORE_ADDR
ppc_linux_skip_trampoline_code (CORE_ADDR pc)
{
  char buf[4];
  struct obj_section *sect;
  struct objfile *objfile;
  unsigned long insn;
  CORE_ADDR plt_start = 0;
  CORE_ADDR symtab = 0;
  CORE_ADDR strtab = 0;
  int num_slots = -1;
  int reloc_index = -1;
  CORE_ADDR plt_table;
  CORE_ADDR reloc;
  CORE_ADDR sym;
  long symidx;
  char symname[1024];
  struct minimal_symbol *msymbol;

  /* Find the section pc is in; return if not in .plt */
  sect = find_pc_section (pc);
  if (!sect || strcmp (sect->the_bfd_section->name, ".plt") != 0)
    return 0;

  objfile = sect->objfile;

  /* Pick up the instruction at pc.  It had better be of the
     form
     li r11, IDX

     where IDX is an index into the plt_table.  */

  if (target_read_memory (pc, buf, 4) != 0)
    return 0;
  insn = extract_unsigned_integer (buf, 4);

  if ((insn & 0xffff0000) != 0x39600000 /* li r11, VAL */ )
    return 0;

  reloc_index = (insn << 16) >> 16;

  /* Find the objfile that pc is in and obtain the information
     necessary for finding the symbol name. */
  for (sect = objfile->sections; sect < objfile->sections_end; ++sect)
    {
      const char *secname = sect->the_bfd_section->name;
      if (strcmp (secname, ".plt") == 0)
	plt_start = sect->addr;
      else if (strcmp (secname, ".rela.plt") == 0)
	num_slots = ((int) sect->endaddr - (int) sect->addr) / 12;
      else if (strcmp (secname, ".dynsym") == 0)
	symtab = sect->addr;
      else if (strcmp (secname, ".dynstr") == 0)
	strtab = sect->addr;
    }

  /* Make sure we have all the information we need. */
  if (plt_start == 0 || num_slots == -1 || symtab == 0 || strtab == 0)
    return 0;

  /* Compute the value of the plt table */
  plt_table = plt_start + 72 + 8 * num_slots;

  /* Get address of the relocation entry (Elf32_Rela) */
  if (target_read_memory (plt_table + reloc_index, buf, 4) != 0)
    return 0;
  reloc = extract_address (buf, 4);

  sect = find_pc_section (reloc);
  if (!sect)
    return 0;

  if (strcmp (sect->the_bfd_section->name, ".text") == 0)
    return reloc;

  /* Now get the r_info field which is the relocation type and symbol
     index. */
  if (target_read_memory (reloc + 4, buf, 4) != 0)
    return 0;
  symidx = extract_unsigned_integer (buf, 4);

  /* Shift out the relocation type leaving just the symbol index */
  /* symidx = ELF32_R_SYM(symidx); */
  symidx = symidx >> 8;

  /* compute the address of the symbol */
  sym = symtab + symidx * 4;

  /* Fetch the string table index */
  if (target_read_memory (sym, buf, 4) != 0)
    return 0;
  symidx = extract_unsigned_integer (buf, 4);

  /* Fetch the string; we don't know how long it is.  Is it possible
     that the following will fail because we're trying to fetch too
     much? */
  if (target_read_memory (strtab + symidx, symname, sizeof (symname)) != 0)
    return 0;

  /* This might not work right if we have multiple symbols with the
     same name; the only way to really get it right is to perform
     the same sort of lookup as the dynamic linker. */
  msymbol = lookup_minimal_symbol_text (symname, NULL, NULL);
  if (!msymbol)
    return 0;

  return SYMBOL_VALUE_ADDRESS (msymbol);
}

/* The rs6000 version of FRAME_SAVED_PC will almost work for us.  The
   signal handler details are different, so we'll handle those here
   and call the rs6000 version to do the rest. */
CORE_ADDR
ppc_linux_frame_saved_pc (struct frame_info *fi)
{
  if (fi->signal_handler_caller)
    {
      CORE_ADDR regs_addr =
	read_memory_integer (fi->frame + PPC_LINUX_REGS_PTR_OFFSET, 4);
      /* return the NIP in the regs array */
      return read_memory_integer (regs_addr + 4 * PPC_LINUX_PT_NIP, 4);
    }
  else if (fi->next && fi->next->signal_handler_caller)
    {
      CORE_ADDR regs_addr =
	read_memory_integer (fi->next->frame + PPC_LINUX_REGS_PTR_OFFSET, 4);
      /* return LNK in the regs array */
      return read_memory_integer (regs_addr + 4 * PPC_LINUX_PT_LNK, 4);
    }
  else
    return rs6000_frame_saved_pc (fi);
}

void
ppc_linux_init_extra_frame_info (int fromleaf, struct frame_info *fi)
{
  rs6000_init_extra_frame_info (fromleaf, fi);

  if (fi->next != 0)
    {
      /* We're called from get_prev_frame_info; check to see if
         this is a signal frame by looking to see if the pc points
         at trampoline code */
      if (ppc_linux_at_sigtramp_return_path (fi->pc))
	fi->signal_handler_caller = 1;
      else
	fi->signal_handler_caller = 0;
    }
}

int
ppc_linux_frameless_function_invocation (struct frame_info *fi)
{
  /* We'll find the wrong thing if we let 
     rs6000_frameless_function_invocation () search for a signal trampoline */
  if (ppc_linux_at_sigtramp_return_path (fi->pc))
    return 0;
  else
    return rs6000_frameless_function_invocation (fi);
}

void
ppc_linux_frame_init_saved_regs (struct frame_info *fi)
{
  if (fi->signal_handler_caller)
    {
      CORE_ADDR regs_addr;
      int i;
      if (fi->saved_regs)
	return;

      frame_saved_regs_zalloc (fi);

      regs_addr =
	read_memory_integer (fi->frame + PPC_LINUX_REGS_PTR_OFFSET, 4);
      fi->saved_regs[PC_REGNUM] = regs_addr + 4 * PPC_LINUX_PT_NIP;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_ps_regnum] =
        regs_addr + 4 * PPC_LINUX_PT_MSR;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_cr_regnum] =
        regs_addr + 4 * PPC_LINUX_PT_CCR;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_lr_regnum] =
        regs_addr + 4 * PPC_LINUX_PT_LNK;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_ctr_regnum] =
        regs_addr + 4 * PPC_LINUX_PT_CTR;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_xer_regnum] =
        regs_addr + 4 * PPC_LINUX_PT_XER;
      fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_mq_regnum] =
	regs_addr + 4 * PPC_LINUX_PT_MQ;
      for (i = 0; i < 32; i++)
	fi->saved_regs[gdbarch_tdep (current_gdbarch)->ppc_gp0_regnum + i] =
	  regs_addr + 4 * PPC_LINUX_PT_R0 + 4 * i;
      for (i = 0; i < 32; i++)
	fi->saved_regs[FP0_REGNUM + i] = regs_addr + 4 * PPC_LINUX_PT_FPR0 + 8 * i;
    }
  else
    rs6000_frame_init_saved_regs (fi);
}

CORE_ADDR
ppc_linux_frame_chain (struct frame_info *thisframe)
{
  /* Kernel properly constructs the frame chain for the handler */
  if (thisframe->signal_handler_caller)
    return read_memory_integer ((thisframe)->frame, 4);
  else
    return rs6000_frame_chain (thisframe);
}

/* ppc_linux_memory_remove_breakpoints attempts to remove a breakpoint
   in much the same fashion as memory_remove_breakpoint in mem-break.c,
   but is careful not to write back the previous contents if the code
   in question has changed in between inserting the breakpoint and
   removing it.

   Here is the problem that we're trying to solve...

   Once upon a time, before introducing this function to remove
   breakpoints from the inferior, setting a breakpoint on a shared
   library function prior to running the program would not work
   properly.  In order to understand the problem, it is first
   necessary to understand a little bit about dynamic linking on
   this platform.

   A call to a shared library function is accomplished via a bl
   (branch-and-link) instruction whose branch target is an entry
   in the procedure linkage table (PLT).  The PLT in the object
   file is uninitialized.  To gdb, prior to running the program, the
   entries in the PLT are all zeros.

   Once the program starts running, the shared libraries are loaded
   and the procedure linkage table is initialized, but the entries in
   the table are not (necessarily) resolved.  Once a function is
   actually called, the code in the PLT is hit and the function is
   resolved.  In order to better illustrate this, an example is in
   order; the following example is from the gdb testsuite.
	    
	We start the program shmain.

	    [kev@arroyo testsuite]$ ../gdb gdb.base/shmain
	    [...]

	We place two breakpoints, one on shr1 and the other on main.

	    (gdb) b shr1
	    Breakpoint 1 at 0x100409d4
	    (gdb) b main
	    Breakpoint 2 at 0x100006a0: file gdb.base/shmain.c, line 44.

	Examine the instruction (and the immediatly following instruction)
	upon which the breakpoint was placed.  Note that the PLT entry
	for shr1 contains zeros.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      .long 0x0
	    0x100409d8 <shr1+4>:    .long 0x0

	Now run 'til main.

	    (gdb) r
	    Starting program: gdb.base/shmain 
	    Breakpoint 1 at 0xffaf790: file gdb.base/shr1.c, line 19.

	    Breakpoint 2, main ()
		at gdb.base/shmain.c:44
	    44        g = 1;

	Examine the PLT again.  Note that the loading of the shared
	library has initialized the PLT to code which loads a constant
	(which I think is an index into the GOT) into r11 and then
	branchs a short distance to the code which actually does the
	resolving.

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      li      r11,4
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>
	    (gdb) c
	    Continuing.

	    Breakpoint 1, shr1 (x=1)
		at gdb.base/shr1.c:19
	    19        l = 1;

	Now we've hit the breakpoint at shr1.  (The breakpoint was
	reset from the PLT entry to the actual shr1 function after the
	shared library was loaded.) Note that the PLT entry has been
	resolved to contain a branch that takes us directly to shr1. 
	(The real one, not the PLT entry.)

	    (gdb) x/2i 0x100409d4
	    0x100409d4 <shr1>:      b       0xffaf76c <shr1>
	    0x100409d8 <shr1+4>:    b       0x10040984 <sg+4>

   The thing to note here is that the PLT entry for shr1 has been
   changed twice.

   Now the problem should be obvious.  GDB places a breakpoint (a
   trap instruction) on the zero value of the PLT entry for shr1. 
   Later on, after the shared library had been loaded and the PLT
   initialized, GDB gets a signal indicating this fact and attempts
   (as it always does when it stops) to remove all the breakpoints.

   The breakpoint removal was causing the former contents (a zero
   word) to be written back to the now initialized PLT entry thus
   destroying a portion of the initialization that had occurred only a
   short time ago.  When execution continued, the zero word would be
   executed as an instruction an an illegal instruction trap was
   generated instead.  (0 is not a legal instruction.)

   The fix for this problem was fairly straightforward.  The function
   memory_remove_breakpoint from mem-break.c was copied to this file,
   modified slightly, and renamed to ppc_linux_memory_remove_breakpoint.
   In tm-linux.h, MEMORY_REMOVE_BREAKPOINT is defined to call this new
   function.

   The differences between ppc_linux_memory_remove_breakpoint () and
   memory_remove_breakpoint () are minor.  All that the former does
   that the latter does not is check to make sure that the breakpoint
   location actually contains a breakpoint (trap instruction) prior
   to attempting to write back the old contents.  If it does contain
   a trap instruction, we allow the old contents to be written back. 
   Otherwise, we silently do nothing.

   The big question is whether memory_remove_breakpoint () should be
   changed to have the same functionality.  The downside is that more
   traffic is generated for remote targets since we'll have an extra
   fetch of a memory word each time a breakpoint is removed.

   For the time being, we'll leave this self-modifying-code-friendly
   version in ppc-linux-tdep.c, but it ought to be migrated somewhere
   else in the event that some other platform has similar needs with
   regard to removing breakpoints in some potentially self modifying
   code.  */
int
ppc_linux_memory_remove_breakpoint (CORE_ADDR addr, char *contents_cache)
{
  const unsigned char *bp;
  int val;
  int bplen;
  char old_contents[BREAKPOINT_MAX];

  /* Determine appropriate breakpoint contents and size for this address.  */
  bp = BREAKPOINT_FROM_PC (&addr, &bplen);
  if (bp == NULL)
    error ("Software breakpoints not implemented for this target.");

  val = target_read_memory (addr, old_contents, bplen);

  /* If our breakpoint is no longer at the address, this means that the
     program modified the code on us, so it is wrong to put back the
     old value */
  if (val == 0 && memcmp (bp, old_contents, bplen) == 0)
    val = target_write_memory (addr, contents_cache, bplen);

  return val;
}

/* Fetch (and possibly build) an appropriate link_map_offsets
   structure for GNU/Linux PPC targets using the struct offsets
   defined in link.h (but without actual reference to that file).

   This makes it possible to access GNU/Linux PPC shared libraries
   from a GDB that was not built on an GNU/Linux PPC host (for cross
   debugging).  */

struct link_map_offsets *
ppc_linux_svr4_fetch_link_map_offsets (void)
{
  static struct link_map_offsets lmo;
  static struct link_map_offsets *lmp = NULL;

  if (lmp == NULL)
    {
      lmp = &lmo;

      lmo.r_debug_size = 8;	/* The actual size is 20 bytes, but
				   this is all we need.  */
      lmo.r_map_offset = 4;
      lmo.r_map_size   = 4;

      lmo.link_map_size = 20;	/* The actual size is 560 bytes, but
				   this is all we need.  */
      lmo.l_addr_offset = 0;
      lmo.l_addr_size   = 4;

      lmo.l_name_offset = 4;
      lmo.l_name_size   = 4;

      lmo.l_next_offset = 12;
      lmo.l_next_size   = 4;

      lmo.l_prev_offset = 16;
      lmo.l_prev_size   = 4;
    }

  return lmp;
}

enum {
  ELF_NGREG = 48,
  ELF_NFPREG = 33,
  ELF_NVRREG = 33
};

enum {
  ELF_GREGSET_SIZE = (ELF_NGREG * 4),
  ELF_FPREGSET_SIZE = (ELF_NFPREG * 8)
};

void
ppc_linux_supply_gregset (char *buf)
{
  int regi;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  for (regi = 0; regi < 32; regi++)
    supply_register (regi, buf + 4 * regi);

  supply_register (PC_REGNUM, buf + 4 * PPC_LINUX_PT_NIP);
  supply_register (tdep->ppc_lr_regnum, buf + 4 * PPC_LINUX_PT_LNK);
  supply_register (tdep->ppc_cr_regnum, buf + 4 * PPC_LINUX_PT_CCR);
  supply_register (tdep->ppc_xer_regnum, buf + 4 * PPC_LINUX_PT_XER);
  supply_register (tdep->ppc_ctr_regnum, buf + 4 * PPC_LINUX_PT_CTR);
  if (tdep->ppc_mq_regnum != -1)
    supply_register (tdep->ppc_mq_regnum, buf + 4 * PPC_LINUX_PT_MQ);
  supply_register (tdep->ppc_ps_regnum, buf + 4 * PPC_LINUX_PT_MSR);
}

void
ppc_linux_supply_fpregset (char *buf)
{
  int regi;
  struct gdbarch_tdep *tdep = gdbarch_tdep (current_gdbarch); 

  for (regi = 0; regi < 32; regi++)
    supply_register (FP0_REGNUM + regi, buf + 8 * regi);

  /* The FPSCR is stored in the low order word of the last doubleword in the
     fpregset.  */
  supply_register (tdep->ppc_fpscr_regnum, buf + 8 * 32 + 4);
}

/*
  Use a local version of this function to get the correct types for regsets.
*/

static void
fetch_core_registers (char *core_reg_sect,
		      unsigned core_reg_size,
		      int which,
		      CORE_ADDR reg_addr)
{
  if (which == 0)
    {
      if (core_reg_size == ELF_GREGSET_SIZE)
	ppc_linux_supply_gregset (core_reg_sect);
      else
	warning ("wrong size gregset struct in core file");
    }
  else if (which == 2)
    {
      if (core_reg_size == ELF_FPREGSET_SIZE)
	ppc_linux_supply_fpregset (core_reg_sect);
      else
	warning ("wrong size fpregset struct in core file");
    }
}

/* Register that we are able to handle ELF file formats using standard
   procfs "regset" structures.  */

static struct core_fns ppc_linux_regset_core_fns =
{
  bfd_target_elf_flavour,	/* core_flavour */
  default_check_format,		/* check_format */
  default_core_sniffer,		/* core_sniffer */
  fetch_core_registers,		/* core_read_registers */
  NULL				/* next */
};

static void
ppc_linux_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  struct gdbarch_tdep *tdep = gdbarch_tdep (gdbarch);

  /* Until November 2001, gcc was not complying to the SYSV ABI for
     returning structures less than or equal to 8 bytes in size. It was
     returning everything in memory. When this was corrected, it wasn't
     fixed for native platforms.  */
  set_gdbarch_use_struct_convention (gdbarch,
                                   ppc_sysv_abi_broken_use_struct_convention);

  if (tdep->wordsize == 4)
    {
      /* Note: kevinb/2002-04-12: See note in rs6000_gdbarch_init regarding
	 *_push_arguments().  The same remarks hold for the methods below.  */
      set_gdbarch_frameless_function_invocation (gdbarch,
        ppc_linux_frameless_function_invocation);
      set_gdbarch_frame_chain (gdbarch, ppc_linux_frame_chain);
      set_gdbarch_frame_saved_pc (gdbarch, ppc_linux_frame_saved_pc);

      set_gdbarch_frame_init_saved_regs (gdbarch,
                                         ppc_linux_frame_init_saved_regs);
      set_gdbarch_init_extra_frame_info (gdbarch,
                                         ppc_linux_init_extra_frame_info);

      set_gdbarch_memory_remove_breakpoint (gdbarch,
                                            ppc_linux_memory_remove_breakpoint);
      set_solib_svr4_fetch_link_map_offsets
        (gdbarch, ppc_linux_svr4_fetch_link_map_offsets);
    }
}

void
_initialize_ppc_linux_tdep (void)
{
  gdbarch_register_osabi (bfd_arch_powerpc, GDB_OSABI_LINUX,
			  ppc_linux_init_abi);
  add_core_fns (&ppc_linux_regset_core_fns);
}
