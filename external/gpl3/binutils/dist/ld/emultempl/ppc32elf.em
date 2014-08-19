# This shell script emits a C file. -*- C -*-
#   Copyright 2003, 2005, 2007, 2008, 2009, 2010, 2011, 2012
#   Free Software Foundation, Inc.
#
# This file is part of the GNU Binutils.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
# MA 02110-1301, USA.
#

# This file is sourced from elf32.em, and defines extra powerpc32-elf
# specific routines.
#
fragment <<EOF

#include "libbfd.h"
#include "elf32-ppc.h"
#include "ldlex.h"

#define is_ppc_elf(bfd) \
  (bfd_get_flavour (bfd) == bfd_target_elf_flavour \
   && elf_object_id (bfd) == PPC32_ELF_DATA)

/* Whether to run tls optimization.  */
static int notlsopt = 0;
static int no_tls_get_addr_opt = 0;

/* Whether to emit symbols for stubs.  */
static int emit_stub_syms = -1;

/* Chooses the correct place for .plt and .got.  */
static enum ppc_elf_plt_type plt_style = PLT_UNSET;
static int old_got = 0;

static void
ppc_after_open (void)
{
  if (is_ppc_elf (link_info.output_bfd))
    {
      int new_plt;
      int keep_new;
      unsigned int num_plt;
      unsigned int num_got;
      lang_output_section_statement_type *os;
      lang_output_section_statement_type *plt_os[2];
      lang_output_section_statement_type *got_os[2];

      if (emit_stub_syms < 0)
	emit_stub_syms = link_info.emitrelocations || link_info.shared;
      new_plt = ppc_elf_select_plt_layout (link_info.output_bfd, &link_info,
					   plt_style, emit_stub_syms);
      if (new_plt < 0)
	einfo ("%X%P: select_plt_layout problem %E\n");

      num_got = 0;
      num_plt = 0;
      for (os = &lang_output_section_statement.head->output_section_statement;
	   os != NULL;
	   os = os->next)
	{
	  if (os->constraint == SPECIAL && strcmp (os->name, ".plt") == 0)
	    {
	      if (num_plt < 2)
		plt_os[num_plt] = os;
	      ++num_plt;
	    }
	  if (os->constraint == SPECIAL && strcmp (os->name, ".got") == 0)
	    {
	      if (num_got < 2)
		got_os[num_got] = os;
	      ++num_got;
	    }
	}

      keep_new = new_plt == 1 ? 0 : -1;
      if (num_plt == 2)
	{
	  plt_os[0]->constraint = keep_new;
	  plt_os[1]->constraint = ~keep_new;
	}
      if (num_got == 2)
	{
	  if (old_got)
	    keep_new = -1;
	  got_os[0]->constraint = keep_new;
	  got_os[1]->constraint = ~keep_new;
	}
    }

  gld${EMULATION_NAME}_after_open ();
}

static void
ppc_before_allocation (void)
{
  if (is_ppc_elf (link_info.output_bfd))
    {
      if (ppc_elf_tls_setup (link_info.output_bfd, &link_info,
			     no_tls_get_addr_opt)
	  && !notlsopt)
	{
	  if (!ppc_elf_tls_optimize (link_info.output_bfd, &link_info))
	    {
	      einfo ("%X%P: TLS problem %E\n");
	      return;
	    }
	}
    }

  gld${EMULATION_NAME}_before_allocation ();

  /* Turn on relaxation if executable sections have addresses that
     might make branches overflow.  */
  if (RELAXATION_DISABLED_BY_DEFAULT)
    {
      bfd_vma low = (bfd_vma) -1;
      bfd_vma high = 0;
      asection *o;

      /* Run lang_size_sections (if not already done).  */
      if (expld.phase != lang_mark_phase_enum)
	{
	  expld.phase = lang_mark_phase_enum;
	  expld.dataseg.phase = exp_dataseg_none;
	  one_lang_size_sections_pass (NULL, FALSE);
	  lang_reset_memory_regions ();
	}

      for (o = link_info.output_bfd->sections; o != NULL; o = o->next)
	{
	  if ((o->flags & (SEC_ALLOC | SEC_CODE)) != (SEC_ALLOC | SEC_CODE))
	    continue;
	  if (o->rawsize == 0)
	    continue;
	  if (low > o->vma)
	    low = o->vma;
	  if (high < o->vma + o->rawsize - 1)
	    high = o->vma + o->rawsize - 1;
	}
      if (high > low && high - low > (1 << 25) - 1)
	ENABLE_RELAXATION;
    }
}

