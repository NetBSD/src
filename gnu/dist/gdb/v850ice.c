/* ICE interface for the NEC V850 for GDB, the GNU debugger.
   Copyright 1996, Free Software Foundation, Inc.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "gdb_string.h"
#if 0
#include "frame.h"
#endif
#include "inferior.h"
#if 0
#include "bfd.h"
#endif
#include "symfile.h"
#include "target.h"
#if 0
#include "wait.h"
#include "gdbcmd.h"
#include "objfiles.h"
#include "gdb-stabs.h"
#include "gdbthread.h"
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* Prototypes for local functions */

static void v850ice_files_info PARAMS ((struct target_ops *ignore));

static int v850ice_xfer_memory PARAMS ((CORE_ADDR memaddr, char *myaddr,
					int len, int should_write,
					struct target_ops *target));

static void v850ice_prepare_to_store PARAMS ((void));

static void v850ice_fetch_registers PARAMS ((int regno));

static void v850ice_resume PARAMS ((int pid, int step,
				   enum target_signal siggnal));

static void v850ice_open PARAMS ((char *name, int from_tty));

static void v850ice_close PARAMS ((int quitting));

static void v850ice_store_registers PARAMS ((int regno));

static void v850ice_mourn PARAMS ((void));

static int v850ice_wait PARAMS ((int pid, struct target_waitstatus *status));

static void v850ice_kill PARAMS ((void));

static void v850ice_detach PARAMS ((char *args, int from_tty));

static int v850ice_insert_breakpoint PARAMS ((CORE_ADDR, char *));

static int v850ice_remove_breakpoint PARAMS ((CORE_ADDR, char *));

static int ice_open = 0;

#ifndef EXPORT
#define EXPORT __declspec(dllexport)
#endif

struct MessageIO
{
  int size;			/* length of input or output in bytes */
  char *buf;			/* buffer having the input/output information */
};

struct MessageIO null_iob = { 0, NULL };

EXPORT long __stdcall ExeAppReq (char *, long, char *, struct MessageIO *);
EXPORT long __stdcall RegisterClient (HWND);
EXPORT long __stdcall UnregisterClient (void);
EXPORT long __stdcall GdbCallBack (void);

#define	MREADREG          0x0001
#define	MWRITEREG         0x0002
#define	MREADMEM          0x0003
#define	MWRITEMEM         0x0004
#define	MSINGLESTEP       0x0005
#define	MRESUME           0x0006
#define	MLOADPROGRAM      0x0007
#define	MSETBREAK         0x0008
#define	MREMOVEBREAK      0x0009
#define	MQUIT             0x000A
#define	MTERMINATE        0x000B
#define	MATTACH           0x000C
#define	MCHECKSTATUS      0x000D
#define	MHALT             0x000E
#define	MDIRECTCMD        0x000F
#define	MSYMADR           0x0010
#define	MGETTASKLIST      0x0011
#define	MREADVECREG       0x0012
#define	MWRITEVECREG      0x0013
#define	MGETCHANGEDREGS   0x0014
#define	MGETSERVERINFO    0x0015
#define	MREADBLOCK        0x0016
#define	MSETHARDBRK       0x0017
#define	MREMOVEHARDBRK    0x0018
#define	MCOPYBLOCK        0x0019
#define	MBLOCKFILL        0x001A
#define	MFINDBLOCK        0x001B
#define	MCOMPAREBLOCK     0x001C
#define	MREFRESH          0x001D
#define	MSPECIAL          0x001E
#define	MGETCMDLIST       0x001F
#define	MEXPVAL           0x0020
#define	MEXPFAILED        0x0021
#define	MSAVESTATE        0x0022
#define	MWRITEBLOCK       0x0023
#define	MDETACH           0x0024
#define	MGETMODULES       0x0025
#define	MREMOTESYMBOL     0x0026
#define	MREADCSTRING      0x0027
#define	MLOADMODULE       0x0028
#define	MDIDSYSCALL       0x0029
#define	MDBPWRITEBUFFERS  0x002A
#define	MBPID		  0x002B
#define MINITEXEC         0x002C
#define	MEXITEXEC	  0x002D
#define	MRCCMD    	  0x002E
#define	MDOWNLOAD	  0x0050

