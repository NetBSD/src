/* Native debugging support for Intel x86 running DJGPP.
   Copyright 1997, 1999 Free Software Foundation, Inc.
   Written by Robert Hoehne.

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

#include <fcntl.h>

#include "defs.h"
#include "inferior.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "command.h"
#include "floatformat.h"
#include "buildsym.h"

#include <stdio.h>		/* required for __DJGPP_MINOR__ */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <io.h>
#include <dpmi.h>
#include <debug/v2load.h>
#include <debug/dbgcom.h>
#if __DJGPP_MINOR__ > 2
#include <debug/redir.h>
#endif

#if __DJGPP_MINOR__ < 3
/* This code will be provided from DJGPP 2.03 on. Until then I code it
   here */
typedef struct
  {
    unsigned short sig0;
    unsigned short sig1;
    unsigned short sig2;
    unsigned short sig3;
    unsigned short exponent:15;
    unsigned short sign:1;
  }
NPXREG;

typedef struct
  {
    unsigned int control;
    unsigned int status;
    unsigned int tag;
    unsigned int eip;
    unsigned int cs;
    unsigned int dataptr;
    unsigned int datasel;
    NPXREG reg[8];
  }
NPX;

static NPX npx;

static void save_npx (void);	/* Save the FPU of the debugged program */
static void load_npx (void);	/* Restore the FPU of the debugged program */

/* ------------------------------------------------------------------------- */
/* Store the contents of the NPX in the global variable `npx'.  */
/* *INDENT-OFF* */

static void
save_npx (void)
{
  asm ("inb    $0xa0, %%al
       testb $0x20, %%al
       jz 1f
       xorb %% al, %%al
       outb %% al, $0xf0
       movb $0x20, %%al
       outb %% al, $0xa0
       outb %% al, $0x20
1:
       fnsave % 0
       fwait "
:     "=m" (npx)
:				/* No input */
:     "%eax");
}

/* *INDENT-ON* */





/* ------------------------------------------------------------------------- */
/* Reload the contents of the NPX from the global variable `npx'.  */

static void
load_npx (void)
{
asm ("frstor %0":"=m" (npx));
}
/* ------------------------------------------------------------------------- */
/* Stubs for the missing redirection functions.  */
typedef struct {
  char *command;
  int redirected;
} cmdline_t;

void redir_cmdline_delete (cmdline_t *ptr) {ptr->redirected = 0;}
int  redir_cmdline_parse (const char *args, cmdline_t *ptr)
{
  return -1;
}
int redir_to_child (cmdline_t *ptr)
{
  return 1;
}
int redir_to_debugger (cmdline_t *ptr)
{
  return 1;
}
int redir_debug_init (cmdline_t *ptr) { return 0; }
#endif /* __DJGPP_MINOR < 3 */

extern void _initialize_go32_nat (void);

typedef enum { wp_insert, wp_remove, wp_count } wp_op;

/* This holds the current reference counts for each debug register.  */
static int dr_ref_count[4];

extern char **environ;

#define SOME_PID 42

static int prog_has_started = 0;
static void go32_open (char *name, int from_tty);
static void go32_close (int quitting);
static void go32_attach (char *args, int from_tty);
static void go32_detach (char *args, int from_tty);
static void go32_resume (int pid, int step, enum target_signal siggnal);
static int go32_wait (int pid, struct target_waitstatus *status);
static void go32_fetch_registers (int regno);
static void store_register (int regno);
static void go32_store_registers (int regno);
static void go32_prepare_to_store (void);
static int go32_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len,
			     int write, struct target_ops *target);
static void go32_files_info (struct target_ops *target);
static void go32_stop (void);
static void go32_kill_inferior (void);
static void go32_create_inferior (char *exec_file, char *args, char **env);
static void cleanup_dregs (void);
static void go32_mourn_inferior (void);
static int go32_can_run (void);
static int go32_insert_aligned_watchpoint (CORE_ADDR waddr, CORE_ADDR addr,
					   int len, int rw);
static int go32_remove_aligned_watchpoint (CORE_ADDR waddr, CORE_ADDR addr,
					   int len, int rw);
static int go32_handle_nonaligned_watchpoint (wp_op what, CORE_ADDR waddr,
					      CORE_ADDR addr, int len, int rw);

static struct target_ops go32_ops;
static void go32_terminal_init (void);
static void go32_terminal_inferior (void);
static void go32_terminal_ours (void);

#define r_ofs(x) (offsetof(TSS,x))

