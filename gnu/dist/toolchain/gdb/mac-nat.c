/* Target-vector operations for controlling Mac applications, for GDB.
   Copyright (C) 1995 Free Software Foundation, Inc.
   Written by Stan Shebs.  Contributed by Cygnus Support.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without eve nthe implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* Note that because all the available Mac compilers are ANSI or very
   close, and this is a native-only file, the code may be purely ANSI.  */

#include "defs.h"
#include "frame.h"		/* required by inferior.h */
#include "inferior.h"
#include "target.h"
#include "gdb_wait.h"
#include "gdbcore.h"
#include "command.h"
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include "buildsym.h"
#include "gdb_string.h"
#include "gdbthread.h"
#include "gdbcmd.h"

#include <Processes.h>

/* We call the functions "child_..." rather than "mac_..." so no one
   is tempted to try to link this with other native-only code.  */

/* Forward declaration */

extern struct target_ops child_ops;

static void child_stop PARAMS ((void));

static void
child_fetch_inferior_registers (int r)
{
  if (r < 0)
    {
      for (r = 0; r < NUM_REGS; r++)
	child_fetch_inferior_registers (r);
    }
  else
    {
      supply_register (r, 0);
    }
}

static void
child_store_inferior_registers (int r)
{
  if (r < 0)
    {
      for (r = 0; r < NUM_REGS; r++)
	child_store_inferior_registers (r);
    }
  else
    {
      read_register_gen (r, 0);
    }
}

static int
child_wait (int pid, struct target_waitstatus *ourstatus)
{
}

/* Attach to process PID, then initialize for debugging it.  */

static void
child_attach (args, from_tty)
     char *args;
     int from_tty;
{
  ProcessSerialNumber psn;
  ProcessInfoRec inforec;
  Str31 name;
  FSSpecPtr fsspec;
  OSType code;
  int pid;
  char *exec_file;

  if (!args)
    error_no_arg ("process-id to attach");

  pid = atoi (args);

  psn.highLongOfPSN = 0;
  psn.lowLongOfPSN = pid;

  inforec.processInfoLength = sizeof (ProcessInfoRec);
  inforec.processName = name;
  inforec.processAppSpec = fsspec;

  if (GetProcessInformation (&psn, &inforec) == noErr)
    {
      if (from_tty)
	{
	  exec_file = (char *) get_exec_file (0);

	  if (exec_file)
	    printf_unfiltered ("Attaching to program `%s', %s\n", exec_file,
			       target_pid_to_str (pid));
	  else
	    printf_unfiltered ("Attaching to %s\n", target_pid_to_str (pid));

	  gdb_flush (gdb_stdout);
	}
      /* Do we need to do anything special? */
      attach_flag = 1;
      inferior_pid = pid;
      push_target (&child_ops);
    }
}

static void
child_detach (args, from_tty)
     char *args;
     int from_tty;
{
  char *exec_file;

  if (from_tty)
    {
      exec_file = get_exec_file (0);
      if (exec_file == 0)
	exec_file = "";
      printf_unfiltered ("Detaching from program: %s %s\n", exec_file,
			 target_pid_to_str (inferior_pid));
      gdb_flush (gdb_stdout);
    }
  inferior_pid = 0;
  unpush_target (&child_ops);
}

/* Print status information about what we're accessing.  */

static void
child_files_info (ignore)
     struct target_ops *ignore;
{
  printf_unfiltered ("\tUsing the running image of %s %s.\n",
      attach_flag ? "attached" : "child", target_pid_to_str (inferior_pid));
}

/* ARGSUSED */
static void
child_open (arg, from_tty)
     char *arg;
     int from_tty;
{
  error ("Use the \"run\" command to start a Mac application.");
}

/* Start an inferior Mac program and sets inferior_pid to its pid.
   EXEC_FILE is the file to run.
   ALLARGS is a string containing the arguments to the program.
   ENV is the environment vector to pass.  Errors reported with error().  */