#define StatRunning	0
#define StatExecBreak	1   /* an execution breakpoint has been reached */
#define StatStepped	2   /* a single step has been completed */
#define StatException	3   /* the target has stopped due to an exception */
#define StatHalted	4   /* target has been halted by a user request */
#define StatExited      5   /* target called exit */
#define StatTerminated  6   /* target program terminated by a user request */
#define StatNoProcess   7   /* no process on target and none of the above */
#define StatNeedInput   8   /* REV: obsolete */
#define StatNeedDirCmd  9   /* waiting for an entry in the remote window */
#define StatHardBreak	10  /* hit hardware breakpoint */
#define StatFailure	11  /* an error occured in the last run/single */

extern struct target_ops v850ice_ops;	/* Forward decl */

/*   "pir", "tkcw", "chcw", "adtre" */

/* Code for opening a connection to the ICE.  */

static void
v850ice_open (name, from_tty)
     char *name;
     int from_tty;
{
  long retval;

  if (name)
    error ("Too many arguments.");

  target_preopen (from_tty);

  unpush_target (&v850ice_ops);

  if (from_tty)
    puts_filtered ("V850ice debugging\n");

  push_target (&v850ice_ops);	/* Switch to using v850ice target now */

  target_terminal_init ();

  /* Without this, some commands which require an active target (such as kill)
     won't work.  This variable serves (at least) double duty as both the pid
     of the target process (if it has such), and as a flag indicating that a
     target is active.  These functions should be split out into seperate
     variables, especially since GDB will someday have a notion of debugging
     several processes.  */

  inferior_pid = 42000;

  /* Start the v850ice connection; if error (0), discard this target.
     In particular, if the user quits, be sure to discard it
     (we'd be in an inconsistent state otherwise).  */

  WinExec ("necsrv", SW_SHOW);	/* Start up necsrv */

  retval = RegisterClient (NULL);

  if (retval == 0)
    {
      ice_open = 1;

      start_remote ();
      return;
    }

  pop_target();

  error ("v850ice_open: MINITEXEC return error: 0x%x", retval);
}

/* Clean up connection to a remote debugger.  */

/* ARGSUSED */
static void
v850ice_close (quitting)
     int quitting;
{
  long retval;

  if (ice_open)
    {
#if 0
      retval = ExeAppReq ("GDB", MEXITEXEC, NULL, &null_iob);
      if (retval)
	error ("ExeAppReq (MEXITEXEC) returned %d", retval);
#endif
      ice_open = 0;
      UnregisterClient ();
    }
}

static void
v850ice_detach (args, from_tty)
     char *args;
     int from_tty;
{
  if (args)
    error ("Argument given to \"detach\" when remotely debugging.");

  pop_target ();
  if (from_tty)
    puts_filtered ("Ending v850ice debugging.\n");
}

/* Tell the remote machine to resume.  */

static void
v850ice_resume (pid, step, siggnal)
     int pid, step;
     enum target_signal siggnal;
{
  long retval;

  if (step)
    retval = ExeAppReq ("GDB", MSINGLESTEP, "step", &null_iob);
  else
    retval = ExeAppReq ("GDB", MRESUME, "run", &null_iob);

  if (retval)
    error ("ExeAppReq (step = %d) returned %d", step, retval);
}

/* Wait until the remote machine stops, then return,
   storing status in STATUS just as `wait' would.
   Returns "pid" (though it's not clear what, if anything, that
   means in the case of this target).  */

static int
v850ice_wait (pid, status)
     int pid;
     struct target_waitstatus *status;
{
  long v850_status;

  v850_status = ExeAppReq ("GDB", MCHECKSTATUS, NULL, &null_iob);

  status->kind = TARGET_WAITKIND_STOPPED;
  status->value.sig = TARGET_SIGNAL_TRAP;

  return inferior_pid;
}

