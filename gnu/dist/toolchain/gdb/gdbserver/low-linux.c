/* Low level interface to ptrace, for the remote server for GDB.
   Copyright (C) 1995, 1996 Free Software Foundation, Inc.

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
#include <sys/wait.h>
#include "frame.h"
#include "inferior.h"

#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <fcntl.h>

/***************Begin MY defs*********************/
int quit_flag = 0;
static char my_registers[REGISTER_BYTES];
char *registers = my_registers;

/* Index within `registers' of the first byte of the space for
   register N.  */


char buf2[MAX_REGISTER_RAW_SIZE];
/***************End MY defs*********************/

#ifdef HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

/* Default the type of the ptrace transfer to int.  */
#ifndef PTRACE_XFER_TYPE
#define PTRACE_XFER_TYPE int
#endif

extern char **environ;
extern int errno;
extern int inferior_pid;
void quit (), perror_with_name ();
int query ();

static void initialize_arch (void);

/* Start an inferior process and returns its pid.
   ALLARGS is a vector of program-name and args.
   ENV is the environment vector to pass.  */

int
create_inferior (program, allargs)
     char *program;
     char **allargs;
{
  int pid;

  pid = fork ();
  if (pid < 0)
    perror_with_name ("fork");

  if (pid == 0)
    {
      ptrace (PTRACE_TRACEME, 0, 0, 0);

      execv (program, allargs);

      fprintf (stderr, "Cannot exec %s: %s.\n", program,
	       errno < sys_nerr ? sys_errlist[errno] : "unknown error");
      fflush (stderr);
      _exit (0177);
    }

  return pid;
}

/* Kill the inferior process.  Make us have no inferior.  */

void
kill_inferior ()
{
  if (inferior_pid == 0)
    return;
  ptrace (PTRACE_KILL, inferior_pid, 0, 0);
  wait (0);
/*************inferior_died ();****VK**************/
}

/* Return nonzero if the given thread is still alive.  */
int
mythread_alive (pid)
     int pid;
{
  return 1;
}

/* Wait for process, returns status */

unsigned char
mywait (status)
     char *status;
{
  int pid;
  union wait w;

  pid = wait (&w);
  if (pid != inferior_pid)
    perror_with_name ("wait");

  if (WIFEXITED (w))
    {
      fprintf (stderr, "\nChild exited with retcode = %x \n", WEXITSTATUS (w));
      *status = 'W';
      return ((unsigned char) WEXITSTATUS (w));
    }
  else if (!WIFSTOPPED (w))
    {
      fprintf (stderr, "\nChild terminated with signal = %x \n", WTERMSIG (w));
      *status = 'X';
      return ((unsigned char) WTERMSIG (w));
    }

  fetch_inferior_registers (0);

  *status = 'T';
  return ((unsigned char) WSTOPSIG (w));
}

/* Resume execution of the inferior process.
   If STEP is nonzero, single-step it.
   If SIGNAL is nonzero, give it that signal.  */

void
myresume (step, signal)
     int step;
     int signal;
{
  errno = 0;
  ptrace (step ? PTRACE_SINGLESTEP : PTRACE_CONT, inferior_pid, 1, signal);
  if (errno)
    perror_with_name ("ptrace");
}


#if !defined (offsetof)
#define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)
#endif

/* U_REGS_OFFSET is the offset of the registers within the u area.  */
#if !defined (U_REGS_OFFSET)
#define U_REGS_OFFSET \
  ptrace (PT_READ_U, inferior_pid, \
          (PTRACE_ARG3_TYPE) (offsetof (struct user, u_ar0)), 0) \
    - KERNEL_U_ADDR
#endif

#ifdef I386_GNULINUX_TARGET
/* i386_register_raw_size[i] is the number of bytes of storage in the
   actual machine representation for register i.  */
int i386_register_raw_size[MAX_NUM_REGS] = {
   4,  4,  4,  4,
   4,  4,  4,  4,
   4,  4,  4,  4,
   4,  4,  4,  4,
  10, 10, 10, 10,
  10, 10, 10, 10,
   4,  4,  4,  4,
   4,  4,  4,  4,
  16, 16, 16, 16,
  16, 16, 16, 16,
   4
};

int i386_register_byte[MAX_NUM_REGS];

static void
initialize_arch ()
{
  /* Initialize the table saying where each register starts in the
     register file.  */
  {
    int i, offset;

    offset = 0;
    for (i = 0; i < MAX_NUM_REGS; i++)
      {
	i386_register_byte[i] = offset;
	offset += i386_register_raw_size[i];
      }
  }
}

/* this table must line up with REGISTER_NAMES in tm-i386v.h */
/* symbols like 'EAX' come from <sys/reg.h> */
static int regmap[] =
{
  EAX, ECX, EDX, EBX,
  UESP, EBP, ESI, EDI,
  EIP, EFL, CS, SS,
  DS, ES, FS, GS,
};

