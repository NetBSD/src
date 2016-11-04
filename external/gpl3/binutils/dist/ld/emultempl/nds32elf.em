# This shell script emits a C file. -*- C -*-
# Copyright (C) 2012-2016 Free Software Foundation, Inc.
# Contributed by Andes Technology Corporation.
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

fragment <<EOF

#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/nds32.h"
#include "bfd_stdint.h"
#include "elf32-nds32.h"

static int relax_fp_as_gp = 1;		/* --mrelax-omit-fp  */
static int eliminate_gc_relocs = 0;	/* --meliminate-gc-relocs  */
static FILE *sym_ld_script = NULL;	/* --mgen-symbol-ld-script=<file>  */
/* Disable if linking a dynamically linked executable.  */
static int load_store_relax = 1;
static int target_optimize = 0;	/* Switch optimization.  */
static int relax_status = 0;	/* Finished optimization.  */
static int relax_round = 0;		/* Going optimization.  */
static FILE *ex9_export_file = NULL;	/* --mexport-ex9=<file>  */
static FILE *ex9_import_file = NULL;	/* --mimport-ex9=<file>  */
static int update_ex9_table = 0;	/* --mupdate-ex9.  */
static int ex9_limit = 511;
static bfd_boolean ex9_loop_aware = FALSE;	/* Ignore ex9 if inside a loop.  */
static bfd_boolean ifc_loop_aware = FALSE;	/* Ignore ifc if inside a loop.  */

/* Save the target options into output bfd to avoid using to many global
   variables. Do this after the output has been created, but before
   inputs are read.  */
static void
nds32_elf_create_output_section_statements (void)
{
  if (strstr (bfd_get_target (link_info.output_bfd), "nds32") == NULL)
    {
      /* Check the output target is nds32.  */
      einfo ("%F%X%P: error: Cannot change output format whilst "
	     "linking NDS32 binaries.\n");
      return;
    }

  bfd_elf32_nds32_set_target_option (&link_info, relax_fp_as_gp,
				     eliminate_gc_relocs,
				     sym_ld_script,
				     load_store_relax,
				     target_optimize, relax_status, relax_round,
				     ex9_export_file, ex9_import_file,
				     update_ex9_table, ex9_limit,
				     ex9_loop_aware, ifc_loop_aware);
}

static void
nds32_elf_after_parse (void)
{
  if (bfd_link_relocatable (&link_info))
    DISABLE_RELAXATION;

  if (!RELAXATION_ENABLED)
    {
      target_optimize = target_optimize & (!NDS32_RELAX_JUMP_IFC_ON);
      target_optimize = target_optimize & (!NDS32_RELAX_EX9_ON);
      relax_fp_as_gp = 0;
    }

  if (ex9_import_file != NULL)
    {
      ex9_export_file = NULL;
      target_optimize = target_optimize & (!NDS32_RELAX_EX9_ON);
    }
  else
    update_ex9_table = 0;

  if (bfd_link_pic (&link_info))
    {
      target_optimize = target_optimize & (!NDS32_RELAX_JUMP_IFC_ON);
      target_optimize = target_optimize & (!NDS32_RELAX_EX9_ON);
    }

  gld${EMULATION_NAME}_after_parse ();
}

