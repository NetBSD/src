/* Simulator for Xilinx MicroBlaze processor
   Copyright 2009-2014 Free Software Foundation, Inc.

   This file is part of GDB, the GNU debugger.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <signal.h>
#include "sysdep.h"
#include <sys/times.h>
#include <sys/param.h>
#include <netinet/in.h>	/* for byte ordering macros */
#include "bfd.h"
#include "gdb/callback.h"
#include "libiberty.h"
#include "gdb/remote-sim.h"
#include "sim-main.h"
#include "sim-utils.h"
#include "microblaze-dis.h"


#ifndef NUM_ELEM
#define NUM_ELEM(A) (sizeof (A) / sizeof (A)[0])
#endif

static int target_big_endian = 1;
static unsigned long heap_ptr = 0;
static unsigned long stack_ptr = 0;
host_callback *callback;

unsigned long
microblaze_extract_unsigned_integer (unsigned char *addr, int len)
{
  unsigned long retval;
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *)addr;
  unsigned char *endaddr = startaddr + len;

  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than "
	    "%d bytes.", sizeof (unsigned long));

  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  if (!target_big_endian)
    {
      for (p = endaddr; p > startaddr;)
	retval = (retval << 8) | * -- p;
    }
  else
    {
      for (p = startaddr; p < endaddr;)
	retval = (retval << 8) | * p ++;
    }

  return retval;
}

void
microblaze_store_unsigned_integer (unsigned char *addr, int len,
				   unsigned long val)
{
  unsigned char *p;
  unsigned char *startaddr = (unsigned char *)addr;
  unsigned char *endaddr = startaddr + len;

  if (!target_big_endian)
    {
      for (p = startaddr; p < endaddr;)
	{
	  *p++ = val & 0xff;
	  val >>= 8;
	}
    }
  else
    {
      for (p = endaddr; p > startaddr;)
	{
	  *--p = val & 0xff;
	  val >>= 8;
	}
    }
}

struct sim_state microblaze_state;

int memcycles = 1;

static SIM_OPEN_KIND sim_kind;
static char *myname;

static int issue_messages = 0;

long
int_sbrk (int inc_bytes)
{
  long addr;

  addr = heap_ptr;

  heap_ptr += inc_bytes;

  if (issue_messages && heap_ptr > SP)
    fprintf (stderr, "Warning: heap_ptr overlaps stack!\n");

  return addr;
}

