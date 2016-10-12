/* run front end support for arm
   Copyright (C) 1995-2015 Free Software Foundation, Inc.

   This file is part of ARM SIM.

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

/* This file provides the interface between the simulator and
   run.c and gdb (when the simulator is linked with gdb).
   All simulator interaction should go through this file.  */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <bfd.h>
#include <signal.h>
#include "gdb/callback.h"
#include "gdb/remote-sim.h"
#include "sim-main.h"
#include "sim-options.h"
#include "armemu.h"
#include "dbg_rdi.h"
#include "ansidecl.h"
#include "gdb/sim-arm.h"
#include "gdb/signals.h"
#include "libiberty.h"
#include "iwmmxt.h"

/* TODO: This should get pulled from the SIM_DESC.  */
host_callback *sim_callback;

/* TODO: This should get merged into sim_cpu.  */
struct ARMul_State *state;

/* Memory size in bytes.  */
/* TODO: Memory should be converted to the common memory module.  */
static int mem_size = (1 << 21);

int stop_simulator;

#include "dis-asm.h"

/* TODO: Tracing should be converted to common tracing module.  */
int trace = 0;
int disas = 0;
int trace_funcs = 0;

static struct disassemble_info  info;
static char opbuf[1000];

static int
op_printf (char *buf, char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start (ap, fmt);
  ret = vsprintf (opbuf + strlen (opbuf), fmt, ap);
  va_end (ap);
  return ret;
}

static int
sim_dis_read (bfd_vma                     memaddr ATTRIBUTE_UNUSED,
	      bfd_byte *                  ptr,
	      unsigned int                length,
	      struct disassemble_info *   info)
{
  ARMword val = (ARMword) *((ARMword *) info->application_data);

  while (length--)
    {
      * ptr ++ = val & 0xFF;
      val >>= 8;
    }
  return 0;
}

void
print_insn (ARMword instr)
{
  int size;

  opbuf[0] = 0;
  info.application_data = & instr;
  size = print_insn_little_arm (0, & info);
  fprintf (stderr, " %*s\n", size, opbuf);
}

/* Cirrus DSP registers.

   We need to define these registers outside of maverick.c because
   maverick.c might not be linked in unless --target=arm9e-* in which
   case wrapper.c will not compile because it tries to access Cirrus
   registers.  This should all go away once we get the Cirrus and ARM
   Coprocessor to coexist in armcopro.c-- aldyh.  */

struct maverick_regs
{
  union
  {
    int i;
    float f;
  } upper;
  
  union
  {
    int i;
    float f;
  } lower;
};

union maverick_acc_regs
{
  long double ld;		/* Acc registers are 72-bits.  */
};

struct maverick_regs     DSPregs[16];
union maverick_acc_regs  DSPacc[4];
ARMword DSPsc;

static void
init (void)
{
  static int done;

  if (!done)
    {
      ARMul_EmulateInit ();
      state = ARMul_NewState ();
      state->bigendSig = (CURRENT_TARGET_BYTE_ORDER == BIG_ENDIAN ? HIGH : LOW);
      ARMul_MemoryInit (state, mem_size);
      ARMul_OSInit (state);
      state->verbose = 0;
      done = 1;
    }
}

void
ARMul_ConsolePrint (ARMul_State * state,
		    const char * format,
		    ...)
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
ARMul_Debug (ARMul_State * state ATTRIBUTE_UNUSED,
	     ARMword       pc    ATTRIBUTE_UNUSED,
	     ARMword       instr ATTRIBUTE_UNUSED)
{
  return 0;
}

int
sim_write (SIM_DESC sd ATTRIBUTE_UNUSED,
	   SIM_ADDR addr,
	   const unsigned char * buffer,
	   int size)
{
  int i;

  init ();

  for (i = 0; i < size; i++)
    ARMul_SafeWriteByte (state, addr + i, buffer[i]);

  return size;
}

