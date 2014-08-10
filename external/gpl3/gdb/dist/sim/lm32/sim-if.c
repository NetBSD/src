/* Main simulator entry points specific to Lattice Mico32.
   Contributed by Jon Beniston <jon@beniston.com>
   
   Copyright (C) 2009-2014 Free Software Foundation, Inc.

   This file is part of GDB.

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

#include "sim-main.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

static void free_state (SIM_DESC);
static void print_lm32_misc_cpu (SIM_CPU * cpu, int verbose);
static DECLARE_OPTION_HANDLER (lm32_option_handler);

enum
{
  OPTION_ENDIAN = OPTION_START,
};

/* GDB passes -E, even though it's fixed, so we have to handle it here. common code only handles it if SIM_HAVE_BIENDIAN is defined, which it isn't for lm32.  */
static const OPTION lm32_options[] = {
  {{"endian", required_argument, NULL, OPTION_ENDIAN},
   'E', "big", "Set endianness",
   lm32_option_handler},
  {{NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL}
};

/* Records simulator descriptor so utilities like lm32_dump_regs can be
   called from gdb.  */
SIM_DESC current_state;

/* Cover function of sim_state_free to free the cpu buffers as well.  */

static void
free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);
  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Find memory range used by program.  */

static unsigned long
find_base (bfd *prog_bfd)
{
  int found;
  unsigned long base = ~(0UL);
  asection *s;

  found = 0;
  for (s = prog_bfd->sections; s; s = s->next)
    {
      if ((strcmp (bfd_get_section_name (prog_bfd, s), ".boot") == 0)
	  || (strcmp (bfd_get_section_name (prog_bfd, s), ".text") == 0)
	  || (strcmp (bfd_get_section_name (prog_bfd, s), ".data") == 0)
	  || (strcmp (bfd_get_section_name (prog_bfd, s), ".bss") == 0))
	{
	  if (!found)
	    {
	      base = bfd_get_section_vma (prog_bfd, s);
	      found = 1;
	    }
	  else
	    base =
	      bfd_get_section_vma (prog_bfd,
				   s) < base ? bfd_get_section_vma (prog_bfd,
								    s) : base;
	}
    }
  return base & ~(0xffffUL);
}

static unsigned long
find_limit (bfd *prog_bfd)
{
  struct bfd_symbol **asymbols;
  long symsize;
  long symbol_count;
  long s;

  symsize = bfd_get_symtab_upper_bound (prog_bfd);
  if (symsize < 0)
    return 0;
  asymbols = (asymbol **) xmalloc (symsize);
  symbol_count = bfd_canonicalize_symtab (prog_bfd, asymbols);
  if (symbol_count < 0)
    return 0;

  for (s = 0; s < symbol_count; s++)
    {
      if (!strcmp (asymbols[s]->name, "_fstack"))
	return (asymbols[s]->value + 65536) & ~(0xffffUL);
    }
  return 0;
}

/* Handle lm32 specific options.  */

static SIM_RC
lm32_option_handler (sd, cpu, opt, arg, is_command)
     SIM_DESC sd;
     sim_cpu *cpu;
     int opt;
     char *arg;
     int is_command;
{
  return SIM_RC_OK;
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (kind, callback, abfd, argv)
     SIM_OPEN_KIND kind;
     host_callback *callback;
     struct bfd *abfd;
     char **argv;
{
  SIM_DESC sd = sim_state_alloc (kind, callback);
  char c;
  int i;
  unsigned long base, limit;

  /* The cpu data is kept in a separately allocated chunk of memory.  */
  if (sim_cpu_alloc_all (sd, 1, cgen_cpu_max_extra_bytes ()) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }
  sim_add_option_table (sd, NULL, lm32_options);

  /* getopt will print the error message so we just have to exit if this fails.
     FIXME: Hmmm...  in the case of gdb we need getopt to call
     print_filtered.  */
  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

#if 0
  /* Allocate a handler for I/O devices
     if no memory for that range has been allocated by the user.
     All are allocated in one chunk to keep things from being
     unnecessarily complicated.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, LM32_DEVICE_ADDR, 1) == 0)
    sim_core_attach (sd, NULL, 0 /*level */ ,
		     access_read_write, 0 /*space ??? */ ,
		     LM32_DEVICE_ADDR, LM32_DEVICE_LEN /*nr_bytes */ ,
		     0 /*modulo */ ,
		     &lm32_devices, NULL /*buffer */ );
#endif

  /* check for/establish the reference program image.  */
  if (sim_analyze_program (sd,
			   (STATE_PROG_ARGV (sd) != NULL
			    ? *STATE_PROG_ARGV (sd)
			    : NULL), abfd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Check to see if memory exists at programs start address.  */
  if (sim_core_read_buffer (sd, NULL, read_map, &c, STATE_START_ADDR (sd), 1)
      == 0)
    {
      if (STATE_PROG_BFD (sd) != NULL)
	{
	  /* It doesn't, so we should try to allocate enough memory to hold program.  */
	  base = find_base (STATE_PROG_BFD (sd));
	  limit = find_limit (STATE_PROG_BFD (sd));
	  if (limit == 0)
	    {
	      sim_io_eprintf (sd,
			      "Failed to find symbol _fstack in program. You must specify memory regions with --memory-region.\n");
	      free_state (sd);
	      return 0;
	    }
	  /*sim_io_printf (sd, "Allocating memory at 0x%x size 0x%x\n", base, limit); */
	  sim_do_commandf (sd, "memory region 0x%x,0x%x", base, limit);
	}
    }

  /* Establish any remaining configuration options.  */
  if (sim_config (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    {
      free_state (sd);
      return 0;
    }

  /* Open a copy of the cpu descriptor table.  */
  {
    CGEN_CPU_DESC cd =
      lm32_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
			    CGEN_ENDIAN_BIG);
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
	SIM_CPU *cpu = STATE_CPU (sd, i);
	CPU_CPU_DESC (cpu) = cd;
	CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
      }
    lm32_cgen_init_dis (cd);
  }

  /* Initialize various cgen things not done by common framework.
     Must be done after lm32_cgen_cpu_open.  */
  cgen_init (sd);

  /* Store in a global so things like lm32_dump_regs can be invoked
     from the gdb command line.  */
  current_state = sd;

  return sd;
}

void
sim_close (sd, quitting)
     SIM_DESC sd;
     int quitting;
{
  lm32_cgen_cpu_close (CPU_CPU_DESC (STATE_CPU (sd, 0)));
  sim_module_uninstall (sd);
}

SIM_RC
sim_create_inferior (sd, abfd, argv, envp)
     SIM_DESC sd;
     struct bfd *abfd;
     char **argv;
     char **envp;
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  SIM_ADDR addr;

  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;
  sim_pc_set (current_cpu, addr);

#if 0
  STATE_ARGV (sd) = sim_copy_argv (argv);
  STATE_ENVP (sd) = sim_copy_argv (envp);
#endif

  return SIM_RC_OK;
}