static struct
{
  size_t tss_ofs;
  size_t size;
}
regno_mapping[] =
{
  {r_ofs (tss_eax), 4},	/* normal registers, from a_tss */
  {r_ofs (tss_ecx), 4},
  {r_ofs (tss_edx), 4},
  {r_ofs (tss_ebx), 4},
  {r_ofs (tss_esp), 4},
  {r_ofs (tss_ebp), 4},
  {r_ofs (tss_esi), 4},
  {r_ofs (tss_edi), 4},
  {r_ofs (tss_eip), 4},
  {r_ofs (tss_eflags), 4},
  {r_ofs (tss_cs), 2},
  {r_ofs (tss_ss), 2},
  {r_ofs (tss_ds), 2},
  {r_ofs (tss_es), 2},
  {r_ofs (tss_fs), 2},
  {r_ofs (tss_gs), 2},
  {0, 10},		/* 8 FP registers, from npx.reg[] */
  {1, 10},
  {2, 10},
  {3, 10},
  {4, 10},
  {5, 10},
  {6, 10},
  {7, 10},
	/* The order of the next 7 registers must be consistent
	   with their numbering in config/i386/tm-i386.h, which see.  */
  {0, 2},		/* control word, from npx */
  {4, 2},		/* status word, from npx */
  {8, 2},		/* tag word, from npx */
  {16, 2},		/* last FP exception CS from npx */
  {12, 4},		/* last FP exception EIP from npx */
  {24, 2},		/* last FP exception operand selector from npx */
  {20, 4},		/* last FP exception operand offset from npx */
  {18, 2}		/* last FP opcode from npx */
};

static struct
  {
    int go32_sig;
    enum target_signal gdb_sig;
  }
sig_map[] =
{
  {0, TARGET_SIGNAL_FPE},
  {1, TARGET_SIGNAL_TRAP},
  /* Exception 2 is triggered by the NMI.  DJGPP handles it as SIGILL,
     but I think SIGBUS is better, since the NMI is usually activated
     as a result of a memory parity check failure.  */
  {2, TARGET_SIGNAL_BUS},
  {3, TARGET_SIGNAL_TRAP},
  {4, TARGET_SIGNAL_FPE},
  {5, TARGET_SIGNAL_SEGV},
  {6, TARGET_SIGNAL_ILL},
  {7, TARGET_SIGNAL_EMT},	/* no-coprocessor exception */
  {8, TARGET_SIGNAL_SEGV},
  {9, TARGET_SIGNAL_SEGV},
  {10, TARGET_SIGNAL_BUS},
  {11, TARGET_SIGNAL_SEGV},
  {12, TARGET_SIGNAL_SEGV},
  {13, TARGET_SIGNAL_SEGV},
  {14, TARGET_SIGNAL_SEGV},
  {16, TARGET_SIGNAL_FPE},
  {17, TARGET_SIGNAL_BUS},
  {31, TARGET_SIGNAL_ILL},
  {0x1b, TARGET_SIGNAL_INT},
  {0x75, TARGET_SIGNAL_FPE},
  {0x78, TARGET_SIGNAL_ALRM},
  {0x79, TARGET_SIGNAL_INT},
  {0x7a, TARGET_SIGNAL_QUIT},
  {-1, TARGET_SIGNAL_LAST}
};

static struct {
  enum target_signal gdb_sig;
  int djgpp_excepno;
} excepn_map[] = {
  {TARGET_SIGNAL_0, -1},
  {TARGET_SIGNAL_ILL, 6},	/* Invalid Opcode */
  {TARGET_SIGNAL_EMT, 7},	/* triggers SIGNOFP */
  {TARGET_SIGNAL_SEGV, 13},	/* GPF */
  {TARGET_SIGNAL_BUS, 17},	/* Alignment Check */
  /* The rest are fake exceptions, see dpmiexcp.c in djlsr*.zip for
     details.  */
  {TARGET_SIGNAL_TERM, 0x1b},	/* triggers Ctrl-Break type of SIGINT */
  {TARGET_SIGNAL_FPE, 0x75},
  {TARGET_SIGNAL_INT, 0x79},
  {TARGET_SIGNAL_QUIT, 0x7a},
  {TARGET_SIGNAL_ALRM, 0x78},	/* triggers SIGTIMR */
  {TARGET_SIGNAL_PROF, 0x78},
  {TARGET_SIGNAL_LAST, -1}
};

static void
go32_open (char *name ATTRIBUTE_UNUSED, int from_tty ATTRIBUTE_UNUSED)
{
  printf_unfiltered ("Done.  Use the \"run\" command to run the program.\n");
}

static void
go32_close (int quitting ATTRIBUTE_UNUSED)
{
}

static void
go32_attach (char *args ATTRIBUTE_UNUSED, int from_tty ATTRIBUTE_UNUSED)
{
  error ("\
You cannot attach to a running program on this platform.\n\
Use the `run' command to run DJGPP programs.");
}

static void
go32_detach (char *args ATTRIBUTE_UNUSED, int from_tty ATTRIBUTE_UNUSED)
{
}

static int resume_is_step;
static int resume_signal = -1;