int
sim_read (SIM_DESC sd ATTRIBUTE_UNUSED,
	  SIM_ADDR addr,
	  unsigned char * buffer,
	  int size)
{
  int i;

  init ();

  for (i = 0; i < size; i++)
    buffer[i] = ARMul_SafeReadByte (state, addr + i);

  return size;
}

int
sim_stop (SIM_DESC sd ATTRIBUTE_UNUSED)
{
  state->Emulate = STOP;
  stop_simulator = 1;
  return 1;
}

void
sim_resume (SIM_DESC sd ATTRIBUTE_UNUSED,
	    int step,
	    int siggnal ATTRIBUTE_UNUSED)
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
      state->NextInstr = RESUME;	/* treat as PC change */
      state->Reg[15] = ARMul_DoProg (state);
    }

  FLUSHPIPE;
}

SIM_RC
sim_create_inferior (SIM_DESC sd ATTRIBUTE_UNUSED,
		     struct bfd * abfd,
		     char ** argv,
		     char ** env)
{
  int argvlen = 0;
  int mach;
  char **arg;

  init ();

  if (abfd != NULL)
    {
      ARMul_SetPC (state, bfd_get_start_address (abfd));
      mach = bfd_get_mach (abfd);
    }
  else
    {
      ARMul_SetPC (state, 0);	/* ??? */
      mach = 0;
    }

#ifdef MODET
  if (abfd != NULL && (bfd_get_start_address (abfd) & 1))
    SETT;
#endif

  switch (mach)
    {
    default:
      (*sim_callback->printf_filtered)
	(sim_callback,
	 "Unknown machine type '%d'; please update sim_create_inferior.\n",
	 mach);
      /* fall through */

    case 0:
      /* We wouldn't set the machine type with earlier toolchains, so we
	 explicitly select a processor capable of supporting all ARMs in
	 32bit mode.  */
      ARMul_SelectProcessor (state, ARM_v5_Prop | ARM_v5e_Prop | ARM_v6_Prop);
      break;

    case bfd_mach_arm_XScale:
      ARMul_SelectProcessor (state, ARM_v5_Prop | ARM_v5e_Prop | ARM_XScale_Prop | ARM_v6_Prop);
      break;

    case bfd_mach_arm_iWMMXt2:
    case bfd_mach_arm_iWMMXt:
      {
	extern int SWI_vector_installed;
	ARMword i;

	if (! SWI_vector_installed)
	  {
	    /* Intialise the hardware vectors to zero.  */
	    if (! SWI_vector_installed)
	      for (i = ARMul_ResetV; i <= ARMFIQV; i += 4)
		ARMul_WriteWord (state, i, 0);

	    /* ARM_WriteWord will have detected the write to the SWI vector,
	       but we want SWI_vector_installed to remain at 0 so that thumb
	       mode breakpoints will work.  */
	    SWI_vector_installed = 0;
	  }
      }
      ARMul_SelectProcessor (state, ARM_v5_Prop | ARM_v5e_Prop | ARM_XScale_Prop | ARM_iWMMXt_Prop);
      break;

    case bfd_mach_arm_ep9312:
      ARMul_SelectProcessor (state, ARM_v4_Prop | ARM_ep9312_Prop);
      break;

    case bfd_mach_arm_5:
      if (bfd_family_coff (abfd))
	{
	  /* This is a special case in order to support COFF based ARM toolchains.
	     The COFF header does not have enough room to store all the different
	     kinds of ARM cpu, so the XScale, v5T and v5TE architectures all default
	     to v5.  (See coff_set_flags() in bdf/coffcode.h).  So if we see a v5
	     machine type here, we assume it could be any of the above architectures
	     and so select the most feature-full.  */
	  ARMul_SelectProcessor (state, ARM_v5_Prop | ARM_v5e_Prop | ARM_XScale_Prop);
	  break;
	}
      /* Otherwise drop through.  */

    case bfd_mach_arm_5T:
      ARMul_SelectProcessor (state, ARM_v5_Prop);
      break;

    case bfd_mach_arm_5TE:
      ARMul_SelectProcessor (state, ARM_v5_Prop | ARM_v5e_Prop);
      break;

    case bfd_mach_arm_4:
    case bfd_mach_arm_4T:
      ARMul_SelectProcessor (state, ARM_v4_Prop);
      break;

    case bfd_mach_arm_3:
    case bfd_mach_arm_3M:
      ARMul_SelectProcessor (state, ARM_Lock_Prop);
      break;

    case bfd_mach_arm_2:
    case bfd_mach_arm_2a:
      ARMul_SelectProcessor (state, ARM_Fix26_Prop);
      break;
    }

  memset (& info, 0, sizeof (info));
  INIT_DISASSEMBLE_INFO (info, stdout, op_printf);
  info.read_memory_func = sim_dis_read;
  info.arch = bfd_get_arch (abfd);
  info.mach = bfd_get_mach (abfd);
  info.endian_code = BFD_ENDIAN_LITTLE;
  if (info.mach == 0)
    info.arch = bfd_arch_arm;
  disassemble_init_for_target (& info);

  if (argv != NULL)
    {
      /* Set up the command line by laboriously stringing together
	 the environment carefully picked apart by our caller.  */

      /* Free any old stuff.  */
      if (state->CommandLine != NULL)
	{
	  free (state->CommandLine);
	  state->CommandLine = NULL;
	}

      /* See how much we need.  */
      for (arg = argv; *arg != NULL; arg++)
	argvlen += strlen (*arg) + 1;

      /* Allocate it.  */
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
      /* Now see if there's a MEMSIZE spec in the environment.  */
      while (*env)
	{
	  if (strncmp (*env, "MEMSIZE=", sizeof ("MEMSIZE=") - 1) == 0)
	    {
	      char *end_of_num;

	      /* Set up memory limit.  */
	      state->MemSize =
		strtoul (*env + sizeof ("MEMSIZE=") - 1, &end_of_num, 0);
	    }
	  env++;
	}
    }

  return SIM_RC_OK;
}

