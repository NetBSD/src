/* Definitions to make GDB run on a mips box under 4.3bsd.
   Copyright 1986, 1987, 1989, 1991, 1992, 1993, 1994, 1995, 1996, 1997,
   1998, 1999, 2000
   Free Software Foundation, Inc.
   Contributed by Per Bothner (bothner@cs.wisc.edu) at U.Wisconsin
   and by Alessandro Forin (af@cs.cmu.edu) at CMU..

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

#ifndef TM_MIPS_H
#define TM_MIPS_H 1

#define GDB_MULTI_ARCH 1

#include "regcache.h"

struct frame_info;
struct symbol;
struct type;
struct value;

#include <bfd.h>
#include "coff/sym.h"		/* Needed for PDR below.  */
#include "coff/symconst.h"

/* PC should be masked to remove possible MIPS16 flag */
#if !defined (GDB_TARGET_MASK_DISAS_PC)
#define GDB_TARGET_MASK_DISAS_PC(addr) UNMAKE_MIPS16_ADDR(addr)
#endif
#if !defined (GDB_TARGET_UNMASK_DISAS_PC)
#define GDB_TARGET_UNMASK_DISAS_PC(addr) MAKE_MIPS16_ADDR(addr)
#endif

/* Return non-zero if PC points to an instruction which will cause a step
   to execute both the instruction at PC and an instruction at PC+4.  */
extern int mips_step_skips_delay (CORE_ADDR);
#define STEP_SKIPS_DELAY_P (1)
#define STEP_SKIPS_DELAY(pc) (mips_step_skips_delay (pc))

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* The size of a register.  This is predefined in tm-mips64.h.  We
   can't use REGISTER_SIZE because that is used for various other
   things.  */

#ifndef MIPS_REGSIZE
#define MIPS_REGSIZE 4
#endif

/* Number of machine registers */

#ifndef NUM_REGS
#define NUM_REGS 90
#endif

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#ifndef MIPS_REGISTER_NAMES
#define MIPS_REGISTER_NAMES 	\
    {	"zero",	"at",	"v0",	"v1",	"a0",	"a1",	"a2",	"a3", \
	"t0",	"t1",	"t2",	"t3",	"t4",	"t5",	"t6",	"t7", \
	"s0",	"s1",	"s2",	"s3",	"s4",	"s5",	"s6",	"s7", \
	"t8",	"t9",	"k0",	"k1",	"gp",	"sp",	"s8",	"ra", \
	"sr",	"lo",	"hi",	"bad",	"cause","pc",    \
	"f0",   "f1",   "f2",   "f3",   "f4",   "f5",   "f6",   "f7", \
	"f8",   "f9",   "f10",  "f11",  "f12",  "f13",  "f14",  "f15", \
	"f16",  "f17",  "f18",  "f19",  "f20",  "f21",  "f22",  "f23",\
	"f24",  "f25",  "f26",  "f27",  "f28",  "f29",  "f30",  "f31",\
	"fsr",  "fir",  "fp",	"", \
	"",	"",	"",	"",	"",	"",	"",	"", \
	"",	"",	"",	"",	"",	"",	"",	"", \
    }
#endif

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define ZERO_REGNUM 0		/* read-only register, always 0 */
#define V0_REGNUM 2		/* Function integer return value */
#define A0_REGNUM 4		/* Loc of first arg during a subr call */
#define T9_REGNUM 25		/* Contains address of callee in PIC */
#define SP_REGNUM 29		/* Contains address of top of stack */
#define RA_REGNUM 31		/* Contains return address value */
#define PS_REGNUM 32		/* Contains processor status */
#define HI_REGNUM 34		/* Multiple/divide temp */
#define LO_REGNUM 33		/* ... */
#define BADVADDR_REGNUM 35	/* bad vaddr for addressing exception */
#define CAUSE_REGNUM 36		/* describes last exception */
#define PC_REGNUM 37		/* Contains program counter */
#define FP0_REGNUM 38		/* Floating point register 0 (single float) */
#define FPA0_REGNUM (FP0_REGNUM+12)	/* First float argument register */
#define FCRCS_REGNUM 70		/* FP control/status */
#define FCRIR_REGNUM 71		/* FP implementation/revision */
#define FP_REGNUM 72		/* Pseudo register that contains true address of executing stack frame */
#define	UNUSED_REGNUM 73	/* Never used, FIXME */
#define	FIRST_EMBED_REGNUM 74	/* First CP0 register for embedded use */
#define	PRID_REGNUM 89		/* Processor ID */
#define	LAST_EMBED_REGNUM 89	/* Last one */

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.  */