static void
go32_resume (int pid ATTRIBUTE_UNUSED, int step, enum target_signal siggnal)
{
  int i;

  resume_is_step = step;

  if (siggnal != TARGET_SIGNAL_0 && siggnal != TARGET_SIGNAL_TRAP)
  {
    for (i = 0, resume_signal = -1;
	 excepn_map[i].gdb_sig != TARGET_SIGNAL_LAST; i++)
      if (excepn_map[i].gdb_sig == siggnal)
      {
	resume_signal = excepn_map[i].djgpp_excepno;
	break;
      }
    if (resume_signal == -1)
      printf_unfiltered ("Cannot deliver signal %s on this platform.\n",
			 target_signal_to_name (siggnal));
  }
}

static char child_cwd[FILENAME_MAX];

static int
go32_wait (int pid ATTRIBUTE_UNUSED, struct target_waitstatus *status)
{
  int i;
  unsigned char saved_opcode;
  unsigned long INT3_addr = 0;
  int stepping_over_INT = 0;

  a_tss.tss_eflags &= 0xfeff;	/* reset the single-step flag (TF) */
  if (resume_is_step)
    {
      /* If the next instruction is INT xx or INTO, we need to handle
	 them specially.  Intel manuals say that these instructions
	 reset the single-step flag (a.k.a. TF).  However, it seems
	 that, at least in the DPMI environment, and at least when
	 stepping over the DPMI interrupt 31h, the problem is having
	 TF set at all when INT 31h is executed: the debuggee either
	 crashes (and takes the system with it) or is killed by a
	 SIGTRAP.

	 So we need to emulate single-step mode: we put an INT3 opcode
	 right after the INT xx instruction, let the debuggee run
	 until it hits INT3 and stops, then restore the original
	 instruction which we overwrote with the INT3 opcode, and back
	 up the debuggee's EIP to that instruction.  */
      read_child (a_tss.tss_eip, &saved_opcode, 1);
      if (saved_opcode == 0xCD || saved_opcode == 0xCE)
	{
	  unsigned char INT3_opcode = 0xCC;

	  INT3_addr
	    = saved_opcode == 0xCD ? a_tss.tss_eip + 2 : a_tss.tss_eip + 1;
	  stepping_over_INT = 1;
	  read_child (INT3_addr, &saved_opcode, 1);
	  write_child (INT3_addr, &INT3_opcode, 1);
	}
      else
	a_tss.tss_eflags |= 0x0100; /* normal instruction: set TF */
    }

  /* The special value FFFFh in tss_trap indicates to run_child that
     tss_irqn holds a signal to be delivered to the debuggee.  */
  if (resume_signal <= -1)
    {
      a_tss.tss_trap = 0;
      a_tss.tss_irqn = 0xff;
    }
  else
    {
      a_tss.tss_trap = 0xffff;	/* run_child looks for this */
      a_tss.tss_irqn = resume_signal;
    }

  /* The child might change working directory behind our back.  The
     GDB users won't like the side effects of that when they work with
     relative file names, and GDB might be confused by its current
     directory not being in sync with the truth.  So we always make a
     point of changing back to where GDB thinks is its cwd, when we
     return control to the debugger, but restore child's cwd before we
     run it.  */
  chdir (child_cwd);

#if __DJGPP_MINOR__ < 3
  load_npx ();
#endif
  run_child ();
#if __DJGPP_MINOR__ < 3
  save_npx ();
#endif

  /* Did we step over an INT xx instruction?  */
  if (stepping_over_INT && a_tss.tss_eip == INT3_addr + 1)
    {
      /* Restore the original opcode.  */
      a_tss.tss_eip--;	/* EIP points *after* the INT3 instruction */
      write_child (a_tss.tss_eip, &saved_opcode, 1);
      /* Simulate a TRAP exception.  */
      a_tss.tss_irqn = 1;
      a_tss.tss_eflags |= 0x0100;
    }

  getcwd (child_cwd, sizeof (child_cwd)); /* in case it has changed */
  chdir (current_directory);

  if (a_tss.tss_irqn == 0x21)
    {
      status->kind = TARGET_WAITKIND_EXITED;
      status->value.integer = a_tss.tss_eax & 0xff;
    }
  else
    {
      status->value.sig = TARGET_SIGNAL_UNKNOWN;
      status->kind = TARGET_WAITKIND_STOPPED;
      for (i = 0; sig_map[i].go32_sig != -1; i++)
	{
	  if (a_tss.tss_irqn == sig_map[i].go32_sig)
	    {
#if __DJGPP_MINOR__ < 3
	      if ((status->value.sig = sig_map[i].gdb_sig) !=
		  TARGET_SIGNAL_TRAP)
		status->kind = TARGET_WAITKIND_SIGNALLED;
#else
	      status->value.sig = sig_map[i].gdb_sig;
#endif
	      break;
	    }
	}
    }
  return SOME_PID;
}

