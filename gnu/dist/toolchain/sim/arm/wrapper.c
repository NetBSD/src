/* run front end support for arm
   Copyright (C) 1995, 1996, 1997 Free Software Foundation, Inc.

This file is part of ARM SIM.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

/* This file provides the interface between the simulator and run.c and gdb
   (when the simulator is linked with gdb).
   All simulator interaction should go through this file.  */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <bfd.h>
#include <signal.h>
#include "callback.h"
#include "remote-sim.h"
#include "armdefs.h"
#include "armemu.h"
#include "dbg_rdi.h"
#include "ansidecl.h"

host_callback *sim_callback;

static struct ARMul_State *state;

/* Who is using the simulator.  */
static SIM_OPEN_KIND sim_kind;

/* argv[0] */
static char *myname;

/* Memory size in bytes.  */
static int mem_size = (1 << 21);

/* Non-zero to display start up banner, and maybe other things.  */
static int verbosity;

/* Non-zero to set big endian mode.  */
static int big_endian;

int stop_simulator;

static void
init ()
{
  static int done;

  if (!done)
    {
      ARMul_EmulateInit ();
      state = ARMul_NewState ();
      state->bigendSig = (big_endian ? HIGH : LOW);
      ARMul_MemoryInit (state, mem_size);
      ARMul_OSInit (state);
      ARMul_CoProInit (state);
      state->verbose = verbosity;
      done = 1;
    }
}

/* Set verbosity level of simulator.
   This is not intended to produce detailed tracing or debugging information.
   Just summaries.  */
/* FIXME: common/run.c doesn't do this yet.  */

void
sim_set_verbose (v)
     int v;
{
  verbosity = v;
}

/* Set the memory size to SIZE bytes.
   Must be called before initializing simulator.  */
/* FIXME: Rename to sim_set_mem_size.  */

void
sim_size (size)
     int size;
{
  mem_size = size;
}

void
ARMul_ConsolePrint (ARMul_State * state, const char *format, ...)
{
  va_list ap;

  if (state->verbose)
    {
      va_start (ap, format);
      vprintf (format, ap);
      va_end (ap);
    }
}

ARMword
ARMul_Debug (ARMul_State * state ATTRIBUTE_UNUSED, ARMword pc ATTRIBUTE_UNUSED, ARMword instr ATTRIBUTE_UNUSED)
{
  return 0;
}

int
sim_write (sd, addr, buffer, size)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  int i;
  init ();
  for (i = 0; i < size; i++)
    {
      ARMul_WriteByte (state, addr + i, buffer[i]);
    }
  return size;
}

int
sim_read (sd, addr, buffer, size)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     SIM_ADDR addr;
     unsigned char *buffer;
     int size;
{
  int i;
  init ();
  for (i = 0; i < size; i++)
    {
      buffer[i] = ARMul_ReadByte (state, addr + i);
    }
  return size;
}

int
sim_trace (sd)
     SIM_DESC sd ATTRIBUTE_UNUSED;
{  
  (*sim_callback->printf_filtered) (sim_callback,
				    "This simulator does not support tracing\n");
  return 1;
}

int
sim_stop (sd)
     SIM_DESC sd ATTRIBUTE_UNUSED;
{
  state->Emulate = STOP;
  stop_simulator = 1;
  return 1;
}

void
sim_resume (sd, step, siggnal)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     int step;
     int siggnal ATTRIBUTE_UNUSED;
{
  state->EndCondition = 0;
  stop_simulator = 0;

  if (step)
    {
      state->Reg[15] = ARMul_DoInstr (state);
      if (state->EndCondition == 0)
	state->EndCondition = RDIError_BreakpointReached;
    }
  else
    {
#if 1				/* JGS */
      state->NextInstr = RESUME;	/* treat as PC change */
#endif
      state->Reg[15] = ARMul_DoProg (state);
    }

  FLUSHPIPE;
}

SIM_RC
sim_create_inferior (sd, abfd, argv, env)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     struct _bfd *abfd;
     char **argv;
     char **env;
{
  int argvlen = 0;
  char **arg;

  if (abfd != NULL)
    ARMul_SetPC (state, bfd_get_start_address (abfd));
  else
    ARMul_SetPC (state, 0);	/* ??? */

#if 1				/* JGS */
  /* We explicitly select a processor capable of supporting the ARM
     32bit mode, and then we force the simulated CPU into the 32bit
     User mode: */
  ARMul_SelectProcessor (state, ARM600);
  ARMul_SetCPSR (state, USER32MODE);
#endif

  if (argv != NULL)
    {
      /*
         ** Set up the command line (by laboriously stringing together the
         ** environment carefully picked apart by our caller...)
       */
      /* Free any old stuff */
      if (state->CommandLine != NULL)
	{
	  free (state->CommandLine);
	  state->CommandLine = NULL;
	}

      /* See how much we need */
      for (arg = argv; *arg != NULL; arg++)
	argvlen += strlen (*arg) + 1;

      /* allocate it... */
      state->CommandLine = malloc (argvlen + 1);
      if (state->CommandLine != NULL)
	{
	  arg = argv;
	  state->CommandLine[0] = '\0';
	  for (arg = argv; *arg != NULL; arg++)
	    {
	      strcat (state->CommandLine, *arg);
	      strcat (state->CommandLine, " ");
	    }
	}
    }

  if (env != NULL)
    {
      /* Now see if there's a MEMSIZE spec in the environment */
      while (*env)
	{
	  if (strncmp (*env, "MEMSIZE=", sizeof ("MEMSIZE=") - 1) == 0)
	    {
	      char *end_of_num;

	      /* Set up memory limit */
	      state->MemSize =
		strtoul (*env + sizeof ("MEMSIZE=") - 1, &end_of_num, 0);
	    }
	  env++;
	}
    }

  return SIM_RC_OK;
}