static int
frommem (struct ARMul_State *state, unsigned char *memory)
{
  if (state->bigendSig == HIGH)
    return (memory[0] << 24) | (memory[1] << 16)
      | (memory[2] << 8) | (memory[3] << 0);
  else
    return (memory[3] << 24) | (memory[2] << 16)
      | (memory[1] << 8) | (memory[0] << 0);
}

static void
tomem (struct ARMul_State *state,
       unsigned char *memory,
       int val)
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
sim_store_register (SIM_DESC sd ATTRIBUTE_UNUSED,
		    int rn,
		    unsigned char *memory,
		    int length)
{
  init ();

  switch ((enum sim_arm_regs) rn)
    {
    case SIM_ARM_R0_REGNUM:
    case SIM_ARM_R1_REGNUM:
    case SIM_ARM_R2_REGNUM:
    case SIM_ARM_R3_REGNUM:
    case SIM_ARM_R4_REGNUM:
    case SIM_ARM_R5_REGNUM:
    case SIM_ARM_R6_REGNUM:
    case SIM_ARM_R7_REGNUM:
    case SIM_ARM_R8_REGNUM:
    case SIM_ARM_R9_REGNUM:
    case SIM_ARM_R10_REGNUM:
    case SIM_ARM_R11_REGNUM:
    case SIM_ARM_R12_REGNUM:
    case SIM_ARM_R13_REGNUM:
    case SIM_ARM_R14_REGNUM:
    case SIM_ARM_R15_REGNUM: /* PC */
    case SIM_ARM_FP0_REGNUM:
    case SIM_ARM_FP1_REGNUM:
    case SIM_ARM_FP2_REGNUM:
    case SIM_ARM_FP3_REGNUM:
    case SIM_ARM_FP4_REGNUM:
    case SIM_ARM_FP5_REGNUM:
    case SIM_ARM_FP6_REGNUM:
    case SIM_ARM_FP7_REGNUM:
    case SIM_ARM_FPS_REGNUM:
      ARMul_SetReg (state, state->Mode, rn, frommem (state, memory));
      break;

    case SIM_ARM_PS_REGNUM:
      state->Cpsr = frommem (state, memory);
      ARMul_CPSRAltered (state);
      break;

    case SIM_ARM_MAVERIC_COP0R0_REGNUM:
    case SIM_ARM_MAVERIC_COP0R1_REGNUM:
    case SIM_ARM_MAVERIC_COP0R2_REGNUM:
    case SIM_ARM_MAVERIC_COP0R3_REGNUM:
    case SIM_ARM_MAVERIC_COP0R4_REGNUM:
    case SIM_ARM_MAVERIC_COP0R5_REGNUM:
    case SIM_ARM_MAVERIC_COP0R6_REGNUM:
    case SIM_ARM_MAVERIC_COP0R7_REGNUM:
    case SIM_ARM_MAVERIC_COP0R8_REGNUM:
    case SIM_ARM_MAVERIC_COP0R9_REGNUM:
    case SIM_ARM_MAVERIC_COP0R10_REGNUM:
    case SIM_ARM_MAVERIC_COP0R11_REGNUM:
    case SIM_ARM_MAVERIC_COP0R12_REGNUM:
    case SIM_ARM_MAVERIC_COP0R13_REGNUM:
    case SIM_ARM_MAVERIC_COP0R14_REGNUM:
    case SIM_ARM_MAVERIC_COP0R15_REGNUM:
      memcpy (& DSPregs [rn - SIM_ARM_MAVERIC_COP0R0_REGNUM],
	      memory, sizeof (struct maverick_regs));
      return sizeof (struct maverick_regs);

    case SIM_ARM_MAVERIC_DSPSC_REGNUM:
      memcpy (&DSPsc, memory, sizeof DSPsc);
      return sizeof DSPsc;

    case SIM_ARM_IWMMXT_COP0R0_REGNUM:
    case SIM_ARM_IWMMXT_COP0R1_REGNUM:
    case SIM_ARM_IWMMXT_COP0R2_REGNUM:
    case SIM_ARM_IWMMXT_COP0R3_REGNUM:
    case SIM_ARM_IWMMXT_COP0R4_REGNUM:
    case SIM_ARM_IWMMXT_COP0R5_REGNUM:
    case SIM_ARM_IWMMXT_COP0R6_REGNUM:
    case SIM_ARM_IWMMXT_COP0R7_REGNUM:
    case SIM_ARM_IWMMXT_COP0R8_REGNUM:
    case SIM_ARM_IWMMXT_COP0R9_REGNUM:
    case SIM_ARM_IWMMXT_COP0R10_REGNUM:
    case SIM_ARM_IWMMXT_COP0R11_REGNUM:
    case SIM_ARM_IWMMXT_COP0R12_REGNUM:
    case SIM_ARM_IWMMXT_COP0R13_REGNUM:
    case SIM_ARM_IWMMXT_COP0R14_REGNUM:
    case SIM_ARM_IWMMXT_COP0R15_REGNUM:
    case SIM_ARM_IWMMXT_COP1R0_REGNUM:
    case SIM_ARM_IWMMXT_COP1R1_REGNUM:
    case SIM_ARM_IWMMXT_COP1R2_REGNUM:
    case SIM_ARM_IWMMXT_COP1R3_REGNUM:
    case SIM_ARM_IWMMXT_COP1R4_REGNUM:
    case SIM_ARM_IWMMXT_COP1R5_REGNUM:
    case SIM_ARM_IWMMXT_COP1R6_REGNUM:
    case SIM_ARM_IWMMXT_COP1R7_REGNUM:
    case SIM_ARM_IWMMXT_COP1R8_REGNUM:
    case SIM_ARM_IWMMXT_COP1R9_REGNUM:
    case SIM_ARM_IWMMXT_COP1R10_REGNUM:
    case SIM_ARM_IWMMXT_COP1R11_REGNUM:
    case SIM_ARM_IWMMXT_COP1R12_REGNUM:
    case SIM_ARM_IWMMXT_COP1R13_REGNUM:
    case SIM_ARM_IWMMXT_COP1R14_REGNUM:
    case SIM_ARM_IWMMXT_COP1R15_REGNUM:
      return Store_Iwmmxt_Register (rn - SIM_ARM_IWMMXT_COP0R0_REGNUM, memory);

    default:
      return 0;
    }

  return length;
}