static int
convert_register (regno, buf)
     int regno;
     char *buf;
{
  if (regno <= 31)
    sprintf (buf, "r%d", regno);
  else if (reg_names[regno][0] == 's'
	   && reg_names[regno][1] == 'r')
    return 0;
  else
    sprintf (buf, "%s", reg_names[regno]);

  return 1;
}

/* Read the remote registers into the block REGS.  */
/* Note that the ICE returns register contents as ascii hex strings.  We have
   to convert that to an unsigned long, and then call store_unsigned_integer to
   convert it to target byte-order if necessary.  */

static void
v850ice_fetch_registers (regno)
     int regno;
{
  long retval;
  char cmd[100];
  char val[100];
  struct MessageIO iob;
  unsigned long regval;
  char *p;

  if (regno == -1)
    {
      for (regno = 0; regno < NUM_REGS; regno++)
	v850ice_fetch_registers (regno);
      return;
    }

  strcpy (cmd, "reg ");
  if (!convert_register (regno, &cmd[4]))
    return;

  iob.size = sizeof val;
  iob.buf = val;
  retval = ExeAppReq ("GDB", MREADREG, cmd, &iob);
  if (retval)
    error ("ExeAppReq returned %d: cmd = %s", retval, cmd);

  regval = strtoul (val, &p, 16);
  if (regval == 0 && p == val)
    error ("v850ice_fetch_registers (%d):  bad value from ICE: %s.",
	   regno, val);

  store_unsigned_integer (val, REGISTER_RAW_SIZE (regno), regval);
  supply_register (regno, val);
}

/* Store register REGNO, or all registers if REGNO == -1, from the contents
   of REGISTERS.  */

static void
v850ice_store_registers (regno)
     int regno;
{
  long retval;
  char cmd[100];
  unsigned long regval;

  if (regno == -1)
    {
      for (regno = 0; regno < NUM_REGS; regno++)
	v850ice_store_registers (regno);
      return;
    }

  regval = extract_unsigned_integer (&registers[REGISTER_BYTE (regno)],
				     REGISTER_RAW_SIZE (regno));
  strcpy (cmd, "reg ");
  if (!convert_register (regno, &cmd[4]))
    return;
  sprintf (cmd + strlen (cmd), "=0x%x", regval);

  retval = ExeAppReq ("GDB", MWRITEREG, cmd, &null_iob);
  if (retval)
    error ("ExeAppReq returned %d: cmd = %s", retval, cmd);
}

/* Prepare to store registers.  Nothing to do here, since the ICE can write one
   register at a time.  */

static void 
v850ice_prepare_to_store ()
{
}

/* Read or write LEN bytes from inferior memory at MEMADDR, transferring
   to or from debugger address MYADDR.  Write to inferior if SHOULD_WRITE is
   nonzero.  Returns length of data written or read; 0 for error.  */

/* ARGSUSED */
static int
v850ice_xfer_memory (memaddr, myaddr, len, should_write, target)
     CORE_ADDR memaddr;
     char *myaddr;
     int len;
     int should_write;
     struct target_ops *target;			/* ignored */
{
  long retval;
  char cmd[100];
  struct MessageIO iob;

  iob.size = len;
  iob.buf = myaddr;

  if (should_write)
    {
#if 1
      sprintf (cmd, "memory b c 0x%x=0x00 l=%d", (int)memaddr, len);
      retval = ExeAppReq ("GDB", MWRITEBLOCK, cmd, &iob);
#else
      sprintf (cmd, "memory b c 0x%x=0x%x", (int)memaddr, *myaddr & 0xff);
      retval = ExeAppReq ("GDB", MWRITEBLOCK, cmd, &iob);
      return 1;
#endif
    }
  else
    {
      unsigned char *tmp;
      int i;

      tmp = alloca (len + 100);
      memset (tmp + len, 0xff, 100);
      
#if 1
      sprintf (cmd, "memory b 0x%x l=%d", (int)memaddr, len);
      retval = ExeAppReq ("GDB", MREADBLOCK, cmd, &iob);
#else
      sprintf (cmd, "memory h 0x%x", (int)memaddr);
      retval = ExeAppReq ("GDB", MREADMEM, cmd, &iob);
#endif
      for (i = 0; i <  100; i++)
	{
	  if (tmp[len + i] != 0xff)
	    {
	      warning ("MREADBLOCK trashed bytes after transfer area.");
	      break;
	    }
	}
      memcpy (myaddr, tmp, len);
    }