void
sim_info (sd, verbose)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     int verbose ATTRIBUTE_UNUSED;
{
}


static int
frommem (state, memory)
     struct ARMul_State *state;
     unsigned char *memory;
{
  if (state->bigendSig == HIGH)
    {
      return (memory[0] << 24)
	| (memory[1] << 16) | (memory[2] << 8) | (memory[3] << 0);
    }
  else
    {
      return (memory[3] << 24)
	| (memory[2] << 16) | (memory[1] << 8) | (memory[0] << 0);
    }
}


static void
tomem (state, memory, val)
     struct ARMul_State *state;
     unsigned char *memory;
     int val;
{
  if (state->bigendSig == HIGH)
    {
      memory[0] = val >> 24;
      memory[1] = val >> 16;
      memory[2] = val >> 8;
      memory[3] = val >> 0;
    }
  else
    {
      memory[3] = val >> 24;
      memory[2] = val >> 16;
      memory[1] = val >> 8;
      memory[0] = val >> 0;
    }
}

int
sim_store_register (sd, rn, memory, length)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     int rn;
     unsigned char *memory;
     int length ATTRIBUTE_UNUSED;
{
  init ();
  ARMul_SetReg (state, state->Mode, rn, frommem (state, memory));
  return -1;
}

int
sim_fetch_register (sd, rn, memory, length)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     int rn;
     unsigned char *memory;
     int length ATTRIBUTE_UNUSED;
{
  ARMword regval;

  init ();
  if (rn < 16)
    regval = ARMul_GetReg (state, state->Mode, rn);
  else if (rn == 25)		/* FIXME: use PS_REGNUM from gdb/config/arm/tm-arm.h */
    regval = ARMul_GetCPSR (state);
  else
    regval = 0;			/* FIXME: should report an error */
  tomem (state, memory, regval);
  return -1;
}

SIM_DESC
sim_open (kind, ptr, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *ptr;
     struct _bfd *abfd;
     char **argv;
{
  sim_kind = kind;
  if (myname) free (myname);
  myname = xstrdup (argv[0]);
  sim_callback = ptr;

  /* Decide upon the endian-ness of the processor.
     If we can, get the information from the bfd itself.
     Otherwise look to see if we have been given a command
     line switch that tells us.  Otherwise default to little endian.  */
  if (abfd != NULL)
    big_endian = bfd_big_endian (abfd);
  else if (argv[1] != NULL)
    {
      int i;

      /* Scan for endian-ness switch.  */
      for (i = 0; (argv[i] != NULL) && (argv[i][0] != 0); i++)
	if (argv[i][0] == '-' && argv[i][1] == 'E')
	  {
	    char c;

	    if ((c = argv[i][2]) == 0)
	      {
		++i;
		c = argv[i][0];
	      }

	    switch (c)
	      {
	      case 0:
		sim_callback->printf_filtered
		  (sim_callback, "No argument to -E option provided\n");
		break;

	      case 'b':
	      case 'B':
		big_endian = 1;
		break;

	      case 'l':
	      case 'L':
		big_endian = 0;
		break;

	      default:
		sim_callback->printf_filtered
		  (sim_callback, "Unrecognised argument to -E option\n");
		break;
	      }
	  }
    }

  return (SIM_DESC) 1;
}

void
sim_close (sd, quitting)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     int quitting ATTRIBUTE_UNUSED;
{
  if (myname) free (myname);
  myname = NULL;
}

SIM_RC
sim_load (sd, prog, abfd, from_tty)
     SIM_DESC sd;
     char *prog;
     bfd *abfd;
     int from_tty ATTRIBUTE_UNUSED;
{
  extern bfd *sim_load_file ();	/* ??? Don't know where this should live.  */
  bfd *prog_bfd;

  prog_bfd = sim_load_file (sd, myname, sim_callback, prog, abfd,
			    sim_kind == SIM_OPEN_DEBUG, 0, sim_write);
  if (prog_bfd == NULL)
    return SIM_RC_FAIL;
  ARMul_SetPC (state, bfd_get_start_address (prog_bfd));
  if (abfd == NULL)
    bfd_close (prog_bfd);
  return SIM_RC_OK;
}

void
sim_stop_reason (sd, reason, sigrc)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     enum sim_stop *reason;
     int *sigrc;
{
  if (stop_simulator)
    {
      *reason = sim_stopped;
      *sigrc = SIGINT;
    }
  else if (state->EndCondition == 0)
    {
      *reason = sim_exited;
      *sigrc = state->Reg[0] & 255;
    }
  else
    {
      *reason = sim_stopped;
      if (state->EndCondition == RDIError_BreakpointReached)
	*sigrc = SIGTRAP;
      else
	*sigrc = 0;
    }
}

void
sim_do_command (sd, cmd)
     SIM_DESC sd ATTRIBUTE_UNUSED;
     char *cmd ATTRIBUTE_UNUSED;
{  
  (*sim_callback->printf_filtered) (sim_callback,
				    "This simulator does not accept any commands.\n");
}


void
sim_set_callbacks (ptr)
     host_callback *ptr;
{
  sim_callback = ptr;
}