static void /* INLINE */
wbat (word x, word v)
{
  if (((uword)x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "byte write to 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
    }
  else
    {
      unsigned char *p = CPU.memory + x;
      p[0] = v;
    }
}

static void /* INLINE */
wlat (word x, word v)
{
  if (((uword)x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "word write to 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
    }
  else
    {
      if ((x & 3) != 0)
	{
	  if (issue_messages)
	    fprintf (stderr, "word write to unaligned memory address: 0x%x\n", x);

	  CPU.exception = SIGBUS;
	}
      else if (!target_big_endian)
	{
	  unsigned char *p = CPU.memory + x;
	  p[3] = v >> 24;
	  p[2] = v >> 16;
	  p[1] = v >> 8;
	  p[0] = v;
	}
      else
	{
	  unsigned char *p = CPU.memory + x;
	  p[0] = v >> 24;
	  p[1] = v >> 16;
	  p[2] = v >> 8;
	  p[3] = v;
	}
    }
}

static void /* INLINE */
what (word x, word v)
{
  if (((uword)x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "short write to 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
    }
  else
    {
      if ((x & 1) != 0)
	{
	  if (issue_messages)
	    fprintf (stderr, "short write to unaligned memory address: 0x%x\n",
		     x);

	  CPU.exception = SIGBUS;
	}
      else if (!target_big_endian)
	{
	  unsigned char *p = CPU.memory + x;
	  p[1] = v >> 8;
	  p[0] = v;
	}
      else
	{
	  unsigned char *p = CPU.memory + x;
	  p[0] = v >> 8;
	  p[1] = v;
	}
    }
}

/* Read functions.  */
static int /* INLINE */
rbat (word x)
{
  if (((uword)x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "byte read from 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
      return 0;
    }
  else
    {
      unsigned char *p = CPU.memory + x;
      return p[0];
    }
}

static int /* INLINE */
rlat (word x)
{
  if (((uword) x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "word read from 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
      return 0;
    }
  else
    {
      if ((x & 3) != 0)
	{
	  if (issue_messages)
	    fprintf (stderr, "word read from unaligned address: 0x%x\n", x);

	  CPU.exception = SIGBUS;
	  return 0;
	}
      else if (! target_big_endian)
	{
	  unsigned char *p = CPU.memory + x;
	  return (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
	}
      else
	{
	  unsigned char *p = CPU.memory + x;
	  return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	}
    }
}

static int /* INLINE */
rhat (word x)
{
  if (((uword)x) >= CPU.msize)
    {
      if (issue_messages)
	fprintf (stderr, "short read from 0x%x outside memory range\n", x);

      CPU.exception = SIGSEGV;
      return 0;
    }
  else
    {
      if ((x & 1) != 0)
	{
	  if (issue_messages)
	    fprintf (stderr, "short read from unaligned address: 0x%x\n", x);

	  CPU.exception = SIGBUS;
	  return 0;
	}
      else if (!target_big_endian)
	{
	  unsigned char *p = CPU.memory + x;
	  return (p[1] << 8) | p[0];
	}
      else
	{
	  unsigned char *p = CPU.memory + x;
	  return (p[0] << 8) | p[1];
	}
    }
}


#define SEXTB(x) (((x & 0xff) ^ (~ 0x7f)) + 0x80)
#define SEXTW(y) ((int)((short)y))

static int
IOMEM (int addr, int write, int value)
{
}

/* Default to a 8 Mbyte (== 2^23) memory space.  */
static int sim_memory_size = 1 << 23;

#define	MEM_SIZE_FLOOR	64
void
sim_size (int size)
{
  sim_memory_size = size;
  CPU.msize = sim_memory_size;

  if (CPU.memory)
    free (CPU.memory);

  CPU.memory = (unsigned char *) calloc (1, CPU.msize);

  if (!CPU.memory)
    {
      if (issue_messages)
	fprintf (stderr,
		 "Not enough VM for simulation of %d bytes of RAM\n",
		 CPU.msize);

      CPU.msize = 1;
      CPU.memory = (unsigned char *) calloc (1, 1);
    }
}

static void
init_pointers ()
{
  if (CPU.msize != (sim_memory_size))
    sim_size (sim_memory_size);
}

static void
set_initial_gprs ()
{
  int i;
  long space;
  unsigned long memsize;

  init_pointers ();

  /* Set up machine just out of reset.  */
  PC = 0;
  MSR = 0;

  memsize = CPU.msize / (1024 * 1024);

  if (issue_messages > 1)
    fprintf (stderr, "Simulated memory of %d Mbytes (0x0 .. 0x%08x)\n",
	     memsize, CPU.msize - 1);

  /* Clean out the GPRs */
  for (i = 0; i < 32; i++)
    CPU.regs[i] = 0;
  CPU.insts = 0;
  CPU.cycles = 0;
  CPU.imm_enable = 0;

}

static void
interrupt ()
{
  CPU.exception = SIGINT;
}

/* Functions so that trapped open/close don't interfere with the
   parent's functions.  We say that we can't close the descriptors
   that we didn't open.  exit() and cleanup() get in trouble here,
   to some extent.  That's the price of emulation.  */

unsigned char opened[100];

static void
log_open (int fd)
{
  if (fd < 0 || fd > NUM_ELEM (opened))
    return;

  opened[fd] = 1;
}

static void
log_close (int fd)
{
  if (fd < 0 || fd > NUM_ELEM (opened))
    return;

  opened[fd] = 0;
}

static int
is_opened (int fd)
{
  if (fd < 0 || fd > NUM_ELEM (opened))
    return 0;

  return opened[fd];
}

static void
handle_trap1 ()
{
}

static void
process_stub (int what)
{
  /* These values should match those in libgloss/microblaze/syscalls.s.  */
  switch (what)
    {
    case 3:  /* _read */
    case 4:  /* _write */
    case 5:  /* _open */
    case 6:  /* _close */
    case 10: /* _unlink */
    case 19: /* _lseek */
    case 43: /* _times */
      handle_trap1 ();
      break;

    default:
      if (issue_messages)
	fprintf (stderr, "Unhandled stub opcode: %d\n", what);
      break;
    }
}

static void
util (unsigned what)
{
  switch (what)
    {
    case 0:	/* exit */
      CPU.exception = SIGQUIT;
      break;

    case 1:	/* printf */
      {
	unsigned long a[6];
	unsigned char *s;
	int i;

	for (s = (unsigned char *)a[0], i = 1 ; *s && i < 6 ; s++)
	  if (*s == '%')
	    i++;
      }
      break;

    case 2:	/* scanf */
      if (issue_messages)
	fprintf (stderr, "WARNING: scanf unimplemented\n");
      break;

    case 3:	/* utime */
      break;

    case 0xFF:
      process_stub (CPU.regs[1]);
      break;

    default:
      if (issue_messages)
	fprintf (stderr, "Unhandled util code: %x\n", what);
      break;
    }
}

/* For figuring out whether we carried; addc/subc use this. */
static int
iu_carry (unsigned long a, unsigned long b, int cin)
{
  unsigned long	x;

  x = (a & 0xffff) + (b & 0xffff) + cin;
  x = (x >> 16) + (a >> 16) + (b >> 16);
  x >>= 16;

  return (x != 0);
}

#define WATCHFUNCTIONS 1
#ifdef WATCHFUNCTIONS

#define MAXWL 80
word WL[MAXWL];
char *WLstr[MAXWL];

int ENDWL=0;
int WLincyc;
int WLcyc[MAXWL];
int WLcnts[MAXWL];
int WLmax[MAXWL];
int WLmin[MAXWL];
word WLendpc;
int WLbcyc;
int WLW;
#endif

static int tracing = 0;

void
sim_resume (SIM_DESC sd, int step, int siggnal)
{
  int needfetch;
  word inst;
  enum microblaze_instr op;
  void (*sigsave)();
  int memops;
  int bonus_cycles;
  int insts;
  int w;
  int cycs;
  word WLhash;
  ubyte carry;
  int imm_unsigned;
  short ra, rb, rd;
  long immword;
  uword oldpc, newpc;
  short delay_slot_enable;
  short branch_taken;
  short num_delay_slot; /* UNUSED except as reqd parameter */
  enum microblaze_instr_type insn_type;

  sigsave = signal (SIGINT, interrupt);
  CPU.exception = step ? SIGTRAP : 0;

  memops = 0;
  bonus_cycles = 0;
  insts = 0;
  
  do
    {
      /* Fetch the initial instructions that we'll decode. */
      inst = rlat (PC & 0xFFFFFFFC);

      op = get_insn_microblaze (inst, &imm_unsigned, &insn_type, 
				&num_delay_slot);

      if (op == invalid_inst)
	fprintf (stderr, "Unknown instruction 0x%04x", inst);

      if (tracing)
	fprintf (stderr, "%.4x: inst = %.4x ", PC, inst);

      rd = GET_RD;
      rb = GET_RB;
      ra = GET_RA;
      /*      immword = IMM_W; */

      oldpc = PC;
      delay_slot_enable = 0;
      branch_taken = 0;
      if (op == microblaze_brk)
	CPU.exception = SIGTRAP;
      else if (inst == MICROBLAZE_HALT_INST)
	{
	  CPU.exception = SIGQUIT;
	  insts += 1;
	  bonus_cycles++;
	}
      else
	{
	  switch(op)
	    {
#define INSTRUCTION(NAME, OPCODE, TYPE, ACTION)		\
	    case NAME:					\
	      ACTION;					\
	      break;
#include "microblaze.isa"
#undef INSTRUCTION

	    default:
	      CPU.exception = SIGILL;
	      fprintf (stderr, "ERROR: Unknown opcode\n");
	    }
	  /* Make R0 consistent */
	  CPU.regs[0] = 0;

	  /* Check for imm instr */
	  if (op == imm)
	    IMM_ENABLE = 1;
	  else
	    IMM_ENABLE = 0;

	  /* Update cycle counts */
	  insts ++;
	  if (insn_type == memory_store_inst || insn_type == memory_load_inst)
	    memops++;
	  if (insn_type == mult_inst)
	    bonus_cycles++;
	  if (insn_type == barrel_shift_inst)
	    bonus_cycles++;
	  if (insn_type == anyware_inst)
	    bonus_cycles++;
	  if (insn_type == div_inst)
	    bonus_cycles += 33;

	  if ((insn_type == branch_inst || insn_type == return_inst)
	      && branch_taken) 
	    {
	      /* Add an extra cycle for taken branches */
	      bonus_cycles++;
	      /* For branch instructions handle the instruction in the delay slot */
	      if (delay_slot_enable) 
	        {
	          newpc = PC;
	          PC = oldpc + INST_SIZE;
	          inst = rlat (PC & 0xFFFFFFFC);
	          op = get_insn_microblaze (inst, &imm_unsigned, &insn_type,
					    &num_delay_slot);
	          if (op == invalid_inst)
		    fprintf (stderr, "Unknown instruction 0x%04x", inst);
	          if (tracing)
		    fprintf (stderr, "%.4x: inst = %.4x ", PC, inst);
	          rd = GET_RD;
	          rb = GET_RB;
	          ra = GET_RA;
	          /*	      immword = IMM_W; */
	          if (op == microblaze_brk)
		    {
		      if (issue_messages)
		        fprintf (stderr, "Breakpoint set in delay slot "
			         "(at address 0x%x) will not be honored\n", PC);
		      /* ignore the breakpoint */
		    }
	          else if (insn_type == branch_inst || insn_type == return_inst)
		    {
		      if (issue_messages)
		        fprintf (stderr, "Cannot have branch or return instructions "
			         "in delay slot (at address 0x%x)\n", PC);
		      CPU.exception = SIGILL;
		    }
	          else
		    {
		      switch(op)
		        {
#define INSTRUCTION(NAME, OPCODE, TYPE, ACTION)		\
		        case NAME:			\
			  ACTION;			\
			  break;
#include "microblaze.isa"
#undef INSTRUCTION

		        default:
		          CPU.exception = SIGILL;
		          fprintf (stderr, "ERROR: Unknown opcode at 0x%x\n", PC);
		        }
		      /* Update cycle counts */
		      insts++;
		      if (insn_type == memory_store_inst
		          || insn_type == memory_load_inst) 
		        memops++;
		      if (insn_type == mult_inst)
		        bonus_cycles++;
		      if (insn_type == barrel_shift_inst)
		        bonus_cycles++;
		      if (insn_type == anyware_inst)
		        bonus_cycles++;
		      if (insn_type == div_inst)
		        bonus_cycles += 33;
		    }
	          /* Restore the PC */
	          PC = newpc;
	          /* Make R0 consistent */
	          CPU.regs[0] = 0;
	          /* Check for imm instr */
	          if (op == imm)
		    IMM_ENABLE = 1;
	          else
		    IMM_ENABLE = 0;
	        }
	      else
		/* no delay slot: increment cycle count */
		bonus_cycles++;
	    }
	}

      if (tracing)
	fprintf (stderr, "\n");
    }
  while (!CPU.exception);

  /* Hide away the things we've cached while executing.  */
  /*  CPU.pc = pc; */
  CPU.insts += insts;		/* instructions done ... */
  CPU.cycles += insts;		/* and each takes a cycle */
  CPU.cycles += bonus_cycles;	/* and extra cycles for branches */
  CPU.cycles += memops; 	/* and memop cycle delays */

  signal (SIGINT, sigsave);
}


int
sim_write (SIM_DESC sd, SIM_ADDR addr, const unsigned char *buffer, int size)
{
  int i;
  init_pointers ();

  memcpy (&CPU.memory[addr], buffer, size);

  return size;
}

int
sim_read (SIM_DESC sd, SIM_ADDR addr, unsigned char *buffer, int size)
{
  int i;
  init_pointers ();

  memcpy (buffer, &CPU.memory[addr], size);

  return size;
}


int
sim_store_register (SIM_DESC sd, int rn, unsigned char *memory, int length)
{
  init_pointers ();

  if (rn < NUM_REGS + NUM_SPECIAL && rn >= 0)
    {
      if (length == 4)
	{
	  /* misalignment safe */
	  long ival = microblaze_extract_unsigned_integer (memory, 4);
	  if (rn < NUM_REGS)
	    CPU.regs[rn] = ival;
	  else
	    CPU.spregs[rn-NUM_REGS] = ival;
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}

int
sim_fetch_register (SIM_DESC sd, int rn, unsigned char *memory, int length)
{
  long ival;
  init_pointers ();

  if (rn < NUM_REGS + NUM_SPECIAL && rn >= 0)
    {
      if (length == 4)
	{
	  if (rn < NUM_REGS)
	    ival = CPU.regs[rn];
	  else
	    ival = CPU.spregs[rn-NUM_REGS];

	  /* misalignment-safe */
	  microblaze_store_unsigned_integer (memory, 4, ival);
	  return 4;
	}
      else
	return 0;
    }
  else
    return 0;
}


int
sim_trace (SIM_DESC sd)
{
  tracing = 1;

  sim_resume (sd, 0, 0);

  tracing = 0;

  return 1;
}

void
sim_stop_reason (SIM_DESC sd, enum sim_stop *reason, int *sigrc)
{
  if (CPU.exception == SIGQUIT)
    {
      *reason = sim_exited;
      *sigrc = RETREG;
    }
  else
    {
      *reason = sim_stopped;
      *sigrc = CPU.exception;
    }
}


int
sim_stop (SIM_DESC sd)
{
  CPU.exception = SIGINT;
  return 1;
}


void
sim_info (SIM_DESC sd, int verbose)
{
#ifdef WATCHFUNCTIONS
  int w, wcyc;
#endif

  callback->printf_filtered (callback, "\n\n# instructions executed  %10d\n",
			     CPU.insts);
  callback->printf_filtered (callback, "# cycles                 %10d\n",
			     (CPU.cycles) ? CPU.cycles+2 : 0);

#ifdef WATCHFUNCTIONS
  callback->printf_filtered (callback, "\nNumber of watched functions: %d\n",
			     ENDWL);

  wcyc = 0;

  for (w = 1; w <= ENDWL; w++)
    {
      callback->printf_filtered (callback, "WL = %s %8x\n",WLstr[w],WL[w]);
      callback->printf_filtered (callback, "  calls = %d, cycles = %d\n",
				 WLcnts[w],WLcyc[w]);

      if (WLcnts[w] != 0)
	callback->printf_filtered (callback,
				   "  maxcpc = %d, mincpc = %d, avecpc = %d\n",
				   WLmax[w],WLmin[w],WLcyc[w]/WLcnts[w]);
      wcyc += WLcyc[w];
    }

  callback->printf_filtered (callback,
			     "Total cycles for watched functions: %d\n",wcyc);
#endif
}

struct	aout
{
  unsigned char  sa_machtype[2];
  unsigned char  sa_magic[2];
  unsigned char  sa_tsize[4];
  unsigned char  sa_dsize[4];
  unsigned char  sa_bsize[4];
  unsigned char  sa_syms[4];
  unsigned char  sa_entry[4];
  unsigned char  sa_trelo[4];
  unsigned char  sa_drelo[4];
} aout;

#define	LONG(x)		(((x)[0]<<24)|((x)[1]<<16)|((x)[2]<<8)|(x)[3])
#define	SHORT(x)	(((x)[0]<<8)|(x)[1])

SIM_DESC
sim_open (SIM_OPEN_KIND kind, host_callback *cb, struct bfd *abfd, char **argv)
{
  /*  SIM_DESC sd = sim_state_alloc(kind, alloc);*/

  int osize = sim_memory_size;
  myname = argv[0];
  callback = cb;

  if (kind == SIM_OPEN_STANDALONE)
    issue_messages = 1;

  /* Discard and reacquire memory -- start with a clean slate.  */
  sim_size (1);		/* small */
  sim_size (osize);	/* and back again */

  set_initial_gprs ();	/* Reset the GPR registers.  */

  return ((SIM_DESC) 1);
}

void
sim_close (SIM_DESC sd, int quitting)
{
  if (CPU.memory)
    {
      free(CPU.memory);
      CPU.memory = NULL;
      CPU.msize = 0;
    }
}

SIM_RC
sim_load (SIM_DESC sd, char *prog, bfd *abfd, int from_tty)
{
  /* Do the right thing for ELF executables; this turns out to be
     just about the right thing for any object format that:
       - we crack using BFD routines
       - follows the traditional UNIX text/data/bss layout
       - calls the bss section ".bss".   */

  extern bfd *sim_load_file (); /* ??? Don't know where this should live.  */
  bfd *prog_bfd;

  {
    bfd *handle;
    asection *s;
    int found_loadable_section = 0;
    bfd_vma max_addr = 0;
    handle = bfd_openr (prog, 0);

    if (!handle)
      {
	printf("``%s'' could not be opened.\n", prog);
	return SIM_RC_FAIL;
      }

    /* Makes sure that we have an object file, also cleans gets the
       section headers in place.  */
    if (!bfd_check_format (handle, bfd_object))
      {
	/* wasn't an object file */
	bfd_close (handle);
	printf ("``%s'' is not appropriate object file.\n", prog);
	return SIM_RC_FAIL;
      }

    for (s = handle->sections; s; s = s->next)
      {
	if (s->flags & SEC_ALLOC)
	  {
	    bfd_vma vma = 0;
	    int size = bfd_get_section_size (s);
	    if (size > 0)
	      {
		vma = bfd_section_vma (handle, s);
		if (vma >= max_addr)
		  {
		    max_addr = vma + size;
		  }
	      }
	    if (s->flags & SEC_LOAD)
	      found_loadable_section = 1;
	  }
      }

    if (!found_loadable_section)
      {
	/* No loadable sections */
	bfd_close(handle);
	printf("No loadable sections in file %s\n", prog);
	return SIM_RC_FAIL;
      }

    sim_memory_size = (unsigned long) max_addr;

    /* Clean up after ourselves.  */
    bfd_close (handle);

  }

  /* from sh -- dac */
  prog_bfd = sim_load_file (sd, myname, callback, prog, abfd,
			    /* sim_kind == SIM_OPEN_DEBUG, */
			    1,
			    0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;

  target_big_endian = bfd_big_endian (prog_bfd);
  PC = bfd_get_start_address (prog_bfd);

  if (abfd == NULL)
    bfd_close (prog_bfd);

  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *prog_bfd, char **argv, char **env)
{
  char **avp;
  int nargs = 0;
  int nenv = 0;
  int s_length;
  int l;
  unsigned long strings;
  unsigned long pointers;
  unsigned long hi_stack;


  /* Set the initial register set.  */
  l = issue_messages;
  issue_messages = 0;
  set_initial_gprs ();
  issue_messages = l;

  hi_stack = CPU.msize - 4;
  PC = bfd_get_start_address (prog_bfd);

  /* For now ignore all parameters to the program */

  return SIM_RC_OK;
}

void
sim_kill (SIM_DESC sd)
{
  /* nothing to do */
}

void
sim_do_command (SIM_DESC sd, char * cmd)
{
  /* Nothing there yet; it's all an error.  */

  if (cmd != NULL)
    {
      char ** simargv = buildargv (cmd);

      if (strcmp (simargv[0], "watch") == 0)
	{
	  if ((simargv[1] == NULL) || (simargv[2] == NULL))
	    {
	      fprintf (stderr, "Error: missing argument to watch cmd.\n");
	      return;
	    }

	  ENDWL++;

	  WL[ENDWL] = strtol (simargv[2], NULL, 0);
	  WLstr[ENDWL] = strdup (simargv[1]);
	  fprintf (stderr, "Added %s (%x) to watchlist, #%d\n",WLstr[ENDWL],
		   WL[ENDWL], ENDWL);

	}
      else if (strcmp (simargv[0], "dumpmem") == 0)
	{
	  unsigned char * p;
	  FILE * dumpfile;

	  if (simargv[1] == NULL)
	    fprintf (stderr, "Error: missing argument to dumpmem cmd.\n");

	  fprintf (stderr, "Writing dumpfile %s...",simargv[1]);

	  dumpfile = fopen (simargv[1], "w");
	  p = CPU.memory;
	  fwrite (p, CPU.msize-1, 1, dumpfile);
	  fclose (dumpfile);

	  fprintf (stderr, "done.\n");
	}
      else if (strcmp (simargv[0], "clearstats") == 0)
	{
	  CPU.cycles = 0;
	  CPU.insts = 0;
	  ENDWL = 0;
	}
      else if (strcmp (simargv[0], "verbose") == 0)
	{
	  issue_messages = 2;
	}
      else
	{
	  fprintf (stderr,"Error: \"%s\" is not a valid M.CORE simulator command.\n",
		   cmd);
	}
    }
  else
    {
      fprintf (stderr, "M.CORE sim commands: \n");
      fprintf (stderr, "  watch <funcname> <addr>\n");
      fprintf (stderr, "  dumpmem <filename>\n");
      fprintf (stderr, "  clearstats\n");
      fprintf (stderr, "  verbose\n");
    }
}

void
sim_set_callbacks (host_callback *ptr)
{
  callback = ptr;
}

char **
sim_complete_command (SIM_DESC sd, const char *text, const char *word)
{
  return NULL;
}