static void
nds32_elf_after_open (void)
{
  unsigned int arch_ver = (unsigned int)-1;
  unsigned int abi_ver = (unsigned int)-1;
  bfd *abfd;

  /* For now, make sure all object files are of the same architecture.
     We may try to merge object files with different architecture together.  */
  for (abfd = link_info.input_bfds; abfd != NULL; abfd = abfd->link.next)
    {
      if (arch_ver == (unsigned int)-1 && E_N1_ARCH != (elf_elfheader (abfd)->e_flags & EF_NDS_ARCH))
	arch_ver = elf_elfheader (abfd)->e_flags & EF_NDS_ARCH ;

      if (abi_ver == (unsigned int)-1)
	{
	  /* Initialize ABI version, if not ABI0.
	     (OS uses empty file to create empty ELF with ABI0).  */
	  if ((elf_elfheader (abfd)->e_flags & EF_NDS_ABI) != 0)
	    abi_ver = elf_elfheader (abfd)->e_flags & EF_NDS_ABI ;
	}
      else if ((elf_elfheader (abfd)->e_flags & EF_NDS_ABI) != 0
	       && abi_ver != (elf_elfheader (abfd)->e_flags & EF_NDS_ABI))
	{
	  /* Incompatible objects.  */
	  einfo (_("%F%B: ABI version of object files mismatched\n"), abfd);
	}

#if defined NDS32_EX9_EXT
      /* Append .ex9.itable section in the last input object file.  */
      if (abfd->link_next == NULL && (target_optimize & NDS32_RELAX_EX9_ON))
	{
	  asection *itable;
	  struct bfd_link_hash_entry *h;
	  itable = bfd_make_section_with_flags (abfd, ".ex9.itable",
						SEC_CODE | SEC_ALLOC | SEC_LOAD
						| SEC_HAS_CONTENTS | SEC_READONLY
						| SEC_IN_MEMORY | SEC_KEEP);
	  if (itable)
	    {
	      itable->gc_mark = 1;
	      itable->alignment_power = 2;
	      itable->size = 0x1000;
	      itable->contents = bfd_zalloc (abfd, itable->size);

	      /* Add a symbol in the head of ex9.itable to objdump clearly.  */
	      h = bfd_link_hash_lookup (link_info.hash, "_EX9_BASE_",
					FALSE, FALSE, FALSE);
	      _bfd_generic_link_add_one_symbol
		(&link_info, link_info.output_bfd, "_EX9_BASE_",
		 BSF_GLOBAL | BSF_WEAK, itable, 0, (const char *) NULL, FALSE,
		 get_elf_backend_data (link_info.output_bfd)->collect, &h);
	    }
	}
#endif
    }

  /* Check object files if the target is dynamic linked executable
     or shared object.  */
  if (elf_hash_table (&link_info)->dynamic_sections_created
      || bfd_link_pic (&link_info))
    {
      for (abfd = link_info.input_bfds; abfd != NULL; abfd = abfd->link.next)
	{
	  if (!(elf_elfheader (abfd)->e_flags & E_NDS32_HAS_PIC))
	    {
	      /* Non-PIC object file is used.  */
	      if (bfd_link_pic (&link_info))
		{
		  /* For PIE or shared object, all input must be PIC.  */
		  einfo (_("%B: must use -fpic to compile this file "
			   "for shared object or PIE\n"), abfd);
		}
	      else
		{
		  /* Dynamic linked executable with SDA and non-PIC.
		     Turn off load/store relaxtion.  */
		  /* TODO: This may support in the future.  */
		  load_store_relax = 0 ;
		  relax_fp_as_gp = 0;
		}
	    }
	}
      /* Turn off relax when building shared object or PIE
	 until we can support their relaxation.  */
    }

  /* Call the standard elf routine.  */
  gld${EMULATION_NAME}_after_open ();
}

static void
nds32_elf_after_allocation (void)
{
  if (target_optimize & NDS32_RELAX_EX9_ON
      || (ex9_import_file != NULL && update_ex9_table == 1))
    {
      /* Initialize ex9 hash table.  */
      if (!nds32_elf_ex9_init ())
        return;
    }

  /* Call default after allocation callback.
     1. This is where relaxation is done.
     2. It calls gld${EMULATION_NAME}_map_segments to build ELF segment table.
     3. Any relaxation requires relax being done must be called after it.  */
  gld${EMULATION_NAME}_after_allocation ();
}

EOF
# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#
PARSE_AND_LIST_PROLOGUE='
#define OPTION_BASELINE			301
#define OPTION_ELIM_GC_RELOCS		(OPTION_BASELINE + 1)
#define OPTION_FP_AS_GP			(OPTION_BASELINE + 2)
#define OPTION_NO_FP_AS_GP		(OPTION_BASELINE + 3)
#define OPTION_REDUCE_FP_UPDATE		(OPTION_BASELINE + 4)
#define OPTION_NO_REDUCE_FP_UPDATE	(OPTION_BASELINE + 5)
#define OPTION_EXPORT_SYMBOLS		(OPTION_BASELINE + 6)

