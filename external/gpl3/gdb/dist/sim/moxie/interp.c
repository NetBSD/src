/* Simulator for the moxie processor
   Copyright (C) 2008-2013 Free Software Foundation, Inc.
   Contributed by Anthony Green

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
along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include "sysdep.h"
#include <sys/times.h>
#include <sys/param.h>
#include <netinet/in.h>	/* for byte ordering macros */
#include "bfd.h"
#include "gdb/callback.h"
#include "libiberty.h"
#include "gdb/remote-sim.h"

#include "sim-main.h"
#include "sim-base.h"

typedef int word;
typedef unsigned int uword;

host_callback *       callback;

FILE *tracefile;

/* Extract the signed 10-bit offset from a 16-bit branch
   instruction.  */
#define INST2OFFSET(o) ((((signed short)((o & ((1<<10)-1))<<6))>>6)<<1)

#define EXTRACT_WORD(addr) \
  ((sim_core_read_aligned_1 (scpu, cia, read_map, addr) << 24) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+1) << 16) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+2) << 8) \
   + (sim_core_read_aligned_1 (scpu, cia, read_map, addr+3)))

unsigned long
moxie_extract_unsigned_integer (addr, len)
     unsigned char * addr;
     int len;
{
  unsigned long retval;
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;
 
  if (len > (int) sizeof (unsigned long))
    printf ("That operation is not available on integers of more than %d bytes.",
	    sizeof (unsigned long));
 
  /* Start at the most significant end of the integer, and work towards
     the least significant.  */
  retval = 0;

  for (p = endaddr; p > startaddr;)
    retval = (retval << 8) | * -- p;
  
  return retval;
}

void
moxie_store_unsigned_integer (addr, len, val)
     unsigned char * addr;
     int len;
     unsigned long val;
{
  unsigned char * p;
  unsigned char * startaddr = (unsigned char *)addr;
  unsigned char * endaddr = startaddr + len;

  for (p = endaddr; p > startaddr;)
    {
      * -- p = val & 0xff;
      val >>= 8;
    }
}

/* moxie register names.  */
static const char *reg_names[16] = 
  { "$fp", "$sp", "$r0", "$r1", "$r2", "$r3", "$r4", "$r5", 
    "$r6", "$r7", "$r8", "$r9", "$r10", "$r11", "$r12", "$r13" };

/* The machine state.

   This state is maintained in host byte order.  The fetch/store
   register functions must translate between host byte order and the
   target processor byte order.  Keeping this data in target byte
   order simplifies the register read/write functions.  Keeping this
   data in native order improves the performance of the simulator.
   Simulation speed is deemed more important.  */

#define NUM_MOXIE_REGS 17 /* Including PC */
#define NUM_MOXIE_SREGS 256 /* The special registers */
#define PC_REGNO     16

/* The ordering of the moxie_regset structure is matched in the
   gdb/config/moxie/tm-moxie.h file in the REGISTER_NAMES macro.  */
struct moxie_regset
{
  word		  regs[NUM_MOXIE_REGS + 1]; /* primary registers */
  word		  sregs[256];             /* special registers */
  word            cc;                   /* the condition code reg */
  int		  exception;
  unsigned long long insts;                /* instruction counter */
};

#define CC_GT  1<<0
#define CC_LT  1<<1
#define CC_EQ  1<<2
#define CC_GTU 1<<3
#define CC_LTU 1<<4

union
{
  struct moxie_regset asregs;
  word asints [1];		/* but accessed larger... */
} cpu;

static char *myname;
static SIM_OPEN_KIND sim_kind;
static int issue_messages = 0;

void
sim_size (int s)
{
}

static void
set_initial_gprs ()
{
  int i;
  long space;
  
  /* Set up machine just out of reset.  */
  cpu.asregs.regs[PC_REGNO] = 0;
  
  /* Clean out the register contents.  */
  for (i = 0; i < NUM_MOXIE_REGS; i++)
    cpu.asregs.regs[i] = 0;
  for (i = 0; i < NUM_MOXIE_SREGS; i++)
    cpu.asregs.sregs[i] = 0;
}