static void
go32_fetch_registers (int regno)
{
  /*JHW */
  int end_reg = regno + 1;	/* just one reg initially */

  if (regno < 0)		/* do the all registers */
    {
      regno = 0;		/* start at first register */
      /* # regs in table */
      end_reg = sizeof (regno_mapping) / sizeof (regno_mapping[0]);
    }

  for (; regno < end_reg; regno++)
    {
      if (regno < 16)
	supply_register (regno,
			 (char *) &a_tss + regno_mapping[regno].tss_ofs);
      else if (regno < 24)
	supply_register (regno,
			 (char *) &npx.reg[regno_mapping[regno].tss_ofs]);
      else if (regno < 32)
	{
	  unsigned regval;

	  switch (regno_mapping[regno].size)
	    {
	      case 2:
		regval = *(unsigned short *)
		  ((char *) &npx + regno_mapping[regno].tss_ofs);
		regval &= 0xffff;
		if (regno == FOP_REGNUM && regval)
		  /* Feature: restore the 5 bits of the opcode
		     stripped by FSAVE/FNSAVE.  */
		  regval |= 0xd800;
		break;
	      case 4:
		regval = *(unsigned *)
		  ((char *) &npx + regno_mapping[regno].tss_ofs);
		break;
	      default:
		internal_error ("\
Invalid native size for register no. %d in go32_fetch_register.", regno);
	    }
	  supply_register (regno, (char *) &regval);
	}
      else
	internal_error ("Invalid register no. %d in go32_fetch_register.",
			regno);
    }
}

static void
store_register (int regno)
{
  void *rp;
  void *v = (void *) &registers[REGISTER_BYTE (regno)];

  if (regno < 16)
    rp = (char *) &a_tss + regno_mapping[regno].tss_ofs;
  else if (regno < 24)
    rp = (char *) &npx.reg[regno_mapping[regno].tss_ofs];
  else if (regno < 32)
    rp = (char *) &npx + regno_mapping[regno].tss_ofs;
  else
    internal_error ("Invalid register no. %d in store_register.", regno);
  memcpy (rp, v, regno_mapping[regno].size);
  if (regno == FOP_REGNUM)
    *(short *)rp &= 0x07ff; /* strip high 5 bits, in case they added them */
}

static void
go32_store_registers (int regno)
{
  unsigned r;

  if (regno >= 0)
    store_register (regno);
  else
    {
      for (r = 0; r < sizeof (regno_mapping) / sizeof (regno_mapping[0]); r++)
	store_register (r);
    }
}

static void
go32_prepare_to_store (void)
{
}

static int
go32_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len, int write,
		  struct target_ops *target ATTRIBUTE_UNUSED)
{
  if (write)
    {
      if (write_child (memaddr, myaddr, len))
	{
	  return 0;
	}
      else
	{
	  return len;
	}
    }
  else
    {
      if (read_child (memaddr, myaddr, len))
	{
	  return 0;
	}
      else
	{
	  return len;
	}
    }
}

static cmdline_t child_cmd;	/* parsed child's command line kept here */

static void
go32_files_info (struct target_ops *target ATTRIBUTE_UNUSED)
{
  printf_unfiltered ("You are running a DJGPP V2 program.\n");
}

static void
go32_stop (void)
{
  normal_stop ();
  cleanup_client ();
  inferior_pid = 0;
  prog_has_started = 0;
}

static void
go32_kill_inferior (void)
{
  redir_cmdline_delete (&child_cmd);
  resume_signal = -1;
  resume_is_step = 0;
  unpush_target (&go32_ops);
}

static void
go32_create_inferior (char *exec_file, char *args, char **env)
{
  jmp_buf start_state;
  char *cmdline;
  char **env_save = environ;

  /* If no exec file handed to us, get it from the exec-file command -- with
     a good, common error message if none is specified.  */
  if (exec_file == 0)
    exec_file = get_exec_file (1);

  if (prog_has_started)
    {
      go32_stop ();
      go32_kill_inferior ();
    }
  resume_signal = -1;
  resume_is_step = 0;
  /* Init command line storage.  */
  if (redir_debug_init (&child_cmd) == -1)
    internal_error ("Cannot allocate redirection storage: not enough memory.\n");

  /* Parse the command line and create redirections.  */
  if (strpbrk (args, "<>"))
    {
      if (redir_cmdline_parse (args, &child_cmd) == 0)
	args = child_cmd.command;
      else
	error ("Syntax error in command line.");
    }
  else
    child_cmd.command = xstrdup (args);

  cmdline = (char *) alloca (strlen (args) + 4);
  cmdline[0] = strlen (args);
  strcpy (cmdline + 1, args);
  cmdline[strlen (args) + 1] = 13;

  environ = env;

  if (v2loadimage (exec_file, cmdline, start_state))
    {
      environ = env_save;
      printf_unfiltered ("Load failed for image %s\n", exec_file);
      exit (1);
    }
  environ = env_save;

  edi_init (start_state);
#if __DJGPP_MINOR__ < 3
  save_npx ();
#endif

  inferior_pid = SOME_PID;
  push_target (&go32_ops);
  clear_proceed_status ();
  insert_breakpoints ();
  proceed ((CORE_ADDR) -1, TARGET_SIGNAL_0, 0);
  prog_has_started = 1;
}