/* These are only available to ex9.  */
#if defined NDS32_EX9_EXT
#define OPTION_EX9_BASELINE		320
#define OPTION_EX9_TABLE		(OPTION_EX9_BASELINE + 1)
#define OPTION_NO_EX9_TABLE		(OPTION_EX9_BASELINE + 2)
#define OPTION_EXPORT_EX9		(OPTION_EX9_BASELINE + 3)
#define OPTION_IMPORT_EX9		(OPTION_EX9_BASELINE + 4)
#define OPTION_UPDATE_EX9		(OPTION_EX9_BASELINE + 5)
#define OPTION_EX9_LIMIT		(OPTION_EX9_BASELINE + 6)
#define OPTION_EX9_LOOP			(OPTION_EX9_BASELINE + 7)
#endif

/* These are only available to link-time ifc.  */
#if defined NDS32_IFC_EXT
#define OPTION_IFC_BASELINE		340
#define OPTION_JUMP_IFC			(OPTION_IFC_BASELINE + 1)
#define OPTION_NO_JUMP_IFC		(OPTION_IFC_BASELINE + 2)
#define OPTION_IFC_LOOP			(OPTION_IFC_BASELINE + 3)
#endif
'
PARSE_AND_LIST_LONGOPTS='
  { "mfp-as-gp", no_argument, NULL, OPTION_FP_AS_GP},
  { "mno-fp-as-gp", no_argument, NULL, OPTION_NO_FP_AS_GP},
  { "mexport-symbols", required_argument, NULL, OPTION_EXPORT_SYMBOLS},
  /* These are deprecated options.  Remove them in the future.  */
  { "mrelax-reduce-fp-update", no_argument, NULL, OPTION_REDUCE_FP_UPDATE},
  { "mrelax-no-reduce-fp-update", no_argument, NULL, OPTION_NO_REDUCE_FP_UPDATE},
  { "mbaseline", required_argument, NULL, OPTION_BASELINE},
  { "meliminate-gc-relocs", no_argument, NULL, OPTION_ELIM_GC_RELOCS},
  { "mrelax-omit-fp", no_argument, NULL, OPTION_FP_AS_GP},
  { "mrelax-no-omit-fp", no_argument, NULL, OPTION_NO_FP_AS_GP},
  { "mgen-symbol-ld-script", required_argument, NULL, OPTION_EXPORT_SYMBOLS},
  /* These are specific optioins for ex9-ext support.  */
#if defined NDS32_EX9_EXT
  { "mex9", no_argument, NULL, OPTION_EX9_TABLE},
  { "mno-ex9", no_argument, NULL, OPTION_NO_EX9_TABLE},
  { "mexport-ex9", required_argument, NULL, OPTION_EXPORT_EX9},
  { "mimport-ex9", required_argument, NULL, OPTION_IMPORT_EX9},
  { "mupdate-ex9", no_argument, NULL, OPTION_UPDATE_EX9},
  { "mex9-limit", required_argument, NULL, OPTION_EX9_LIMIT},
  { "mex9-loop-aware", no_argument, NULL, OPTION_EX9_LOOP},
#endif
  /* These are specific optioins for ifc-ext support.  */
#if defined NDS32_IFC_EXT
  { "mifc", no_argument, NULL, OPTION_JUMP_IFC},
  { "mno-ifc", no_argument, NULL, OPTION_NO_JUMP_IFC},
  { "mifc-loop-aware", no_argument, NULL, OPTION_IFC_LOOP},
#endif
'
PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("\
  --m[no-]fp-as-gp            Disable/enable fp-as-gp relaxation\n\
  --mexport-symbols=FILE      Exporting symbols in linker script\n\
"));

#if defined NDS32_EX9_EXT
  fprintf (file, _("\
  --m[no-]ex9                 Disable/enable link-time EX9 relaxation\n\
  --mexport-ex9=FILE          Export EX9 table after linking\n\
  --mimport-ex9=FILE          Import Ex9 table for EX9 relaxation\n\
  --mupdate-ex9               Update existing EX9 table\n\
  --mex9-limit=NUM            Maximum number of entries in ex9 table\n\
  --mex9-loop-aware           Avoid generate EX9 instruction inside loop\n\
"));
#endif