static void
interrupt ()
{
  cpu.asregs.exception = SIGINT;
}

/* Write a 1 byte value to memory.  */

static void INLINE 
wbat (sim_cpu *scpu, word pc, word x, word v)
{
  address_word cia = CIA_GET (scpu);
  
  sim_core_write_aligned_1 (scpu, cia, write_map, x, v);
}

/* Write a 2 byte value to memory.  */

static void INLINE 
wsat (sim_cpu *scpu, word pc, word x, word v)
{
  address_word cia = CIA_GET (scpu);
  
  sim_core_write_aligned_2 (scpu, cia, write_map, x, v);
}

/* Write a 4 byte value to memory.  */

static void INLINE 
wlat (sim_cpu *scpu, word pc, word x, word v)
{
  address_word cia = CIA_GET (scpu);
	
  sim_core_write_aligned_4 (scpu, cia, write_map, x, v);
}

/* Read 2 bytes from memory.  */

static int INLINE 
rsat (sim_cpu *scpu, word pc, word x)
{
  address_word cia = CIA_GET (scpu);
  
  return (sim_core_read_aligned_2 (scpu, cia, read_map, x));
}

/* Read 1 byte from memory.  */

static int INLINE 
rbat (sim_cpu *scpu, word pc, word x)
{
  address_word cia = CIA_GET (scpu);
  
  return (sim_core_read_aligned_1 (scpu, cia, read_map, x));
}

/* Read 4 bytes from memory.  */

static int INLINE 
rlat (sim_cpu *scpu, word pc, word x)
{
  address_word cia = CIA_GET (scpu);
  
  return (sim_core_read_aligned_4 (scpu, cia, read_map, x));
}

#define CHECK_FLAG(T,H) if (tflags & T) { hflags |= H; tflags ^= T; }

unsigned int 
convert_target_flags (unsigned int tflags)
{
  unsigned int hflags = 0x0;

  CHECK_FLAG(0x0001, O_WRONLY);
  CHECK_FLAG(0x0002, O_RDWR);
  CHECK_FLAG(0x0008, O_APPEND);
  CHECK_FLAG(0x0200, O_CREAT);
  CHECK_FLAG(0x0400, O_TRUNC);
  CHECK_FLAG(0x0800, O_EXCL);
  CHECK_FLAG(0x2000, O_SYNC);

  if (tflags != 0x0)
    fprintf (stderr, 
	     "Simulator Error: problem converting target open flags for host.  0x%x\n", 
	     tflags);

  return hflags;
}

#define TRACE(str) if (tracing) fprintf(tracefile,"0x%08x, %s, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", opc, str, cpu.asregs.regs[0], cpu.asregs.regs[1], cpu.asregs.regs[2], cpu.asregs.regs[3], cpu.asregs.regs[4], cpu.asregs.regs[5], cpu.asregs.regs[6], cpu.asregs.regs[7], cpu.asregs.regs[8], cpu.asregs.regs[9], cpu.asregs.regs[10], cpu.asregs.regs[11], cpu.asregs.regs[12], cpu.asregs.regs[13], cpu.asregs.regs[14], cpu.asregs.regs[15]);