#define REGISTER_BYTES (NUM_REGS*MIPS_REGSIZE)

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N) ((N) * MIPS_REGSIZE)

/* Return the GDB type object for the "standard" data type of data in
   register N.  */

#ifndef REGISTER_VIRTUAL_TYPE
#define REGISTER_VIRTUAL_TYPE(N) \
	(((N) >= FP0_REGNUM && (N) < FP0_REGNUM+32) ? builtin_type_float \
	 : ((N) == 32 /*SR*/) ? builtin_type_uint32 \
	 : ((N) >= 70 && (N) <= 89) ? builtin_type_uint32 \
	 : builtin_type_int)
#endif

/* All mips targets store doubles in a register pair with the least
   significant register in the lower numbered register.
   If the target is big endian, double register values need conversion
   between memory and register formats.  */

extern void mips_register_convert_to_type (int regnum, 
					   struct type *type,
					   char *buffer);
extern void mips_register_convert_from_type (int regnum, 
					     struct type *type,
					     char *buffer);

#define REGISTER_CONVERT_TO_TYPE(n, type, buffer)	\
  mips_register_convert_to_type ((n), (type), (buffer))

#define REGISTER_CONVERT_FROM_TYPE(n, type, buffer)	\
  mips_register_convert_from_type ((n), (type), (buffer))


/* Special symbol found in blocks associated with routines.  We can hang
   mips_extra_func_info_t's off of this.  */

#define MIPS_EFI_SYMBOL_NAME "__GDB_EFI_INFO__"
extern void ecoff_relocate_efi (struct symbol *, CORE_ADDR);

/* Specific information about a procedure.
   This overlays the MIPS's PDR records, 
   mipsread.c (ab)uses this to save memory */

typedef struct mips_extra_func_info
  {
    long numargs;		/* number of args to procedure (was iopt) */
    bfd_vma high_addr;		/* upper address bound */
    long frame_adjust;		/* offset of FP from SP (used on MIPS16) */
    PDR pdr;			/* Procedure descriptor record */
  }
 *mips_extra_func_info_t;

extern void mips_print_extra_frame_info (struct frame_info *frame);
#define	PRINT_EXTRA_FRAME_INFO(fi) \
  mips_print_extra_frame_info (fi)

/* It takes two values to specify a frame on the MIPS.

   In fact, the *PC* is the primary value that sets up a frame.  The
   PC is looked up to see what function it's in; symbol information
   from that function tells us which register is the frame pointer
   base, and what offset from there is the "virtual frame pointer".
   (This is usually an offset from SP.)  On most non-MIPS machines,
   the primary value is the SP, and the PC, if needed, disambiguates
   multiple functions with the same SP.  But on the MIPS we can't do
   that since the PC is not stored in the same part of the frame every
   time.  This does not seem to be a very clever way to set up frames,
   but there is nothing we can do about that.  */

#define SETUP_ARBITRARY_FRAME(argc, argv) setup_arbitrary_frame (argc, argv)
extern struct frame_info *setup_arbitrary_frame (int, CORE_ADDR *);

/* Select the default mips disassembler */

#define TM_PRINT_INSN_MACH 0

/* These are defined in mdebugread.c and are used in mips-tdep.c  */
extern CORE_ADDR sigtramp_address, sigtramp_end;
extern void fixup_sigtramp (void);

/* Defined in mips-tdep.c and used in remote-mips.c */
extern char *mips_read_processor_type (void);

/* Functions for dealing with MIPS16 call and return stubs.  */
#define IGNORE_HELPER_CALL(pc)			mips_ignore_helper (pc)
extern int mips_ignore_helper (CORE_ADDR pc);

#ifndef TARGET_MIPS
#define TARGET_MIPS
#endif

/* Definitions and declarations used by mips-tdep.c and remote-mips.c  */
#define MIPS_INSTLEN 4		/* Length of an instruction */
#define MIPS16_INSTLEN 2	/* Length of an instruction on MIPS16 */
#define MIPS_NUMREGS 32		/* Number of integer or float registers */
typedef unsigned long t_inst;	/* Integer big enough to hold an instruction */

/* MIPS16 function addresses are odd (bit 0 is set).  Here are some
   macros to test, set, or clear bit 0 of addresses.  */
#define IS_MIPS16_ADDR(addr)	 ((addr) & 1)
#define MAKE_MIPS16_ADDR(addr)	 ((addr) | 1)
#define UNMAKE_MIPS16_ADDR(addr) ((addr) & ~1)

#endif /* TM_MIPS_H */

/* Command to set the processor type. */
extern void mips_set_processor_type_command (char *, int);

/* Single step based on where the current instruction will take us.  */
extern void mips_software_single_step (enum target_signal, int);
