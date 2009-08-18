# This shell script emits a C file. -*- C -*-
#   Copyright 1991, 1993, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
#   2004, 2005, 2007, 2008
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

# This file is sourced from elf32.em, and defines extra arm-elf
# specific routines.
#
test -z "$TARGET2_TYPE" && TARGET2_TYPE="rel"
fragment <<EOF

#include "ldctor.h"
#include "elf/arm.h"

static char *thumb_entry_symbol = NULL;
static bfd *bfd_for_interwork;
static int byteswap_code = 0;
static int target1_is_rel = 0${TARGET1_IS_REL};
static char *target2_type = "${TARGET2_TYPE}";
static int fix_v4bx = 0;
static int use_blx = 0;
static bfd_arm_vfp11_fix vfp11_denorm_fix = BFD_ARM_VFP11_FIX_DEFAULT;
static int no_enum_size_warning = 0;
static int no_wchar_size_warning = 0;
static int pic_veneer = 0;

static void
gld${EMULATION_NAME}_before_parse (void)
{
#ifndef TARGET_			/* I.e., if not generic.  */
  ldfile_set_output_arch ("`echo ${ARCH}`", bfd_arch_unknown);
#endif /* not TARGET_ */
  config.dynamic_link = ${DYNAMIC_LINK-TRUE};
  config.has_shared = `if test -n "$GENERATE_SHLIB_SCRIPT" ; then echo TRUE ; else echo FALSE ; fi`;
}

static void
arm_elf_after_open (void)
{
  if (strstr (bfd_get_target (link_info.output_bfd), "arm") == NULL)
    {
      /* The arm backend needs special fields in the output hash structure.
	 These will only be created if the output format is an arm format,
	 hence we do not support linking and changing output formats at the
	 same time.  Use a link followed by objcopy to change output formats.  */
      einfo ("%F%X%P: error: cannot change output format whilst linking ARM binaries\n");
      return;
    }

  {
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
	bfd_elf32_arm_add_glue_sections_to_bfd (is->the_bfd, & link_info);
      }
  }

  /* Call the standard elf routine.  */
  gld${EMULATION_NAME}_after_open ();
}

static void
arm_elf_set_bfd_for_interworking (lang_statement_union_type *statement)
{
  if (statement->header.type == lang_input_section_enum)
    {
      asection *i = statement->input_section.section;

      if (!((lang_input_statement_type *) i->owner->usrdata)->just_syms_flag
	  && (i->flags & SEC_EXCLUDE) == 0)
	{
	  asection *output_section = i->output_section;

	  ASSERT (output_section->owner == link_info.output_bfd);

	  /* Don't attach the interworking stubs to a dynamic object, to
	     an empty section, etc.  */
	  if ((output_section->flags & SEC_HAS_CONTENTS) != 0
	      && (i->flags & SEC_NEVER_LOAD) == 0
	      && ! (i->owner->flags & DYNAMIC))
	    bfd_for_interwork = i->owner;
	}
    }
}

static void
arm_elf_before_allocation (void)
{
  if (link_info.input_bfds != NULL)
    {
      /* The interworking bfd must be the last one in the link.  */
      bfd_for_interwork = NULL;

      lang_for_each_statement (arm_elf_set_bfd_for_interworking);

      /* If bfd_for_interwork is NULL, then there are no loadable sections
	 with real contents to be linked, so we are not going to have to
	 create any interworking stubs, so it is OK not to call
	 bfd_elf32_arm_get_bfd_for_interworking.  */
      if (bfd_for_interwork != NULL)
	bfd_elf32_arm_get_bfd_for_interworking (bfd_for_interwork, &link_info);
    }

  bfd_elf32_arm_set_byteswap_code (&link_info, byteswap_code);

  /* Choose type of VFP11 erratum fix, or warn if specified fix is unnecessary
     due to architecture version.  */
  bfd_elf32_arm_set_vfp11_fix (link_info.output_bfd, &link_info);

  /* We should be able to set the size of the interworking stub section.  We
     can't do it until later if we have dynamic sections, though.  */
  if (! elf_hash_table (&link_info)->dynamic_sections_created)
    {
      /* Here we rummage through the found bfds to collect glue information.  */
      LANG_FOR_EACH_INPUT_STATEMENT (is)
	{
          /* Initialise mapping tables for code/data.  */
          bfd_elf32_arm_init_maps (is->the_bfd);

	  if (!bfd_elf32_arm_process_before_allocation (is->the_bfd,
							&link_info)
	      || !bfd_elf32_arm_vfp11_erratum_scan (is->the_bfd, &link_info))
	    /* xgettext:c-format */
	    einfo (_("Errors encountered processing file %s"), is->filename);
	}
    }

  /* Call the standard elf routine.  */
  gld${EMULATION_NAME}_before_allocation ();

  /* We have seen it all. Allocate it, and carry on.  */
  bfd_elf32_arm_allocate_interworking_sections (& link_info);
}

