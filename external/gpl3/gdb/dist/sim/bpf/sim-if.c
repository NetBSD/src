/* Main simulator entry points specific to the eBPF.
   Copyright (C) 2020 Free Software Foundation, Inc.

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

#include <stdlib.h>

#include "sim-main.h"
#include "sim-options.h"
#include "libiberty.h"
#include "bfd.h"

/* Globals.  */

/* String with the name of the section containing the BPF program to
   run.  */
static char *bpf_program_section = NULL;

extern uint64_t skb_data_offset;


/* Handle BPF-specific options.  */

static SIM_RC bpf_option_handler (SIM_DESC, sim_cpu *, int, char *, int);

typedef enum
{
 OPTION_BPF_SET_PROGRAM = OPTION_START,
 OPTION_BPF_LIST_PROGRAMS,
 OPTION_BPF_VERIFY_PROGRAM,
 OPTION_BPF_SKB_DATA_OFFSET,
} BPF_OPTION;

static const OPTION bpf_options[] =
{
 { {"bpf-set-program", required_argument, NULL, OPTION_BPF_SET_PROGRAM},
   '\0', "SECTION_NAME", "Set the entry point",
   bpf_option_handler },
 { {"bpf-list-programs", no_argument, NULL, OPTION_BPF_LIST_PROGRAMS},
   '\0', "", "List loaded bpf programs",
   bpf_option_handler },
 { {"bpf-verify-program", required_argument, NULL, OPTION_BPF_VERIFY_PROGRAM},
   '\0', "PROGRAM", "Run the verifier on the given BPF program",
   bpf_option_handler },
 { {"skb-data-offset", required_argument, NULL, OPTION_BPF_SKB_DATA_OFFSET},
   '\0', "OFFSET", "Configure offsetof(struct sk_buff, data)",
   bpf_option_handler },

 { {NULL, no_argument, NULL, 0}, '\0', NULL, NULL, NULL, NULL }
};

static SIM_RC
bpf_option_handler (SIM_DESC sd, sim_cpu *cpu ATTRIBUTE_UNUSED, int opt,
                    char *arg, int is_command ATTRIBUTE_UNUSED)
{
  switch ((BPF_OPTION) opt)
    {
    case OPTION_BPF_VERIFY_PROGRAM:
      /* XXX call the verifier. */
      sim_io_printf (sd, "Verifying BPF program %s...\n", arg);
      break;

    case OPTION_BPF_LIST_PROGRAMS:
      /* XXX list programs.  */
      sim_io_printf (sd, "BPF programs available:\n");
      break;

    case OPTION_BPF_SET_PROGRAM:
      /* XXX: check that the section exists and tell the user about a
         new start_address.  */
      bpf_program_section = xstrdup (arg);
      break;

    case OPTION_BPF_SKB_DATA_OFFSET:
      skb_data_offset = strtoul (arg, NULL, 0);
      break;

    default:
      sim_io_eprintf (sd, "Unknown option `%s'\n", arg);
      return SIM_RC_FAIL;
    }

  return SIM_RC_OK;
}

/* Like sim_state_free, but free the cpu buffers as well.  */

static void
bpf_free_state (SIM_DESC sd)
{
  if (STATE_MODULES (sd) != NULL)
    sim_module_uninstall (sd);

  sim_cpu_free_all (sd);
  sim_state_free (sd);
}

/* Create an instance of the simulator.  */

SIM_DESC
sim_open (SIM_OPEN_KIND kind,
          host_callback *callback,
          struct bfd *abfd,
	  char * const *argv)
{
  /* XXX Analyze the program, and collect per-function information
     like the kernel verifier does.  The implementation of the CALL
     instruction will need that information, to update %fp.  */

  SIM_DESC sd = sim_state_alloc (kind, callback);

  if (sim_cpu_alloc_all (sd, 1, cgen_cpu_max_extra_bytes ())
      != SIM_RC_OK)
    goto error;

  if (sim_pre_argv_init (sd, argv[0]) != SIM_RC_OK)
    goto error;

  /* Add the BPF-specific option list to the simulator.  */
  if (sim_add_option_table (sd, NULL, bpf_options) != SIM_RC_OK)
    {
      bpf_free_state (sd);
      return 0;
    }

  if (sim_parse_args (sd, argv) != SIM_RC_OK)
    goto error;

  if (sim_analyze_program (sd,
                           (STATE_PROG_ARGV (sd) != NULL
                            ? *STATE_PROG_ARGV (sd)
                            : NULL), abfd) != SIM_RC_OK)
    goto error;

  if (sim_config (sd) != SIM_RC_OK)
    goto error;

  if (sim_post_argv_init (sd) != SIM_RC_OK)
    goto error;

  /* ... */

  /* Initialize the CPU descriptors and the disassemble in the cpu
     descriptor table entries.  */
  {
    int i;
    CGEN_CPU_DESC cd = bpf_cgen_cpu_open_1 (STATE_ARCHITECTURE (sd)->printable_name,
                                            CGEN_ENDIAN_LITTLE);

    /* We have one cpu per installed program! MAX_NR_PROCESSORS is an
       arbitrary upper limit.  XXX where is it defined?  */
    for (i = 0; i < MAX_NR_PROCESSORS; ++i)
      {
        SIM_CPU *cpu = STATE_CPU (sd, i);

        CPU_CPU_DESC (cpu) = cd;
        CPU_DISASSEMBLER (cpu) = sim_cgen_disassemble_insn;
      }

    bpf_cgen_init_dis (cd);
  }

  /* Initialize various cgen things not done by common framework.
     Must be done after bpf_cgen_cpu_open.  */
  cgen_init (sd);

  /* XXX do eBPF sim specific initializations.  */

  return sd;

 error:
      bpf_free_state (sd);
      return NULL;
}


SIM_RC
sim_create_inferior (SIM_DESC sd, struct bfd *abfd,
		     char *const *argv, char *const *envp)
{
  SIM_CPU *current_cpu = STATE_CPU (sd, 0);
  SIM_ADDR addr;

  /* Determine the start address.

     XXX acknowledge bpf_program_section.  If it is NULL, emit a
     warning explaining that we are using the ELF file start address,
     which often is not what is actually wanted.  */
  if (abfd != NULL)
    addr = bfd_get_start_address (abfd);
  else
    addr = 0;

  sim_pc_set (current_cpu, addr);

  if (STATE_PROG_ARGV (sd) != argv)
    {
      freeargv (STATE_PROG_ARGV (sd));
      STATE_PROG_ARGV (sd) = dupargv (argv);
    }

  return SIM_RC_OK;
}