int
sim_fetch_register (SIM_DESC sd ATTRIBUTE_UNUSED,
		    int rn,
		    unsigned char *memory,
		    int length)
{
  ARMword regval;
  int len = length;

  init ();

  switch ((enum sim_arm_regs) rn)
    {
    case SIM_ARM_R0_REGNUM:
    case SIM_ARM_R1_REGNUM:
    case SIM_ARM_R2_REGNUM:
    case SIM_ARM_R3_REGNUM:
    case SIM_ARM_R4_REGNUM:
    case SIM_ARM_R5_REGNUM:
    case SIM_ARM_R6_REGNUM:
    case SIM_ARM_R7_REGNUM:
    case SIM_ARM_R8_REGNUM:
    case SIM_ARM_R9_REGNUM:
    case SIM_ARM_R10_REGNUM:
    case SIM_ARM_R11_REGNUM:
    case SIM_ARM_R12_REGNUM:
    case SIM_ARM_R13_REGNUM:
    case SIM_ARM_R14_REGNUM:
    case SIM_ARM_R15_REGNUM: /* PC */
      regval = ARMul_GetReg (state, state->Mode, rn);
      break;

    case SIM_ARM_FP0_REGNUM:
    case SIM_ARM_FP1_REGNUM:
    case SIM_ARM_FP2_REGNUM:
    case SIM_ARM_FP3_REGNUM:
    case SIM_ARM_FP4_REGNUM:
    case SIM_ARM_FP5_REGNUM:
    case SIM_ARM_FP6_REGNUM:
    case SIM_ARM_FP7_REGNUM:
    case SIM_ARM_FPS_REGNUM:
      memset (memory, 0, length);
      return 0;

    case SIM_ARM_PS_REGNUM:
      regval = ARMul_GetCPSR (state);
      break;

    case SIM_ARM_MAVERIC_COP0R0_REGNUM:
    case SIM_ARM_MAVERIC_COP0R1_REGNUM:
    case SIM_ARM_MAVERIC_COP0R2_REGNUM:
    case SIM_ARM_MAVERIC_COP0R3_REGNUM:
    case SIM_ARM_MAVERIC_COP0R4_REGNUM:
    case SIM_ARM_MAVERIC_COP0R5_REGNUM:
    case SIM_ARM_MAVERIC_COP0R6_REGNUM:
    case SIM_ARM_MAVERIC_COP0R7_REGNUM:
    case SIM_ARM_MAVERIC_COP0R8_REGNUM:
    case SIM_ARM_MAVERIC_COP0R9_REGNUM:
    case SIM_ARM_MAVERIC_COP0R10_REGNUM:
    case SIM_ARM_MAVERIC_COP0R11_REGNUM:
    case SIM_ARM_MAVERIC_COP0R12_REGNUM:
    case SIM_ARM_MAVERIC_COP0R13_REGNUM:
    case SIM_ARM_MAVERIC_COP0R14_REGNUM:
    case SIM_ARM_MAVERIC_COP0R15_REGNUM:
      memcpy (memory, & DSPregs [rn - SIM_ARM_MAVERIC_COP0R0_REGNUM],
	      sizeof (struct maverick_regs));
      return sizeof (struct maverick_regs);

    case SIM_ARM_MAVERIC_DSPSC_REGNUM:
      memcpy (memory, & DSPsc, sizeof DSPsc);
      return sizeof DSPsc;

    case SIM_ARM_IWMMXT_COP0R0_REGNUM:
    case SIM_ARM_IWMMXT_COP0R1_REGNUM:
    case SIM_ARM_IWMMXT_COP0R2_REGNUM:
    case SIM_ARM_IWMMXT_COP0R3_REGNUM:
    case SIM_ARM_IWMMXT_COP0R4_REGNUM:
    case SIM_ARM_IWMMXT_COP0R5_REGNUM:
    case SIM_ARM_IWMMXT_COP0R6_REGNUM:
    case SIM_ARM_IWMMXT_COP0R7_REGNUM:
    case SIM_ARM_IWMMXT_COP0R8_REGNUM:
    case SIM_ARM_IWMMXT_COP0R9_REGNUM:
    case SIM_ARM_IWMMXT_COP0R10_REGNUM:
    case SIM_ARM_IWMMXT_COP0R11_REGNUM:
    case SIM_ARM_IWMMXT_COP0R12_REGNUM:
    case SIM_ARM_IWMMXT_COP0R13_REGNUM:
    case SIM_ARM_IWMMXT_COP0R14_REGNUM:
    case SIM_ARM_IWMMXT_COP0R15_REGNUM:
    case SIM_ARM_IWMMXT_COP1R0_REGNUM:
    case SIM_ARM_IWMMXT_COP1R1_REGNUM:
    case SIM_ARM_IWMMXT_COP1R2_REGNUM:
    case SIM_ARM_IWMMXT_COP1R3_REGNUM:
    case SIM_ARM_IWMMXT_COP1R4_REGNUM:
    case SIM_ARM_IWMMXT_COP1R5_REGNUM:
    case SIM_ARM_IWMMXT_COP1R6_REGNUM:
    case SIM_ARM_IWMMXT_COP1R7_REGNUM:
    case SIM_ARM_IWMMXT_COP1R8_REGNUM:
    case SIM_ARM_IWMMXT_COP1R9_REGNUM:
    case SIM_ARM_IWMMXT_COP1R10_REGNUM:
    case SIM_ARM_IWMMXT_COP1R11_REGNUM:
    case SIM_ARM_IWMMXT_COP1R12_REGNUM:
    case SIM_ARM_IWMMXT_COP1R13_REGNUM:
    case SIM_ARM_IWMMXT_COP1R14_REGNUM:
    case SIM_ARM_IWMMXT_COP1R15_REGNUM:
      return Fetch_Iwmmxt_Register (rn - SIM_ARM_IWMMXT_COP0R0_REGNUM, memory);

    default:
      return 0;
    }

  while (len)
    {
      tomem (state, memory, regval);

      len -= 4;
      memory += 4;
      regval = 0;
    }  

  return length;
}