#if defined NDS32_IFC_EXT
  fprintf (file, _("\
  --m[no-]ifc                 Disable/enable link-time IFC optimization\n\
  --mifc-loop-aware           Avoid generate IFC instruction inside loop\n\
"));
#endif
'
PARSE_AND_LIST_ARGS_CASES='
  case OPTION_BASELINE:
    einfo ("%P: --mbaseline is not used anymore.\n");
    break;
  case OPTION_ELIM_GC_RELOCS:
    eliminate_gc_relocs = 1;
    break;
  case OPTION_FP_AS_GP:
  case OPTION_NO_FP_AS_GP:
    relax_fp_as_gp = (optc == OPTION_FP_AS_GP);
    break;
  case OPTION_REDUCE_FP_UPDATE:
  case OPTION_NO_REDUCE_FP_UPDATE:
    einfo ("%P: --relax-[no-]reduce-fp-updat is not used anymore.\n");
    break;
  case OPTION_EXPORT_SYMBOLS:
    if (!optarg)
      einfo (_("Missing file for --mexport-symbols.\n"), optarg);

    if(strcmp (optarg, "-") == 0)
      sym_ld_script = stdout;
    else
      {
	sym_ld_script = fopen (optarg, FOPEN_WT);
	if(sym_ld_script == NULL)
	  einfo (_("%P%F: cannot open map file %s: %E.\n"), optarg);
      }
    break;
#if defined NDS32_EX9_EXT
  case OPTION_EX9_TABLE:
    target_optimize = target_optimize | NDS32_RELAX_EX9_ON;
    break;
  case OPTION_NO_EX9_TABLE:
    target_optimize = target_optimize & (!NDS32_RELAX_EX9_ON);
    break;
  case OPTION_EXPORT_EX9:
    if (!optarg)
      einfo (_("Missing file for --mexport-ex9=<file>.\n"));

    if(strcmp (optarg, "-") == 0)
      ex9_export_file = stdout;
    else
      {
	ex9_export_file = fopen (optarg, "wb");
	if(ex9_export_file == NULL)
	  einfo (_("ERROR %P%F: cannot open ex9 export file %s.\n"), optarg);
      }
    break;
  case OPTION_IMPORT_EX9:
    if (!optarg)
      einfo (_("Missing file for --mimport-ex9=<file>.\n"));

    ex9_import_file = fopen (optarg, "rb+");
    if(ex9_import_file == NULL)
      einfo (_("ERROR %P%F: cannot open ex9 import file %s.\n"), optarg);
    break;
  case OPTION_UPDATE_EX9:
    update_ex9_table = 1;
    break;
  case OPTION_EX9_LIMIT:
    if (optarg)
      {
	ex9_limit = atoi (optarg);
	if (ex9_limit > 511 || ex9_limit < 1)
	  {
	    einfo (_("ERROR: the range of ex9_limit must between 1 and 511\n"));
	    exit (1);
	  }
      }
    break;
  case OPTION_EX9_LOOP:
    target_optimize = target_optimize | NDS32_RELAX_EX9_ON;
    ex9_loop_aware = 1;
    break;
#endif
#if defined NDS32_IFC_EXT
  case OPTION_JUMP_IFC:
    target_optimize = target_optimize | NDS32_RELAX_JUMP_IFC_ON;
    break;
  case OPTION_NO_JUMP_IFC:
    target_optimize = target_optimize & (!NDS32_RELAX_JUMP_IFC_ON);
    break;
  case OPTION_IFC_LOOP:
    target_optimize = target_optimize | NDS32_RELAX_JUMP_IFC_ON;
    ifc_loop_aware = 1;
    break;
#endif
'
LDEMUL_AFTER_OPEN=nds32_elf_after_open
LDEMUL_AFTER_PARSE=nds32_elf_after_parse
LDEMUL_AFTER_ALLOCATION=nds32_elf_after_allocation
LDEMUL_CREATE_OUTPUT_SECTION_STATEMENTS=nds32_elf_create_output_section_statements