static void
arm_elf_after_allocation (void)
{
  /* Call the standard elf routine.  */
  after_allocation_default ();

  {
    LANG_FOR_EACH_INPUT_STATEMENT (is)
      {
        /* Figure out where VFP11 erratum veneers (and the labels returning
           from same) have been placed.  */
        bfd_elf32_arm_vfp11_fix_veneer_locations (is->the_bfd, &link_info);
      }
  }
}

/* Fake input file for stubs.  */
static lang_input_statement_type *stub_file;

/* Whether we need to call gldarm_layout_sections_again.  */
static int need_laying_out = 0;

/* Maximum size of a group of input sections that can be handled by
   one stub section.  A value of +/-1 indicates the bfd back-end
   should use a suitable default size.  */
static bfd_signed_vma group_size = 1;

struct hook_stub_info
{
  lang_statement_list_type add;
  asection *input_section;
};

/* Traverse the linker tree to find the spot where the stub goes.  */

static bfd_boolean
hook_in_stub (struct hook_stub_info *info, lang_statement_union_type **lp)
{
  lang_statement_union_type *l;
  bfd_boolean ret;

  for (; (l = *lp) != NULL; lp = &l->header.next)
    {
      switch (l->header.type)
	{
	case lang_constructors_statement_enum:
	  ret = hook_in_stub (info, &constructor_list.head);
	  if (ret)
	    return ret;
	  break;

	case lang_output_section_statement_enum:
	  ret = hook_in_stub (info,
			      &l->output_section_statement.children.head);
	  if (ret)
	    return ret;
	  break;

	case lang_wild_statement_enum:
	  ret = hook_in_stub (info, &l->wild_statement.children.head);
	  if (ret)
	    return ret;
	  break;

	case lang_group_statement_enum:
	  ret = hook_in_stub (info, &l->group_statement.children.head);
	  if (ret)
	    return ret;
	  break;

	case lang_input_section_enum:
	  if (l->input_section.section == info->input_section)
	    {
	      /* We've found our section.  Insert the stub immediately
		 before its associated input section.  */
	      *lp = info->add.head;
	      *(info->add.tail) = l;
	      return TRUE;
	    }
	  break;

	case lang_data_statement_enum:
	case lang_reloc_statement_enum:
	case lang_object_symbols_statement_enum:
	case lang_output_statement_enum:
	case lang_target_statement_enum:
	case lang_input_statement_enum:
	case lang_assignment_statement_enum:
	case lang_padding_statement_enum:
	case lang_address_statement_enum:
	case lang_fill_statement_enum:
	  break;

	default:
	  FAIL ();
	  break;
	}
    }
  return FALSE;
}


/* Call-back for elf32_arm_size_stubs.  */

/* Create a new stub section, and arrange for it to be linked
   immediately before INPUT_SECTION.  */

static asection *
elf32_arm_add_stub_section (const char *stub_sec_name,
			    asection *input_section)
{
  asection *stub_sec;
  flagword flags;
  asection *output_section;
  const char *secname;
  lang_output_section_statement_type *os;
  struct hook_stub_info info;

  flags = (SEC_ALLOC | SEC_LOAD | SEC_READONLY | SEC_CODE
	   | SEC_HAS_CONTENTS | SEC_RELOC | SEC_IN_MEMORY | SEC_KEEP);
  stub_sec = bfd_make_section_anyway_with_flags (stub_file->the_bfd,
						 stub_sec_name, flags);
  if (stub_sec == NULL)
    goto err_ret;

  bfd_set_section_alignment (stub_file->the_bfd, stub_sec, 3);

  output_section = input_section->output_section;
  secname = bfd_get_section_name (output_section->owner, output_section);
  os = lang_output_section_find (secname);

  info.input_section = input_section;
  lang_list_init (&info.add);
  lang_add_section (&info.add, stub_sec, os);

  if (info.add.head == NULL)
    goto err_ret;

  if (hook_in_stub (&info, &os->children.head))
    return stub_sec;

 err_ret:
  einfo ("%X%P: can not make stub section: %E\n");
  return NULL;
}