typedef struct
{
  char * 	swi_option;
  unsigned int	swi_mask;
} swi_options;

#define SWI_SWITCH	"--swi-support"

static swi_options options[] =
  {
    { "none",    0 },
    { "demon",   SWI_MASK_DEMON },
    { "angel",   SWI_MASK_ANGEL },
    { "redboot", SWI_MASK_REDBOOT },
    { "all",     -1 },
    { "NONE",    0 },
    { "DEMON",   SWI_MASK_DEMON },
    { "ANGEL",   SWI_MASK_ANGEL },
    { "REDBOOT", SWI_MASK_REDBOOT },
    { "ALL",     -1 }
  };


static int
sim_target_parse_command_line (int argc, char ** argv)
{
  int i;

  for (i = 1; i < argc; i++)
    {
      char * ptr = argv[i];
      int arg;

      if ((ptr == NULL) || (* ptr != '-'))
	break;

      if (strcmp (ptr, "-t") == 0)
	{
	  trace = 1;
	  continue;
	}
      
      if (strcmp (ptr, "-z") == 0)
	{
	  /* Remove this option from the argv array.  */
	  for (arg = i; arg < argc; arg ++)
	    argv[arg] = argv[arg + 1];
	  argc --;
	  i --;
	  trace_funcs = 1;
	  continue;
	}
      
      if (strcmp (ptr, "-d") == 0)
	{
	  /* Remove this option from the argv array.  */
	  for (arg = i; arg < argc; arg ++)
	    argv[arg] = argv[arg + 1];
	  argc --;
	  i --;
	  disas = 1;
	  continue;
	}

      if (strncmp (ptr, SWI_SWITCH, sizeof SWI_SWITCH - 1) != 0)
	continue;

      if (ptr[sizeof SWI_SWITCH - 1] == 0)
	{
	  /* Remove this option from the argv array.  */
	  for (arg = i; arg < argc; arg ++)
	    argv[arg] = argv[arg + 1];
	  argc --;
	  
	  ptr = argv[i];
	}
      else
	ptr += sizeof SWI_SWITCH;

      swi_mask = 0;
      
      while (* ptr)
	{
	  int i;

	  for (i = sizeof options / sizeof options[0]; i--;)
	    if (strncmp (ptr, options[i].swi_option,
			 strlen (options[i].swi_option)) == 0)
	      {
		swi_mask |= options[i].swi_mask;
		ptr += strlen (options[i].swi_option);

		if (* ptr == ',')
		  ++ ptr;

		break;
	      }

	  if (i < 0)
	    break;
	}

      if (* ptr != 0)
	fprintf (stderr, "Ignoring swi options: %s\n", ptr);
      
      /* Remove this option from the argv array.  */
      for (arg = i; arg < argc; arg ++)
	argv[arg] = argv[arg + 1];
      argc --;
      i --;
    }
  return argc;
}