EOF

if grep -q 'ld_elf32_spu_emulation' ldemul-list.h; then
  fragment <<EOF
/* Special handling for embedded SPU executables.  */
extern bfd_boolean embedded_spu_file (lang_input_statement_type *, const char *);
static bfd_boolean gld${EMULATION_NAME}_load_symbols (lang_input_statement_type *);

static bfd_boolean
ppc_recognized_file (lang_input_statement_type *entry)
{
  if (embedded_spu_file (entry, "-m32"))
    return TRUE;

  return gld${EMULATION_NAME}_load_symbols (entry);
}

EOF
LDEMUL_RECOGNIZED_FILE=ppc_recognized_file
fi

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#
PARSE_AND_LIST_PROLOGUE=${PARSE_AND_LIST_PROLOGUE}'
#define OPTION_NO_TLS_OPT		321
#define OPTION_NO_TLS_GET_ADDR_OPT	(OPTION_NO_TLS_OPT + 1)
#define OPTION_NEW_PLT			(OPTION_NO_TLS_GET_ADDR_OPT + 1)
#define OPTION_OLD_PLT			(OPTION_NEW_PLT + 1)
#define OPTION_OLD_GOT			(OPTION_OLD_PLT + 1)
#define OPTION_STUBSYMS			(OPTION_OLD_GOT + 1)
#define OPTION_NO_STUBSYMS		(OPTION_STUBSYMS + 1)
'

PARSE_AND_LIST_LONGOPTS=${PARSE_AND_LIST_LONGOPTS}'
  { "emit-stub-syms", no_argument, NULL, OPTION_STUBSYMS },
  { "no-emit-stub-syms", no_argument, NULL, OPTION_NO_STUBSYMS },
  { "no-tls-optimize", no_argument, NULL, OPTION_NO_TLS_OPT },
  { "no-tls-get-addr-optimize", no_argument, NULL, OPTION_NO_TLS_GET_ADDR_OPT },
  { "secure-plt", no_argument, NULL, OPTION_NEW_PLT },
  { "bss-plt", no_argument, NULL, OPTION_OLD_PLT },
  { "sdata-got", no_argument, NULL, OPTION_OLD_GOT },
'

PARSE_AND_LIST_OPTIONS=${PARSE_AND_LIST_OPTIONS}'
  fprintf (file, _("\
  --emit-stub-syms            Label linker stubs with a symbol.\n\
  --no-emit-stub-syms         Don'\''t label linker stubs with a symbol.\n\
  --no-tls-optimize           Don'\''t try to optimize TLS accesses.\n\
  --no-tls-get-addr-optimize  Don'\''t use a special __tls_get_addr call.\n\
  --secure-plt                Use new-style PLT if possible.\n\
  --bss-plt                   Force old-style BSS PLT.\n\
  --sdata-got                 Force GOT location just before .sdata.\n"
		   ));
'

PARSE_AND_LIST_ARGS_CASES=${PARSE_AND_LIST_ARGS_CASES}'
    case OPTION_STUBSYMS:
      emit_stub_syms = 1;
      break;

    case OPTION_NO_STUBSYMS:
      emit_stub_syms = 0;
      break;

    case OPTION_NO_TLS_OPT:
      notlsopt = 1;
      break;

    case OPTION_NO_TLS_GET_ADDR_OPT:
      no_tls_get_addr_opt = 1;
      break;

    case OPTION_NEW_PLT:
      plt_style = PLT_NEW;
      break;

    case OPTION_OLD_PLT:
      plt_style = PLT_OLD;
      break;

    case OPTION_OLD_GOT:
      old_got = 1;
      break;

    case OPTION_TRADITIONAL_FORMAT:
      notlsopt = 1;
      no_tls_get_addr_opt = 1;
      return FALSE;
'

# Put these extra ppc32elf routines in ld_${EMULATION_NAME}_emulation
#
LDEMUL_AFTER_OPEN=ppc_after_open
LDEMUL_BEFORE_ALLOCATION=ppc_before_allocation