static void
child_create_inferior (exec_file, allargs, env)
     char *exec_file;
     char *allargs;
     char **env;
{
  LaunchParamBlockRec launchparms;
  FSSpec fsspec;
  OSErr launch_err;

  if (!exec_file)
    {
      error ("No executable specified, use `target exec'.\n");
    }

  launchparms.launchBlockID = extendedBlock;
  launchparms.launchEPBLength = extendedBlockLen;
  launchparms.launchFileFlags = 0;
  launchparms.launchControlFlags = launchContinue | launchNoFileFlags;
  fsspec.vRefNum = 0;
  fsspec.parID = 0;
  strcpy (fsspec.name + 1, exec_file);
  fsspec.name[0] = strlen (exec_file);
  launchparms.launchAppSpec = &fsspec;
  launchparms.launchAppParameters = nil;

  launch_err = LaunchApplication (&launchparms);

  if (launch_err == 999 /*memFullErr */ )
    {
      error ("Not enough memory to launch %s\n", exec_file);
    }
  else if (launch_err != noErr)
    {
      error ("Error launching %s, code %d\n", exec_file, launch_err);
    }

  inferior_pid = launchparms.launchProcessSN.lowLongOfPSN;
  /* FIXME be sure that high long of PSN is 0 */

  push_target (&child_ops);
  init_wait_for_inferior ();
  clear_proceed_status ();

/*  proceed ((CORE_ADDR) -1, TARGET_SIGNAL_0, 0);  */
}

static void
child_mourn_inferior ()
{
  unpush_target (&child_ops);
  generic_mourn_inferior ();
}

static void
child_stop ()
{
}

int
child_xfer_memory (CORE_ADDR memaddr, char *myaddr, int len,
		   int write, struct target_ops *target)
{
  int i;

  for (i = 0; i < len; ++i)
    {
      if (write)
	{
	  ((char *) memaddr)[i] = myaddr[i];
	}
      else
	{
	  myaddr[i] = ((char *) memaddr)[i];
	}
    }
  return len;
}

void
child_kill_inferior (void)
{
}

void
child_resume (int pid, int step, enum target_signal signal)
{
}

static void
child_prepare_to_store ()
{
  /* Do nothing, since we can store individual regs */
}

static int
child_can_run ()
{
  return 1;
}

static void
child_close ()
{
}

static void
info_proc (args, from_tty)
     char *args;
     int from_tty;
{
  ProcessSerialNumber psn;
  ProcessInfoRec inforec;
  Str31 name;
  FSSpecPtr fsspec;
  OSType code;

  /* Eventually use args, but not right now. */

  psn.highLongOfPSN = 0;
  psn.lowLongOfPSN = kNoProcess;

  inforec.processInfoLength = sizeof (ProcessInfoRec);
  inforec.processName = name;
  inforec.processAppSpec = fsspec;

  printf_filtered ("Process Name                     Sgnt Type    PSN   Loc Size FreeMem Time\n");

  while (GetNextProcess (&psn) == noErr)
    {
      if (GetProcessInformation (&psn, &inforec) == noErr)
	{
	  name[name[0] + 1] = '\0';
	  printf_filtered ("%-32.32s", name + 1);
	  code = inforec.processSignature;
	  printf_filtered (" %c%c%c%c",
			   (code >> 24) & 0xff,
			   (code >> 16) & 0xff,
			   (code >> 8) & 0xff,
			   (code >> 0) & 0xff);
	  code = inforec.processType;
	  printf_filtered (" %c%c%c%c",
			   (code >> 24) & 0xff,
			   (code >> 16) & 0xff,
			   (code >> 8) & 0xff,
			   (code >> 0) & 0xff);
	  if (psn.highLongOfPSN == 0)
	    printf_filtered (" %9d", psn.lowLongOfPSN);
	  else
	    printf_filtered (" %9d,%9d\n",
			     psn.highLongOfPSN, psn.lowLongOfPSN);
	  printf_filtered (" 0x%x", inforec.processLocation);
	  printf_filtered (" %9d", inforec.processSize);
	  printf_filtered (" %9d", inforec.processFreeMem);
	  printf_filtered (" %9d", inforec.processActiveTime);
	  printf_filtered ("\n");
	}
    }
}