/* Another call-back for elf_arm_size_stubs.  */

static void
gldarm_layout_sections_again (void)
{
  /* If we have changed sizes of the stub sections, then we need
     to recalculate all the section offsets.  This may mean we need to
     add even more stubs.  */
  gld${EMULATION_NAME}_map_segments (TRUE);
  need_laying_out = -1;
}

static void
build_section_lists (lang_statement_union_type *statement)
{
  if (statement->header.type == lang_input_section_enum)
    {
      asection *i = statement->input_section.section;

      if (!((lang_input_statement_type *) i->owner->usrdata)->just_syms_flag
	  && (i->flags & SEC_EXCLUDE) == 0
	  && i->output_section != NULL
	  && i->output_section->owner == link_info.output_bfd)
	elf32_arm_next_input_section (& link_info, i);
    }
}

static void
gld${EMULATION_NAME}_finish (void)
{
  struct bfd_link_hash_entry * h;

  /* bfd_elf32_discard_info just plays with debugging sections,
     ie. doesn't affect any code, so we can delay resizing the
     sections.  It's likely we'll resize everything in the process of
     adding stubs.  */
  if (bfd_elf_discard_info (link_info.output_bfd, & link_info))
    need_laying_out = 1;

  /* If generating a relocatable output file, then we don't
     have to examine the relocs.  */
  if (stub_file != NULL && !link_info.relocatable)
    {
      int  ret = elf32_arm_setup_section_lists (link_info.output_bfd, & link_info);

      if (ret != 0)
	{
	  if (ret < 0)
	    {
	      einfo ("%X%P: could not compute sections lists for stub generation: %E\n");
	      return;
	    }

	  lang_for_each_statement (build_section_lists);

	  /* Call into the BFD backend to do the real work.  */
	  if (! elf32_arm_size_stubs (link_info.output_bfd,
				      stub_file->the_bfd,
				      & link_info,
				      group_size,
				      & elf32_arm_add_stub_section,
				      & gldarm_layout_sections_again))
	    {
	      einfo ("%X%P: cannot size stub section: %E\n");
	      return;
	    }
	}
    }

  if (need_laying_out != -1)
    gld${EMULATION_NAME}_map_segments (need_laying_out);

  if (! link_info.relocatable)
    {
      /* Now build the linker stubs.  */
      if (stub_file->the_bfd->sections != NULL)
	{
	  if (! elf32_arm_build_stubs (& link_info))
	    einfo ("%X%P: can not build stubs: %E\n");
	}
    }

  finish_default ();

  if (thumb_entry_symbol)
    {
      h = bfd_link_hash_lookup (link_info.hash, thumb_entry_symbol,
				FALSE, FALSE, TRUE);
    }
  else
    {
      struct elf_link_hash_entry * eh;

      if (!entry_symbol.name)
	return;

      h = bfd_link_hash_lookup (link_info.hash, entry_symbol.name,
				FALSE, FALSE, TRUE);
      eh = (struct elf_link_hash_entry *)h;
      if (!h || ELF_ST_TYPE(eh->type) != STT_ARM_TFUNC)
	return;
    }


  if (h != (struct bfd_link_hash_entry *) NULL
      && (h->type == bfd_link_hash_defined
	  || h->type == bfd_link_hash_defweak)
      && h->u.def.section->output_section != NULL)
    {
      static char buffer[32];
      bfd_vma val;

      /* Special procesing is required for a Thumb entry symbol.  The
	 bottom bit of its address must be set.  */
      val = (h->u.def.value
	     + bfd_get_section_vma (link_info.output_bfd,
				    h->u.def.section->output_section)
	     + h->u.def.section->output_offset);

      val |= 1;

      /* Now convert this value into a string and store it in entry_symbol
	 where the lang_finish() function will pick it up.  */
      buffer[0] = '0';
      buffer[1] = 'x';

      sprintf_vma (buffer + 2, val);

      if (thumb_entry_symbol != NULL && entry_symbol.name != NULL
	  && entry_from_cmdline)
	einfo (_("%P: warning: '--thumb-entry %s' is overriding '-e %s'\n"),
	       thumb_entry_symbol, entry_symbol.name);
      entry_symbol.name = buffer;
    }
  else
    einfo (_("%P: warning: cannot find thumb start symbol %s\n"),
	   thumb_entry_symbol);
}