static void
go32_mourn_inferior (void)
{
  /* We need to make sure all the breakpoint enable bits in the DR7
     register are reset when the inferior exits.  Otherwise, if they
     rerun the inferior, the uncleared bits may cause random SIGTRAPs,
     failure to set more watchpoints, and other calamities.  It would
     be nice if GDB itself would take care to remove all breakpoints
     at all times, but it doesn't, probably under an assumption that
     the OS cleans up when the debuggee exits.  */
  cleanup_dregs ();
  go32_kill_inferior ();
  generic_mourn_inferior ();
}

static int
go32_can_run (void)
{
  return 1;
}

/* Hardware watchpoint support.  */

#define DR_STATUS 6
#define DR_CONTROL 7
#define DR_ENABLE_SIZE 2
#define DR_LOCAL_ENABLE_SHIFT 0
#define DR_GLOBAL_ENABLE_SHIFT 1
#define DR_LOCAL_SLOWDOWN 0x100
#define DR_GLOBAL_SLOWDOWN 0x200
#define DR_CONTROL_SHIFT 16
#define DR_CONTROL_SIZE 4
#define DR_RW_READWRITE 0x3
#define DR_RW_WRITE 0x1
#define DR_CONTROL_MASK 0xf
#define DR_ENABLE_MASK 0x3
#define DR_LEN_1 0x0
#define DR_LEN_2 0x4
#define DR_LEN_4 0xc

#define D_REGS edi.dr
#define CONTROL D_REGS[DR_CONTROL]
#define STATUS D_REGS[DR_STATUS]

#define IS_REG_FREE(index) \
  (!(CONTROL & (3 << (DR_ENABLE_SIZE * (index)))))

#define LOCAL_ENABLE_REG(index) \
  (CONTROL |= (1 << (DR_LOCAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (index))))

#define GLOBAL_ENABLE_REG(index) \
  (CONTROL |= (1 << (DR_GLOBAL_ENABLE_SHIFT + DR_ENABLE_SIZE * (index))))

#define DISABLE_REG(index) \
  (CONTROL &= ~(3 << (DR_ENABLE_SIZE * (index))))

#define SET_LOCAL_EXACT() \
  (CONTROL |= DR_LOCAL_SLOWDOWN)

#define SET_GLOBAL_EXACT() \
  (CONTROL |= DR_GLOBAL_SLOWDOWN)

#define RESET_LOCAL_EXACT() \
   (CONTROL &= ~(DR_LOCAL_SLOWDOWN))

#define RESET_GLOBAL_EXACT() \
   (CONTROL &= ~(DR_GLOBAL_SLOWDOWN))

#define SET_BREAK(index,address) \
  do {\
    CONTROL &= ~(DR_CONTROL_MASK << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (index)));\
    D_REGS[index] = address;\
    dr_ref_count[index]++;\
  } while(0)

#define SET_WATCH(index,address,rw,len) \
  do {\
    SET_BREAK(index,address);\
    CONTROL |= ((len)|(rw)) << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (index));\
  } while (0)

#define IS_WATCH(index) \
  (CONTROL & (DR_CONTROL_MASK << (DR_CONTROL_SHIFT + DR_CONTROL_SIZE*(index))))

#define WATCH_HIT(index) ((STATUS & (1 << (index))) && IS_WATCH(index))

#define DR_DEF(index) \
  ((CONTROL >> (DR_CONTROL_SHIFT + DR_CONTROL_SIZE * (index))) & 0x0f)
    