int
i386_register_u_addr (blockend, regnum)
     int blockend;
     int regnum;
{
#if 0
  /* this will be needed if fp registers are reinstated */
  /* for now, you can look at them with 'info float'
   * sys5 wont let you change them with ptrace anyway
   */
  if (regnum >= FP0_REGNUM && regnum <= FP7_REGNUM)
    {
      int ubase, fpstate;
      struct user u;
      ubase = blockend + 4 * (SS + 1) - KSTKSZ;
      fpstate = ubase + ((char *) &u.u_fpstate - (char *) &u);
      return (fpstate + 0x1c + 10 * (regnum - FP0_REGNUM));
    }
  else
#endif
    return (blockend + 4 * regmap[regnum]);

}
#elif defined(TARGET_M68K)
static void
initialize_arch ()
{
  return;
}

/* This table must line up with REGISTER_NAMES in tm-m68k.h */
static int regmap[] =
{
#ifdef PT_D0
  PT_D0, PT_D1, PT_D2, PT_D3, PT_D4, PT_D5, PT_D6, PT_D7,
  PT_A0, PT_A1, PT_A2, PT_A3, PT_A4, PT_A5, PT_A6, PT_USP,
  PT_SR, PT_PC,
#else
  14, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15,
  17, 18,
#endif
#ifdef PT_FP0
  PT_FP0, PT_FP1, PT_FP2, PT_FP3, PT_FP4, PT_FP5, PT_FP6, PT_FP7,
  PT_FPCR, PT_FPSR, PT_FPIAR
#else
  21, 24, 27, 30, 33, 36, 39, 42, 45, 46, 47
#endif
};

/* BLOCKEND is the value of u.u_ar0, and points to the place where GS
   is stored.  */

int
m68k_linux_register_u_addr (blockend, regnum)
     int blockend;
     int regnum;
{
  return (blockend + 4 * regmap[regnum]);
}
#elif defined(IA64_GNULINUX_TARGET)
#undef NUM_FREGS
#define NUM_FREGS 0

#include <asm/ptrace_offsets.h>

static int u_offsets[] =
  {
    /* general registers */
    -1,		/* gr0 not available; i.e, it's always zero */
    PT_R1,
    PT_R2,
    PT_R3,
    PT_R4,
    PT_R5,
    PT_R6,
    PT_R7,
    PT_R8,
    PT_R9,
    PT_R10,
    PT_R11,
    PT_R12,
    PT_R13,
    PT_R14,
    PT_R15,
    PT_R16,
    PT_R17,
    PT_R18,
    PT_R19,
    PT_R20,
    PT_R21,
    PT_R22,
    PT_R23,
    PT_R24,
    PT_R25,
    PT_R26,
    PT_R27,
    PT_R28,
    PT_R29,
    PT_R30,
    PT_R31,
    /* gr32 through gr127 not directly available via the ptrace interface */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* Floating point registers */
    -1, -1,	/* f0 and f1 not available (f0 is +0.0 and f1 is +1.0) */
    PT_F2,
    PT_F3,
    PT_F4,
    PT_F5,
    PT_F6,
    PT_F7,
    PT_F8,
    PT_F9,
    PT_F10,
    PT_F11,
    PT_F12,
    PT_F13,
    PT_F14,
    PT_F15,
    PT_F16,
    PT_F17,
    PT_F18,
    PT_F19,
    PT_F20,
    PT_F21,
    PT_F22,
    PT_F23,
    PT_F24,
    PT_F25,
    PT_F26,
    PT_F27,
    PT_F28,
    PT_F29,
    PT_F30,
    PT_F31,
    PT_F32,
    PT_F33,
    PT_F34,
    PT_F35,
    PT_F36,
    PT_F37,
    PT_F38,
    PT_F39,
    PT_F40,
    PT_F41,
    PT_F42,
    PT_F43,
    PT_F44,
    PT_F45,
    PT_F46,
    PT_F47,
    PT_F48,
    PT_F49,
    PT_F50,
    PT_F51,
    PT_F52,
    PT_F53,
    PT_F54,
    PT_F55,
    PT_F56,
    PT_F57,
    PT_F58,
    PT_F59,
    PT_F60,
    PT_F61,
    PT_F62,
    PT_F63,
    PT_F64,
    PT_F65,
    PT_F66,
    PT_F67,
    PT_F68,
    PT_F69,
    PT_F70,
    PT_F71,
    PT_F72,
    PT_F73,
    PT_F74,
    PT_F75,
    PT_F76,
    PT_F77,
    PT_F78,
    PT_F79,
    PT_F80,
    PT_F81,
    PT_F82,
    PT_F83,
    PT_F84,
    PT_F85,
    PT_F86,
    PT_F87,
    PT_F88,
    PT_F89,
    PT_F90,
    PT_F91,
    PT_F92,
    PT_F93,
    PT_F94,
    PT_F95,
    PT_F96,
    PT_F97,
    PT_F98,
    PT_F99,
    PT_F100,
    PT_F101,
    PT_F102,
    PT_F103,
    PT_F104,
    PT_F105,
    PT_F106,
    PT_F107,
    PT_F108,
    PT_F109,
    PT_F110,
    PT_F111,
    PT_F112,
    PT_F113,
    PT_F114,
    PT_F115,
    PT_F116,
    PT_F117,
    PT_F118,
    PT_F119,
    PT_F120,
    PT_F121,
    PT_F122,
    PT_F123,
    PT_F124,
    PT_F125,
    PT_F126,
    PT_F127,
    /* predicate registers - we don't fetch these individually */
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    /* branch registers */
    PT_B0,
    PT_B1,
    PT_B2,
    PT_B3,
    PT_B4,
    PT_B5,
    PT_B6,
    PT_B7,
    /* virtual frame pointer and virtual return address pointer */
    -1, -1,
    /* other registers */
    PT_PR,
    PT_CR_IIP,	/* ip */
    PT_CR_IPSR, /* psr */
    PT_CR_IFS,	/* cfm */
    /* kernel registers not visible via ptrace interface (?) */
    -1, -1, -1, -1, -1, -1, -1, -1,
    /* hole */
    -1, -1, -1, -1, -1, -1, -1, -1,
    PT_AR_RSC,
    PT_AR_BSP,
    PT_AR_BSPSTORE,
    PT_AR_RNAT,
    -1,
    -1,		/* Not available: FCR, IA32 floating control register */
    -1, -1,
    -1,		/* Not available: EFLAG */
    -1,		/* Not available: CSD */
    -1,		/* Not available: SSD */
    -1,		/* Not available: CFLG */
    -1,		/* Not available: FSR */
    -1,		/* Not available: FIR */
    -1,		/* Not available: FDR */
    -1,
    PT_AR_CCV,
    -1, -1, -1,
    PT_AR_UNAT,
    -1, -1, -1,
    PT_AR_FPSR,
    -1, -1, -1,
    -1,		/* Not available: ITC */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1,
    PT_AR_PFS,
    PT_AR_LC,
    -1,		/* Not available: EC, the Epilog Count register */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1,
    /* nat bits - not fetched directly; instead we obtain these bits from
       either rnat or unat or from memory. */
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
  };