/* This is a convenient point to tell BFD about target specific flags.
   After the output has been created, but before inputs are read.  */
static void
arm_elf_create_output_section_statements (void)
{
  bfd_elf32_arm_set_target_relocs (link_info.output_bfd, &link_info,
				   target1_is_rel,
				   target2_type, fix_v4bx, use_blx,
				   vfp11_denorm_fix, no_enum_size_warning,
				   no_wchar_size_warning,
				   pic_veneer);

  stub_file = lang_add_input_file ("linker stubs",
 				   lang_input_file_is_fake_enum,
 				   NULL);
  stub_file->the_bfd = bfd_create ("linker stubs", link_info.output_bfd);
  if (stub_file->the_bfd == NULL
      || ! bfd_set_arch_mach (stub_file->the_bfd,
 			      bfd_get_arch (link_info.output_bfd),
 			      bfd_get_mach (link_info.output_bfd)))
    {
      einfo ("%X%P: can not create BFD %E\n");
      return;
    }
 
  stub_file->the_bfd->flags |= BFD_LINKER_CREATED;
  ldlang_add_file (stub_file);
}

/* Avoid processing the fake stub_file in vercheck, stat_needed and
   check_needed routines.  */

static void (*real_func) (lang_input_statement_type *);

static void arm_for_each_input_file_wrapper (lang_input_statement_type *l)
{
  if (l != stub_file)
    (*real_func) (l);
}

static void
arm_lang_for_each_input_file (void (*func) (lang_input_statement_type *))
{
  real_func = func;
  lang_for_each_input_file (&arm_for_each_input_file_wrapper);
}

#define lang_for_each_input_file arm_lang_for_each_input_file

EOF

# Define some shell vars to insert bits of code into the standard elf
# parse_args and list_options functions.
#
PARSE_AND_LIST_PROLOGUE='
#define OPTION_THUMB_ENTRY		301
#define OPTION_BE8			302
#define OPTION_TARGET1_REL		303
#define OPTION_TARGET1_ABS		304
#define OPTION_TARGET2			305
#define OPTION_FIX_V4BX			306
#define OPTION_USE_BLX			307
#define OPTION_VFP11_DENORM_FIX		308
#define OPTION_NO_ENUM_SIZE_WARNING	309
#define OPTION_PIC_VENEER		310
#define OPTION_FIX_V4BX_INTERWORKING	311
#define OPTION_STUBGROUP_SIZE           312
#define OPTION_NO_WCHAR_SIZE_WARNING	313
'

PARSE_AND_LIST_SHORTOPTS=p

PARSE_AND_LIST_LONGOPTS='
  { "no-pipeline-knowledge", no_argument, NULL, '\'p\''},
  { "thumb-entry", required_argument, NULL, OPTION_THUMB_ENTRY},
  { "be8", no_argument, NULL, OPTION_BE8},
  { "target1-rel", no_argument, NULL, OPTION_TARGET1_REL},
  { "target1-abs", no_argument, NULL, OPTION_TARGET1_ABS},
  { "target2", required_argument, NULL, OPTION_TARGET2},
  { "fix-v4bx", no_argument, NULL, OPTION_FIX_V4BX},
  { "fix-v4bx-interworking", no_argument, NULL, OPTION_FIX_V4BX_INTERWORKING},
  { "use-blx", no_argument, NULL, OPTION_USE_BLX},
  { "vfp11-denorm-fix", required_argument, NULL, OPTION_VFP11_DENORM_FIX},
  { "no-enum-size-warning", no_argument, NULL, OPTION_NO_ENUM_SIZE_WARNING},
  { "pic-veneer", no_argument, NULL, OPTION_PIC_VENEER},
  { "stub-group-size", required_argument, NULL, OPTION_STUBGROUP_SIZE },
  { "no-wchar-size-warning", no_argument, NULL, OPTION_NO_WCHAR_SIZE_WARNING},
'