  if (retval)
    error ("ExeAppReq returned %d: cmd = %s", retval, cmd);

  return len;
}

static void
v850ice_files_info (ignore)
     struct target_ops *ignore;
{
  puts_filtered ("Debugging a target via the NEC V850 ICE.\n");
}

static int
v850ice_insert_breakpoint (addr, contents_cache)
     CORE_ADDR addr;
     char *contents_cache;
{
  long retval;
  char cmd[100];

  sprintf (cmd, "%d, ", addr);

#if 1
  retval = ExeAppReq ("GDB", MSETBREAK, cmd, &null_iob);
#else
  retval = ExeAppReq ("GDB", MSETHARDBRK, cmd, &null_iob);
#endif
  if (retval)
    error ("ExeAppReq (MSETBREAK) returned %d: cmd = %s", retval, cmd);

  return 0;
}

static int
v850ice_remove_breakpoint (addr, contents_cache)
     CORE_ADDR addr;
     char *contents_cache;
{
  long retval;
  char cmd[100];

  sprintf (cmd, "%d, ", addr);

#if 1
  retval = ExeAppReq ("GDB", MREMOVEBREAK, cmd, &null_iob);
#else
  retval = ExeAppReq ("GDB", MREMOVEHARDBRK, cmd, &null_iob);
#endif
  if (retval)
    error ("ExeAppReq (MREMOVEBREAK) returned %d: cmd = %s", retval, cmd);

  return 0;
}

static void
v850ice_kill ()
{
  target_mourn_inferior ();
}

static void
v850ice_mourn ()
{
}

/* Define the target subroutine names */

struct target_ops v850ice_ops = {
  "ice",			/* to_shortname */
  "NEC V850 ICE interface",	/* to_longname */
  "Debug a system controlled by a NEC 850 ICE.", /* to_doc */
  v850ice_open,			/* to_open */
  v850ice_close,		/* to_close */
  NULL,				/* to_attach */
  v850ice_detach,		/* to_detach */
  v850ice_resume,		/* to_resume */
  v850ice_wait,			/* to_wait */
  v850ice_fetch_registers,	/* to_fetch_registers */
  v850ice_store_registers,	/* to_store_registers */
  v850ice_prepare_to_store,	/* to_prepare_to_store */
  v850ice_xfer_memory,		/* to_xfer_memory */
  v850ice_files_info,		/* to_files_info */
  v850ice_insert_breakpoint,	/* to_insert_breakpoint */
  v850ice_remove_breakpoint,	/* to_remove_breakpoint */
  NULL,				/* to_terminal_init */
  NULL,				/* to_terminal_inferior */
  NULL,				/* to_terminal_ours_for_output */
  NULL,				/* to_terminal_ours */
  NULL,				/* to_terminal_info */
  v850ice_kill,			/* to_kill */
  generic_load,			/* to_load */
  NULL,				/* to_lookup_symbol */
  NULL,				/* to_create_inferior */
  v850ice_mourn,		/* to_mourn_inferior */
  0,				/* to_can_run */
  0,				/* to_notice_signals */
  NULL,				/* to_thread_alive */
  0,				/* to_stop */
  process_stratum,		/* to_stratum */
  NULL,				/* to_next */
  1,				/* to_has_all_memory */
  1,				/* to_has_memory */
  1,				/* to_has_stack */
  1,				/* to_has_registers */
  1,				/* to_has_execution */
  NULL,				/* sections */
  NULL,				/* sections_end */
  OPS_MAGIC			/* to_magic */
};

void
_initialize_v850ice ()
{
  add_target (&v850ice_ops);
}