struct target_ops child_ops;

static void
init_child_ops (void)
{
  child_ops.to_shortname = "mac";
  child_ops.to_longname = "MacOS application";
  child_ops.to_doc = "MacOS application (started by the \"run\" command).";
  child_ops.to_open = child_open;
  child_ops.to_close = child_close;
  child_ops.to_attach = child_attach;
  child_ops.to_post_attach = NULL;
  child_ops.to_require_attach = NULL;	/* to_require_attach */
  child_ops.to_detach = child_detach;
  child_ops.to_require_detach = NULL;	/* to_require_detach */
  child_ops.to_resume = child_resume;
  child_ops.to_wait = child_wait;
  child_ops.to_post_wait = NULL;	/* to_post_wait */
  child_ops.to_fetch_registers = child_fetch_inferior_registers;
  child_ops.to_store_registers = child_store_inferior_registers;
  child_ops.to_prepare_to_store = child_prepare_to_store;
  child_ops.to_xfer_memory = child_xfer_memory;
  child_ops.to_files_info = child_files_info;
  child_ops.to_insert_breakpoint = memory_insert_breakpoint;
  child_ops.to_remove_breakpoint = memory_remove_breakpoint;
  child_ops.to_terminal_init = 0;
  child_ops.to_terminal_inferior = 0;
  child_ops.to_terminal_ours_for_output = 0;
  child_ops.to_terminal_ours = 0;
  child_ops.to_terminal_info = 0;
  child_ops.to_kill = child_kill_inferior;
  child_ops.to_load = 0;
  child_ops.to_lookup_symbol = 0;
  child_ops.to_create_inferior = child_create_inferior;
  child_ops.to_post_startup_inferior = NULL;	/* to_post_startup_inferior */
  child_ops.to_acknowledge_created_inferior = NULL;	/* to_acknowledge_created_inferior */
  child_ops.to_clone_and_follow_inferior = NULL;	/* to_clone_and_follow_inferior */
  child_ops.to_post_follow_inferior_by_clone = NULL;	/* to_post_follow_inferior_by_clone */
  child_ops.to_insert_fork_catchpoint = NULL;
  child_ops.to_remove_fork_catchpoint = NULL;
  child_ops.to_insert_vfork_catchpoint = NULL;
  child_ops.to_remove_vfork_catchpoint = NULL;
  child_ops.to_has_forked = NULL;	/* to_has_forked */
  child_ops.to_has_vforked = NULL;	/* to_has_vforked */
  child_ops.to_can_follow_vfork_prior_to_exec = NULL;
  child_ops.to_post_follow_vfork = NULL;	/* to_post_follow_vfork */
  child_ops.to_insert_exec_catchpoint = NULL;
  child_ops.to_remove_exec_catchpoint = NULL;
  child_ops.to_has_execd = NULL;
  child_ops.to_reported_exec_events_per_exec_call = NULL;
  child_ops.to_has_exited = NULL;
  child_ops.to_mourn_inferior = child_mourn_inferior;
  child_ops.to_can_run = child_can_run;
  child_ops.to_notice_signals = 0;
  child_ops.to_thread_alive = 0;
  child_ops.to_stop = child_stop;
  child_ops.to_pid_to_exec_file = NULL;		/* to_pid_to_exec_file */
  child_ops.to_core_file_to_sym_file = NULL;
  child_ops.to_stratum = process_stratum;
  child_ops.DONT_USE = 0;
  child_ops.to_has_all_memory = 1;
  child_ops.to_has_memory = 1;
  child_ops.to_has_stack = 1;
  child_ops.to_has_registers = 1;
  child_ops.to_has_execution = 1;
  child_ops.to_sections = 0;
  child_ops.to_sections_end = 0;
  child_ops.to_magic = OPS_MAGIC;
};

void
_initialize_mac_nat ()
{
  init_child_ops ();

  add_info ("proc", info_proc,
	    "Show information about processes.");
}