static void
sim_target_parse_arg_array (char ** argv)
{
  int i;

  for (i = 0; argv[i]; i++)
    ;

  sim_target_parse_command_line (i, argv);
}

static sim_cia
arm_pc_get (sim_cpu *cpu)
{
  return PC;
}

static void
arm_pc_set (sim_cpu *cpu, sim_cia pc)
{
  ARMul_SetPC (state, pc);
}

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
	  host_callback *cb,
	  struct bfd *abfd,
	  char **argv)
{
  int i;
  SIM_DESC sd = sim_state_alloc (kind, cb);
  SIM_ASSERT (STATE_MAGIC (sd) == SIM_MAGIC_NUMBER);

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 1, /*cgen_cpu_max_extra_bytes ()*/0) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check for/establish the a reference program image.  */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

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

  /* CPU specific initialization.  */
  for (i = 0; i < MAX_NR_PROCESSORS; ++i)
    {
      SIM_CPU *cpu = STATE_CPU (sd, i);

      CPU_PC_FETCH (cpu) = arm_pc_get;
      CPU_PC_STORE (cpu) = arm_pc_set;
    }

  sim_callback = cb;

  sim_target_parse_arg_array (argv);

  if (argv[1] != NULL)
    {
      int i;

      /* Scan for memory-size switches.  */
      for (i = 0; (argv[i] != NULL) && (argv[i][0] != 0); i++)
	if (argv[i][0] == '-' && argv[i][1] == 'm')
	  {
	    if (argv[i][2] != '\0')
	      mem_size = atoi (&argv[i][2]);
	    else if (argv[i + 1] != NULL)
	      {
		mem_size = atoi (argv[i + 1]);
		i++;
	      }
	    else
	      {
		sim_callback->printf_filtered (sim_callback,
					       "Missing argument to -m option\n");
		return NULL;
	      }
	      
	  }
    }

  return sd;
}

void
sim_close (SIM_DESC sd ATTRIBUTE_UNUSED,
	   int quitting ATTRIBUTE_UNUSED)
{
  /* Nothing to do.  */
}

void
sim_stop_reason (SIM_DESC sd ATTRIBUTE_UNUSED,
		 enum sim_stop *reason,
		 int *sigrc)
{
  if (stop_simulator)
    {
      *reason = sim_stopped;
      *sigrc = GDB_SIGNAL_INT;
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
	*sigrc = GDB_SIGNAL_TRAP;
      else if (   state->EndCondition == RDIError_DataAbort
	       || state->EndCondition == RDIError_AddressException)
	*sigrc = GDB_SIGNAL_BUS;
      else
	*sigrc = 0;
    }
}