static int tracing = 0;

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd;
     int step, siggnal;
{
  word pc, opc;
  unsigned long long insts;
  unsigned short inst;
  void (* sigsave)();
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */
  address_word cia = CIA_GET (scpu);

  sigsave = signal (SIGINT, interrupt);
  cpu.asregs.exception = step ? SIGTRAP: 0;
  pc = cpu.asregs.regs[PC_REGNO];
  insts = cpu.asregs.insts;

  /* Run instructions here. */
  do 
    {
      opc = pc;

      /* Fetch the instruction at pc.  */
      inst = (sim_core_read_aligned_1 (scpu, cia, read_map, pc) << 8)
	+ sim_core_read_aligned_1 (scpu, cia, read_map, pc+1);

      /* Decode instruction.  */
      if (inst & (1 << 15))
	{
	  if (inst & (1 << 14))
	    {
	      /* This is a Form 3 instruction.  */
	      int opcode = (inst >> 10 & 0xf);

	      switch (opcode)
		{
		case 0x00: /* beq */
		  {
		    TRACE("beq");
		    if (cpu.asregs.cc & CC_EQ)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x01: /* bne */
		  {
		    TRACE("bne");
		    if (! (cpu.asregs.cc & CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x02: /* blt */
		  {
		    TRACE("blt");
		    if (cpu.asregs.cc & CC_LT)
		      pc += INST2OFFSET(inst);
		  }		  break;
		case 0x03: /* bgt */
		  {
		    TRACE("bgt");
		    if (cpu.asregs.cc & CC_GT)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x04: /* bltu */
		  {
		    TRACE("bltu");
		    if (cpu.asregs.cc & CC_LTU)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x05: /* bgtu */
		  {
		    TRACE("bgtu");
		    if (cpu.asregs.cc & CC_GTU)
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x06: /* bge */
		  {
		    TRACE("bge");
		    if (cpu.asregs.cc & (CC_GT | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x07: /* ble */
		  {
		    TRACE("ble");
		    if (cpu.asregs.cc & (CC_LT | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x08: /* bgeu */
		  {
		    TRACE("bgeu");
		    if (cpu.asregs.cc & (CC_GTU | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		case 0x09: /* bleu */
		  {
		    TRACE("bleu");
		    if (cpu.asregs.cc & (CC_LTU | CC_EQ))
		      pc += INST2OFFSET(inst);
		  }
		  break;
		default:
		  {
		    TRACE("SIGILL3");
		    cpu.asregs.exception = SIGILL;
		    break;
		  }
		}
	    }
	  else
	    {
	      /* This is a Form 2 instruction.  */
	      int opcode = (inst >> 12 & 0x3);
	      switch (opcode)
		{
		case 0x00: /* inc */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned av = cpu.asregs.regs[a];
		    unsigned v = (inst & 0xff);
		    TRACE("inc");
		    cpu.asregs.regs[a] = av + v;
		  }
		  break;
		case 0x01: /* dec */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned av = cpu.asregs.regs[a];
		    unsigned v = (inst & 0xff);
		    TRACE("dec");
		    cpu.asregs.regs[a] = av - v;
		  }
		  break;
		case 0x02: /* gsr */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned v = (inst & 0xff);
		    TRACE("gsr");
		    cpu.asregs.regs[a] = cpu.asregs.sregs[v];
		  }
		  break;
		case 0x03: /* ssr */
		  {
		    int a = (inst >> 8) & 0xf;
		    unsigned v = (inst & 0xff);
		    TRACE("ssr");
		    cpu.asregs.sregs[v] = cpu.asregs.regs[a];
		  }
		  break;
		default:
		  TRACE("SIGILL2");
		  cpu.asregs.exception = SIGILL;
		  break;
		}
	    }
	}
      else
	{
	  /* This is a Form 1 instruction.  */
	  int opcode = inst >> 8;
	  switch (opcode)
	    {
	    case 0x00: /* bad */
	      opc = opcode;
	      TRACE("SIGILL0");
	      cpu.asregs.exception = SIGILL;
	      break;
	    case 0x01: /* ldi.l (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;
		TRACE("ldi.l");
		unsigned int val = EXTRACT_WORD(pc+2);
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x02: /* mov (register-to-register) */
	      {
		int dest  = (inst >> 4) & 0xf;
		int src = (inst ) & 0xf;
		TRACE("mov");
		cpu.asregs.regs[dest] = cpu.asregs.regs[src];
	      }
	      break;
 	    case 0x03: /* jsra */
 	      {
 		unsigned int fn = EXTRACT_WORD(pc+2);
 		unsigned int sp = cpu.asregs.regs[1];
		TRACE("jsra");
 		/* Save a slot for the static chain.  */
		sp -= 4;

 		/* Push the return address.  */
		sp -= 4;
 		wlat (scpu, opc, sp, pc + 6);
 		
 		/* Push the current frame pointer.  */
 		sp -= 4;
 		wlat (scpu, opc, sp, cpu.asregs.regs[0]);
 
 		/* Uncache the stack pointer and set the pc and $fp.  */
		cpu.asregs.regs[1] = sp;
		cpu.asregs.regs[0] = sp;
 		pc = fn - 2;
 	      }
 	      break;
 	    case 0x04: /* ret */
 	      {
 		unsigned int sp = cpu.asregs.regs[0];

		TRACE("ret");
 
 		/* Pop the frame pointer.  */
 		cpu.asregs.regs[0] = rlat (scpu, opc, sp);
 		sp += 4;
 		
 		/* Pop the return address.  */
 		pc = rlat (scpu, opc, sp) - 2;
 		sp += 4;

		/* Skip over the static chain slot.  */
		sp += 4;
 
 		/* Uncache the stack pointer.  */
 		cpu.asregs.regs[1] = sp;
  	      }
  	      break;
	    case 0x05: /* add.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];
		TRACE("add.l");
		cpu.asregs.regs[a] = av + bv;
	      }
	      break;
	    case 0x06: /* push */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int sp = cpu.asregs.regs[a] - 4;
		TRACE("push");
		wlat (scpu, opc, sp, cpu.asregs.regs[b]);
		cpu.asregs.regs[a] = sp;
	      }
	      break;
	    case 0x07: /* pop */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int sp = cpu.asregs.regs[a];
		TRACE("pop");
		cpu.asregs.regs[b] = rlat (scpu, opc, sp);
		cpu.asregs.regs[a] = sp + 4;
	      }
	      break;
	    case 0x08: /* lda.l */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("lda.l");
		cpu.asregs.regs[reg] = rlat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x09: /* sta.l */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("sta.l");
		wlat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x0a: /* ld.l (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;
		TRACE("ld.l");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rlat (scpu, opc, xv);
	      }
	      break;
	    case 0x0b: /* st.l */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;
		TRACE("st.l");
		wlat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x0c: /* ldo.l */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("ldo.l");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rlat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x0d: /* sto.l */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("sto.l");
		addr += cpu.asregs.regs[a];
		wlat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 4;
	      }
	      break;
	    case 0x0e: /* cmp */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int cc = 0;
		int va = cpu.asregs.regs[a];
		int vb = cpu.asregs.regs[b]; 

		TRACE("cmp");

		if (va == vb)
		  cc = CC_EQ;
		else
		  {
		    cc |= (va < vb ? CC_LT : 0);
		    cc |= (va > vb ? CC_GT : 0);
		    cc |= ((unsigned int) va < (unsigned int) vb ? CC_LTU : 0);
		    cc |= ((unsigned int) va > (unsigned int) vb ? CC_GTU : 0);
		  }

		cpu.asregs.cc = cc;
	      }
	      break;
	    case 0x0f: /* nop */
	      break;
	    case 0x10: /* bad */
	    case 0x11: /* bad */
	    case 0x12: /* bad */
	    case 0x13: /* bad */
	    case 0x14: /* bad */
	    case 0x15: /* bad */
	    case 0x16: /* bad */
	    case 0x17: /* bad */
	    case 0x18: /* bad */
	      {
		opc = opcode;
		TRACE("SIGILL0");
		cpu.asregs.exception = SIGILL;
		break;
	      }
	    case 0x19: /* jsr */
	      {
		unsigned int fn = cpu.asregs.regs[(inst >> 4) & 0xf];
		unsigned int sp = cpu.asregs.regs[1];

		TRACE("jsr");

 		/* Save a slot for the static chain.  */
		sp -= 4;

		/* Push the return address.  */
		sp -= 4;
		wlat (scpu, opc, sp, pc + 2);
		
		/* Push the current frame pointer.  */
		sp -= 4;
		wlat (scpu, opc, sp, cpu.asregs.regs[0]);

		/* Uncache the stack pointer and set the fp & pc.  */
		cpu.asregs.regs[1] = sp;
		cpu.asregs.regs[0] = sp;
		pc = fn - 2;
	      }
	      break;
	    case 0x1a: /* jmpa */
	      {
		unsigned int tgt = EXTRACT_WORD(pc+2);
		TRACE("jmpa");
		pc = tgt - 2;
	      }
	      break;
	    case 0x1b: /* ldi.b (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;

		unsigned int val = EXTRACT_WORD(pc+2);
		TRACE("ldi.b");
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x1c: /* ld.b (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;
		TRACE("ld.b");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rbat (scpu, opc, xv);
	      }
	      break;
	    case 0x1d: /* lda.b */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("lda.b");
		cpu.asregs.regs[reg] = rbat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x1e: /* st.b */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;
		TRACE("st.b");
		wbat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x1f: /* sta.b */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("sta.b");
		wbat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x20: /* ldi.s (immediate) */
	      {
		int reg = (inst >> 4) & 0xf;

		unsigned int val = EXTRACT_WORD(pc+2);
		TRACE("ldi.s");
		cpu.asregs.regs[reg] = val;
		pc += 4;
	      }
	      break;
	    case 0x21: /* ld.s (register indirect) */
	      {
		int src  = inst & 0xf;
		int dest = (inst >> 4) & 0xf;
		int xv;
		TRACE("ld.s");
		xv = cpu.asregs.regs[src];
		cpu.asregs.regs[dest] = rsat (scpu, opc, xv);
	      }
	      break;
	    case 0x22: /* lda.s */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("lda.s");
		cpu.asregs.regs[reg] = rsat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x23: /* st.s */
	      {
		int dest = (inst >> 4) & 0xf;
		int val  = inst & 0xf;
		TRACE("st.s");
		wsat (scpu, opc, cpu.asregs.regs[dest], cpu.asregs.regs[val]);
	      }
	      break;
	    case 0x24: /* sta.s */
	      {
		int reg = (inst >> 4) & 0xf;
		unsigned int addr = EXTRACT_WORD(pc+2);
		TRACE("sta.s");
		wsat (scpu, opc, addr, cpu.asregs.regs[reg]);
		pc += 4;
	      }
	      break;
	    case 0x25: /* jmp */
	      {
		int reg = (inst >> 4) & 0xf;
		TRACE("jmp");
		pc = cpu.asregs.regs[reg] - 2;
	      }
	      break;
	    case 0x26: /* and */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;
		TRACE("and");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av & bv;
	      }
	      break;
	    case 0x27: /* lshr */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];
		TRACE("lshr");
		cpu.asregs.regs[a] = (unsigned) ((unsigned) av >> bv);
	      }
	      break;
	    case 0x28: /* ashl */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];
		TRACE("ashl");
		cpu.asregs.regs[a] = av << bv;
	      }
	      break;
	    case 0x29: /* sub.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];
		TRACE("sub.l");
		cpu.asregs.regs[a] = av - bv;
	      }
	      break;
	    case 0x2a: /* neg */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int bv = cpu.asregs.regs[b];
		TRACE("neg");
		cpu.asregs.regs[a] = - bv;
	      }
	      break;
	    case 0x2b: /* or */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;
		TRACE("or");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av | bv;
	      }
	      break;
	    case 0x2c: /* not */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int bv = cpu.asregs.regs[b];
		TRACE("not");
		cpu.asregs.regs[a] = 0xffffffff ^ bv;
	      }
	      break;
	    case 0x2d: /* ashr */
	      {
		int a  = (inst >> 4) & 0xf;
		int b  = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];
		TRACE("ashr");
		cpu.asregs.regs[a] = av >> bv;
	      }
	      break;
	    case 0x2e: /* xor */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av, bv;
		TRACE("xor");
		av = cpu.asregs.regs[a];
		bv = cpu.asregs.regs[b];
		cpu.asregs.regs[a] = av ^ bv;
	      }
	      break;
	    case 0x2f: /* mul.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned av = cpu.asregs.regs[a];
		unsigned bv = cpu.asregs.regs[b];
		TRACE("mul.l");
		cpu.asregs.regs[a] = av * bv;
	      }
	      break;
	    case 0x30: /* swi */
	      {
		unsigned int inum = EXTRACT_WORD(pc+2);
		TRACE("swi");
		/* Set the special registers appropriately.  */
		cpu.asregs.sregs[2] = 3; /* MOXIE_EX_SWI */
	        cpu.asregs.sregs[3] = inum;
		switch (inum)
		  {
		  case 0x1: /* SYS_exit */
		    {
		      cpu.asregs.exception = SIGQUIT;
		      break;
		    }
		  case 0x2: /* SYS_open */
		    {
		      char fname[1024];
		      int mode = (int) convert_target_flags ((unsigned) cpu.asregs.regs[3]);
		      int perm = (int) cpu.asregs.regs[4];
		      int fd = open (fname, mode, perm);
		      sim_core_read_buffer (sd, scpu, read_map, fname,
					    cpu.asregs.regs[2], 1024);
		      /* FIXME - set errno */
		      cpu.asregs.regs[2] = fd;
		      break;
		    }
		  case 0x4: /* SYS_read */
		    {
		      int fd = cpu.asregs.regs[2];
		      unsigned len = (unsigned) cpu.asregs.regs[4];
		      char *buf = malloc (len);
		      cpu.asregs.regs[2] = read (fd, buf, len);
		      sim_core_write_buffer (sd, scpu, write_map, buf,
					     cpu.asregs.regs[3], len);
		      free (buf);
		      break;
		    }
		  case 0x5: /* SYS_write */
		    {
		      char *str;
		      /* String length is at 0x12($fp) */
		      unsigned count, len = (unsigned) cpu.asregs.regs[4];
		      str = malloc (len);
		      sim_core_read_buffer (sd, scpu, read_map, str,
					    cpu.asregs.regs[3], len);
		      count = write (cpu.asregs.regs[2], str, len);
		      free (str);
		      cpu.asregs.regs[2] = count;
		      break;
		    }
		  case 0xffffffff: /* Linux System Call */
		    {
		      unsigned int handler = cpu.asregs.sregs[1];
		      unsigned int sp = cpu.asregs.regs[1];

		      /* Save a slot for the static chain.  */
		      sp -= 4;

		      /* Push the return address.  */
		      sp -= 4;
		      wlat (scpu, opc, sp, pc + 6);
		
		      /* Push the current frame pointer.  */
		      sp -= 4;
		      wlat (scpu, opc, sp, cpu.asregs.regs[0]);

		      /* Uncache the stack pointer and set the fp & pc.  */
		      cpu.asregs.regs[1] = sp;
		      cpu.asregs.regs[0] = sp;
		      pc = handler - 6;
		    }
		  default:
		    break;
		  }
		pc += 4;
	      }
	      break;
	    case 0x31: /* div.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];
		TRACE("div.l");
		cpu.asregs.regs[a] = av / bv;
	      }
	      break;
	    case 0x32: /* udiv.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned int av = cpu.asregs.regs[a];
		unsigned int bv = cpu.asregs.regs[b];
		TRACE("udiv.l");
		cpu.asregs.regs[a] = (av / bv);
	      }
	      break;
	    case 0x33: /* mod.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		int av = cpu.asregs.regs[a];
		int bv = cpu.asregs.regs[b];
		TRACE("mod.l");
		cpu.asregs.regs[a] = av % bv;
	      }
	      break;
	    case 0x34: /* umod.l */
	      {
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		unsigned int av = cpu.asregs.regs[a];
		unsigned int bv = cpu.asregs.regs[b];
		TRACE("umod.l");
		cpu.asregs.regs[a] = (av % bv);
	      }
	      break;
	    case 0x35: /* brk */
	      TRACE("brk");
	      cpu.asregs.exception = SIGTRAP;
	      pc -= 2; /* Adjust pc */
	      break;
	    case 0x36: /* ldo.b */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("ldo.b");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rbat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x37: /* sto.b */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("sto.b");
		addr += cpu.asregs.regs[a];
		wbat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 4;
	      }
	      break;
	    case 0x38: /* ldo.s */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("ldo.s");
		addr += cpu.asregs.regs[b];
		cpu.asregs.regs[a] = rsat (scpu, opc, addr);
		pc += 4;
	      }
	      break;
	    case 0x39: /* sto.s */
	      {
		unsigned int addr = EXTRACT_WORD(pc+2);
		int a = (inst >> 4) & 0xf;
		int b = inst & 0xf;
		TRACE("sto.s");
		addr += cpu.asregs.regs[a];
		wsat (scpu, opc, addr, cpu.asregs.regs[b]);
		pc += 4;
	      }
	      break;
	    default:
	      opc = opcode;
	      TRACE("SIGILL1");
	      cpu.asregs.exception = SIGILL;
	      break;
	    }
	}

      insts++;
      pc += 2;

    } while (!cpu.asregs.exception);

  /* Hide away the things we've cached while executing.  */
  cpu.asregs.regs[PC_REGNO] = pc;
  cpu.asregs.insts += insts;		/* instructions done ... */

  signal (SIGINT, sigsave);
}