#if 0 /* use debugging macro */
#define SHOW_DR(text,len) \
do { \
  if (!getenv ("GDB_SHOW_DR")) break; \
  fprintf(stderr,"%08x %08x ",edi.dr[7],edi.dr[6]); \
  fprintf(stderr,"%08x %d %08x %d ", \
	  edi.dr[0],dr_ref_count[0],edi.dr[1],dr_ref_count[1]); \
  fprintf(stderr,"%08x %d %08x %d ", \
	  edi.dr[2],dr_ref_count[2],edi.dr[3],dr_ref_count[3]); \
  fprintf(stderr,(len)?"(%s:%d)\n":"(%s)\n",#text,len); \
} while (0)
#else
#define SHOW_DR(text,len) do {} while (0)
#endif

static void
cleanup_dregs (void)
{
  int i;

  CONTROL = 0;
  STATUS = 0;
  for (i = 0; i < 4; i++)
    {
      D_REGS[i] = 0;
      dr_ref_count[i] = 0;
    }
}

/* Insert a watchpoint.  */

int
go32_insert_watchpoint (int pid ATTRIBUTE_UNUSED, CORE_ADDR addr,
			int len, int rw)
{
  int ret = go32_insert_aligned_watchpoint (addr, addr, len, rw);

  SHOW_DR (insert_watch, len);
  return ret;
}

static int
go32_insert_aligned_watchpoint (CORE_ADDR waddr, CORE_ADDR addr,
				int len, int rw)
{
  int i;
  int read_write_bits, len_bits;

  /* Values of rw: 0 - write, 1 - read, 2 - access (read and write).
     However, x86 doesn't support read-only data breakpoints.  */
  read_write_bits = rw ? DR_RW_READWRITE : DR_RW_WRITE;

  switch (len)
  {
  case 4:
    len_bits = DR_LEN_4;
    break;
  case 2:
    len_bits = DR_LEN_2;
    break;
  case 1:
    len_bits = DR_LEN_1;
    break;
  default:
    /* The debug registers only have 2 bits for the length, so
       so this value will always fail the loop below.  */
    len_bits = 0x10;
  }

  /* Look for an occupied debug register with the same address and the
     same RW and LEN definitions.  If we find one, we can use it for
     this watchpoint as well (and save a register).  */
  for (i = 0; i < 4; i++)
  {
    if (!IS_REG_FREE (i) && D_REGS[i] == addr
	&& DR_DEF (i) == (unsigned)(len_bits | read_write_bits))
    {
      dr_ref_count[i]++;
      return 0;
    }
  }

  /* Look for a free debug register.  */
  for (i = 0; i <= 3; i++)
  {
    if (IS_REG_FREE (i))
      break;
  }

  /* No more debug registers!  */
  if (i > 3)
    return -1;

  if (len == 2)
  {
    if (addr % 2)
      return go32_handle_nonaligned_watchpoint (wp_insert, waddr, addr,
						len, rw);
  }
  else if (len == 4)
  {
    if (addr % 4)
      return go32_handle_nonaligned_watchpoint (wp_insert, waddr, addr,
						len, rw);
  }
  else if (len != 1)
    return go32_handle_nonaligned_watchpoint (wp_insert, waddr, addr, len, rw);

  SET_WATCH (i, addr, read_write_bits, len_bits);
  LOCAL_ENABLE_REG (i);
  SET_LOCAL_EXACT ();
  SET_GLOBAL_EXACT ();
  return 0;
}

static int
go32_handle_nonaligned_watchpoint (wp_op what, CORE_ADDR waddr, CORE_ADDR addr,
				   int len, int rw)
{
  int align;
  int size;
  int rv = 0, status = 0;

  static int size_try_array[4][4] =
  {
    { 1, 1, 1, 1 },		/* trying size one */
    { 2, 1, 2, 1 },		/* trying size two */
    { 2, 1, 2, 1 },		/* trying size three */
    { 4, 1, 2, 1 }		/* trying size four */
  };

  while (len > 0)
    {
      align = addr % 4;
      /* Four is the maximum length a 386 debug register can watch.  */
      size = size_try_array[len > 4 ? 3 : len - 1][align];
      if (what == wp_insert)
	status = go32_insert_aligned_watchpoint (waddr, addr, size, rw);
      else if (what == wp_remove)
	status = go32_remove_aligned_watchpoint (waddr, addr, size, rw);
      else if (what == wp_count)
	rv++;
      else
	status = EINVAL;
      /* We keep the loop going even after a failure, because some of
	 the other aligned watchpoints might still succeed, e.g. if
	 they watch addresses that are already watched, and thus just
	 increment the reference counts of occupied debug registers.
	 If we break out of the loop too early, we could cause those
	 addresses watched by other watchpoints to be disabled when
	 GDB reacts to our failure to insert this watchpoint and tries
	 to remove it.  */
      if (status)
	rv = status;
      addr += size;
      len -= size;
    }
  return rv;
}

/* Remove a watchpoint.  */

int
go32_remove_watchpoint (int pid ATTRIBUTE_UNUSED, CORE_ADDR addr,
			int len, int rw)
{
  int ret = go32_remove_aligned_watchpoint (addr, addr, len, rw);

  SHOW_DR (remove_watch, len);
  return ret;
}

static int
go32_remove_aligned_watchpoint (CORE_ADDR waddr, CORE_ADDR addr,
				int len, int rw)
{
  int i;
  int read_write_bits, len_bits;

  /* Values of rw: 0 - write, 1 - read, 2 - access (read and write).
     However, x86 doesn't support read-only data breakpoints.  */
  read_write_bits = rw ? DR_RW_READWRITE : DR_RW_WRITE;

  switch (len)
    {
      case 4:
	len_bits = DR_LEN_4;
	break;
      case 2:
	len_bits = DR_LEN_2;
	break;
      case 1:
	len_bits = DR_LEN_1;
	break;
      default:
	/* The debug registers only have 2 bits for the length, so
	   so this value will always fail the loop below.  */
	len_bits = 0x10;
    }

  if (len == 2)
    {
      if (addr % 2)
	return go32_handle_nonaligned_watchpoint (wp_remove, waddr, addr,
						  len, rw);
    }
  else if (len == 4)
    {
      if (addr % 4)
	return go32_handle_nonaligned_watchpoint (wp_remove, waddr, addr,
						  len, rw);
    }
  else if (len != 1)
    return go32_handle_nonaligned_watchpoint (wp_remove, waddr, addr, len, rw);

  for (i = 0; i <= 3; i++)
    {
      if (!IS_REG_FREE (i) && D_REGS[i] == addr
	  && DR_DEF (i) == (unsigned)(len_bits | read_write_bits))
	{
	  dr_ref_count[i]--;
	  if (dr_ref_count[i] == 0)
	    DISABLE_REG (i);
	}
    }
  RESET_LOCAL_EXACT ();
  RESET_GLOBAL_EXACT ();

  return 0;
}

/* Can we use debug registers to watch a region whose address is ADDR
   and whose length is LEN bytes?  */

int
go32_region_ok_for_watchpoint (CORE_ADDR addr, int len)
{
  /* Compute how many aligned watchpoints we would need to cover this
     region.  */
  int nregs = go32_handle_nonaligned_watchpoint (wp_count, addr, addr, len, 0);

  return nregs <= 4 ? 1 : 0;
}

/* Check if stopped by a data watchpoint.  If so, return the address
   whose access triggered the watchpoint.  */

CORE_ADDR
go32_stopped_by_watchpoint (int pid ATTRIBUTE_UNUSED, int data_watchpoint)
{
  int i, ret = 0;
  int status;

  status = edi.dr[DR_STATUS];
  SHOW_DR (stopped_by, 0);
  for (i = 0; i <= 3; i++)
    {
      if (WATCH_HIT (i) && data_watchpoint)
	{
	  SHOW_DR (WP_HIT, 0);
	  ret = D_REGS[i];
	}
    }

  return ret;
}

/* Remove a breakpoint.  */

int
go32_remove_hw_breakpoint (CORE_ADDR addr, void *shadow ATTRIBUTE_UNUSED)
{
  int i;
  for (i = 0; i <= 3; i++)
    {
      if (!IS_REG_FREE (i) && D_REGS[i] == addr && DR_DEF (i) == 0)
	{
	  dr_ref_count[i]--;
	  if (dr_ref_count[i] == 0)
	    DISABLE_REG (i);
	}
    }
  SHOW_DR (remove_hw, 0);
  return 0;
}

int
go32_insert_hw_breakpoint (CORE_ADDR addr, void *shadow ATTRIBUTE_UNUSED)
{
  int i;

  /* Look for an occupied debug register with the same address and the
     same RW and LEN definitions.  If we find one, we can use it for
     this breakpoint as well (and save a register).  */
  for (i = 0; i < 4; i++)
    {
      if (!IS_REG_FREE (i) && D_REGS[i] == addr && DR_DEF (i) == 0)
	{
	  dr_ref_count[i]++;
	  SHOW_DR (insert_hw, 0);
	  return 0;
	}
    }

  /* Look for a free debug register.  */
  for (i = 0; i <= 3; i++)
    {
      if (IS_REG_FREE (i))
	break;
    }

  /* No more debug registers?  */
  if (i < 4)
    {
      SET_BREAK (i, addr);
      LOCAL_ENABLE_REG (i);
    }
  SHOW_DR (insert_hw, 0);

  return i < 4 ? 0 : EBUSY;
}

/* Put the device open on handle FD into either raw or cooked
   mode, return 1 if it was in raw mode, zero otherwise.  */

static int
device_mode (int fd, int raw_p)
{
  int oldmode, newmode;
  __dpmi_regs regs;

  regs.x.ax = 0x4400;
  regs.x.bx = fd;
  __dpmi_int (0x21, &regs);
  if (regs.x.flags & 1)
    return -1;
  newmode = oldmode = regs.x.dx;

  if (raw_p)
    newmode |= 0x20;
  else
    newmode &= ~0x20;

  if (oldmode & 0x80)	/* Only for character dev */
  {
    regs.x.ax = 0x4401;
    regs.x.bx = fd;
    regs.x.dx = newmode & 0xff;   /* Force upper byte zero, else it fails */
    __dpmi_int (0x21, &regs);
    if (regs.x.flags & 1)
      return -1;
  }
  return (oldmode & 0x20) == 0x20;
}


static int inf_mode_valid = 0;
static int inf_terminal_mode;

/* This semaphore is needed because, amazingly enough, GDB calls
   target.to_terminal_ours more than once after the inferior stops.
   But we need the information from the first call only, since the
   second call will always see GDB's own cooked terminal.  */
static int terminal_is_ours = 1;

static void
go32_terminal_init (void)
{
  inf_mode_valid = 0;	/* reinitialize, in case they are restarting child */
  terminal_is_ours = 1;
}

static void
go32_terminal_info (char *args ATTRIBUTE_UNUSED, int from_tty ATTRIBUTE_UNUSED)
{
  printf_unfiltered ("Inferior's terminal is in %s mode.\n",
		     !inf_mode_valid
		     ? "default" : inf_terminal_mode ? "raw" : "cooked");

#if __DJGPP_MINOR__ > 2
  if (child_cmd.redirection)
  {
    int i;

    for (i = 0; i < DBG_HANDLES; i++)
    {
      if (child_cmd.redirection[i]->file_name)
	printf_unfiltered ("\tFile handle %d is redirected to `%s'.\n",
			   i, child_cmd.redirection[i]->file_name);
      else if (_get_dev_info (child_cmd.redirection[i]->inf_handle) == -1)
	printf_unfiltered
	  ("\tFile handle %d appears to be closed by inferior.\n", i);
      /* Mask off the raw/cooked bit when comparing device info words.  */
      else if ((_get_dev_info (child_cmd.redirection[i]->inf_handle) & 0xdf)
	       != (_get_dev_info (i) & 0xdf))
	printf_unfiltered
	  ("\tFile handle %d appears to be redirected by inferior.\n", i);
    }
  }
#endif
}

static void
go32_terminal_inferior (void)
{
  /* Redirect standard handles as child wants them.  */
  errno = 0;
  if (redir_to_child (&child_cmd) == -1)
  {
    redir_to_debugger (&child_cmd);
    error ("Cannot redirect standard handles for program: %s.",
	   strerror (errno));
  }
  /* set the console device of the inferior to whatever mode
     (raw or cooked) we found it last time */
  if (terminal_is_ours)
  {
    if (inf_mode_valid)
      device_mode (0, inf_terminal_mode);
    terminal_is_ours = 0;
  }
}

static void
go32_terminal_ours (void)
{
  /* Switch to cooked mode on the gdb terminal and save the inferior
     terminal mode to be restored when it is resumed */
  if (!terminal_is_ours)
  {
    inf_terminal_mode = device_mode (0, 0);
    if (inf_terminal_mode != -1)
      inf_mode_valid = 1;
    else
      /* If device_mode returned -1, we don't know what happens with
	 handle 0 anymore, so make the info invalid.  */
      inf_mode_valid = 0;
    terminal_is_ours = 1;

    /* Restore debugger's standard handles.  */
    errno = 0;
    if (redir_to_debugger (&child_cmd) == -1)
    {
      redir_to_child (&child_cmd);
      error ("Cannot redirect standard handles for debugger: %s.",
	     strerror (errno));
    }
  }
}

static void
init_go32_ops (void)
{
  go32_ops.to_shortname = "djgpp";
  go32_ops.to_longname = "djgpp target process";
  go32_ops.to_doc =
    "Program loaded by djgpp, when gdb is used as an external debugger";
  go32_ops.to_open = go32_open;
  go32_ops.to_close = go32_close;
  go32_ops.to_attach = go32_attach;
  go32_ops.to_detach = go32_detach;
  go32_ops.to_resume = go32_resume;
  go32_ops.to_wait = go32_wait;
  go32_ops.to_fetch_registers = go32_fetch_registers;
  go32_ops.to_store_registers = go32_store_registers;
  go32_ops.to_prepare_to_store = go32_prepare_to_store;
  go32_ops.to_xfer_memory = go32_xfer_memory;
  go32_ops.to_files_info = go32_files_info;
  go32_ops.to_insert_breakpoint = memory_insert_breakpoint;
  go32_ops.to_remove_breakpoint = memory_remove_breakpoint;
  go32_ops.to_terminal_init = go32_terminal_init;
  go32_ops.to_terminal_inferior = go32_terminal_inferior;
  go32_ops.to_terminal_ours_for_output = go32_terminal_ours;
  go32_ops.to_terminal_ours = go32_terminal_ours;
  go32_ops.to_terminal_info = go32_terminal_info;
  go32_ops.to_kill = go32_kill_inferior;
  go32_ops.to_create_inferior = go32_create_inferior;
  go32_ops.to_mourn_inferior = go32_mourn_inferior;
  go32_ops.to_can_run = go32_can_run;
  go32_ops.to_stop = go32_stop;
  go32_ops.to_stratum = process_stratum;
  go32_ops.to_has_all_memory = 1;
  go32_ops.to_has_memory = 1;
  go32_ops.to_has_stack = 1;
  go32_ops.to_has_registers = 1;
  go32_ops.to_has_execution = 1;
  go32_ops.to_magic = OPS_MAGIC;

  /* Initialize child's cwd with the current one.  */
  getcwd (child_cwd, sizeof (child_cwd));

  /* Initialize child's command line storage.  */
  if (redir_debug_init (&child_cmd) == -1)
    internal_error ("Cannot allocate redirection storage: not enough memory.\n");

  /* We are always processing GCC-compiled programs.  */
  processing_gcc_compilation = 2;
}

void
_initialize_go32_nat (void)
{
  init_go32_ops ();
  add_target (&go32_ops);
}

pid_t
tcgetpgrp (int fd)
{
  if (isatty (fd))
    return SOME_PID;
  errno = ENOTTY;
  return -1;
}

int
tcsetpgrp (int fd, pid_t pgid)
{
  if (isatty (fd) && pgid == SOME_PID)
    return 0;
  errno = pgid == SOME_PID ? ENOTTY : ENOSYS;
  return -1;
}