int
ia64_register_u_addr (int blockend, int regnum)
{
  int addr;

  if (regnum < 0 || regnum >= NUM_REGS)
    error ("Invalid register number %d.", regnum);

  addr = u_offsets[regnum];
  if (addr == -1)
    addr = 0;

  return addr;
}

static void
initialize_arch ()
{
  return;
}
#endif

CORE_ADDR
register_addr (regno, blockend)
     int regno;
     CORE_ADDR blockend;
{
  CORE_ADDR addr;

  if (regno < 0 || regno >= ARCH_NUM_REGS)
    error ("Invalid register number %d.", regno);

  REGISTER_U_ADDR (addr, blockend, regno);

  return addr;
}

/* Fetch one register.  */

static void
fetch_register (regno)
     int regno;
{
  CORE_ADDR regaddr;
  register int i;

  /* Offset of registers within the u area.  */
  unsigned int offset;

  offset = U_REGS_OFFSET;

  regaddr = register_addr (regno, offset);
  for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      *(PTRACE_XFER_TYPE *) &registers[REGISTER_BYTE (regno) + i] =
	ptrace (PTRACE_PEEKUSER, inferior_pid, (PTRACE_ARG3_TYPE) regaddr, 0);
      regaddr += sizeof (PTRACE_XFER_TYPE);
      if (errno != 0)
	{
	  /* Warning, not error, in case we are attached; sometimes the
	     kernel doesn't let us at the registers.  */
	  char *err = strerror (errno);
	  char *msg = alloca (strlen (err) + 128);
	  sprintf (msg, "reading register %d: %s", regno, err);
	  error (msg);
	  goto error_exit;
	}
    }
error_exit:;
}

/* Fetch all registers, or just one, from the child process.  */

void
fetch_inferior_registers (regno)
     int regno;
{
  if (regno == -1 || regno == 0)
    for (regno = 0; regno < NUM_REGS - NUM_FREGS; regno++)
      fetch_register (regno);
  else
    fetch_register (regno);
}

/* Store our register values back into the inferior.
   If REGNO is -1, do this for all registers.
   Otherwise, REGNO specifies which register (so we can save time).  */