int
sim_write (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     const unsigned char * buffer;
     int size;
{
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */

  sim_core_write_buffer (sd, scpu, write_map, buffer, addr, size);

  return size;
}

int
sim_read (sd, addr, buffer, size)
     SIM_DESC sd;
     SIM_ADDR addr;
     unsigned char * buffer;
     int size;
{
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */

  sim_core_read_buffer (sd, scpu, read_map, buffer, addr, size);
  
  return size;
}


int
sim_store_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char * memory;
     int length;
{
  if (rn < NUM_MOXIE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival;
	  
	  /* misalignment safe */
	  ival = moxie_extract_unsigned_integer (memory, 4);
	  cpu.asints[rn] = ival;
	}

      return 4;
    }
  else
    return 0;
}

int
sim_fetch_register (sd, rn, memory, length)
     SIM_DESC sd;
     int rn;
     unsigned char * memory;
     int length;
{
  if (rn < NUM_MOXIE_REGS && rn >= 0)
    {
      if (length == 4)
	{
	  long ival = cpu.asints[rn];

	  /* misalignment-safe */
	  moxie_store_unsigned_integer (memory, 4, ival);
	}
      
      return 4;
    }
  else
    return 0;
}


int
sim_trace (sd)
     SIM_DESC sd;
{
  if (tracefile == 0)
    tracefile = fopen("trace.csv", "wb");

  tracing = 1;
  
  sim_resume (sd, 0, 0);

  tracing = 0;
  
  return 1;
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd;
     enum sim_stop * reason;
     int * sigrc;
{
  if (cpu.asregs.exception == SIGQUIT)
    {
      * reason = sim_exited;
      * sigrc = cpu.asregs.regs[2];
    }
  else
    {
      * reason = sim_stopped;
      * sigrc = cpu.asregs.exception;
    }
}


int
sim_stop (sd)
     SIM_DESC sd;
{
  cpu.asregs.exception = SIGINT;
  return 1;
}


void
sim_info (sd, verbose)
     SIM_DESC sd;
     int verbose;
{
  callback->printf_filtered (callback, "\n\n# instructions executed  %llu\n",
			     cpu.asregs.insts);
}


SIM_DESC
sim_open (kind, cb, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback * cb;
     struct bfd * abfd;
     char ** argv;
{
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    return 0;

  sim_do_command(sd," memory region 0x00000000,0x4000000") ; 
  sim_do_command(sd," memory region 0xE0000000,0x10000") ; 

  myname = argv[0];
  callback = cb;
  
  if (kind == SIM_OPEN_STANDALONE)
    issue_messages = 1;
  
  set_initial_gprs ();	/* Reset the GPR registers.  */
  
  /* Configure/verify the target byte order and other runtime
     configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      sim_module_uninstall (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      /* Uninstall the modules to avoid memory leaks,
	 file descriptor leaks, etc.  */
      sim_module_uninstall (sd);
      return 0;
    }

  return sd;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  /* nothing to do */
}


/* Load the device tree blob.  */

static void
load_dtb (SIM_DESC sd, const char *filename)
{
  int size = 0;
  FILE *f = fopen (filename, "rb");
  char *buf;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */ 
 if (f == NULL)
    {
      printf ("WARNING: ``%s'' could not be opened.\n", filename);
      return;
    }
  fseek (f, 0, SEEK_END);
  size = ftell(f);
  fseek (f, 0, SEEK_SET);
  buf = alloca (size);
  if (size != fread (buf, 1, size, f))
    {
      printf ("ERROR: error reading ``%s''.\n", filename);
      return;
    }
  sim_core_write_buffer (sd, scpu, write_map, buf, 0xE0000000, size);
  cpu.asregs.sregs[9] = 0xE0000000;
  fclose (f);
}

SIM_RC
sim_load (sd, prog, abfd, from_tty)
     SIM_DESC sd;
     char * prog;
     bfd * abfd;
     int from_tty;
{

  /* Do the right thing for ELF executables; this turns out to be
     just about the right thing for any object format that:
       - we crack using BFD routines
       - follows the traditional UNIX text/data/bss layout
       - calls the bss section ".bss".   */

  extern bfd * sim_load_file (); /* ??? Don't know where this should live.  */
  bfd * prog_bfd;

  {
    bfd * handle;
    handle = bfd_openr (prog, 0);	/* could be "moxie" */
    
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

    /* Clean up after ourselves.  */
    bfd_close (handle);
  }

  /* from sh -- dac */
  prog_bfd = sim_load_file (sd, myname, callback, prog, abfd,
                            sim_kind == SIM_OPEN_DEBUG,
                            0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;
  
  if (abfd == NULL)
    bfd_close (prog_bfd);

  return SIM_RC_OK;
}

SIM_RC
sim_create_inferior (sd, prog_bfd, argv, env)
     SIM_DESC sd;
     struct bfd * prog_bfd;
     char ** argv;
     char ** env;
{
  char ** avp;
  int l, argc, i, tp;
  sim_cpu *scpu = STATE_CPU (sd, 0); /* FIXME */

  /* Set the initial register set.  */
  l = issue_messages;
  issue_messages = 0;
  set_initial_gprs ();
  issue_messages = l;
  
  if (prog_bfd != NULL)
    cpu.asregs.regs[PC_REGNO] = bfd_get_start_address (prog_bfd);

  /* Copy args into target memory.  */
  avp = argv;
  for (argc = 0; avp && *avp; avp++)
    argc++;

  /* Target memory looks like this:
     0x00000000 zero word
     0x00000004 argc word
     0x00000008 start of argv
     .
     0x0000???? end of argv
     0x0000???? zero word 
     0x0000???? start of data pointed to by argv  */

  wlat (scpu, 0, 0, 0);
  wlat (scpu, 0, 4, argc);

  /* tp is the offset of our first argv data.  */
  tp = 4 + 4 + argc * 4 + 4;

  for (i = 0; i < argc; i++)
    {
      /* Set the argv value.  */
      wlat (scpu, 0, 4 + 4 + i * 4, tp);

      /* Store the string.  */
      sim_core_write_buffer (sd, scpu, write_map, argv[i],
			     tp, strlen(argv[i])+1);
      tp += strlen (argv[i]) + 1;
    }

  wlat (scpu, 0, 4 + 4 + i * 4, 0);

  load_dtb (sd, DTB);

  return SIM_RC_OK;
}

void
sim_kill (sd)
     SIM_DESC sd;
{
  if (tracefile)
    fclose(tracefile);
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd;
     char * cmd;
{
  if (sim_args_command (sd, cmd) != SIM_RC_OK)
    sim_io_printf (sd, 
		   "Error: \"%s\" is not a valid moxie simulator command.\n",
		   cmd);
}

void
sim_set_callbacks (ptr)
     host_callback * ptr;
{
  callback = ptr; 
}