PARSE_AND_LIST_OPTIONS='
  fprintf (file, _("  --thumb-entry=<sym>         Set the entry point to be Thumb symbol <sym>\n"));
  fprintf (file, _("  --be8                       Oputput BE8 format image\n"));
  fprintf (file, _("  --target1=rel               Interpret R_ARM_TARGET1 as R_ARM_REL32\n"));
  fprintf (file, _("  --target1=abs               Interpret R_ARM_TARGET1 as R_ARM_ABS32\n"));
  fprintf (file, _("  --target2=<type>            Specify definition of R_ARM_TARGET2\n"));
  fprintf (file, _("  --fix-v4bx                  Rewrite BX rn as MOV pc, rn for ARMv4\n"));
  fprintf (file, _("  --fix-v4bx-interworking     Rewrite BX rn branch to ARMv4 interworking veneer\n"));
  fprintf (file, _("  --use-blx                   Enable use of BLX instructions\n"));
  fprintf (file, _("  --vfp11-denorm-fix          Specify how to fix VFP11 denorm erratum\n"));
  fprintf (file, _("  --no-enum-size-warning      Don'\''t warn about objects with incompatible"
		   "                                enum sizes\n"));
  fprintf (file, _("  --no-wchar-size-warning     Don'\''t warn about objects with incompatible"
		   "                                wchar_t sizes\n"));
  fprintf (file, _("  --pic-veneer                Always generate PIC interworking veneers\n"));
  fprintf (file, _("\
   --stub-group-size=N   Maximum size of a group of input sections that can be\n\
                           handled by one stub section.  A negative value\n\
                           locates all stubs before their branches (with a\n\
                           group size of -N), while a positive value allows\n\
                           two groups of input sections, one before, and one\n\
                           after each stub section.  Values of +/-1 indicate\n\
                           the linker should choose suitable defaults.\n"
 		   ));
'

PARSE_AND_LIST_ARGS_CASES='
    case '\'p\'':
      /* Only here for backwards compatibility.  */
      break;

    case OPTION_THUMB_ENTRY:
      thumb_entry_symbol = optarg;
      break;

    case OPTION_BE8:
      byteswap_code = 1;
      break;

    case OPTION_TARGET1_REL:
      target1_is_rel = 1;
      break;

    case OPTION_TARGET1_ABS:
      target1_is_rel = 0;
      break;

    case OPTION_TARGET2:
      target2_type = optarg;
      break;

    case OPTION_FIX_V4BX:
      fix_v4bx = 1;
      break;

    case OPTION_FIX_V4BX_INTERWORKING:
      fix_v4bx = 2;
      break;

    case OPTION_USE_BLX:
      use_blx = 1;
      break;

    case OPTION_VFP11_DENORM_FIX:
      if (strcmp (optarg, "none") == 0)
        vfp11_denorm_fix = BFD_ARM_VFP11_FIX_NONE;
      else if (strcmp (optarg, "scalar") == 0)
        vfp11_denorm_fix = BFD_ARM_VFP11_FIX_SCALAR;
      else if (strcmp (optarg, "vector") == 0)
        vfp11_denorm_fix = BFD_ARM_VFP11_FIX_VECTOR;
      else
        einfo (_("Unrecognized VFP11 fix type '\''%s'\''.\n"), optarg);
      break;

    case OPTION_NO_ENUM_SIZE_WARNING:
      no_enum_size_warning = 1;
      break;

    case OPTION_NO_WCHAR_SIZE_WARNING:
      no_wchar_size_warning = 1;
      break;

    case OPTION_PIC_VENEER:
      pic_veneer = 1;
      break;

    case OPTION_STUBGROUP_SIZE:
      {
	const char *end;

        group_size = bfd_scan_vma (optarg, &end, 0);
        if (*end)
	  einfo (_("%P%F: invalid number `%s'\''\n"), optarg);
      }
      break;
'

# We have our own after_open and before_allocation functions, but they call
# the standard routines, so give them a different name.
LDEMUL_AFTER_OPEN=arm_elf_after_open
LDEMUL_BEFORE_ALLOCATION=arm_elf_before_allocation
LDEMUL_AFTER_ALLOCATION=arm_elf_after_allocation
LDEMUL_CREATE_OUTPUT_SECTION_STATEMENTS=arm_elf_create_output_section_statements

# Replace the elf before_parse function with our own.
LDEMUL_BEFORE_PARSE=gld"${EMULATION_NAME}"_before_parse

# Call the extra arm-elf function
LDEMUL_FINISH=gld${EMULATION_NAME}_finish