void
store_inferior_registers (regno)
     int regno;
{
  CORE_ADDR regaddr;
  int i;
  unsigned int offset = U_REGS_OFFSET;

  if (regno >= 0)
    {
#if 0
      if (CANNOT_STORE_REGISTER (regno))
	return;
#endif
      regaddr = register_addr (regno, offset);
      errno = 0;
#if 0
      if (regno == PCOQ_HEAD_REGNUM || regno == PCOQ_TAIL_REGNUM)
	{
	  scratch = *(int *) &registers[REGISTER_BYTE (regno)] | 0x3;
	  ptrace (PT_WUREGS, inferior_pid, (PTRACE_ARG3_TYPE) regaddr,
		  scratch, 0);
	  if (errno != 0)
	    {
	      /* Error, even if attached.  Failing to write these two
	         registers is pretty serious.  */
	      sprintf (buf, "writing register number %d", regno);
	      perror_with_name (buf);
	    }
	}
      else
#endif
	for (i = 0; i < REGISTER_RAW_SIZE (regno); i += sizeof (int))
	  {
	    errno = 0;
	    ptrace (PTRACE_POKEUSER, inferior_pid, (PTRACE_ARG3_TYPE) regaddr,
		    *(int *) &registers[REGISTER_BYTE (regno) + i]);
	    if (errno != 0)
	      {
		/* Warning, not error, in case we are attached; sometimes the
		   kernel doesn't let us at the registers.  */
		char *err = strerror (errno);
		char *msg = alloca (strlen (err) + 128);
		sprintf (msg, "writing register %d: %s",
			 regno, err);
		error (msg);
		return;
	      }
	    regaddr += sizeof (int);
	  }
    }
  else
    for (regno = 0; regno < NUM_REGS - NUM_FREGS; regno++)
      store_inferior_registers (regno);
}

/* NOTE! I tried using PTRACE_READDATA, etc., to read and write memory
   in the NEW_SUN_PTRACE case.
   It ought to be straightforward.  But it appears that writing did
   not write the data that I specified.  I cannot understand where
   it got the data that it actually did write.  */

/* Copy LEN bytes from inferior's memory starting at MEMADDR
   to debugger memory starting at MYADDR.  */

void
read_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & -sizeof (PTRACE_XFER_TYPE);
  /* Round ending address up; get number of longwords that makes.  */
  register int count 
    = (((memaddr + len) - addr) + sizeof (PTRACE_XFER_TYPE) - 1) 
      / sizeof (PTRACE_XFER_TYPE);
  /* Allocate buffer of that many longwords.  */
  register PTRACE_XFER_TYPE *buffer 
    = (PTRACE_XFER_TYPE *) alloca (count * sizeof (PTRACE_XFER_TYPE));

  /* Read all the longwords */
  for (i = 0; i < count; i++, addr += sizeof (PTRACE_XFER_TYPE))
    {
      buffer[i] = ptrace (PTRACE_PEEKTEXT, inferior_pid, addr, 0);
    }

  /* Copy appropriate bytes out of the buffer.  */
  memcpy (myaddr, (char *) buffer + (memaddr & (sizeof (PTRACE_XFER_TYPE) - 1)), len);
}

/* Copy LEN bytes of data from debugger memory at MYADDR
   to inferior's memory at MEMADDR.
   On failure (cannot write the inferior)
   returns the value of errno.  */

int
write_inferior_memory (memaddr, myaddr, len)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
{
  register int i;
  /* Round starting address down to longword boundary.  */
  register CORE_ADDR addr = memaddr & -sizeof (PTRACE_XFER_TYPE);
  /* Round ending address up; get number of longwords that makes.  */
  register int count
  = (((memaddr + len) - addr) + sizeof (PTRACE_XFER_TYPE) - 1) / sizeof (PTRACE_XFER_TYPE);
  /* Allocate buffer of that many longwords.  */
  register PTRACE_XFER_TYPE *buffer = (PTRACE_XFER_TYPE *) alloca (count * sizeof (PTRACE_XFER_TYPE));
  extern int errno;

  /* Fill start and end extra bytes of buffer with existing memory data.  */

  buffer[0] = ptrace (PTRACE_PEEKTEXT, inferior_pid, addr, 0);

  if (count > 1)
    {
      buffer[count - 1]
	= ptrace (PTRACE_PEEKTEXT, inferior_pid,
		  addr + (count - 1) * sizeof (PTRACE_XFER_TYPE), 0);
    }

  /* Copy data to be written over corresponding part of buffer */

  memcpy ((char *) buffer + (memaddr & (sizeof (PTRACE_XFER_TYPE) - 1)), myaddr, len);

  /* Write the entire buffer.  */

  for (i = 0; i < count; i++, addr += sizeof (PTRACE_XFER_TYPE))
    {
      errno = 0;
      ptrace (PTRACE_POKETEXT, inferior_pid, addr, buffer[i]);
      if (errno)
	return errno;
    }

  return 0;
}

void
initialize_low ()
{
  initialize_arch ();
}
