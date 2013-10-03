/* Mach-O support for BFD.
   Copyright 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
   2009, 2010, 2011, 2012
   Free Software Foundation, Inc.

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#include "sysdep.h"
#include "mach-o.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"
#include "aout/stab_gnu.h"
#include "mach-o/reloc.h"
#include "mach-o/external.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define bfd_mach_o_object_p bfd_mach_o_gen_object_p
#define bfd_mach_o_core_p bfd_mach_o_gen_core_p
#define bfd_mach_o_mkobject bfd_mach_o_gen_mkobject

#define FILE_ALIGN(off, algn) \
  (((off) + ((file_ptr) 1 << (algn)) - 1) & ((file_ptr) -1 << (algn)))

unsigned int
bfd_mach_o_version (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = NULL;

  BFD_ASSERT (bfd_mach_o_valid (abfd));
  mdata = bfd_mach_o_get_data (abfd);

  return mdata->header.version;
}

bfd_boolean
bfd_mach_o_valid (bfd *abfd)
{
  if (abfd == NULL || abfd->xvec == NULL)
    return FALSE;

  if (abfd->xvec->flavour != bfd_target_mach_o_flavour)
    return FALSE;

  if (bfd_mach_o_get_data (abfd) == NULL)
    return FALSE;
  return TRUE;
}

static INLINE bfd_boolean
mach_o_wide_p (bfd_mach_o_header *header)
{
  switch (header->version)
    {
    case 1:
      return FALSE;
    case 2:
      return TRUE;
    default:
      BFD_FAIL ();
      return FALSE;
    }
}

static INLINE bfd_boolean
bfd_mach_o_wide_p (bfd *abfd)
{
  return mach_o_wide_p (&bfd_mach_o_get_data (abfd)->header);
}

/* Tables to translate well known Mach-O segment/section names to bfd
   names.  Use of canonical names (such as .text or .debug_frame) is required
   by gdb.  */

/* __TEXT Segment.  */
static const mach_o_section_name_xlat text_section_names_xlat[] =
  {
    {	".text",				"__text",
	SEC_CODE | SEC_LOAD,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS,	0},
    {	".const",				"__const",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,			0},
    {	".static_const",			"__static_const",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,			0},
    {	".cstring",				"__cstring",
	SEC_READONLY | SEC_DATA | SEC_LOAD | SEC_MERGE | SEC_STRINGS,
						BFD_MACH_O_S_CSTRING_LITERALS,
	BFD_MACH_O_S_ATTR_NONE,			0},
    {	".literal4",				"__literal4",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_4BYTE_LITERALS,
	BFD_MACH_O_S_ATTR_NONE,			2},
    {	".literal8",				"__literal8",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_8BYTE_LITERALS,
	BFD_MACH_O_S_ATTR_NONE,			3},
    {	".literal16",				"__literal16",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_16BYTE_LITERALS,
	BFD_MACH_O_S_ATTR_NONE,			4},
    {	".constructor",				"__constructor",
	SEC_CODE | SEC_LOAD,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,			0},
    {	".destructor",				"__destructor",
	SEC_CODE | SEC_LOAD,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,			0},
    {	".eh_frame",				"__eh_frame",
	SEC_READONLY | SEC_DATA | SEC_LOAD,	BFD_MACH_O_S_COALESCED,
	BFD_MACH_O_S_ATTR_LIVE_SUPPORT
	| BFD_MACH_O_S_ATTR_STRIP_STATIC_SYMS
	| BFD_MACH_O_S_ATTR_NO_TOC,		2},
    { NULL, NULL, 0, 0, 0, 0}
  };

/* __DATA Segment.  */
static const mach_o_section_name_xlat data_section_names_xlat[] =
  {
    {	".data",			"__data",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,		0},
    {	".bss",				"__bss",
	SEC_NO_FLAGS,			BFD_MACH_O_S_ZEROFILL,
	BFD_MACH_O_S_ATTR_NONE,		0},
    {	".const_data",			"__const",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,		0},
    {	".static_data",			"__static_data",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,		0},
    {	".mod_init_func",		"__mod_init_func",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_MOD_INIT_FUNC_POINTERS,
	BFD_MACH_O_S_ATTR_NONE,		2},
    {	".mod_term_func",		"__mod_term_func",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_MOD_FINI_FUNC_POINTERS,
	BFD_MACH_O_S_ATTR_NONE,		2},
    {	".dyld",			"__dyld",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,		0},
    {	".cfstring",			"__cfstring",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NONE,		2},
    { NULL, NULL, 0, 0, 0, 0}
  };

/* __DWARF Segment.  */
static const mach_o_section_name_xlat dwarf_section_names_xlat[] =
  {
    {	".debug_frame",			"__debug_frame",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_info",			"__debug_info",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_abbrev",		"__debug_abbrev",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_aranges",		"__debug_aranges",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_macinfo",		"__debug_macinfo",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_line",			"__debug_line",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_loc",			"__debug_loc",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_pubnames",		"__debug_pubnames",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_pubtypes",		"__debug_pubtypes",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_str",			"__debug_str",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_ranges",		"__debug_ranges",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    {	".debug_macro",			"__debug_macro",
	SEC_DEBUGGING,			BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_DEBUG,	0},
    { NULL, NULL, 0, 0, 0, 0}
  };

/* __OBJC Segment.  */
static const mach_o_section_name_xlat objc_section_names_xlat[] =
  {
    {	".objc_class",			"__class",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_meta_class",		"__meta_class",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_cat_cls_meth",		"__cat_cls_meth",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_cat_inst_meth",		"__cat_inst_meth",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_protocol",		"__protocol",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_string_object",		"__string_object",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_cls_meth",		"__cls_meth",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_inst_meth",		"__inst_meth",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_cls_refs",		"__cls_refs",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_LITERAL_POINTERS,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_message_refs",		"__message_refs",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_LITERAL_POINTERS,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_symbols",		"__symbols",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_category",		"__category",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_class_vars",		"__class_vars",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_instance_vars",		"__instance_vars",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_module_info",		"__module_info",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_selector_strs",		"__selector_strs",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_CSTRING_LITERALS,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_image_info",		"__image_info",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc_selector_fixup",		"__sel_fixup",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    /* Objc V1 */
    {	".objc1_class_ext",		"__class_ext",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc1_property_list",		"__property",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    {	".objc1_protocol_ext",		"__protocol_ext",
	SEC_DATA | SEC_LOAD,		BFD_MACH_O_S_REGULAR,
	BFD_MACH_O_S_ATTR_NO_DEAD_STRIP, 0},
    { NULL, NULL, 0, 0, 0, 0}
  };

static const mach_o_segment_name_xlat segsec_names_xlat[] =
  {
    { "__TEXT", text_section_names_xlat },
    { "__DATA", data_section_names_xlat },
    { "__DWARF", dwarf_section_names_xlat },
    { "__OBJC", objc_section_names_xlat },
    { NULL, NULL }
  };

static const char dsym_subdir[] = ".dSYM/Contents/Resources/DWARF";

/* For both cases bfd-name => mach-o name and vice versa, the specific target
   is checked before the generic.  This allows a target (e.g. ppc for cstring)
   to override the generic definition with a more specific one.  */

/* Fetch the translation from a Mach-O section designation (segment, section)
   as a bfd short name, if one exists.  Otherwise return NULL.

   Allow the segment and section names to be unterminated 16 byte arrays.  */

const mach_o_section_name_xlat *
bfd_mach_o_section_data_for_mach_sect (bfd *abfd, const char *segname,
				       const char *sectname)
{
  const struct mach_o_segment_name_xlat *seg;
  const mach_o_section_name_xlat *sec;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  /* First try any target-specific translations defined...  */
  if (bed->segsec_names_xlat)
    for (seg = bed->segsec_names_xlat; seg->segname; seg++)
      if (strncmp (seg->segname, segname, BFD_MACH_O_SEGNAME_SIZE) == 0)
	for (sec = seg->sections; sec->mach_o_name; sec++)
	  if (strncmp (sec->mach_o_name, sectname,
		       BFD_MACH_O_SECTNAME_SIZE) == 0)
	    return sec;

  /* ... and then the Mach-O generic ones.  */
  for (seg = segsec_names_xlat; seg->segname; seg++)
    if (strncmp (seg->segname, segname, BFD_MACH_O_SEGNAME_SIZE) == 0)
      for (sec = seg->sections; sec->mach_o_name; sec++)
        if (strncmp (sec->mach_o_name, sectname,
		     BFD_MACH_O_SECTNAME_SIZE) == 0)
          return sec;

  return NULL;
}

/* If the bfd_name for this section is a 'canonical' form for which we
   know the Mach-O data, return the segment name and the data for the
   Mach-O equivalent.  Otherwise return NULL.  */

const mach_o_section_name_xlat *
bfd_mach_o_section_data_for_bfd_name (bfd *abfd, const char *bfd_name,
				      const char **segname)
{
  const struct mach_o_segment_name_xlat *seg;
  const mach_o_section_name_xlat *sec;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  *segname = NULL;

  if (bfd_name[0] != '.')
    return NULL;

  /* First try any target-specific translations defined...  */
  if (bed->segsec_names_xlat)
    for (seg = bed->segsec_names_xlat; seg->segname; seg++)
      for (sec = seg->sections; sec->bfd_name; sec++)
	if (strcmp (bfd_name, sec->bfd_name) == 0)
	  {
	    *segname = seg->segname;
	    return sec;
	  }

  /* ... and then the Mach-O generic ones.  */
  for (seg = segsec_names_xlat; seg->segname; seg++)
    for (sec = seg->sections; sec->bfd_name; sec++)
      if (strcmp (bfd_name, sec->bfd_name) == 0)
	{
	  *segname = seg->segname;
	  return sec;
	}

  return NULL;
}

/* Convert Mach-O section name to BFD.

   Try to use standard/canonical names, for which we have tables including
   default flag settings - which are returned.  Otherwise forge a new name
   in the form "<segmentname>.<sectionname>" this will be prefixed with
   LC_SEGMENT. if the segment name does not begin with an underscore.

   SEGNAME and SECTNAME are 16 byte arrays (they do not need to be NUL-
   terminated if the name length is exactly 16 bytes - but must be if the name
   length is less than 16 characters).  */

void
bfd_mach_o_convert_section_name_to_bfd (bfd *abfd, const char *segname,
					const char *secname, const char **name,
					flagword *flags)
{
  const mach_o_section_name_xlat *xlat;
  char *res;
  unsigned int len;
  const char *pfx = "";

  *name = NULL;
  *flags = SEC_NO_FLAGS;

  /* First search for a canonical name...
     xlat will be non-null if there is an entry for segname, secname.  */
  xlat = bfd_mach_o_section_data_for_mach_sect (abfd, segname, secname);
  if (xlat)
    {
      len = strlen (xlat->bfd_name);
      res = bfd_alloc (abfd, len+1);
      if (res == NULL)
	return;
      memcpy (res, xlat->bfd_name, len+1);
      *name = res;
      *flags = xlat->bfd_flags;
      return;
    }

  /* ... else we make up a bfd name from the segment concatenated with the
     section.  */

  len = 16 + 1 + 16 + 1;

  /* Put "LC_SEGMENT." prefix if the segment name is weird (ie doesn't start
     with an underscore.  */
  if (segname[0] != '_')
    {
      static const char seg_pfx[] = "LC_SEGMENT.";

      pfx = seg_pfx;
      len += sizeof (seg_pfx) - 1;
    }

  res = bfd_alloc (abfd, len);
  if (res == NULL)
    return;
  snprintf (res, len, "%s%.16s.%.16s", pfx, segname, secname);
  *name = res;
}

/* Convert a bfd section name to a Mach-O segment + section name.

   If the name is a canonical one for which we have a Darwin match
   return the translation table - which contains defaults for flags,
   type, attribute and default alignment data.

   Otherwise, expand the bfd_name (assumed to be in the form
   "[LC_SEGMENT.]<segmentname>.<sectionname>") and return NULL.  */

static const mach_o_section_name_xlat *
bfd_mach_o_convert_section_name_to_mach_o (bfd *abfd ATTRIBUTE_UNUSED,
                                           asection *sect,
                                           bfd_mach_o_section *section)
{
  const mach_o_section_name_xlat *xlat;
  const char *name = bfd_get_section_name (abfd, sect);
  const char *segname;
  const char *dot;
  unsigned int len;
  unsigned int seglen;
  unsigned int seclen;

  memset (section->segname, 0, BFD_MACH_O_SEGNAME_SIZE + 1);
  memset (section->sectname, 0, BFD_MACH_O_SECTNAME_SIZE + 1);

  /* See if is a canonical name ... */
  xlat = bfd_mach_o_section_data_for_bfd_name (abfd, name, &segname);
  if (xlat)
    {
      strcpy (section->segname, segname);
      strcpy (section->sectname, xlat->mach_o_name);
      return xlat;
    }

  /* .. else we convert our constructed one back to Mach-O.
     Strip LC_SEGMENT. prefix, if present.  */
  if (strncmp (name, "LC_SEGMENT.", 11) == 0)
    name += 11;

  /* Find a dot.  */
  dot = strchr (name, '.');
  len = strlen (name);

  /* Try to split name into segment and section names.  */
  if (dot && dot != name)
    {
      seglen = dot - name;
      seclen = len - (dot + 1 - name);

      if (seglen < 16 && seclen < 16)
        {
          memcpy (section->segname, name, seglen);
          section->segname[seglen] = 0;
          memcpy (section->sectname, dot + 1, seclen);
          section->sectname[seclen] = 0;
          return NULL;
        }
    }

  /* The segment and section names are both missing - don't make them
     into dots.  */
  if (dot && dot == name)
    return NULL;

  /* Just duplicate the name into both segment and section.  */
  if (len > 16)
    len = 16;
  memcpy (section->segname, name, len);
  section->segname[len] = 0;
  memcpy (section->sectname, name, len);
  section->sectname[len] = 0;
  return NULL;
}

/* Return the size of an entry for section SEC.
   Must be called only for symbol pointer section and symbol stubs
   sections.  */

unsigned int
bfd_mach_o_section_get_entry_size (bfd *abfd, bfd_mach_o_section *sec)
{
  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
    {
    case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
    case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
      return bfd_mach_o_wide_p (abfd) ? 8 : 4;
    case BFD_MACH_O_S_SYMBOL_STUBS:
      return sec->reserved2;
    default:
      BFD_FAIL ();
      return 0;
    }
}

/* Return the number of indirect symbols for a section.
   Must be called only for symbol pointer section and symbol stubs
   sections.  */

unsigned int
bfd_mach_o_section_get_nbr_indirect (bfd *abfd, bfd_mach_o_section *sec)
{
  unsigned int elsz;

  elsz = bfd_mach_o_section_get_entry_size (abfd, sec);
  if (elsz == 0)
    return 0;
  else
    return sec->size / elsz;
}


/* Copy any private info we understand from the input symbol
   to the output symbol.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_symbol_data (bfd *ibfd ATTRIBUTE_UNUSED,
					 asymbol *isymbol,
					 bfd *obfd ATTRIBUTE_UNUSED,
					 asymbol *osymbol)
{
  bfd_mach_o_asymbol *os, *is;
  os = (bfd_mach_o_asymbol *)osymbol;
  is = (bfd_mach_o_asymbol *)isymbol;
  os->n_type = is->n_type;
  os->n_sect = is->n_sect;
  os->n_desc = is->n_desc;
  os->symbol.udata.i = is->symbol.udata.i;
  return TRUE;
}

/* Copy any private info we understand from the input section
   to the output section.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_section_data (bfd *ibfd ATTRIBUTE_UNUSED,
					  asection *isection,
					  bfd *obfd ATTRIBUTE_UNUSED,
					  asection *osection)
{
  if (osection->used_by_bfd == NULL)
    osection->used_by_bfd = isection->used_by_bfd;
  else
    if (isection->used_by_bfd != NULL)
      memcpy (osection->used_by_bfd, isection->used_by_bfd,
	      sizeof (bfd_mach_o_section));

  if (osection->used_by_bfd != NULL)
    ((bfd_mach_o_section *)osection->used_by_bfd)->bfdsection = osection;

  return TRUE;
}

/* Copy any private info we understand from the input bfd
   to the output bfd.  */

bfd_boolean
bfd_mach_o_bfd_copy_private_bfd_data (bfd *ibfd, bfd *obfd)
{
  if (bfd_get_flavour (ibfd) != bfd_target_mach_o_flavour
      || bfd_get_flavour (obfd) != bfd_target_mach_o_flavour)
    return TRUE;

  BFD_ASSERT (bfd_mach_o_valid (ibfd));
  BFD_ASSERT (bfd_mach_o_valid (obfd));

  /* FIXME: copy commands.  */

  return TRUE;
}

/* This allows us to set up to 32 bits of flags (unless we invent some
   fiendish scheme to subdivide).  For now, we'll just set the file flags
   without error checking - just overwrite.  */

bfd_boolean
bfd_mach_o_bfd_set_private_flags (bfd *abfd, flagword flags)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (!mdata)
    return FALSE;

  mdata->header.flags = flags;
  return TRUE;
}

/* Count the total number of symbols.  */

static long
bfd_mach_o_count_symbols (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (mdata->symtab == NULL)
    return 0;
  return mdata->symtab->nsyms;
}

long
bfd_mach_o_get_symtab_upper_bound (bfd *abfd)
{
  long nsyms = bfd_mach_o_count_symbols (abfd);

  return ((nsyms + 1) * sizeof (asymbol *));
}

long
bfd_mach_o_canonicalize_symtab (bfd *abfd, asymbol **alocation)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  long nsyms = bfd_mach_o_count_symbols (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;
  unsigned long j;

  if (nsyms < 0)
    return nsyms;

  if (nsyms == 0)
    {
      /* Do not try to read symbols if there are none.  */
      alocation[0] = NULL;
      return 0;
    }

  if (!bfd_mach_o_read_symtab_symbols (abfd))
    {
      (*_bfd_error_handler)
        (_("bfd_mach_o_canonicalize_symtab: unable to load symbols"));
      return 0;
    }

  BFD_ASSERT (sym->symbols != NULL);

  for (j = 0; j < sym->nsyms; j++)
    alocation[j] = &sym->symbols[j].symbol;

  alocation[j] = NULL;

  return nsyms;
}

/* Create synthetic symbols for indirect symbols.  */

long
bfd_mach_o_get_synthetic_symtab (bfd *abfd,
                                 long symcount ATTRIBUTE_UNUSED,
                                 asymbol **syms ATTRIBUTE_UNUSED,
                                 long dynsymcount ATTRIBUTE_UNUSED,
                                 asymbol **dynsyms ATTRIBUTE_UNUSED,
                                 asymbol **ret)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_dysymtab_command *dysymtab = mdata->dysymtab;
  bfd_mach_o_symtab_command *symtab = mdata->symtab;
  asymbol *s;
  unsigned long count, i, j, n;
  size_t size;
  char *names;
  char *nul_name;

  *ret = NULL;

  /* Stop now if no symbols or no indirect symbols.  */
  if (dysymtab == NULL || symtab == NULL || symtab->symbols == NULL)
    return 0;

  if (dysymtab->nindirectsyms == 0)
    return 0;

  /* We need to allocate a bfd symbol for every indirect symbol and to
     allocate the memory for its name.  */
  count = dysymtab->nindirectsyms;
  size = count * sizeof (asymbol) + 1;

  for (j = 0; j < count; j++)
    {
      unsigned int isym = dysymtab->indirect_syms[j];

      /* Some indirect symbols are anonymous.  */
      if (isym < symtab->nsyms && symtab->symbols[isym].symbol.name)
        size += strlen (symtab->symbols[isym].symbol.name) + sizeof ("$stub");
    }

  s = *ret = (asymbol *) bfd_malloc (size);
  if (s == NULL)
    return -1;
  names = (char *) (s + count);
  nul_name = names;
  *names++ = 0;

  n = 0;
  for (i = 0; i < mdata->nsects; i++)
    {
      bfd_mach_o_section *sec = mdata->sections[i];
      unsigned int first, last;
      bfd_vma addr;
      bfd_vma entry_size;

      switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
        {
        case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
        case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
        case BFD_MACH_O_S_SYMBOL_STUBS:
          /* Only these sections have indirect symbols.  */
          first = sec->reserved1;
          last = first + bfd_mach_o_section_get_nbr_indirect (abfd, sec);
          addr = sec->addr;
          entry_size = bfd_mach_o_section_get_entry_size (abfd, sec);
          for (j = first; j < last; j++)
            {
              unsigned int isym = dysymtab->indirect_syms[j];

              s->flags = BSF_GLOBAL | BSF_SYNTHETIC;
              s->section = sec->bfdsection;
              s->value = addr - sec->addr;
              s->udata.p = NULL;

              if (isym < symtab->nsyms
                  && symtab->symbols[isym].symbol.name)
                {
                  const char *sym = symtab->symbols[isym].symbol.name;
                  size_t len;

                  s->name = names;
                  len = strlen (sym);
                  memcpy (names, sym, len);
                  names += len;
                  memcpy (names, "$stub", sizeof ("$stub"));
                  names += sizeof ("$stub");
                }
              else
                s->name = nul_name;

              addr += entry_size;
              s++;
              n++;
            }
          break;
        default:
          break;
        }
    }

  return n;
}

void
bfd_mach_o_get_symbol_info (bfd *abfd ATTRIBUTE_UNUSED,
			    asymbol *symbol,
			    symbol_info *ret)
{
  bfd_symbol_info (symbol, ret);
}

void
bfd_mach_o_print_symbol (bfd *abfd,
			 void * afile,
			 asymbol *symbol,
			 bfd_print_symbol_type how)
{
  FILE *file = (FILE *) afile;
  const char *name;
  bfd_mach_o_asymbol *asym = (bfd_mach_o_asymbol *)symbol;

  switch (how)
    {
    case bfd_print_symbol_name:
      fprintf (file, "%s", symbol->name);
      break;
    default:
      bfd_print_symbol_vandf (abfd, (void *) file, symbol);
      if (asym->n_type & BFD_MACH_O_N_STAB)
	name = bfd_get_stab_name (asym->n_type);
      else
	switch (asym->n_type & BFD_MACH_O_N_TYPE)
	  {
	  case BFD_MACH_O_N_UNDF:
            if (symbol->value == 0)
              name = "UND";
            else
              name = "COM";
	    break;
	  case BFD_MACH_O_N_ABS:
	    name = "ABS";
	    break;
	  case BFD_MACH_O_N_INDR:
	    name = "INDR";
	    break;
	  case BFD_MACH_O_N_PBUD:
	    name = "PBUD";
	    break;
	  case BFD_MACH_O_N_SECT:
	    name = "SECT";
	    break;
	  default:
	    name = "???";
	    break;
	  }
      if (name == NULL)
	name = "";
      fprintf (file, " %02x %-6s %02x %04x",
               asym->n_type, name, asym->n_sect, asym->n_desc);
      if ((asym->n_type & BFD_MACH_O_N_STAB) == 0
	  && (asym->n_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_SECT)
	fprintf (file, " [%s]", symbol->section->name);
      fprintf (file, " %s", symbol->name);
    }
}

static void
bfd_mach_o_convert_architecture (bfd_mach_o_cpu_type mtype,
				 bfd_mach_o_cpu_subtype msubtype,
				 enum bfd_architecture *type,
				 unsigned long *subtype)
{
  *subtype = bfd_arch_unknown;

  switch (mtype)
    {
    case BFD_MACH_O_CPU_TYPE_VAX:
      *type = bfd_arch_vax;
      break;
    case BFD_MACH_O_CPU_TYPE_MC680x0:
      *type = bfd_arch_m68k;
      break;
    case BFD_MACH_O_CPU_TYPE_I386:
      *type = bfd_arch_i386;
      *subtype = bfd_mach_i386_i386;
      break;
    case BFD_MACH_O_CPU_TYPE_X86_64:
      *type = bfd_arch_i386;
      *subtype = bfd_mach_x86_64;
      break;
    case BFD_MACH_O_CPU_TYPE_MIPS:
      *type = bfd_arch_mips;
      break;
    case BFD_MACH_O_CPU_TYPE_MC98000:
      *type = bfd_arch_m98k;
      break;
    case BFD_MACH_O_CPU_TYPE_HPPA:
      *type = bfd_arch_hppa;
      break;
    case BFD_MACH_O_CPU_TYPE_ARM:
      *type = bfd_arch_arm;
      switch (msubtype)
        {
        case BFD_MACH_O_CPU_SUBTYPE_ARM_V4T:
          *subtype = bfd_mach_arm_4T;
          break;
        case BFD_MACH_O_CPU_SUBTYPE_ARM_V6:
          *subtype = bfd_mach_arm_4T;	/* Best fit ?  */
          break;
        case BFD_MACH_O_CPU_SUBTYPE_ARM_V5TEJ:
          *subtype = bfd_mach_arm_5TE;
          break;
        case BFD_MACH_O_CPU_SUBTYPE_ARM_XSCALE:
          *subtype = bfd_mach_arm_XScale;
          break;
        case BFD_MACH_O_CPU_SUBTYPE_ARM_V7:
          *subtype = bfd_mach_arm_5TE;	/* Best fit ?  */
          break;
        case BFD_MACH_O_CPU_SUBTYPE_ARM_ALL:
        default:
          break;
        }
      break;
    case BFD_MACH_O_CPU_TYPE_MC88000:
      *type = bfd_arch_m88k;
      break;
    case BFD_MACH_O_CPU_TYPE_SPARC:
      *type = bfd_arch_sparc;
      *subtype = bfd_mach_sparc;
      break;
    case BFD_MACH_O_CPU_TYPE_I860:
      *type = bfd_arch_i860;
      break;
    case BFD_MACH_O_CPU_TYPE_ALPHA:
      *type = bfd_arch_alpha;
      break;
    case BFD_MACH_O_CPU_TYPE_POWERPC:
      *type = bfd_arch_powerpc;
      *subtype = bfd_mach_ppc;
      break;
    case BFD_MACH_O_CPU_TYPE_POWERPC_64:
      *type = bfd_arch_powerpc;
      *subtype = bfd_mach_ppc64;
      break;
    default:
      *type = bfd_arch_unknown;
      break;
    }
}

static bfd_boolean
bfd_mach_o_write_header (bfd *abfd, bfd_mach_o_header *header)
{
  struct mach_o_header_external raw;
  unsigned int size;

  size = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  bfd_h_put_32 (abfd, header->magic, raw.magic);
  bfd_h_put_32 (abfd, header->cputype, raw.cputype);
  bfd_h_put_32 (abfd, header->cpusubtype, raw.cpusubtype);
  bfd_h_put_32 (abfd, header->filetype, raw.filetype);
  bfd_h_put_32 (abfd, header->ncmds, raw.ncmds);
  bfd_h_put_32 (abfd, header->sizeofcmds, raw.sizeofcmds);
  bfd_h_put_32 (abfd, header->flags, raw.flags);

  if (mach_o_wide_p (header))
    bfd_h_put_32 (abfd, header->reserved, raw.reserved);

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bwrite (&raw, size, abfd) != size)
    return FALSE;

  return TRUE;
}

static int
bfd_mach_o_write_thread (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_thread_command *cmd = &command->command.thread;
  unsigned int i;
  struct mach_o_thread_command_external raw;
  unsigned int offset;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_THREAD)
	      || (command->type == BFD_MACH_O_LC_UNIXTHREAD));

  offset = 8;
  for (i = 0; i < cmd->nflavours; i++)
    {
      BFD_ASSERT ((cmd->flavours[i].size % 4) == 0);
      BFD_ASSERT (cmd->flavours[i].offset ==
                  (command->offset + offset + BFD_MACH_O_LC_SIZE));

      bfd_h_put_32 (abfd, cmd->flavours[i].flavour, raw.flavour);
      bfd_h_put_32 (abfd, (cmd->flavours[i].size / 4), raw.count);

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
	return -1;

      offset += cmd->flavours[i].size + sizeof (raw);
    }

  return 0;
}

long
bfd_mach_o_get_reloc_upper_bound (bfd *abfd ATTRIBUTE_UNUSED,
                                  asection *asect)
{
  return (asect->reloc_count + 1) * sizeof (arelent *);
}

/* In addition to the need to byte-swap the symbol number, the bit positions
   of the fields in the relocation information vary per target endian-ness.  */

static void
bfd_mach_o_swap_in_non_scattered_reloc (bfd *abfd, bfd_mach_o_reloc_info *rel,
				       unsigned char *fields)
{
  unsigned char info = fields[3];

  if (bfd_big_endian (abfd))
    {
      rel->r_value = (fields[0] << 16) | (fields[1] << 8) | fields[2];
      rel->r_type = (info >> BFD_MACH_O_BE_TYPE_SHIFT) & BFD_MACH_O_TYPE_MASK;
      rel->r_pcrel = (info & BFD_MACH_O_BE_PCREL) ? 1 : 0;
      rel->r_length = (info >> BFD_MACH_O_BE_LENGTH_SHIFT)
		      & BFD_MACH_O_LENGTH_MASK;
      rel->r_extern = (info & BFD_MACH_O_BE_EXTERN) ? 1 : 0;
    }
  else
    {
      rel->r_value = (fields[2] << 16) | (fields[1] << 8) | fields[0];
      rel->r_type = (info >> BFD_MACH_O_LE_TYPE_SHIFT) & BFD_MACH_O_TYPE_MASK;
      rel->r_pcrel = (info & BFD_MACH_O_LE_PCREL) ? 1 : 0;
      rel->r_length = (info >> BFD_MACH_O_LE_LENGTH_SHIFT)
		      & BFD_MACH_O_LENGTH_MASK;
      rel->r_extern = (info & BFD_MACH_O_LE_EXTERN) ? 1 : 0;
    }
}

static int
bfd_mach_o_canonicalize_one_reloc (bfd *abfd,
                                   struct mach_o_reloc_info_external *raw,
                                   arelent *res, asymbol **syms)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  bfd_mach_o_reloc_info reloc;
  bfd_vma addr;
  asymbol **sym;

  addr = bfd_get_32 (abfd, raw->r_address);
  res->sym_ptr_ptr = NULL;
  res->addend = 0;

  if (addr & BFD_MACH_O_SR_SCATTERED)
    {
      unsigned int j;
      bfd_vma symnum = bfd_get_32 (abfd, raw->r_symbolnum);

      /* Scattered relocation, can't be extern. */
      reloc.r_scattered = 1;
      reloc.r_extern = 0;

      /*   Extract section and offset from r_value (symnum).  */
      reloc.r_value = symnum;
      /* FIXME: This breaks when a symbol in a reloc exactly follows the
	 end of the data for the section (e.g. in a calculation of section
	 data length).  At present, the symbol will end up associated with
	 the following section or, if it falls within alignment padding, as
	 null - which will assert later.  */
      for (j = 0; j < mdata->nsects; j++)
        {
          bfd_mach_o_section *sect = mdata->sections[j];
          if (symnum >= sect->addr && symnum < sect->addr + sect->size)
            {
              res->sym_ptr_ptr = sect->bfdsection->symbol_ptr_ptr;
              res->addend = symnum - sect->addr;
              break;
            }
        }

      /* Extract the info and address fields from r_address.  */
      reloc.r_type = BFD_MACH_O_GET_SR_TYPE (addr);
      reloc.r_length = BFD_MACH_O_GET_SR_LENGTH (addr);
      reloc.r_pcrel = addr & BFD_MACH_O_SR_PCREL;
      reloc.r_address = BFD_MACH_O_GET_SR_TYPE (addr);
      res->address = BFD_MACH_O_GET_SR_ADDRESS (addr);
    }
  else
    {
      unsigned int num;

      /* Non-scattered relocation.  */
      reloc.r_scattered = 0;

      /* The value and info fields have to be extracted dependent on target
         endian-ness.  */
      bfd_mach_o_swap_in_non_scattered_reloc (abfd, &reloc, raw->r_symbolnum);
      num = reloc.r_value;

      if (reloc.r_extern)
	{
	  /* An external symbol number.  */
	  sym = syms + num;
	}
      else if (num == 0x00ffffff)
	{
	  /* The 'symnum' in a non-scattered PAIR is 0x00ffffff.  But as this
	     is generic code, we don't know wether this is really a PAIR.
	     This value is almost certainly not a valid section number, hence
	     this specific case to avoid an assertion failure.
	     Target specific swap_reloc_in routine should adjust that.  */
	  sym = bfd_abs_section_ptr->symbol_ptr_ptr;
	}
      else
        {
	  /* A section number.  */
          BFD_ASSERT (num != 0);
          BFD_ASSERT (num <= mdata->nsects);

          sym = mdata->sections[num - 1]->bfdsection->symbol_ptr_ptr;
          /* For a symbol defined in section S, the addend (stored in the
             binary) contains the address of the section.  To comply with
             bfd convention, subtract the section address.
             Use the address from the header, so that the user can modify
             the vma of the section.  */
          res->addend = -mdata->sections[num - 1]->addr;
        }
      /* Note: Pairs for PPC LO/HI/HA are not scattered, but contain the offset
	 in the lower 16bits of the address value.  So we have to find the
	 'symbol' from the preceding reloc.  We do this even though the
	 section symbol is probably not needed here, because NULL symbol
	 values cause an assert in generic BFD code.  This must be done in
	 the PPC swap_reloc_in routine.  */
      res->sym_ptr_ptr = sym;

      /* The 'address' is just r_address.
         ??? maybe this should be masked with  0xffffff for safety.  */
      res->address = addr;
      reloc.r_address = addr;
    }

  /* We have set up a reloc with all the information present, so the swapper
     can modify address, value and addend fields, if necessary, to convey
     information in the generic BFD reloc that is mach-o specific.  */

  if (!(*bed->_bfd_mach_o_swap_reloc_in)(res, &reloc))
    return -1;
  return 0;
}

static int
bfd_mach_o_canonicalize_relocs (bfd *abfd, unsigned long filepos,
                                unsigned long count,
                                arelent *res, asymbol **syms)
{
  unsigned long i;
  struct mach_o_reloc_info_external *native_relocs;
  bfd_size_type native_size;

  /* Allocate and read relocs.  */
  native_size = count * BFD_MACH_O_RELENT_SIZE;
  native_relocs =
    (struct mach_o_reloc_info_external *) bfd_malloc (native_size);
  if (native_relocs == NULL)
    return -1;

  if (bfd_seek (abfd, filepos, SEEK_SET) != 0
      || bfd_bread (native_relocs, native_size, abfd) != native_size)
    goto err;

  for (i = 0; i < count; i++)
    {
      if (bfd_mach_o_canonicalize_one_reloc (abfd, &native_relocs[i],
                                             &res[i], syms) < 0)
        goto err;
    }
  free (native_relocs);
  return i;
 err:
  free (native_relocs);
  return -1;
}

long
bfd_mach_o_canonicalize_reloc (bfd *abfd, asection *asect,
                               arelent **rels, asymbol **syms)
{
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  unsigned long i;
  arelent *res;

  if (asect->reloc_count == 0)
    return 0;

  /* No need to go further if we don't know how to read relocs.  */
  if (bed->_bfd_mach_o_swap_reloc_in == NULL)
    return 0;

  if (asect->relocation == NULL)
    {
      res = bfd_malloc (asect->reloc_count * sizeof (arelent));
      if (res == NULL)
        return -1;

      if (bfd_mach_o_canonicalize_relocs (abfd, asect->rel_filepos,
                                          asect->reloc_count, res, syms) < 0)
        {
          free (res);
          return -1;
        }
      asect->relocation = res;
    }

  res = asect->relocation;
  for (i = 0; i < asect->reloc_count; i++)
    rels[i] = &res[i];
  rels[i] = NULL;

  return i;
}

long
bfd_mach_o_get_dynamic_reloc_upper_bound (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  if (mdata->dysymtab == NULL)
    return 1;
  return (mdata->dysymtab->nextrel + mdata->dysymtab->nlocrel + 1)
    * sizeof (arelent *);
}

long
bfd_mach_o_canonicalize_dynamic_reloc (bfd *abfd, arelent **rels,
                                       struct bfd_symbol **syms)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_dysymtab_command *dysymtab = mdata->dysymtab;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);
  unsigned long i;
  arelent *res;

  if (dysymtab == NULL)
    return 0;
  if (dysymtab->nextrel == 0 && dysymtab->nlocrel == 0)
    return 0;

  /* No need to go further if we don't know how to read relocs.  */
  if (bed->_bfd_mach_o_swap_reloc_in == NULL)
    return 0;

  if (mdata->dyn_reloc_cache == NULL)
    {
      res = bfd_malloc ((dysymtab->nextrel + dysymtab->nlocrel)
                        * sizeof (arelent));
      if (res == NULL)
        return -1;

      if (bfd_mach_o_canonicalize_relocs (abfd, dysymtab->extreloff,
                                          dysymtab->nextrel, res, syms) < 0)
        {
          free (res);
          return -1;
        }

      if (bfd_mach_o_canonicalize_relocs (abfd, dysymtab->locreloff,
                                          dysymtab->nlocrel,
                                          res + dysymtab->nextrel, syms) < 0)
        {
          free (res);
          return -1;
        }

      mdata->dyn_reloc_cache = res;
    }

  res = mdata->dyn_reloc_cache;
  for (i = 0; i < dysymtab->nextrel + dysymtab->nlocrel; i++)
    rels[i] = &res[i];
  rels[i] = NULL;
  return i;
}

/* In addition to the need to byte-swap the symbol number, the bit positions
   of the fields in the relocation information vary per target endian-ness.  */

static void
bfd_mach_o_swap_out_non_scattered_reloc (bfd *abfd, unsigned char *fields,
				       bfd_mach_o_reloc_info *rel)
{
  unsigned char info = 0;

  BFD_ASSERT (rel->r_type <= 15);
  BFD_ASSERT (rel->r_length <= 3);

  if (bfd_big_endian (abfd))
    {
      fields[0] = (rel->r_value >> 16) & 0xff;
      fields[1] = (rel->r_value >> 8) & 0xff;
      fields[2] = rel->r_value & 0xff;
      info |= rel->r_type << BFD_MACH_O_BE_TYPE_SHIFT;
      info |= rel->r_pcrel ? BFD_MACH_O_BE_PCREL : 0;
      info |= rel->r_length << BFD_MACH_O_BE_LENGTH_SHIFT;
      info |= rel->r_extern ? BFD_MACH_O_BE_EXTERN : 0;
    }
  else
    {
      fields[2] = (rel->r_value >> 16) & 0xff;
      fields[1] = (rel->r_value >> 8) & 0xff;
      fields[0] = rel->r_value & 0xff;
      info |= rel->r_type << BFD_MACH_O_LE_TYPE_SHIFT;
      info |= rel->r_pcrel ? BFD_MACH_O_LE_PCREL : 0;
      info |= rel->r_length << BFD_MACH_O_LE_LENGTH_SHIFT;
      info |= rel->r_extern ? BFD_MACH_O_LE_EXTERN : 0;
    }
  fields[3] = info;
}

static bfd_boolean
bfd_mach_o_write_relocs (bfd *abfd, bfd_mach_o_section *section)
{
  unsigned int i;
  arelent **entries;
  asection *sec;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  sec = section->bfdsection;
  if (sec->reloc_count == 0)
    return TRUE;

  if (bed->_bfd_mach_o_swap_reloc_out == NULL)
    return TRUE;

  if (bfd_seek (abfd, section->reloff, SEEK_SET) != 0)
    return FALSE;

  /* Convert and write.  */
  entries = section->bfdsection->orelocation;
  for (i = 0; i < section->nreloc; i++)
    {
      arelent *rel = entries[i];
      struct mach_o_reloc_info_external raw;
      bfd_mach_o_reloc_info info, *pinfo = &info;

      /* Convert relocation to an intermediate representation.  */
      if (!(*bed->_bfd_mach_o_swap_reloc_out) (rel, pinfo))
        return FALSE;

      /* Lower the relocation info.  */
      if (pinfo->r_scattered)
        {
          unsigned long v;

          v = BFD_MACH_O_SR_SCATTERED
            | (pinfo->r_pcrel ? BFD_MACH_O_SR_PCREL : 0)
            | BFD_MACH_O_SET_SR_LENGTH(pinfo->r_length)
            | BFD_MACH_O_SET_SR_TYPE(pinfo->r_type)
            | BFD_MACH_O_SET_SR_ADDRESS(pinfo->r_address);
          /* Note: scattered relocs have field in reverse order...  */
          bfd_put_32 (abfd, v, raw.r_address);
          bfd_put_32 (abfd, pinfo->r_value, raw.r_symbolnum);
        }
      else
        {
          bfd_put_32 (abfd, pinfo->r_address, raw.r_address);
          bfd_mach_o_swap_out_non_scattered_reloc (abfd, raw.r_symbolnum,
						   pinfo);
        }

      if (bfd_bwrite (&raw, BFD_MACH_O_RELENT_SIZE, abfd)
          != BFD_MACH_O_RELENT_SIZE)
        return FALSE;
    }
  return TRUE;
}

static int
bfd_mach_o_write_section_32 (bfd *abfd, bfd_mach_o_section *section)
{
  struct mach_o_section_32_external raw;

  memcpy (raw.sectname, section->sectname, 16);
  memcpy (raw.segname, section->segname, 16);
  bfd_h_put_32 (abfd, section->addr, raw.addr);
  bfd_h_put_32 (abfd, section->size, raw.size);
  bfd_h_put_32 (abfd, section->offset, raw.offset);
  bfd_h_put_32 (abfd, section->align, raw.align);
  bfd_h_put_32 (abfd, section->reloff, raw.reloff);
  bfd_h_put_32 (abfd, section->nreloc, raw.nreloc);
  bfd_h_put_32 (abfd, section->flags, raw.flags);
  bfd_h_put_32 (abfd, section->reserved1, raw.reserved1);
  bfd_h_put_32 (abfd, section->reserved2, raw.reserved2);

  if (bfd_bwrite (&raw, BFD_MACH_O_SECTION_SIZE, abfd)
      != BFD_MACH_O_SECTION_SIZE)
    return -1;

  return 0;
}

static int
bfd_mach_o_write_section_64 (bfd *abfd, bfd_mach_o_section *section)
{
  struct mach_o_section_64_external raw;

  memcpy (raw.sectname, section->sectname, 16);
  memcpy (raw.segname, section->segname, 16);
  bfd_h_put_64 (abfd, section->addr, raw.addr);
  bfd_h_put_64 (abfd, section->size, raw.size);
  bfd_h_put_32 (abfd, section->offset, raw.offset);
  bfd_h_put_32 (abfd, section->align, raw.align);
  bfd_h_put_32 (abfd, section->reloff, raw.reloff);
  bfd_h_put_32 (abfd, section->nreloc, raw.nreloc);
  bfd_h_put_32 (abfd, section->flags, raw.flags);
  bfd_h_put_32 (abfd, section->reserved1, raw.reserved1);
  bfd_h_put_32 (abfd, section->reserved2, raw.reserved2);
  bfd_h_put_32 (abfd, section->reserved3, raw.reserved3);

  if (bfd_bwrite (&raw, BFD_MACH_O_SECTION_64_SIZE, abfd)
      != BFD_MACH_O_SECTION_64_SIZE)
    return -1;

  return 0;
}

static int
bfd_mach_o_write_segment_32 (bfd *abfd, bfd_mach_o_load_command *command)
{
  struct mach_o_segment_command_32_external raw;
  bfd_mach_o_segment_command *seg = &command->command.segment;
  bfd_mach_o_section *sec;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT);

  for (sec = seg->sect_head; sec != NULL; sec = sec->next)
    if (!bfd_mach_o_write_relocs (abfd, sec))
      return -1;

  memcpy (raw.segname, seg->segname, 16);
  bfd_h_put_32 (abfd, seg->vmaddr, raw.vmaddr);
  bfd_h_put_32 (abfd, seg->vmsize, raw.vmsize);
  bfd_h_put_32 (abfd, seg->fileoff, raw.fileoff);
  bfd_h_put_32 (abfd, seg->filesize, raw.filesize);
  bfd_h_put_32 (abfd, seg->maxprot, raw.maxprot);
  bfd_h_put_32 (abfd, seg->initprot, raw.initprot);
  bfd_h_put_32 (abfd, seg->nsects, raw.nsects);
  bfd_h_put_32 (abfd, seg->flags, raw.flags);

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  for (sec = seg->sect_head; sec != NULL; sec = sec->next)
    if (bfd_mach_o_write_section_32 (abfd, sec))
      return -1;

  return 0;
}

static int
bfd_mach_o_write_segment_64 (bfd *abfd, bfd_mach_o_load_command *command)
{
  struct mach_o_segment_command_64_external raw;
  bfd_mach_o_segment_command *seg = &command->command.segment;
  bfd_mach_o_section *sec;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT_64);

  for (sec = seg->sect_head; sec != NULL; sec = sec->next)
    if (!bfd_mach_o_write_relocs (abfd, sec))
      return -1;

  memcpy (raw.segname, seg->segname, 16);
  bfd_h_put_64 (abfd, seg->vmaddr, raw.vmaddr);
  bfd_h_put_64 (abfd, seg->vmsize, raw.vmsize);
  bfd_h_put_64 (abfd, seg->fileoff, raw.fileoff);
  bfd_h_put_64 (abfd, seg->filesize, raw.filesize);
  bfd_h_put_32 (abfd, seg->maxprot, raw.maxprot);
  bfd_h_put_32 (abfd, seg->initprot, raw.initprot);
  bfd_h_put_32 (abfd, seg->nsects, raw.nsects);
  bfd_h_put_32 (abfd, seg->flags, raw.flags);

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  for (sec = seg->sect_head; sec != NULL; sec = sec->next)
    if (bfd_mach_o_write_section_64 (abfd, sec))
      return -1;

  return 0;
}

static bfd_boolean
bfd_mach_o_write_symtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = &command->command.symtab;
  unsigned long i;
  unsigned int wide = bfd_mach_o_wide_p (abfd);
  unsigned int symlen = wide ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE;
  struct bfd_strtab_hash *strtab;
  asymbol **symbols = bfd_get_outsymbols (abfd);

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SYMTAB);

  /* Write the symbols first.  */
  mdata->filelen = FILE_ALIGN(mdata->filelen, wide ? 3 : 2);
  sym->symoff = mdata->filelen;
  if (bfd_seek (abfd, sym->symoff, SEEK_SET) != 0)
    return FALSE;

  sym->nsyms = bfd_get_symcount (abfd);
  mdata->filelen += sym->nsyms * symlen;

  strtab = _bfd_stringtab_init ();
  if (strtab == NULL)
    return FALSE;

  if (sym->nsyms > 0)
    /* Although we don't strictly need to do this, for compatibility with
       Darwin system tools, actually output an empty string for the index
       0 entry.  */
    _bfd_stringtab_add (strtab, "", TRUE, FALSE);

  for (i = 0; i < sym->nsyms; i++)
    {
      bfd_size_type str_index;
      bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];

      if (s->symbol.name == 0 || s->symbol.name[0] == '\0')
	/* An index of 0 always means the empty string.  */
        str_index = 0;
      else
        {
          str_index = _bfd_stringtab_add (strtab, s->symbol.name, TRUE, FALSE);

          if (str_index == (bfd_size_type) -1)
            goto err;
        }

      if (wide)
        {
          struct mach_o_nlist_64_external raw;

          bfd_h_put_32 (abfd, str_index, raw.n_strx);
          bfd_h_put_8 (abfd, s->n_type, raw.n_type);
          bfd_h_put_8 (abfd, s->n_sect, raw.n_sect);
          bfd_h_put_16 (abfd, s->n_desc, raw.n_desc);
          bfd_h_put_64 (abfd, s->symbol.section->vma + s->symbol.value,
                        raw.n_value);

          if (bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
            goto err;
        }
      else
        {
          struct mach_o_nlist_external raw;

          bfd_h_put_32 (abfd, str_index, raw.n_strx);
          bfd_h_put_8 (abfd, s->n_type, raw.n_type);
          bfd_h_put_8 (abfd, s->n_sect, raw.n_sect);
          bfd_h_put_16 (abfd, s->n_desc, raw.n_desc);
          bfd_h_put_32 (abfd, s->symbol.section->vma + s->symbol.value,
                        raw.n_value);

          if (bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
            goto err;
        }
    }
  sym->strsize = _bfd_stringtab_size (strtab);
  sym->stroff = mdata->filelen;
  mdata->filelen += sym->strsize;

  if (_bfd_stringtab_emit (abfd, strtab) != TRUE)
    goto err;
  _bfd_stringtab_free (strtab);

  /* The command.  */
  {
    struct mach_o_symtab_command_external raw;

    bfd_h_put_32 (abfd, sym->symoff, raw.symoff);
    bfd_h_put_32 (abfd, sym->nsyms, raw.nsyms);
    bfd_h_put_32 (abfd, sym->stroff, raw.stroff);
    bfd_h_put_32 (abfd, sym->strsize, raw.strsize);

    if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
        || bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
      return FALSE;
  }

  return TRUE;

 err:
  _bfd_stringtab_free (strtab);
  return FALSE;
}

/* Write a dysymtab command.
   TODO: Possibly coalesce writes of smaller objects.  */

static bfd_boolean
bfd_mach_o_write_dysymtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dysymtab_command *cmd = &command->command.dysymtab;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_DYSYMTAB);

  if (cmd->nmodtab != 0)
    {
      unsigned int i;

      if (bfd_seek (abfd, cmd->modtaboff, SEEK_SET) != 0)
	return FALSE;

      for (i = 0; i < cmd->nmodtab; i++)
	{
	  bfd_mach_o_dylib_module *module = &cmd->dylib_module[i];
	  unsigned int iinit;
	  unsigned int ninit;

	  iinit = module->iinit & 0xffff;
	  iinit |= ((module->iterm & 0xffff) << 16);

	  ninit = module->ninit & 0xffff;
	  ninit |= ((module->nterm & 0xffff) << 16);

	  if (bfd_mach_o_wide_p (abfd))
	    {
	      struct mach_o_dylib_module_64_external w;

	      bfd_h_put_32 (abfd, module->module_name_idx, &w.module_name);
	      bfd_h_put_32 (abfd, module->iextdefsym, &w.iextdefsym);
	      bfd_h_put_32 (abfd, module->nextdefsym, &w.nextdefsym);
	      bfd_h_put_32 (abfd, module->irefsym, &w.irefsym);
	      bfd_h_put_32 (abfd, module->nrefsym, &w.nrefsym);
	      bfd_h_put_32 (abfd, module->ilocalsym, &w.ilocalsym);
	      bfd_h_put_32 (abfd, module->nlocalsym, &w.nlocalsym);
	      bfd_h_put_32 (abfd, module->iextrel, &w.iextrel);
	      bfd_h_put_32 (abfd, module->nextrel, &w.nextrel);
	      bfd_h_put_32 (abfd, iinit, &w.iinit_iterm);
	      bfd_h_put_32 (abfd, ninit, &w.ninit_nterm);
	      bfd_h_put_64 (abfd, module->objc_module_info_addr,
			    &w.objc_module_info_addr);
	      bfd_h_put_32 (abfd, module->objc_module_info_size,
			    &w.objc_module_info_size);

	      if (bfd_bwrite ((void *) &w, sizeof (w), abfd) != sizeof (w))
		return FALSE;
	    }
	  else
	    {
	      struct mach_o_dylib_module_external n;

	      bfd_h_put_32 (abfd, module->module_name_idx, &n.module_name);
	      bfd_h_put_32 (abfd, module->iextdefsym, &n.iextdefsym);
	      bfd_h_put_32 (abfd, module->nextdefsym, &n.nextdefsym);
	      bfd_h_put_32 (abfd, module->irefsym, &n.irefsym);
	      bfd_h_put_32 (abfd, module->nrefsym, &n.nrefsym);
	      bfd_h_put_32 (abfd, module->ilocalsym, &n.ilocalsym);
	      bfd_h_put_32 (abfd, module->nlocalsym, &n.nlocalsym);
	      bfd_h_put_32 (abfd, module->iextrel, &n.iextrel);
	      bfd_h_put_32 (abfd, module->nextrel, &n.nextrel);
	      bfd_h_put_32 (abfd, iinit, &n.iinit_iterm);
	      bfd_h_put_32 (abfd, ninit, &n.ninit_nterm);
	      bfd_h_put_32 (abfd, module->objc_module_info_addr,
			    &n.objc_module_info_addr);
	      bfd_h_put_32 (abfd, module->objc_module_info_size,
			    &n.objc_module_info_size);

	      if (bfd_bwrite ((void *) &n, sizeof (n), abfd) != sizeof (n))
		return FALSE;
	    }
	}
    }

  if (cmd->ntoc != 0)
    {
      unsigned int i;

      if (bfd_seek (abfd, cmd->tocoff, SEEK_SET) != 0)
	return FALSE;

      for (i = 0; i < cmd->ntoc; i++)
	{
	  struct mach_o_dylib_table_of_contents_external raw;
	  bfd_mach_o_dylib_table_of_content *toc = &cmd->dylib_toc[i];

	  bfd_h_put_32 (abfd, toc->symbol_index, &raw.symbol_index);
	  bfd_h_put_32 (abfd, toc->module_index, &raw.module_index);

	  if (bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
	    return FALSE;
	}
    }

  if (cmd->nindirectsyms > 0)
    {
      unsigned int i;

      if (bfd_seek (abfd, cmd->indirectsymoff, SEEK_SET) != 0)
	return FALSE;

      for (i = 0; i < cmd->nindirectsyms; ++i)
	{
	  unsigned char raw[4];

	  bfd_h_put_32 (abfd, cmd->indirect_syms[i], &raw);
	  if (bfd_bwrite (raw, sizeof (raw), abfd) != sizeof (raw))
	    return FALSE;
	}
    }

  if (cmd->nextrefsyms != 0)
    {
      unsigned int i;

      if (bfd_seek (abfd, cmd->extrefsymoff, SEEK_SET) != 0)
	return FALSE;

      for (i = 0; i < cmd->nextrefsyms; i++)
	{
	  unsigned long v;
	  unsigned char raw[4];
	  bfd_mach_o_dylib_reference *ref = &cmd->ext_refs[i];

	  /* Fields isym and flags are written as bit-fields, thus we need
	     a specific processing for endianness.  */

	  if (bfd_big_endian (abfd))
	    {
	      v = ((ref->isym & 0xffffff) << 8);
	      v |= ref->flags & 0xff;
	    }
	  else
	    {
	      v = ref->isym  & 0xffffff;
	      v |= ((ref->flags & 0xff) << 24);
	    }

	  bfd_h_put_32 (abfd, v, raw);
	  if (bfd_bwrite (raw, sizeof (raw), abfd) != sizeof (raw))
	    return FALSE;
	}
    }

  /* The command.  */
  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0)
    return FALSE;
  else
    {
      struct mach_o_dysymtab_command_external raw;

      bfd_h_put_32 (abfd, cmd->ilocalsym, &raw.ilocalsym);
      bfd_h_put_32 (abfd, cmd->nlocalsym, &raw.nlocalsym);
      bfd_h_put_32 (abfd, cmd->iextdefsym, &raw.iextdefsym);
      bfd_h_put_32 (abfd, cmd->nextdefsym, &raw.nextdefsym);
      bfd_h_put_32 (abfd, cmd->iundefsym, &raw.iundefsym);
      bfd_h_put_32 (abfd, cmd->nundefsym, &raw.nundefsym);
      bfd_h_put_32 (abfd, cmd->tocoff, &raw.tocoff);
      bfd_h_put_32 (abfd, cmd->ntoc, &raw.ntoc);
      bfd_h_put_32 (abfd, cmd->modtaboff, &raw.modtaboff);
      bfd_h_put_32 (abfd, cmd->nmodtab, &raw.nmodtab);
      bfd_h_put_32 (abfd, cmd->extrefsymoff, &raw.extrefsymoff);
      bfd_h_put_32 (abfd, cmd->nextrefsyms, &raw.nextrefsyms);
      bfd_h_put_32 (abfd, cmd->indirectsymoff, &raw.indirectsymoff);
      bfd_h_put_32 (abfd, cmd->nindirectsyms, &raw.nindirectsyms);
      bfd_h_put_32 (abfd, cmd->extreloff, &raw.extreloff);
      bfd_h_put_32 (abfd, cmd->nextrel, &raw.nextrel);
      bfd_h_put_32 (abfd, cmd->locreloff, &raw.locreloff);
      bfd_h_put_32 (abfd, cmd->nlocrel, &raw.nlocrel);

      if (bfd_bwrite (&raw, sizeof (raw), abfd) != sizeof (raw))
	return FALSE;
    }

  return TRUE;
}

static unsigned
bfd_mach_o_primary_symbol_sort_key (bfd_mach_o_asymbol *s)
{
  unsigned mtyp = s->n_type & BFD_MACH_O_N_TYPE;

  /* Just leave debug symbols where they are (pretend they are local, and
     then they will just be sorted on position).  */
  if (s->n_type & BFD_MACH_O_N_STAB)
    return 0;

  /* Local (we should never see an undefined local AFAICT).  */
  if (! (s->n_type & (BFD_MACH_O_N_EXT | BFD_MACH_O_N_PEXT)))
    return 0;

  /* Common symbols look like undefined externs.  */
  if (mtyp == BFD_MACH_O_N_UNDF)
    return 2;

  /* A defined non-local, non-debug symbol.  */
  return 1;
}

static int
bfd_mach_o_cf_symbols (const void *a, const void *b)
{
  bfd_mach_o_asymbol *sa = *(bfd_mach_o_asymbol **) a;
  bfd_mach_o_asymbol *sb = *(bfd_mach_o_asymbol **) b;
  unsigned int soa, sob;

  soa = bfd_mach_o_primary_symbol_sort_key (sa);
  sob = bfd_mach_o_primary_symbol_sort_key (sb);
  if (soa < sob)
    return -1;

  if (soa > sob)
    return 1;

  /* If it's local or stab, just preserve the input order.  */
  if (soa == 0)
    {
      if (sa->symbol.udata.i < sb->symbol.udata.i)
        return -1;
      if (sa->symbol.udata.i > sb->symbol.udata.i)
        return  1;

      /* This is probably an error.  */
      return 0;
    }

  /* The second sort key is name.  */
  return strcmp (sa->symbol.name, sb->symbol.name);
}

/* Process the symbols.

   This should be OK for single-module files - but it is not likely to work
   for multi-module shared libraries.

   (a) If the application has not filled in the relevant mach-o fields, make
       an estimate.

   (b) Order them, like this:
	(  i) local.
		(unsorted)
	( ii) external defined
		(by name)
	(iii) external undefined/common
		(by name)
	( iv) common
		(by name)
*/

static bfd_boolean
bfd_mach_o_mangle_symbols (bfd *abfd)
{
  unsigned long i;
  asymbol **symbols = bfd_get_outsymbols (abfd);

  if (symbols == NULL || bfd_get_symcount (abfd) == 0)
    return TRUE;

  for (i = 0; i < bfd_get_symcount (abfd); i++)
    {
      bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];

      /* We use this value, which is out-of-range as a symbol index, to signal
	 that the mach-o-specific data are not filled in and need to be created
	 from the bfd values.  It is much preferable for the application to do
	 this, since more meaningful diagnostics can be made that way.  */

      if (s->symbol.udata.i == SYM_MACHO_FIELDS_UNSET)
        {
          /* No symbol information has been set - therefore determine
             it from the bfd symbol flags/info.  */
          if (s->symbol.section == bfd_abs_section_ptr)
            s->n_type = BFD_MACH_O_N_ABS;
          else if (s->symbol.section == bfd_und_section_ptr)
            {
              s->n_type = BFD_MACH_O_N_UNDF;
              if (s->symbol.flags & BSF_WEAK)
                s->n_desc |= BFD_MACH_O_N_WEAK_REF;
              /* mach-o automatically makes undefined symbols extern.  */
	      s->n_type |= BFD_MACH_O_N_EXT;
	      s->symbol.flags |= BSF_GLOBAL;
            }
          else if (s->symbol.section == bfd_com_section_ptr)
	    {
              s->n_type = BFD_MACH_O_N_UNDF | BFD_MACH_O_N_EXT;
              s->symbol.flags |= BSF_GLOBAL;
            }
          else
            s->n_type = BFD_MACH_O_N_SECT;

          if (s->symbol.flags & BSF_GLOBAL)
            s->n_type |= BFD_MACH_O_N_EXT;
        }

      /* Put the section index in, where required.  */
      if ((s->symbol.section != bfd_abs_section_ptr
          && s->symbol.section != bfd_und_section_ptr
          && s->symbol.section != bfd_com_section_ptr)
          || ((s->n_type & BFD_MACH_O_N_STAB) != 0
               && s->symbol.name == NULL))
	s->n_sect = s->symbol.section->target_index;

      /* Number to preserve order for local and debug syms.  */
      s->symbol.udata.i = i;
    }

  /* Sort the symbols.  */
  qsort ((void *) symbols, (size_t) bfd_get_symcount (abfd),
	 sizeof (asymbol *), bfd_mach_o_cf_symbols);

  for (i = 0; i < bfd_get_symcount (abfd); ++i)
    {
      bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];
      s->symbol.udata.i = i;  /* renumber.  */
    }

  return TRUE;
}

/* We build a flat table of sections, which can be re-ordered if necessary.
   Fill in the section number and other mach-o-specific data.  */

static bfd_boolean
bfd_mach_o_mangle_sections (bfd *abfd, bfd_mach_o_data_struct *mdata)
{
  asection *sec;
  unsigned target_index;
  unsigned nsect;

  nsect = bfd_count_sections (abfd);

  /* Don't do it if it's already set - assume the application knows what it's
     doing.  */
  if (mdata->nsects == nsect
      && (mdata->nsects == 0 || mdata->sections != NULL))
    return TRUE;

  mdata->nsects = nsect;
  mdata->sections = bfd_alloc (abfd,
			       mdata->nsects * sizeof (bfd_mach_o_section *));
  if (mdata->sections == NULL)
    return FALSE;

  /* We need to check that this can be done...  */
  if (nsect > 255)
    (*_bfd_error_handler) (_("mach-o: there are too many sections (%d)"
			     " maximum is 255,\n"), nsect);

  /* Create Mach-O sections.
     Section type, attribute and align should have been set when the
     section was created - either read in or specified.  */
  target_index = 0;
  for (sec = abfd->sections; sec; sec = sec->next)
    {
      unsigned bfd_align = bfd_get_section_alignment (abfd, sec);
      bfd_mach_o_section *msect = bfd_mach_o_get_mach_o_section (sec);

      mdata->sections[target_index] = msect;

      msect->addr = bfd_get_section_vma (abfd, sec);
      msect->size = bfd_get_section_size (sec);

      /* Use the largest alignment set, in case it was bumped after the
	 section was created.  */
      msect->align = msect->align > bfd_align ? msect->align : bfd_align;

      msect->offset = 0;
      sec->target_index = ++target_index;
    }

  return TRUE;
}

bfd_boolean
bfd_mach_o_write_contents (bfd *abfd)
{
  unsigned int i;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  /* Make the commands, if not already present.  */
  if (mdata->header.ncmds == 0)
    if (!bfd_mach_o_build_commands (abfd))
      return FALSE;

  if (!bfd_mach_o_write_header (abfd, &mdata->header))
    return FALSE;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      struct mach_o_load_command_external raw;
      bfd_mach_o_load_command *cur = &mdata->commands[i];
      unsigned long typeflag;

      typeflag = cur->type | (cur->type_required ? BFD_MACH_O_LC_REQ_DYLD : 0);

      bfd_h_put_32 (abfd, typeflag, raw.cmd);
      bfd_h_put_32 (abfd, cur->len, raw.cmdsize);

      if (bfd_seek (abfd, cur->offset, SEEK_SET) != 0
          || bfd_bwrite (&raw, BFD_MACH_O_LC_SIZE, abfd) != 8)
	return FALSE;

      switch (cur->type)
	{
	case BFD_MACH_O_LC_SEGMENT:
	  if (bfd_mach_o_write_segment_32 (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SEGMENT_64:
	  if (bfd_mach_o_write_segment_64 (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SYMTAB:
	  if (!bfd_mach_o_write_symtab (abfd, cur))
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_DYSYMTAB:
	  if (!bfd_mach_o_write_dysymtab (abfd, cur))
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_SYMSEG:
	  break;
	case BFD_MACH_O_LC_THREAD:
	case BFD_MACH_O_LC_UNIXTHREAD:
	  if (bfd_mach_o_write_thread (abfd, cur) != 0)
	    return FALSE;
	  break;
	case BFD_MACH_O_LC_LOADFVMLIB:
	case BFD_MACH_O_LC_IDFVMLIB:
	case BFD_MACH_O_LC_IDENT:
	case BFD_MACH_O_LC_FVMFILE:
	case BFD_MACH_O_LC_PREPAGE:
	case BFD_MACH_O_LC_LOAD_DYLIB:
	case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
	case BFD_MACH_O_LC_ID_DYLIB:
	case BFD_MACH_O_LC_REEXPORT_DYLIB:
        case BFD_MACH_O_LC_LOAD_UPWARD_DYLIB:
	case BFD_MACH_O_LC_LOAD_DYLINKER:
	case BFD_MACH_O_LC_ID_DYLINKER:
	case BFD_MACH_O_LC_PREBOUND_DYLIB:
	case BFD_MACH_O_LC_ROUTINES:
	case BFD_MACH_O_LC_SUB_FRAMEWORK:
	  break;
	default:
	  (*_bfd_error_handler) (_("unable to write unknown load command 0x%lx"),
				 (unsigned long) cur->type);
	  return FALSE;
	}
    }

  return TRUE;
}

static void
bfd_mach_o_append_section_to_segment (bfd_mach_o_segment_command *seg,
                                      asection *sec)
{
  bfd_mach_o_section *s = (bfd_mach_o_section *)sec->used_by_bfd;
  if (seg->sect_head == NULL)
    seg->sect_head = s;
  else
    seg->sect_tail->next = s;
  seg->sect_tail = s;
}

/* Create section Mach-O flags from BFD flags.  */

static void
bfd_mach_o_set_section_flags_from_bfd (bfd *abfd ATTRIBUTE_UNUSED, asection *sec)
{
  flagword bfd_flags;
  bfd_mach_o_section *s = bfd_mach_o_get_mach_o_section (sec);

  /* Create default flags.  */
  bfd_flags = bfd_get_section_flags (abfd, sec);
  if ((bfd_flags & SEC_CODE) == SEC_CODE)
    s->flags = BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS
      | BFD_MACH_O_S_ATTR_SOME_INSTRUCTIONS
      | BFD_MACH_O_S_REGULAR;
  else if ((bfd_flags & (SEC_ALLOC | SEC_LOAD)) == SEC_ALLOC)
    s->flags = BFD_MACH_O_S_ZEROFILL;
  else if (bfd_flags & SEC_DEBUGGING)
    s->flags = BFD_MACH_O_S_REGULAR |  BFD_MACH_O_S_ATTR_DEBUG;
  else
    s->flags = BFD_MACH_O_S_REGULAR;
}

/* Count the number of sections in the list for the segment named.

   The special case of NULL or "" for the segment name is valid for
   an MH_OBJECT file and means 'all sections available'.

   Requires that the sections table in mdata be filled in.

   Returns the number of sections (0 is valid).
   Any number > 255 signals an invalid section count, although we will,
   perhaps, allow the file to be written (in line with Darwin tools up
   to XCode 4).

   A section count of (unsigned long) -1 signals a definite error.  */

static unsigned long
bfd_mach_o_count_sections_for_seg (const char *segment,
				   bfd_mach_o_data_struct *mdata)
{
  unsigned i,j;
  if (mdata == NULL || mdata->sections == NULL)
    return (unsigned long) -1;

  /* The MH_OBJECT case, all sections are considered; Although nsects is
     is an unsigned long, the maximum valid section count is 255 and this
     will have been checked already by mangle_sections.  */
  if (segment == NULL || segment[0] == '\0')
    return mdata->nsects;

  /* Count the number of sections we see in this segment.  */
  j = 0;
  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *s = mdata->sections[i];
      if (strncmp (segment, s->segname, BFD_MACH_O_SEGNAME_SIZE) == 0)
        j++;
    }
  return j;
}

static bfd_boolean
bfd_mach_o_build_seg_command (const char *segment,
			      bfd_mach_o_data_struct *mdata,
			      bfd_mach_o_segment_command *seg)
{
  unsigned i;
  int is_mho = (segment == NULL || segment[0] == '\0');

  /* Fill segment command.  */
  if (is_mho)
    memset (seg->segname, 0, sizeof (seg->segname));
  else
    strncpy (seg->segname, segment, sizeof (seg->segname));

  /* TODO: fix this up for non-MH_OBJECT cases.  */
  seg->vmaddr = 0;
  seg->vmsize = 0;

  seg->fileoff = mdata->filelen;
  seg->filesize = 0;
  seg->maxprot = BFD_MACH_O_PROT_READ | BFD_MACH_O_PROT_WRITE
		 | BFD_MACH_O_PROT_EXECUTE;
  seg->initprot = seg->maxprot;
  seg->flags = 0;
  seg->sect_head = NULL;
  seg->sect_tail = NULL;

  /*  Append sections to the segment.

      This is a little tedious, we have to honor the need to account zerofill
      sections after all the rest.  This forces us to do the calculation of
      total vmsize in three passes so that any alignment increments are
      properly accounted.  */

  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *s = mdata->sections[i];
      asection *sec = s->bfdsection;

      /* If we're not making an MH_OBJECT, check whether this section is from
	 our segment, and skip if not.  Otherwise, just add all sections.  */
      if (! is_mho
	  && strncmp (segment, s->segname, BFD_MACH_O_SEGNAME_SIZE) != 0)
	continue;

      /* Although we account for zerofill section sizes in vm order, they are
	 placed in the file in source sequence.  */
      bfd_mach_o_append_section_to_segment (seg, sec);
      s->offset = 0;

      /* Zerofill sections have zero file size & offset,
	 and are not written.  */
      if ((s->flags & BFD_MACH_O_SECTION_TYPE_MASK) == BFD_MACH_O_S_ZEROFILL
          || (s->flags & BFD_MACH_O_SECTION_TYPE_MASK)
	      == BFD_MACH_O_S_GB_ZEROFILL)
        continue;

      if (s->size > 0)
       {
	  seg->vmsize = FILE_ALIGN (seg->vmsize, s->align);
	  seg->vmsize += s->size;

	  seg->filesize = FILE_ALIGN (seg->filesize, s->align);
	  seg->filesize += s->size;

          mdata->filelen = FILE_ALIGN (mdata->filelen, s->align);
          s->offset = mdata->filelen;
        }

      sec->filepos = s->offset;
      mdata->filelen += s->size;
    }

  /* Now pass through again, for zerofill, only now we just update the vmsize.  */
  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *s = mdata->sections[i];

      if ((s->flags & BFD_MACH_O_SECTION_TYPE_MASK) != BFD_MACH_O_S_ZEROFILL)
        continue;

      if (! is_mho
	  && strncmp (segment, s->segname, BFD_MACH_O_SEGNAME_SIZE) != 0)
	continue;

      if (s->size > 0)
	{
	  seg->vmsize = FILE_ALIGN (seg->vmsize, s->align);
	  seg->vmsize += s->size;
	}
    }

  /* Now pass through again, for zerofill_GB.  */
  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *s = mdata->sections[i];

      if ((s->flags & BFD_MACH_O_SECTION_TYPE_MASK) != BFD_MACH_O_S_GB_ZEROFILL)
        continue;

      if (! is_mho
	  && strncmp (segment, s->segname, BFD_MACH_O_SEGNAME_SIZE) != 0)
	continue;

      if (s->size > 0)
	{
	  seg->vmsize = FILE_ALIGN (seg->vmsize, s->align);
	  seg->vmsize += s->size;
	}
    }

  /* Allocate space for the relocations.  */
  mdata->filelen = FILE_ALIGN(mdata->filelen, 2);

  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *ms = mdata->sections[i];
      asection *sec = ms->bfdsection;

      if ((ms->nreloc = sec->reloc_count) == 0)
        {
	  ms->reloff = 0;
	  continue;
        }
      sec->rel_filepos = mdata->filelen;
      ms->reloff = sec->rel_filepos;
      mdata->filelen += sec->reloc_count * BFD_MACH_O_RELENT_SIZE;
    }

  return TRUE;
}

/* Count the number of indirect symbols in the image.
   Requires that the sections are in their final order.  */

static unsigned int
bfd_mach_o_count_indirect_symbols (bfd *abfd, bfd_mach_o_data_struct *mdata)
{
  unsigned int i;
  unsigned int nisyms = 0;

  for (i = 0; i < mdata->nsects; ++i)
    {
      bfd_mach_o_section *sec = mdata->sections[i];

      switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
	{
	  case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
	  case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
	  case BFD_MACH_O_S_SYMBOL_STUBS:
	    nisyms += bfd_mach_o_section_get_nbr_indirect (abfd, sec);
	    break;
	  default:
	    break;
	}
    }
  return nisyms;
}

static bfd_boolean
bfd_mach_o_build_dysymtab_command (bfd *abfd,
				   bfd_mach_o_data_struct *mdata,
				   bfd_mach_o_load_command *cmd)
{
  bfd_mach_o_dysymtab_command *dsym = &cmd->command.dysymtab;

  /* TODO:
     We are not going to try and fill these in yet and, moreover, we are
     going to bail if they are already set.  */
  if (dsym->nmodtab != 0
      || dsym->ntoc != 0
      || dsym->nextrefsyms != 0)
    {
      (*_bfd_error_handler) (_("sorry: modtab, toc and extrefsyms are not yet"
				" implemented for dysymtab commands."));
      return FALSE;
    }

  dsym->ilocalsym = 0;

  if (bfd_get_symcount (abfd) > 0)
    {
      asymbol **symbols = bfd_get_outsymbols (abfd);
      unsigned long i;

       /* Count the number of each kind of symbol.  */
      for (i = 0; i < bfd_get_symcount (abfd); ++i)
	{
	  bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];
	  if (s->n_type & (BFD_MACH_O_N_EXT | BFD_MACH_O_N_PEXT))
	    break;
	}
      dsym->nlocalsym = i;
      dsym->iextdefsym = i;
      for (; i < bfd_get_symcount (abfd); ++i)
	{
	  bfd_mach_o_asymbol *s = (bfd_mach_o_asymbol *)symbols[i];
	  if ((s->n_type & BFD_MACH_O_N_TYPE) == BFD_MACH_O_N_UNDF)
	    break;
	}
      dsym->nextdefsym = i - dsym->nlocalsym;
      dsym->iundefsym = dsym->nextdefsym + dsym->iextdefsym;
      dsym->nundefsym = bfd_get_symcount (abfd)
			- dsym->nlocalsym
			- dsym->nextdefsym;
    }
  else
    {
      dsym->nlocalsym = 0;
      dsym->iextdefsym = 0;
      dsym->nextdefsym = 0;
      dsym->iundefsym = 0;
      dsym->nundefsym = 0;
    }

  dsym->nindirectsyms = bfd_mach_o_count_indirect_symbols (abfd, mdata);
  if (dsym->nindirectsyms > 0)
    {
      unsigned i;
      unsigned n;

      mdata->filelen = FILE_ALIGN (mdata->filelen, 2);
      dsym->indirectsymoff = mdata->filelen;
      mdata->filelen += dsym->nindirectsyms * 4;

      dsym->indirect_syms = bfd_zalloc (abfd, dsym->nindirectsyms * 4);
      if (dsym->indirect_syms == NULL)
        return FALSE;

      n = 0;
      for (i = 0; i < mdata->nsects; ++i)
	{
	  bfd_mach_o_section *sec = mdata->sections[i];

	  switch (sec->flags & BFD_MACH_O_SECTION_TYPE_MASK)
	    {
	      case BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS:
	      case BFD_MACH_O_S_LAZY_SYMBOL_POINTERS:
	      case BFD_MACH_O_S_SYMBOL_STUBS:
		{
		  unsigned j, num;
		  bfd_mach_o_asymbol **isyms = sec->indirect_syms;

		  num = bfd_mach_o_section_get_nbr_indirect (abfd, sec);
		  if (isyms == NULL || num == 0)
		    break;
		  /* Record the starting index in the reserved1 field.  */
		  sec->reserved1 = n;
		  for (j = 0; j < num; j++, n++)
		    {
		      if (isyms[j] == NULL)
		        dsym->indirect_syms[n] = BFD_MACH_O_INDIRECT_SYM_LOCAL;
		      else if (isyms[j]->symbol.section == bfd_abs_section_ptr
			       && ! (isyms[j]->n_type & BFD_MACH_O_N_EXT))
		        dsym->indirect_syms[n] = BFD_MACH_O_INDIRECT_SYM_LOCAL
						 | BFD_MACH_O_INDIRECT_SYM_ABS;
		      else
		        dsym->indirect_syms[n] = isyms[j]->symbol.udata.i;
		    }
		}
		break;
	      default:
		break;
	    }
	}
    }

  return TRUE;
}

/* Build Mach-O load commands (currently assuming an MH_OBJECT file).
   TODO: Other file formats, rebuilding symtab/dysymtab commands for strip
   and copy functionality.  */

bfd_boolean
bfd_mach_o_build_commands (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned wide = mach_o_wide_p (&mdata->header);
  int segcmd_idx = -1;
  int symtab_idx = -1;
  int dysymtab_idx = -1;
  unsigned long base_offset = 0;

  /* Return now if commands are already present.  */
  if (mdata->header.ncmds)
    return FALSE;

  /* Fill in the file type, if not already set.  */

  if (mdata->header.filetype == 0)
    {
      if (abfd->flags & EXEC_P)
        mdata->header.filetype = BFD_MACH_O_MH_EXECUTE;
      else if (abfd->flags & DYNAMIC)
        mdata->header.filetype = BFD_MACH_O_MH_DYLIB;
      else
        mdata->header.filetype = BFD_MACH_O_MH_OBJECT;
    }

  /* If hasn't already been done, flatten sections list, and sort
     if/when required.  Must be done before the symbol table is adjusted,
     since that depends on properly numbered sections.  */
  if (mdata->nsects == 0 || mdata->sections == NULL)
    if (! bfd_mach_o_mangle_sections (abfd, mdata))
      return FALSE;

  /* Order the symbol table, fill-in/check mach-o specific fields and
     partition out any indirect symbols.  */
  if (!bfd_mach_o_mangle_symbols (abfd))
    return FALSE;

  /* Very simple command set (only really applicable to MH_OBJECTs):
     All the commands are optional - present only when there is suitable data.
     (i.e. it is valid to have an empty file)

	a command (segment) to contain all the sections,
	command for the symbol table,
	a command for the dysymtab.

     ??? maybe we should assert that this is an MH_OBJECT?  */

  if (mdata->nsects > 0)
    {
      segcmd_idx = 0;
      mdata->header.ncmds = 1;
    }

  if (bfd_get_symcount (abfd) > 0)
    {
      mdata->header.ncmds++;
      symtab_idx = segcmd_idx + 1; /* 0 if the seg command is absent.  */
    }

  /* FIXME:
     This is a rather crude test for whether we should build a dysymtab.  */
  if (bfd_mach_o_should_emit_dysymtab ()
      && bfd_get_symcount (abfd))
    {
      mdata->header.ncmds++;
      /* If there should be a case where a dysymtab could be emitted without
	 a symtab (seems improbable), this would need amending.  */
      dysymtab_idx = symtab_idx + 1;
    }

  if (wide)
    base_offset = BFD_MACH_O_HEADER_64_SIZE;
  else
    base_offset = BFD_MACH_O_HEADER_SIZE;

  /* Well, we must have a header, at least.  */
  mdata->filelen = base_offset;

  /* A bit unusual, but no content is valid;
     as -n empty.s -o empty.o  */
  if (mdata->header.ncmds == 0)
    return TRUE;

  mdata->commands = bfd_zalloc (abfd, mdata->header.ncmds
                                * sizeof (bfd_mach_o_load_command));
  if (mdata->commands == NULL)
    return FALSE;

  if (segcmd_idx >= 0)
    {
      bfd_mach_o_load_command *cmd = &mdata->commands[segcmd_idx];
      bfd_mach_o_segment_command *seg = &cmd->command.segment;

      /* Count the segctions in the special blank segment used for MH_OBJECT.  */
      seg->nsects = bfd_mach_o_count_sections_for_seg (NULL, mdata);
      if (seg->nsects == (unsigned long) -1)
	return FALSE;

      /* Init segment command.  */
      cmd->offset = base_offset;
      if (wide)
	{
	  cmd->type = BFD_MACH_O_LC_SEGMENT_64;
	  cmd->len = BFD_MACH_O_LC_SEGMENT_64_SIZE
			+ BFD_MACH_O_SECTION_64_SIZE * seg->nsects;
	}
      else
	{
	  cmd->type = BFD_MACH_O_LC_SEGMENT;
	  cmd->len = BFD_MACH_O_LC_SEGMENT_SIZE
			+ BFD_MACH_O_SECTION_SIZE * seg->nsects;
	}

      cmd->type_required = FALSE;
      mdata->header.sizeofcmds = cmd->len;
      mdata->filelen += cmd->len;
    }

  if (symtab_idx >= 0)
    {
      /* Init symtab command.  */
      bfd_mach_o_load_command *cmd = &mdata->commands[symtab_idx];

      cmd->type = BFD_MACH_O_LC_SYMTAB;
      cmd->offset = base_offset;
      if (segcmd_idx >= 0)
        cmd->offset += mdata->commands[segcmd_idx].len;

      cmd->len = sizeof (struct mach_o_symtab_command_external)
		 + BFD_MACH_O_LC_SIZE;
      cmd->type_required = FALSE;
      mdata->header.sizeofcmds += cmd->len;
      mdata->filelen += cmd->len;
    }

  /* If required, setup symtab command, see comment above about the quality
     of this test.  */
  if (dysymtab_idx >= 0)
    {
      bfd_mach_o_load_command *cmd = &mdata->commands[dysymtab_idx];

      cmd->type = BFD_MACH_O_LC_DYSYMTAB;
      if (symtab_idx >= 0)
        cmd->offset = mdata->commands[symtab_idx].offset
		    + mdata->commands[symtab_idx].len;
      else if (segcmd_idx >= 0)
        cmd->offset = mdata->commands[segcmd_idx].offset
		    + mdata->commands[segcmd_idx].len;
      else
	cmd->offset = base_offset;

      cmd->type_required = FALSE;
      cmd->len = sizeof (struct mach_o_dysymtab_command_external)
		 + BFD_MACH_O_LC_SIZE;

      mdata->header.sizeofcmds += cmd->len;
      mdata->filelen += cmd->len;
    }

  /* So, now we have sized the commands and the filelen set to that.
     Now we can build the segment command and set the section file offsets.  */
  if (segcmd_idx >= 0
      && ! bfd_mach_o_build_seg_command
		(NULL, mdata, &mdata->commands[segcmd_idx].command.segment))
    return FALSE;

  /* If we're doing a dysymtab, cmd points to its load command.  */
  if (dysymtab_idx >= 0
      && ! bfd_mach_o_build_dysymtab_command (abfd, mdata,
					      &mdata->commands[dysymtab_idx]))
    return FALSE;

  /* The symtab command is filled in when the symtab is written.  */
  return TRUE;
}

/* Set the contents of a section.  */

bfd_boolean
bfd_mach_o_set_section_contents (bfd *abfd,
				 asection *section,
				 const void * location,
				 file_ptr offset,
				 bfd_size_type count)
{
  file_ptr pos;

  /* Trying to write the first section contents will trigger the creation of
     the load commands if they are not already present.  */
  if (! abfd->output_has_begun && ! bfd_mach_o_build_commands (abfd))
    return FALSE;

  if (count == 0)
    return TRUE;

  pos = section->filepos + offset;
  if (bfd_seek (abfd, pos, SEEK_SET) != 0
      || bfd_bwrite (location, count, abfd) != count)
    return FALSE;

  return TRUE;
}

int
bfd_mach_o_sizeof_headers (bfd *a ATTRIBUTE_UNUSED,
			   struct bfd_link_info *info ATTRIBUTE_UNUSED)
{
  return 0;
}

/* Make an empty symbol.  This is required only because
   bfd_make_section_anyway wants to create a symbol for the section.  */

asymbol *
bfd_mach_o_make_empty_symbol (bfd *abfd)
{
  asymbol *new_symbol;

  new_symbol = bfd_zalloc (abfd, sizeof (bfd_mach_o_asymbol));
  if (new_symbol == NULL)
    return new_symbol;
  new_symbol->the_bfd = abfd;
  new_symbol->udata.i = SYM_MACHO_FIELDS_UNSET;
  return new_symbol;
}

static bfd_boolean
bfd_mach_o_read_header (bfd *abfd, bfd_mach_o_header *header)
{
  struct mach_o_header_external raw;
  unsigned int size;
  bfd_vma (*get32) (const void *) = NULL;

  /* Just read the magic number.  */
  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread (raw.magic, sizeof (raw.magic), abfd) != 4)
    return FALSE;

  if (bfd_getb32 (raw.magic) == BFD_MACH_O_MH_MAGIC)
    {
      header->byteorder = BFD_ENDIAN_BIG;
      header->magic = BFD_MACH_O_MH_MAGIC;
      header->version = 1;
      get32 = bfd_getb32;
    }
  else if (bfd_getl32 (raw.magic) == BFD_MACH_O_MH_MAGIC)
    {
      header->byteorder = BFD_ENDIAN_LITTLE;
      header->magic = BFD_MACH_O_MH_MAGIC;
      header->version = 1;
      get32 = bfd_getl32;
    }
  else if (bfd_getb32 (raw.magic) == BFD_MACH_O_MH_MAGIC_64)
    {
      header->byteorder = BFD_ENDIAN_BIG;
      header->magic = BFD_MACH_O_MH_MAGIC_64;
      header->version = 2;
      get32 = bfd_getb32;
    }
  else if (bfd_getl32 (raw.magic) == BFD_MACH_O_MH_MAGIC_64)
    {
      header->byteorder = BFD_ENDIAN_LITTLE;
      header->magic = BFD_MACH_O_MH_MAGIC_64;
      header->version = 2;
      get32 = bfd_getl32;
    }
  else
    {
      header->byteorder = BFD_ENDIAN_UNKNOWN;
      return FALSE;
    }

  /* Once the size of the header is known, read the full header.  */
  size = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread (&raw, size, abfd) != size)
    return FALSE;

  header->cputype = (*get32) (raw.cputype);
  header->cpusubtype = (*get32) (raw.cpusubtype);
  header->filetype = (*get32) (raw.filetype);
  header->ncmds = (*get32) (raw.ncmds);
  header->sizeofcmds = (*get32) (raw.sizeofcmds);
  header->flags = (*get32) (raw.flags);

  if (mach_o_wide_p (header))
    header->reserved = (*get32) (raw.reserved);
  else
    header->reserved = 0;

  return TRUE;
}

bfd_boolean
bfd_mach_o_new_section_hook (bfd *abfd, asection *sec)
{
  bfd_mach_o_section *s;
  unsigned bfdalign = bfd_get_section_alignment (abfd, sec);

  s = bfd_mach_o_get_mach_o_section (sec);
  if (s == NULL)
    {
      flagword bfd_flags;
      static const mach_o_section_name_xlat * xlat;

      s = (bfd_mach_o_section *) bfd_zalloc (abfd, sizeof (*s));
      if (s == NULL)
	return FALSE;
      sec->used_by_bfd = s;
      s->bfdsection = sec;

      /* Create the Darwin seg/sect name pair from the bfd name.
	 If this is a canonical name for which a specific paiting exists
	 there will also be defined flags, type, attribute and alignment
	 values.  */
      xlat = bfd_mach_o_convert_section_name_to_mach_o (abfd, sec, s);
      if (xlat != NULL)
	{
	  s->flags = xlat->macho_sectype | xlat->macho_secattr;
	  s->align = xlat->sectalign > bfdalign ? xlat->sectalign
						: bfdalign;
	  bfd_set_section_alignment (abfd, sec, s->align);
	  bfd_flags = bfd_get_section_flags (abfd, sec);
	  if (bfd_flags == SEC_NO_FLAGS)
	    bfd_set_section_flags (abfd, sec, xlat->bfd_flags);
	}
      else
	/* Create default flags.  */
	bfd_mach_o_set_section_flags_from_bfd (abfd, sec);
    }

  return _bfd_generic_new_section_hook (abfd, sec);
}

static void
bfd_mach_o_init_section_from_mach_o (bfd *abfd, asection *sec,
                                     unsigned long prot)
{
  flagword flags;
  bfd_mach_o_section *section;

  flags = bfd_get_section_flags (abfd, sec);
  section = bfd_mach_o_get_mach_o_section (sec);

  /* TODO: see if we should use the xlat system for doing this by
     preference and fall back to this for unknown sections.  */

  if (flags == SEC_NO_FLAGS)
    {
      /* Try to guess flags.  */
      if (section->flags & BFD_MACH_O_S_ATTR_DEBUG)
        flags = SEC_DEBUGGING;
      else
        {
          flags = SEC_ALLOC;
          if ((section->flags & BFD_MACH_O_SECTION_TYPE_MASK)
              != BFD_MACH_O_S_ZEROFILL)
            {
              flags |= SEC_LOAD;
              if (prot & BFD_MACH_O_PROT_EXECUTE)
                flags |= SEC_CODE;
              if (prot & BFD_MACH_O_PROT_WRITE)
                flags |= SEC_DATA;
              else if (prot & BFD_MACH_O_PROT_READ)
                flags |= SEC_READONLY;
            }
        }
    }
  else
    {
      if ((flags & SEC_DEBUGGING) == 0)
        flags |= SEC_ALLOC;
    }

  if (section->offset != 0)
    flags |= SEC_HAS_CONTENTS;
  if (section->nreloc != 0)
    flags |= SEC_RELOC;

  bfd_set_section_flags (abfd, sec, flags);

  sec->vma = section->addr;
  sec->lma = section->addr;
  sec->size = section->size;
  sec->filepos = section->offset;
  sec->alignment_power = section->align;
  sec->segment_mark = 0;
  sec->reloc_count = section->nreloc;
  sec->rel_filepos = section->reloff;
}

static asection *
bfd_mach_o_make_bfd_section (bfd *abfd,
                             const unsigned char *segname,
                             const unsigned char *sectname)
{
  const char *sname;
  flagword flags;

  bfd_mach_o_convert_section_name_to_bfd
    (abfd, (const char *)segname, (const char *)sectname, &sname, &flags);
  if (sname == NULL)
    return NULL;

  return bfd_make_section_anyway_with_flags (abfd, sname, flags);
}

static asection *
bfd_mach_o_read_section_32 (bfd *abfd,
                            unsigned int offset,
                            unsigned long prot)
{
  struct mach_o_section_32_external raw;
  asection *sec;
  bfd_mach_o_section *section;

  if (bfd_seek (abfd, offset, SEEK_SET) != 0
      || (bfd_bread (&raw, BFD_MACH_O_SECTION_SIZE, abfd)
          != BFD_MACH_O_SECTION_SIZE))
    return NULL;

  sec = bfd_mach_o_make_bfd_section (abfd, raw.segname, raw.sectname);
  if (sec == NULL)
    return NULL;

  section = bfd_mach_o_get_mach_o_section (sec);
  memcpy (section->segname, raw.segname, sizeof (raw.segname));
  section->segname[BFD_MACH_O_SEGNAME_SIZE] = 0;
  memcpy (section->sectname, raw.sectname, sizeof (raw.sectname));
  section->sectname[BFD_MACH_O_SECTNAME_SIZE] = 0;
  section->addr = bfd_h_get_32 (abfd, raw.addr);
  section->size = bfd_h_get_32 (abfd, raw.size);
  section->offset = bfd_h_get_32 (abfd, raw.offset);
  section->align = bfd_h_get_32 (abfd, raw.align);
  section->reloff = bfd_h_get_32 (abfd, raw.reloff);
  section->nreloc = bfd_h_get_32 (abfd, raw.nreloc);
  section->flags = bfd_h_get_32 (abfd, raw.flags);
  section->reserved1 = bfd_h_get_32 (abfd, raw.reserved1);
  section->reserved2 = bfd_h_get_32 (abfd, raw.reserved2);
  section->reserved3 = 0;

  bfd_mach_o_init_section_from_mach_o (abfd, sec, prot);

  return sec;
}

static asection *
bfd_mach_o_read_section_64 (bfd *abfd,
                            unsigned int offset,
                            unsigned long prot)
{
  struct mach_o_section_64_external raw;
  asection *sec;
  bfd_mach_o_section *section;

  if (bfd_seek (abfd, offset, SEEK_SET) != 0
      || (bfd_bread (&raw, BFD_MACH_O_SECTION_64_SIZE, abfd)
          != BFD_MACH_O_SECTION_64_SIZE))
    return NULL;

  sec = bfd_mach_o_make_bfd_section (abfd, raw.segname, raw.sectname);
  if (sec == NULL)
    return NULL;

  section = bfd_mach_o_get_mach_o_section (sec);
  memcpy (section->segname, raw.segname, sizeof (raw.segname));
  section->segname[BFD_MACH_O_SEGNAME_SIZE] = 0;
  memcpy (section->sectname, raw.sectname, sizeof (raw.sectname));
  section->sectname[BFD_MACH_O_SECTNAME_SIZE] = 0;
  section->addr = bfd_h_get_64 (abfd, raw.addr);
  section->size = bfd_h_get_64 (abfd, raw.size);
  section->offset = bfd_h_get_32 (abfd, raw.offset);
  section->align = bfd_h_get_32 (abfd, raw.align);
  section->reloff = bfd_h_get_32 (abfd, raw.reloff);
  section->nreloc = bfd_h_get_32 (abfd, raw.nreloc);
  section->flags = bfd_h_get_32 (abfd, raw.flags);
  section->reserved1 = bfd_h_get_32 (abfd, raw.reserved1);
  section->reserved2 = bfd_h_get_32 (abfd, raw.reserved2);
  section->reserved3 = bfd_h_get_32 (abfd, raw.reserved3);

  bfd_mach_o_init_section_from_mach_o (abfd, sec, prot);

  return sec;
}

static asection *
bfd_mach_o_read_section (bfd *abfd,
                         unsigned int offset,
                         unsigned long prot,
                         unsigned int wide)
{
  if (wide)
    return bfd_mach_o_read_section_64 (abfd, offset, prot);
  else
    return bfd_mach_o_read_section_32 (abfd, offset, prot);
}

static bfd_boolean
bfd_mach_o_read_symtab_symbol (bfd *abfd,
                               bfd_mach_o_symtab_command *sym,
                               bfd_mach_o_asymbol *s,
                               unsigned long i)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned int wide = mach_o_wide_p (&mdata->header);
  unsigned int symwidth =
    wide ? BFD_MACH_O_NLIST_64_SIZE : BFD_MACH_O_NLIST_SIZE;
  unsigned int symoff = sym->symoff + (i * symwidth);
  struct mach_o_nlist_64_external raw;
  unsigned char type = -1;
  unsigned char section = -1;
  short desc = -1;
  symvalue value = -1;
  unsigned long stroff = -1;
  unsigned int symtype = -1;

  BFD_ASSERT (sym->strtab != NULL);

  if (bfd_seek (abfd, symoff, SEEK_SET) != 0
      || bfd_bread (&raw, symwidth, abfd) != symwidth)
    {
      (*_bfd_error_handler)
        (_("bfd_mach_o_read_symtab_symbol: unable to read %d bytes at %lu"),
         symwidth, (unsigned long) symoff);
      return FALSE;
    }

  stroff = bfd_h_get_32 (abfd, raw.n_strx);
  type = bfd_h_get_8 (abfd, raw.n_type);
  symtype = type & BFD_MACH_O_N_TYPE;
  section = bfd_h_get_8 (abfd, raw.n_sect);
  desc = bfd_h_get_16 (abfd, raw.n_desc);
  if (wide)
    value = bfd_h_get_64 (abfd, raw.n_value);
  else
    value = bfd_h_get_32 (abfd, raw.n_value);

  if (stroff >= sym->strsize)
    {
      (*_bfd_error_handler)
        (_("bfd_mach_o_read_symtab_symbol: name out of range (%lu >= %lu)"),
         (unsigned long) stroff,
         (unsigned long) sym->strsize);
      return FALSE;
    }

  s->symbol.the_bfd = abfd;
  s->symbol.name = sym->strtab + stroff;
  s->symbol.value = value;
  s->symbol.flags = 0x0;
  s->symbol.udata.i = i;
  s->n_type = type;
  s->n_sect = section;
  s->n_desc = desc;

  if (type & BFD_MACH_O_N_STAB)
    {
      s->symbol.flags |= BSF_DEBUGGING;
      s->symbol.section = bfd_und_section_ptr;
      switch (type)
	{
	case N_FUN:
	case N_STSYM:
	case N_LCSYM:
	case N_BNSYM:
	case N_SLINE:
	case N_ENSYM:
	case N_ECOMM:
	case N_ECOML:
	case N_GSYM:
	  if ((section > 0) && (section <= mdata->nsects))
	    {
	      s->symbol.section = mdata->sections[section - 1]->bfdsection;
	      s->symbol.value =
                s->symbol.value - mdata->sections[section - 1]->addr;
	    }
	  break;
	}
    }
  else
    {
      if (type & (BFD_MACH_O_N_PEXT | BFD_MACH_O_N_EXT))
	s->symbol.flags |= BSF_GLOBAL;
      else
	s->symbol.flags |= BSF_LOCAL;

      switch (symtype)
	{
	case BFD_MACH_O_N_UNDF:
          if (type == (BFD_MACH_O_N_UNDF | BFD_MACH_O_N_EXT)
              && s->symbol.value != 0)
            {
              /* A common symbol.  */
              s->symbol.section = bfd_com_section_ptr;
              s->symbol.flags = BSF_NO_FLAGS;
            }
          else
            {
              s->symbol.section = bfd_und_section_ptr;
              if (s->n_desc & BFD_MACH_O_N_WEAK_REF)
                s->symbol.flags |= BSF_WEAK;
            }
	  break;
	case BFD_MACH_O_N_PBUD:
	  s->symbol.section = bfd_und_section_ptr;
	  break;
	case BFD_MACH_O_N_ABS:
	  s->symbol.section = bfd_abs_section_ptr;
	  break;
	case BFD_MACH_O_N_SECT:
	  if ((section > 0) && (section <= mdata->nsects))
	    {
	      s->symbol.section = mdata->sections[section - 1]->bfdsection;
	      s->symbol.value =
                s->symbol.value - mdata->sections[section - 1]->addr;
	    }
	  else
	    {
	      /* Mach-O uses 0 to mean "no section"; not an error.  */
	      if (section != 0)
		{
		  (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: "
					   "symbol \"%s\" specified invalid section %d (max %lu): setting to undefined"),
					 s->symbol.name, section, mdata->nsects);
		}
	      s->symbol.section = bfd_und_section_ptr;
	    }
	  break;
	case BFD_MACH_O_N_INDR:
	  /* FIXME: we don't follow the BFD convention as this indirect symbol
	     won't be followed by the referenced one.  This looks harmless
	     unless we start using the linker.	*/
	  s->symbol.flags |= BSF_INDIRECT;
	  s->symbol.section = bfd_ind_section_ptr;
	  s->symbol.value = 0;
	  break;
	default:
	  (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbol: "
				   "symbol \"%s\" specified invalid type field 0x%x: setting to undefined"),
				 s->symbol.name, symtype);
	  s->symbol.section = bfd_und_section_ptr;
	  break;
	}
    }

  return TRUE;
}

bfd_boolean
bfd_mach_o_read_symtab_strtab (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;

  /* Fail if there is no symtab.  */
  if (sym == NULL)
    return FALSE;

  /* Success if already loaded.  */
  if (sym->strtab)
    return TRUE;

  if (abfd->flags & BFD_IN_MEMORY)
    {
      struct bfd_in_memory *b;

      b = (struct bfd_in_memory *) abfd->iostream;

      if ((sym->stroff + sym->strsize) > b->size)
	{
	  bfd_set_error (bfd_error_file_truncated);
	  return FALSE;
	}
      sym->strtab = (char *) b->buffer + sym->stroff;
    }
  else
    {
      sym->strtab = bfd_alloc (abfd, sym->strsize);
      if (sym->strtab == NULL)
        return FALSE;

      if (bfd_seek (abfd, sym->stroff, SEEK_SET) != 0
          || bfd_bread (sym->strtab, sym->strsize, abfd) != sym->strsize)
        {
          bfd_set_error (bfd_error_file_truncated);
          return FALSE;
        }
    }

  return TRUE;
}

bfd_boolean
bfd_mach_o_read_symtab_symbols (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_symtab_command *sym = mdata->symtab;
  unsigned long i;

  if (sym == NULL || sym->symbols)
    {
      /* Return now if there are no symbols or if already loaded.  */
      return TRUE;
    }

  sym->symbols = bfd_alloc (abfd, sym->nsyms * sizeof (bfd_mach_o_asymbol));

  if (sym->symbols == NULL)
    {
      (*_bfd_error_handler) (_("bfd_mach_o_read_symtab_symbols: unable to allocate memory for symbols"));
      return FALSE;
    }

  if (!bfd_mach_o_read_symtab_strtab (abfd))
    return FALSE;

  for (i = 0; i < sym->nsyms; i++)
    {
      if (!bfd_mach_o_read_symtab_symbol (abfd, sym, &sym->symbols[i], i))
	return FALSE;
    }

  return TRUE;
}

static const char *
bfd_mach_o_i386_flavour_string (unsigned int flavour)
{
  switch ((int) flavour)
    {
    case BFD_MACH_O_x86_THREAD_STATE32:    return "x86_THREAD_STATE32";
    case BFD_MACH_O_x86_FLOAT_STATE32:     return "x86_FLOAT_STATE32";
    case BFD_MACH_O_x86_EXCEPTION_STATE32: return "x86_EXCEPTION_STATE32";
    case BFD_MACH_O_x86_THREAD_STATE64:    return "x86_THREAD_STATE64";
    case BFD_MACH_O_x86_FLOAT_STATE64:     return "x86_FLOAT_STATE64";
    case BFD_MACH_O_x86_EXCEPTION_STATE64: return "x86_EXCEPTION_STATE64";
    case BFD_MACH_O_x86_THREAD_STATE:      return "x86_THREAD_STATE";
    case BFD_MACH_O_x86_FLOAT_STATE:       return "x86_FLOAT_STATE";
    case BFD_MACH_O_x86_EXCEPTION_STATE:   return "x86_EXCEPTION_STATE";
    case BFD_MACH_O_x86_DEBUG_STATE32:     return "x86_DEBUG_STATE32";
    case BFD_MACH_O_x86_DEBUG_STATE64:     return "x86_DEBUG_STATE64";
    case BFD_MACH_O_x86_DEBUG_STATE:       return "x86_DEBUG_STATE";
    case BFD_MACH_O_x86_THREAD_STATE_NONE: return "x86_THREAD_STATE_NONE";
    default: return "UNKNOWN";
    }
}

static const char *
bfd_mach_o_ppc_flavour_string (unsigned int flavour)
{
  switch ((int) flavour)
    {
    case BFD_MACH_O_PPC_THREAD_STATE:      return "PPC_THREAD_STATE";
    case BFD_MACH_O_PPC_FLOAT_STATE:       return "PPC_FLOAT_STATE";
    case BFD_MACH_O_PPC_EXCEPTION_STATE:   return "PPC_EXCEPTION_STATE";
    case BFD_MACH_O_PPC_VECTOR_STATE:      return "PPC_VECTOR_STATE";
    case BFD_MACH_O_PPC_THREAD_STATE64:    return "PPC_THREAD_STATE64";
    case BFD_MACH_O_PPC_EXCEPTION_STATE64: return "PPC_EXCEPTION_STATE64";
    default: return "UNKNOWN";
    }
}

static int
bfd_mach_o_read_dylinker (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dylinker_command *cmd = &command->command.dylinker;
  struct mach_o_str_command_external raw;
  unsigned int nameoff;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_ID_DYLINKER)
	      || (command->type == BFD_MACH_O_LC_LOAD_DYLINKER));

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  nameoff = bfd_h_get_32 (abfd, raw.str);

  cmd->name_offset = command->offset + nameoff;
  cmd->name_len = command->len - nameoff;
  cmd->name_str = bfd_alloc (abfd, cmd->name_len);
  if (cmd->name_str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->name_offset, SEEK_SET) != 0
      || bfd_bread (cmd->name_str, cmd->name_len, abfd) != cmd->name_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_dylib (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dylib_command *cmd = &command->command.dylib;
  struct mach_o_dylib_command_external raw;
  unsigned int nameoff;

  switch (command->type)
    {
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB:
    case BFD_MACH_O_LC_REEXPORT_DYLIB:
    case BFD_MACH_O_LC_LOAD_UPWARD_DYLIB:
      break;
    default:
      BFD_FAIL ();
      return -1;
    }

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  nameoff = bfd_h_get_32 (abfd, raw.name);
  cmd->timestamp = bfd_h_get_32 (abfd, raw.timestamp);
  cmd->current_version = bfd_h_get_32 (abfd, raw.current_version);
  cmd->compatibility_version = bfd_h_get_32 (abfd, raw.compatibility_version);

  cmd->name_offset = command->offset + nameoff;
  cmd->name_len = command->len - nameoff;
  cmd->name_str = bfd_alloc (abfd, cmd->name_len);
  if (cmd->name_str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->name_offset, SEEK_SET) != 0
      || bfd_bread (cmd->name_str, cmd->name_len, abfd) != cmd->name_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_prebound_dylib (bfd *abfd ATTRIBUTE_UNUSED,
                                bfd_mach_o_load_command *command ATTRIBUTE_UNUSED)
{
  /* bfd_mach_o_prebound_dylib_command *cmd = &command->command.prebound_dylib; */

  BFD_ASSERT (command->type == BFD_MACH_O_LC_PREBOUND_DYLIB);
  return 0;
}

static int
bfd_mach_o_read_fvmlib (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_fvmlib_command *fvm = &command->command.fvmlib;
  struct mach_o_fvmlib_command_external raw;
  unsigned int nameoff;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  nameoff = bfd_h_get_32 (abfd, raw.name);
  fvm->minor_version = bfd_h_get_32 (abfd, raw.minor_version);
  fvm->header_addr = bfd_h_get_32 (abfd, raw.header_addr);

  fvm->name_offset = command->offset + nameoff;
  fvm->name_len = command->len - nameoff;
  fvm->name_str = bfd_alloc (abfd, fvm->name_len);
  if (fvm->name_str == NULL)
    return -1;
  if (bfd_seek (abfd, fvm->name_offset, SEEK_SET) != 0
      || bfd_bread (fvm->name_str, fvm->name_len, abfd) != fvm->name_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_thread (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_thread_command *cmd = &command->command.thread;
  unsigned int offset;
  unsigned int nflavours;
  unsigned int i;

  BFD_ASSERT ((command->type == BFD_MACH_O_LC_THREAD)
	      || (command->type == BFD_MACH_O_LC_UNIXTHREAD));

  /* Count the number of threads.  */
  offset = 8;
  nflavours = 0;
  while (offset != command->len)
    {
      struct mach_o_thread_command_external raw;

      if (offset >= command->len)
	return -1;

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
	return -1;

      offset += sizeof (raw) + bfd_h_get_32 (abfd, raw.count) * 4;
      nflavours++;
    }

  /* Allocate threads.  */
  cmd->flavours = bfd_alloc
    (abfd, nflavours * sizeof (bfd_mach_o_thread_flavour));
  if (cmd->flavours == NULL)
    return -1;
  cmd->nflavours = nflavours;

  offset = 8;
  nflavours = 0;
  while (offset != command->len)
    {
      struct mach_o_thread_command_external raw;

      if (offset >= command->len)
	return -1;

      if (nflavours >= cmd->nflavours)
	return -1;

      if (bfd_seek (abfd, command->offset + offset, SEEK_SET) != 0
          || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
	return -1;

      cmd->flavours[nflavours].flavour = bfd_h_get_32 (abfd, raw.flavour);
      cmd->flavours[nflavours].offset = command->offset + offset + sizeof (raw);
      cmd->flavours[nflavours].size = bfd_h_get_32 (abfd, raw.count) * 4;
      offset += cmd->flavours[nflavours].size + sizeof (raw);
      nflavours++;
    }

  for (i = 0; i < nflavours; i++)
    {
      asection *bfdsec;
      unsigned int snamelen;
      char *sname;
      const char *flavourstr;
      const char *prefix = "LC_THREAD";
      unsigned int j = 0;

      switch (mdata->header.cputype)
	{
	case BFD_MACH_O_CPU_TYPE_POWERPC:
	case BFD_MACH_O_CPU_TYPE_POWERPC_64:
	  flavourstr = bfd_mach_o_ppc_flavour_string (cmd->flavours[i].flavour);
	  break;
	case BFD_MACH_O_CPU_TYPE_I386:
	case BFD_MACH_O_CPU_TYPE_X86_64:
	  flavourstr = bfd_mach_o_i386_flavour_string (cmd->flavours[i].flavour);
	  break;
	default:
	  flavourstr = "UNKNOWN_ARCHITECTURE";
	  break;
	}

      snamelen = strlen (prefix) + 1 + 20 + 1 + strlen (flavourstr) + 1;
      sname = bfd_alloc (abfd, snamelen);
      if (sname == NULL)
	return -1;

      for (;;)
	{
	  sprintf (sname, "%s.%s.%u", prefix, flavourstr, j);
	  if (bfd_get_section_by_name (abfd, sname) == NULL)
	    break;
	  j++;
	}

      bfdsec = bfd_make_section_with_flags (abfd, sname, SEC_HAS_CONTENTS);

      bfdsec->vma = 0;
      bfdsec->lma = 0;
      bfdsec->size = cmd->flavours[i].size;
      bfdsec->filepos = cmd->flavours[i].offset;
      bfdsec->alignment_power = 0x0;

      cmd->section = bfdsec;
    }

  return 0;
}

static int
bfd_mach_o_read_dysymtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dysymtab_command *cmd = &command->command.dysymtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);

  BFD_ASSERT (command->type == BFD_MACH_O_LC_DYSYMTAB);

  {
    struct mach_o_dysymtab_command_external raw;

    if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
        || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
      return -1;

    cmd->ilocalsym = bfd_h_get_32 (abfd, raw.ilocalsym);
    cmd->nlocalsym = bfd_h_get_32 (abfd, raw.nlocalsym);
    cmd->iextdefsym = bfd_h_get_32 (abfd, raw.iextdefsym);
    cmd->nextdefsym = bfd_h_get_32 (abfd, raw.nextdefsym);
    cmd->iundefsym = bfd_h_get_32 (abfd, raw.iundefsym);
    cmd->nundefsym = bfd_h_get_32 (abfd, raw.nundefsym);
    cmd->tocoff = bfd_h_get_32 (abfd, raw.tocoff);
    cmd->ntoc = bfd_h_get_32 (abfd, raw.ntoc);
    cmd->modtaboff = bfd_h_get_32 (abfd, raw.modtaboff);
    cmd->nmodtab = bfd_h_get_32 (abfd, raw.nmodtab);
    cmd->extrefsymoff = bfd_h_get_32 (abfd, raw.extrefsymoff);
    cmd->nextrefsyms = bfd_h_get_32 (abfd, raw.nextrefsyms);
    cmd->indirectsymoff = bfd_h_get_32 (abfd, raw.indirectsymoff);
    cmd->nindirectsyms = bfd_h_get_32 (abfd, raw.nindirectsyms);
    cmd->extreloff = bfd_h_get_32 (abfd, raw.extreloff);
    cmd->nextrel = bfd_h_get_32 (abfd, raw.nextrel);
    cmd->locreloff = bfd_h_get_32 (abfd, raw.locreloff);
    cmd->nlocrel = bfd_h_get_32 (abfd, raw.nlocrel);
  }

  if (cmd->nmodtab != 0)
    {
      unsigned int i;
      int wide = bfd_mach_o_wide_p (abfd);
      unsigned int module_len = wide ? 56 : 52;

      cmd->dylib_module =
        bfd_alloc (abfd, cmd->nmodtab * sizeof (bfd_mach_o_dylib_module));
      if (cmd->dylib_module == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->modtaboff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nmodtab; i++)
        {
          bfd_mach_o_dylib_module *module = &cmd->dylib_module[i];
          unsigned long v;
          unsigned char buf[56];

          if (bfd_bread ((void *) buf, module_len, abfd) != module_len)
            return -1;

          module->module_name_idx = bfd_h_get_32 (abfd, buf + 0);
          module->iextdefsym = bfd_h_get_32 (abfd, buf + 4);
          module->nextdefsym = bfd_h_get_32 (abfd, buf + 8);
          module->irefsym = bfd_h_get_32 (abfd, buf + 12);
          module->nrefsym = bfd_h_get_32 (abfd, buf + 16);
          module->ilocalsym = bfd_h_get_32 (abfd, buf + 20);
          module->nlocalsym = bfd_h_get_32 (abfd, buf + 24);
          module->iextrel = bfd_h_get_32 (abfd, buf + 28);
          module->nextrel = bfd_h_get_32 (abfd, buf + 32);
          v = bfd_h_get_32 (abfd, buf +36);
          module->iinit = v & 0xffff;
          module->iterm = (v >> 16) & 0xffff;
          v = bfd_h_get_32 (abfd, buf + 40);
          module->ninit = v & 0xffff;
          module->nterm = (v >> 16) & 0xffff;
          if (wide)
            {
              module->objc_module_info_size = bfd_h_get_32 (abfd, buf + 44);
              module->objc_module_info_addr = bfd_h_get_64 (abfd, buf + 48);
            }
          else
            {
              module->objc_module_info_addr = bfd_h_get_32 (abfd, buf + 44);
              module->objc_module_info_size = bfd_h_get_32 (abfd, buf + 48);
            }
        }
    }

  if (cmd->ntoc != 0)
    {
      unsigned int i;

      cmd->dylib_toc = bfd_alloc
        (abfd, cmd->ntoc * sizeof (bfd_mach_o_dylib_table_of_content));
      if (cmd->dylib_toc == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->tocoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->ntoc; i++)
        {
          struct mach_o_dylib_table_of_contents_external raw;
          bfd_mach_o_dylib_table_of_content *toc = &cmd->dylib_toc[i];

          if (bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
            return -1;

          toc->symbol_index = bfd_h_get_32 (abfd, raw.symbol_index);
          toc->module_index = bfd_h_get_32 (abfd, raw.module_index);
        }
    }

  if (cmd->nindirectsyms != 0)
    {
      unsigned int i;

      cmd->indirect_syms = bfd_alloc
        (abfd, cmd->nindirectsyms * sizeof (unsigned int));
      if (cmd->indirect_syms == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->indirectsymoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nindirectsyms; i++)
        {
          unsigned char raw[4];
          unsigned int *is = &cmd->indirect_syms[i];

          if (bfd_bread (raw, sizeof (raw), abfd) != sizeof (raw))
            return -1;

          *is = bfd_h_get_32 (abfd, raw);
        }
    }

  if (cmd->nextrefsyms != 0)
    {
      unsigned long v;
      unsigned int i;

      cmd->ext_refs = bfd_alloc
        (abfd, cmd->nextrefsyms * sizeof (bfd_mach_o_dylib_reference));
      if (cmd->ext_refs == NULL)
        return -1;

      if (bfd_seek (abfd, cmd->extrefsymoff, SEEK_SET) != 0)
        return -1;

      for (i = 0; i < cmd->nextrefsyms; i++)
        {
          unsigned char raw[4];
          bfd_mach_o_dylib_reference *ref = &cmd->ext_refs[i];

          if (bfd_bread (raw, sizeof (raw), abfd) != sizeof (raw))
            return -1;

          /* Fields isym and flags are written as bit-fields, thus we need
             a specific processing for endianness.  */
          v = bfd_h_get_32 (abfd, raw);
          if (bfd_big_endian (abfd))
            {
              ref->isym = (v >> 8) & 0xffffff;
              ref->flags = v & 0xff;
            }
          else
            {
              ref->isym = v & 0xffffff;
              ref->flags = (v >> 24) & 0xff;
            }
        }
    }

  if (mdata->dysymtab)
    return -1;
  mdata->dysymtab = cmd;

  return 0;
}

static int
bfd_mach_o_read_symtab (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_symtab_command *symtab = &command->command.symtab;
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  struct mach_o_symtab_command_external raw;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_SYMTAB);

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  symtab->symoff = bfd_h_get_32 (abfd, raw.symoff);
  symtab->nsyms = bfd_h_get_32 (abfd, raw.nsyms);
  symtab->stroff = bfd_h_get_32 (abfd, raw.stroff);
  symtab->strsize = bfd_h_get_32 (abfd, raw.strsize);
  symtab->symbols = NULL;
  symtab->strtab = NULL;

  if (symtab->nsyms != 0)
    abfd->flags |= HAS_SYMS;

  if (mdata->symtab)
    return -1;
  mdata->symtab = symtab;
  return 0;
}

static int
bfd_mach_o_read_uuid (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_uuid_command *cmd = &command->command.uuid;

  BFD_ASSERT (command->type == BFD_MACH_O_LC_UUID);

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (cmd->uuid, 16, abfd) != 16)
    return -1;

  return 0;
}

static int
bfd_mach_o_read_linkedit (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_linkedit_command *cmd = &command->command.linkedit;
  struct mach_o_linkedit_data_command_external raw;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  cmd->dataoff = bfd_get_32 (abfd, raw.dataoff);
  cmd->datasize = bfd_get_32 (abfd, raw.datasize);
  return 0;
}

static int
bfd_mach_o_read_str (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_str_command *cmd = &command->command.str;
  struct mach_o_str_command_external raw;
  unsigned long off;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  off = bfd_get_32 (abfd, raw.str);
  cmd->stroff = command->offset + off;
  cmd->str_len = command->len - off;
  cmd->str = bfd_alloc (abfd, cmd->str_len);
  if (cmd->str == NULL)
    return -1;
  if (bfd_seek (abfd, cmd->stroff, SEEK_SET) != 0
      || bfd_bread ((void *) cmd->str, cmd->str_len, abfd) != cmd->str_len)
    return -1;
  return 0;
}

static int
bfd_mach_o_read_dyld_info (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_dyld_info_command *cmd = &command->command.dyld_info;
  struct mach_o_dyld_info_command_external raw;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return -1;

  cmd->rebase_off = bfd_get_32 (abfd, raw.rebase_off);
  cmd->rebase_size = bfd_get_32 (abfd, raw.rebase_size);
  cmd->bind_off = bfd_get_32 (abfd, raw.bind_off);
  cmd->bind_size = bfd_get_32 (abfd, raw.bind_size);
  cmd->weak_bind_off = bfd_get_32 (abfd, raw.weak_bind_off);
  cmd->weak_bind_size = bfd_get_32 (abfd, raw.weak_bind_size);
  cmd->lazy_bind_off = bfd_get_32 (abfd, raw.lazy_bind_off);
  cmd->lazy_bind_size = bfd_get_32 (abfd, raw.lazy_bind_size);
  cmd->export_off = bfd_get_32 (abfd, raw.export_off);
  cmd->export_size = bfd_get_32 (abfd, raw.export_size);
  return 0;
}

static bfd_boolean
bfd_mach_o_read_version_min (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_version_min_command *cmd = &command->command.version_min;
  struct mach_o_version_min_command_external raw;
  unsigned int ver;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return FALSE;

  ver = bfd_get_32 (abfd, raw.version);
  cmd->rel = ver >> 16;
  cmd->maj = ver >> 8;
  cmd->min = ver;
  cmd->reserved = bfd_get_32 (abfd, raw.reserved);
  return TRUE;
}

static bfd_boolean
bfd_mach_o_read_encryption_info (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_encryption_info_command *cmd = &command->command.encryption_info;
  struct mach_o_encryption_info_command_external raw;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return FALSE;

  cmd->cryptoff = bfd_get_32 (abfd, raw.cryptoff);
  cmd->cryptsize = bfd_get_32 (abfd, raw.cryptsize);
  cmd->cryptid = bfd_get_32 (abfd, raw.cryptid);
  return TRUE;
}

static bfd_boolean
bfd_mach_o_read_main (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_main_command *cmd = &command->command.main;
  struct mach_o_entry_point_command_external raw;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return FALSE;

  cmd->entryoff = bfd_get_64 (abfd, raw.entryoff);
  cmd->stacksize = bfd_get_64 (abfd, raw.stacksize);
  return TRUE;
}

static bfd_boolean
bfd_mach_o_read_source_version (bfd *abfd, bfd_mach_o_load_command *command)
{
  bfd_mach_o_source_version_command *cmd = &command->command.source_version;
  struct mach_o_source_version_command_external raw;
  bfd_uint64_t ver;

  if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
      || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
    return FALSE;

  ver = bfd_get_64 (abfd, raw.version);
  /* Note: we use a serie of shift to avoid shift > 32 (for which gcc
     generates warnings) in case of the host doesn't support 64 bit
     integers.  */
  cmd->e = ver & 0x3ff;
  ver >>= 10;
  cmd->d = ver & 0x3ff;
  ver >>= 10;
  cmd->c = ver & 0x3ff;
  ver >>= 10;
  cmd->b = ver & 0x3ff;
  ver >>= 10;
  cmd->a = ver & 0xffffff;
  return TRUE;
}

static int
bfd_mach_o_read_segment (bfd *abfd,
                         bfd_mach_o_load_command *command,
                         unsigned int wide)
{
  bfd_mach_o_segment_command *seg = &command->command.segment;
  unsigned long i;

  if (wide)
    {
      struct mach_o_segment_command_64_external raw;

      BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT_64);

      if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
          || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
        return -1;

      memcpy (seg->segname, raw.segname, 16);
      seg->segname[16] = '\0';

      seg->vmaddr = bfd_h_get_64 (abfd, raw.vmaddr);
      seg->vmsize = bfd_h_get_64 (abfd, raw.vmsize);
      seg->fileoff = bfd_h_get_64 (abfd, raw.fileoff);
      seg->filesize = bfd_h_get_64 (abfd, raw.filesize);
      seg->maxprot = bfd_h_get_32 (abfd, raw.maxprot);
      seg->initprot = bfd_h_get_32 (abfd, raw.initprot);
      seg->nsects = bfd_h_get_32 (abfd, raw.nsects);
      seg->flags = bfd_h_get_32 (abfd, raw.flags);
    }
  else
    {
      struct mach_o_segment_command_32_external raw;

      BFD_ASSERT (command->type == BFD_MACH_O_LC_SEGMENT);

      if (bfd_seek (abfd, command->offset + BFD_MACH_O_LC_SIZE, SEEK_SET) != 0
          || bfd_bread (&raw, sizeof (raw), abfd) != sizeof (raw))
        return -1;

      memcpy (seg->segname, raw.segname, 16);
      seg->segname[16] = '\0';

      seg->vmaddr = bfd_h_get_32 (abfd, raw.vmaddr);
      seg->vmsize = bfd_h_get_32 (abfd, raw.vmsize);
      seg->fileoff = bfd_h_get_32 (abfd, raw.fileoff);
      seg->filesize = bfd_h_get_32 (abfd, raw.filesize);
      seg->maxprot = bfd_h_get_32 (abfd, raw.maxprot);
      seg->initprot = bfd_h_get_32 (abfd, raw.initprot);
      seg->nsects = bfd_h_get_32 (abfd, raw.nsects);
      seg->flags = bfd_h_get_32 (abfd, raw.flags);
    }
  seg->sect_head = NULL;
  seg->sect_tail = NULL;

  for (i = 0; i < seg->nsects; i++)
    {
      bfd_vma segoff;
      asection *sec;

      if (wide)
        segoff = command->offset + BFD_MACH_O_LC_SEGMENT_64_SIZE
          + (i * BFD_MACH_O_SECTION_64_SIZE);
      else
        segoff = command->offset + BFD_MACH_O_LC_SEGMENT_SIZE
          + (i * BFD_MACH_O_SECTION_SIZE);

      sec = bfd_mach_o_read_section (abfd, segoff, seg->initprot, wide);
      if (sec == NULL)
        return -1;

      bfd_mach_o_append_section_to_segment (seg, sec);
    }

  return 0;
}

static int
bfd_mach_o_read_segment_32 (bfd *abfd, bfd_mach_o_load_command *command)
{
  return bfd_mach_o_read_segment (abfd, command, 0);
}

static int
bfd_mach_o_read_segment_64 (bfd *abfd, bfd_mach_o_load_command *command)
{
  return bfd_mach_o_read_segment (abfd, command, 1);
}

static int
bfd_mach_o_read_command (bfd *abfd, bfd_mach_o_load_command *command)
{
  struct mach_o_load_command_external raw;
  unsigned int cmd;

  /* Read command type and length.  */
  if (bfd_seek (abfd, command->offset, SEEK_SET) != 0
      || bfd_bread (&raw, BFD_MACH_O_LC_SIZE, abfd) != BFD_MACH_O_LC_SIZE)
    return -1;

  cmd = bfd_h_get_32 (abfd, raw.cmd);
  command->type =  cmd & ~BFD_MACH_O_LC_REQ_DYLD;
  command->type_required = cmd & BFD_MACH_O_LC_REQ_DYLD ? TRUE : FALSE;
  command->len = bfd_h_get_32 (abfd, raw.cmdsize);

  switch (command->type)
    {
    case BFD_MACH_O_LC_SEGMENT:
      if (bfd_mach_o_read_segment_32 (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SEGMENT_64:
      if (bfd_mach_o_read_segment_64 (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SYMTAB:
      if (bfd_mach_o_read_symtab (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_SYMSEG:
      break;
    case BFD_MACH_O_LC_THREAD:
    case BFD_MACH_O_LC_UNIXTHREAD:
      if (bfd_mach_o_read_thread (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOAD_DYLINKER:
    case BFD_MACH_O_LC_ID_DYLINKER:
      if (bfd_mach_o_read_dylinker (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOAD_DYLIB:
    case BFD_MACH_O_LC_ID_DYLIB:
    case BFD_MACH_O_LC_LOAD_WEAK_DYLIB:
    case BFD_MACH_O_LC_REEXPORT_DYLIB:
    case BFD_MACH_O_LC_LOAD_UPWARD_DYLIB:
      if (bfd_mach_o_read_dylib (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_PREBOUND_DYLIB:
      if (bfd_mach_o_read_prebound_dylib (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_LOADFVMLIB:
    case BFD_MACH_O_LC_IDFVMLIB:
      if (bfd_mach_o_read_fvmlib (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_IDENT:
    case BFD_MACH_O_LC_FVMFILE:
    case BFD_MACH_O_LC_PREPAGE:
    case BFD_MACH_O_LC_ROUTINES:
    case BFD_MACH_O_LC_ROUTINES_64:
      break;
    case BFD_MACH_O_LC_SUB_FRAMEWORK:
    case BFD_MACH_O_LC_SUB_UMBRELLA:
    case BFD_MACH_O_LC_SUB_LIBRARY:
    case BFD_MACH_O_LC_SUB_CLIENT:
    case BFD_MACH_O_LC_RPATH:
      if (bfd_mach_o_read_str (abfd, command) != 0)
        return -1;
      break;
    case BFD_MACH_O_LC_DYSYMTAB:
      if (bfd_mach_o_read_dysymtab (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_TWOLEVEL_HINTS:
    case BFD_MACH_O_LC_PREBIND_CKSUM:
      break;
    case BFD_MACH_O_LC_UUID:
      if (bfd_mach_o_read_uuid (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_CODE_SIGNATURE:
    case BFD_MACH_O_LC_SEGMENT_SPLIT_INFO:
    case BFD_MACH_O_LC_FUNCTION_STARTS:
    case BFD_MACH_O_LC_DATA_IN_CODE:
    case BFD_MACH_O_LC_DYLIB_CODE_SIGN_DRS:
      if (bfd_mach_o_read_linkedit (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_ENCRYPTION_INFO:
      if (!bfd_mach_o_read_encryption_info (abfd, command))
	return -1;
      break;
    case BFD_MACH_O_LC_DYLD_INFO:
      if (bfd_mach_o_read_dyld_info (abfd, command) != 0)
	return -1;
      break;
    case BFD_MACH_O_LC_VERSION_MIN_MACOSX:
    case BFD_MACH_O_LC_VERSION_MIN_IPHONEOS:
      if (!bfd_mach_o_read_version_min (abfd, command))
	return -1;
      break;
    case BFD_MACH_O_LC_MAIN:
      if (!bfd_mach_o_read_main (abfd, command))
	return -1;
      break;
    case BFD_MACH_O_LC_SOURCE_VERSION:
      if (!bfd_mach_o_read_source_version (abfd, command))
	return -1;
      break;
    default:
      (*_bfd_error_handler)(_("%B: unknown load command 0x%lx"),
         abfd, (unsigned long) command->type);
      break;
    }

  return 0;
}

static void
bfd_mach_o_flatten_sections (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  long csect = 0;
  unsigned long i;

  /* Count total number of sections.  */
  mdata->nsects = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      if (mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT
	  || mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT_64)
	{
	  bfd_mach_o_segment_command *seg;

	  seg = &mdata->commands[i].command.segment;
	  mdata->nsects += seg->nsects;
	}
    }

  /* Allocate sections array.  */
  mdata->sections = bfd_alloc (abfd,
			       mdata->nsects * sizeof (bfd_mach_o_section *));

  /* Fill the array.  */
  csect = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      if (mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT
	  || mdata->commands[i].type == BFD_MACH_O_LC_SEGMENT_64)
	{
	  bfd_mach_o_segment_command *seg;
          bfd_mach_o_section *sec;

	  seg = &mdata->commands[i].command.segment;
	  BFD_ASSERT (csect + seg->nsects <= mdata->nsects);

          for (sec = seg->sect_head; sec != NULL; sec = sec->next)
	    mdata->sections[csect++] = sec;
	}
    }
}

static bfd_boolean
bfd_mach_o_scan_start_address (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  bfd_mach_o_thread_command *cmd = NULL;
  unsigned long i;

  for (i = 0; i < mdata->header.ncmds; i++)
    if ((mdata->commands[i].type == BFD_MACH_O_LC_THREAD) ||
        (mdata->commands[i].type == BFD_MACH_O_LC_UNIXTHREAD))
      {
        cmd = &mdata->commands[i].command.thread;
        break;
      }
    else if (mdata->commands[i].type == BFD_MACH_O_LC_MAIN
	     && mdata->nsects > 1)
      {
	bfd_mach_o_main_command *main_cmd = &mdata->commands[i].command.main;
	bfd_mach_o_section *text_sect = mdata->sections[0];
	if (text_sect)
	  {
	    abfd->start_address = main_cmd->entryoff
	      + (text_sect->addr - text_sect->offset);
	    return TRUE;
	  }
      }

  /* An object file has no start address, so do not fail if not found.  */
  if (cmd == NULL)
    return TRUE;

  /* FIXME: create a subtarget hook ?  */
  for (i = 0; i < cmd->nflavours; i++)
    {
      if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_I386)
	  && (cmd->flavours[i].flavour
	      == (unsigned long) BFD_MACH_O_x86_THREAD_STATE32))
	{
	  unsigned char buf[4];

	  if (bfd_seek (abfd, cmd->flavours[i].offset + 40, SEEK_SET) != 0
              || bfd_bread (buf, 4, abfd) != 4)
	    return FALSE;

	  abfd->start_address = bfd_h_get_32 (abfd, buf);
	}
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_POWERPC)
	       && (cmd->flavours[i].flavour == BFD_MACH_O_PPC_THREAD_STATE))
	{
	  unsigned char buf[4];

	  if (bfd_seek (abfd, cmd->flavours[i].offset + 0, SEEK_SET) != 0
              || bfd_bread (buf, 4, abfd) != 4)
	    return FALSE;

	  abfd->start_address = bfd_h_get_32 (abfd, buf);
	}
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_POWERPC_64)
               && (cmd->flavours[i].flavour == BFD_MACH_O_PPC_THREAD_STATE64))
        {
          unsigned char buf[8];

          if (bfd_seek (abfd, cmd->flavours[i].offset + 0, SEEK_SET) != 0
              || bfd_bread (buf, 8, abfd) != 8)
            return FALSE;

          abfd->start_address = bfd_h_get_64 (abfd, buf);
        }
      else if ((mdata->header.cputype == BFD_MACH_O_CPU_TYPE_X86_64)
               && (cmd->flavours[i].flavour == BFD_MACH_O_x86_THREAD_STATE64))
        {
          unsigned char buf[8];

          if (bfd_seek (abfd, cmd->flavours[i].offset + (16 * 8), SEEK_SET) != 0
              || bfd_bread (buf, 8, abfd) != 8)
            return FALSE;

          abfd->start_address = bfd_h_get_64 (abfd, buf);
        }
    }

  return TRUE;
}

bfd_boolean
bfd_mach_o_set_arch_mach (bfd *abfd,
                          enum bfd_architecture arch,
                          unsigned long machine)
{
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  /* If this isn't the right architecture for this backend, and this
     isn't the generic backend, fail.  */
  if (arch != bed->arch
      && arch != bfd_arch_unknown
      && bed->arch != bfd_arch_unknown)
    return FALSE;

  return bfd_default_set_arch_mach (abfd, arch, machine);
}

static bfd_boolean
bfd_mach_o_scan (bfd *abfd,
		 bfd_mach_o_header *header,
		 bfd_mach_o_data_struct *mdata)
{
  unsigned int i;
  enum bfd_architecture cputype;
  unsigned long cpusubtype;
  unsigned int hdrsize;

  hdrsize = mach_o_wide_p (header) ?
    BFD_MACH_O_HEADER_64_SIZE : BFD_MACH_O_HEADER_SIZE;

  mdata->header = *header;

  abfd->flags = abfd->flags & BFD_IN_MEMORY;
  switch (header->filetype)
    {
    case BFD_MACH_O_MH_OBJECT:
      abfd->flags |= HAS_RELOC;
      break;
    case BFD_MACH_O_MH_EXECUTE:
      abfd->flags |= EXEC_P;
      break;
    case BFD_MACH_O_MH_DYLIB:
    case BFD_MACH_O_MH_BUNDLE:
      abfd->flags |= DYNAMIC;
      break;
    }

  abfd->tdata.mach_o_data = mdata;

  bfd_mach_o_convert_architecture (header->cputype, header->cpusubtype,
				   &cputype, &cpusubtype);
  if (cputype == bfd_arch_unknown)
    {
      (*_bfd_error_handler)
        (_("bfd_mach_o_scan: unknown architecture 0x%lx/0x%lx"),
         header->cputype, header->cpusubtype);
      return FALSE;
    }

  bfd_set_arch_mach (abfd, cputype, cpusubtype);

  if (header->ncmds != 0)
    {
      mdata->commands = bfd_alloc
        (abfd, header->ncmds * sizeof (bfd_mach_o_load_command));
      if (mdata->commands == NULL)
	return FALSE;

      for (i = 0; i < header->ncmds; i++)
	{
	  bfd_mach_o_load_command *cur = &mdata->commands[i];

	  if (i == 0)
	    cur->offset = hdrsize;
	  else
	    {
	      bfd_mach_o_load_command *prev = &mdata->commands[i - 1];
	      cur->offset = prev->offset + prev->len;
	    }

	  if (bfd_mach_o_read_command (abfd, cur) < 0)
	    return FALSE;
	}
    }

  /* Sections should be flatten before scanning start address.  */
  bfd_mach_o_flatten_sections (abfd);
  if (!bfd_mach_o_scan_start_address (abfd))
    return FALSE;

  return TRUE;
}

bfd_boolean
bfd_mach_o_mkobject_init (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = NULL;

  mdata = bfd_alloc (abfd, sizeof (bfd_mach_o_data_struct));
  if (mdata == NULL)
    return FALSE;
  abfd->tdata.mach_o_data = mdata;

  mdata->header.magic = 0;
  mdata->header.cputype = 0;
  mdata->header.cpusubtype = 0;
  mdata->header.filetype = 0;
  mdata->header.ncmds = 0;
  mdata->header.sizeofcmds = 0;
  mdata->header.flags = 0;
  mdata->header.byteorder = BFD_ENDIAN_UNKNOWN;
  mdata->commands = NULL;
  mdata->nsects = 0;
  mdata->sections = NULL;
  mdata->dyn_reloc_cache = NULL;

  return TRUE;
}

static bfd_boolean
bfd_mach_o_gen_mkobject (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_mkobject_init (abfd))
    return FALSE;

  mdata = bfd_mach_o_get_data (abfd);
  mdata->header.magic = BFD_MACH_O_MH_MAGIC;
  mdata->header.cputype = 0;
  mdata->header.cpusubtype = 0;
  mdata->header.byteorder = abfd->xvec->byteorder;
  mdata->header.version = 1;

  return TRUE;
}

const bfd_target *
bfd_mach_o_header_p (bfd *abfd,
                     bfd_mach_o_filetype filetype,
                     bfd_mach_o_cpu_type cputype)
{
  bfd_mach_o_header header;
  bfd_mach_o_data_struct *mdata;

  if (!bfd_mach_o_read_header (abfd, &header))
    goto wrong;

  if (! (header.byteorder == BFD_ENDIAN_BIG
	 || header.byteorder == BFD_ENDIAN_LITTLE))
    {
      (*_bfd_error_handler) (_("unknown header byte-order value 0x%lx"),
			     (unsigned long) header.byteorder);
      goto wrong;
    }

  if (! ((header.byteorder == BFD_ENDIAN_BIG
	  && abfd->xvec->byteorder == BFD_ENDIAN_BIG
	  && abfd->xvec->header_byteorder == BFD_ENDIAN_BIG)
	 || (header.byteorder == BFD_ENDIAN_LITTLE
	     && abfd->xvec->byteorder == BFD_ENDIAN_LITTLE
	     && abfd->xvec->header_byteorder == BFD_ENDIAN_LITTLE)))
    goto wrong;

  /* Check cputype and filetype.
     In case of wildcard, do not accept magics that are handled by existing
     targets.  */
  if (cputype)
    {
      if (header.cputype != cputype)
        goto wrong;
    }

  if (filetype)
    {
      if (header.filetype != filetype)
        goto wrong;
    }
  else
    {
      switch (header.filetype)
        {
        case BFD_MACH_O_MH_CORE:
          /* Handled by core_p */
          goto wrong;
        default:
          break;
        }
    }

  mdata = (bfd_mach_o_data_struct *) bfd_zalloc (abfd, sizeof (*mdata));
  if (mdata == NULL)
    goto fail;

  if (!bfd_mach_o_scan (abfd, &header, mdata))
    goto wrong;

  return abfd->xvec;

 wrong:
  bfd_set_error (bfd_error_wrong_format);

 fail:
  return NULL;
}

static const bfd_target *
bfd_mach_o_gen_object_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, 0, 0);
}

static const bfd_target *
bfd_mach_o_gen_core_p (bfd *abfd)
{
  return bfd_mach_o_header_p (abfd, BFD_MACH_O_MH_CORE, 0);
}

typedef struct mach_o_fat_archentry
{
  unsigned long cputype;
  unsigned long cpusubtype;
  unsigned long offset;
  unsigned long size;
  unsigned long align;
} mach_o_fat_archentry;

typedef struct mach_o_fat_data_struct
{
  unsigned long magic;
  unsigned long nfat_arch;
  mach_o_fat_archentry *archentries;
} mach_o_fat_data_struct;

const bfd_target *
bfd_mach_o_archive_p (bfd *abfd)
{
  mach_o_fat_data_struct *adata = NULL;
  struct mach_o_fat_header_external hdr;
  unsigned long i;

  if (bfd_seek (abfd, 0, SEEK_SET) != 0
      || bfd_bread (&hdr, sizeof (hdr), abfd) != sizeof (hdr))
    goto error;

  adata = bfd_alloc (abfd, sizeof (mach_o_fat_data_struct));
  if (adata == NULL)
    goto error;

  adata->magic = bfd_getb32 (hdr.magic);
  adata->nfat_arch = bfd_getb32 (hdr.nfat_arch);
  if (adata->magic != 0xcafebabe)
    goto error;
  /* Avoid matching Java bytecode files, which have the same magic number.
     In the Java bytecode file format this field contains the JVM version,
     which starts at 43.0.  */
  if (adata->nfat_arch > 30)
    goto error;

  adata->archentries =
    bfd_alloc (abfd, adata->nfat_arch * sizeof (mach_o_fat_archentry));
  if (adata->archentries == NULL)
    goto error;

  for (i = 0; i < adata->nfat_arch; i++)
    {
      struct mach_o_fat_arch_external arch;
      if (bfd_bread (&arch, sizeof (arch), abfd) != sizeof (arch))
	goto error;
      adata->archentries[i].cputype = bfd_getb32 (arch.cputype);
      adata->archentries[i].cpusubtype = bfd_getb32 (arch.cpusubtype);
      adata->archentries[i].offset = bfd_getb32 (arch.offset);
      adata->archentries[i].size = bfd_getb32 (arch.size);
      adata->archentries[i].align = bfd_getb32 (arch.align);
    }

  abfd->tdata.mach_o_fat_data = adata;
  return abfd->xvec;

 error:
  if (adata != NULL)
    bfd_release (abfd, adata);
  bfd_set_error (bfd_error_wrong_format);
  return NULL;
}

/* Set the filename for a fat binary member ABFD, whose bfd architecture is
   ARCH_TYPE/ARCH_SUBTYPE and corresponding entry in header is ENTRY.
   Set arelt_data and origin fields too.  */

static void
bfd_mach_o_fat_member_init (bfd *abfd,
                            enum bfd_architecture arch_type,
                            unsigned long arch_subtype,
                            mach_o_fat_archentry *entry)
{
  struct areltdata *areltdata;
  /* Create the member filename. Use ARCH_NAME.  */
  const bfd_arch_info_type *ap = bfd_lookup_arch (arch_type, arch_subtype);

  if (ap)
    {
      /* Use the architecture name if known.  */
      abfd->filename = ap->printable_name;
    }
  else
    {
      /* Forge a uniq id.  */
      const size_t namelen = 2 + 8 + 1 + 2 + 8 + 1;
      char *name = bfd_alloc (abfd, namelen);
      snprintf (name, namelen, "0x%lx-0x%lx",
                entry->cputype, entry->cpusubtype);
      abfd->filename = name;
    }

  areltdata = bfd_zmalloc (sizeof (struct areltdata));
  areltdata->parsed_size = entry->size;
  abfd->arelt_data = areltdata;
  abfd->iostream = NULL;
  abfd->origin = entry->offset;
}

bfd *
bfd_mach_o_openr_next_archived_file (bfd *archive, bfd *prev)
{
  mach_o_fat_data_struct *adata;
  mach_o_fat_archentry *entry = NULL;
  unsigned long i;
  bfd *nbfd;
  enum bfd_architecture arch_type;
  unsigned long arch_subtype;

  adata = (mach_o_fat_data_struct *) archive->tdata.mach_o_fat_data;
  BFD_ASSERT (adata != NULL);

  /* Find index of previous entry.  */
  if (prev == NULL)
    {
      /* Start at first one.  */
      i = 0;
    }
  else
    {
      /* Find index of PREV.  */
      for (i = 0; i < adata->nfat_arch; i++)
	{
	  if (adata->archentries[i].offset == prev->origin)
	    break;
	}

      if (i == adata->nfat_arch)
	{
	  /* Not found.  */
	  bfd_set_error (bfd_error_bad_value);
	  return NULL;
	}

      /* Get next entry.  */
      i++;
    }

  if (i >= adata->nfat_arch)
    {
      bfd_set_error (bfd_error_no_more_archived_files);
      return NULL;
    }

  entry = &adata->archentries[i];
  nbfd = _bfd_new_bfd_contained_in (archive);
  if (nbfd == NULL)
    return NULL;

  bfd_mach_o_convert_architecture (entry->cputype, entry->cpusubtype,
				   &arch_type, &arch_subtype);

  bfd_mach_o_fat_member_init (nbfd, arch_type, arch_subtype, entry);

  bfd_set_arch_mach (nbfd, arch_type, arch_subtype);

  return nbfd;
}

/* Analogous to stat call.  */

static int
bfd_mach_o_fat_stat_arch_elt (bfd *abfd, struct stat *buf)
{
  if (abfd->arelt_data == NULL)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return -1;
    }

  buf->st_mtime = 0;
  buf->st_uid = 0;
  buf->st_gid = 0;
  buf->st_mode = 0644;
  buf->st_size = arelt_size (abfd);

  return 0;
}

/* If ABFD format is FORMAT and architecture is ARCH, return it.
   If ABFD is a fat image containing a member that corresponds to FORMAT
   and ARCH, returns it.
   In other case, returns NULL.
   This function allows transparent uses of fat images.  */

bfd *
bfd_mach_o_fat_extract (bfd *abfd,
			bfd_format format,
			const bfd_arch_info_type *arch)
{
  bfd *res;
  mach_o_fat_data_struct *adata;
  unsigned int i;

  if (bfd_check_format (abfd, format))
    {
      if (bfd_get_arch_info (abfd) == arch)
	return abfd;
      return NULL;
    }
  if (!bfd_check_format (abfd, bfd_archive)
      || abfd->xvec != &mach_o_fat_vec)
    return NULL;

  /* This is a Mach-O fat image.  */
  adata = (mach_o_fat_data_struct *) abfd->tdata.mach_o_fat_data;
  BFD_ASSERT (adata != NULL);

  for (i = 0; i < adata->nfat_arch; i++)
    {
      struct mach_o_fat_archentry *e = &adata->archentries[i];
      enum bfd_architecture cpu_type;
      unsigned long cpu_subtype;

      bfd_mach_o_convert_architecture (e->cputype, e->cpusubtype,
				       &cpu_type, &cpu_subtype);
      if (cpu_type != arch->arch || cpu_subtype != arch->mach)
	continue;

      /* The architecture is found.  */
      res = _bfd_new_bfd_contained_in (abfd);
      if (res == NULL)
	return NULL;

      bfd_mach_o_fat_member_init (res, cpu_type, cpu_subtype, e);

      if (bfd_check_format (res, format))
	{
	  BFD_ASSERT (bfd_get_arch_info (res) == arch);
	  return res;
	}
      bfd_close (res);
      return NULL;
    }

  return NULL;
}

int
bfd_mach_o_lookup_command (bfd *abfd,
			   bfd_mach_o_load_command_type type,
			   bfd_mach_o_load_command **mcommand)
{
  struct mach_o_data_struct *md = bfd_mach_o_get_data (abfd);
  bfd_mach_o_load_command *ncmd = NULL;
  unsigned int i, num;

  BFD_ASSERT (md != NULL);
  BFD_ASSERT (mcommand != NULL);

  num = 0;
  for (i = 0; i < md->header.ncmds; i++)
    {
      struct bfd_mach_o_load_command *cmd = &md->commands[i];

      if (cmd->type != type)
	continue;

      if (num == 0)
	ncmd = cmd;
      num++;
    }

  *mcommand = ncmd;
  return num;
}

unsigned long
bfd_mach_o_stack_addr (enum bfd_mach_o_cpu_type type)
{
  switch (type)
    {
    case BFD_MACH_O_CPU_TYPE_MC680x0:
      return 0x04000000;
    case BFD_MACH_O_CPU_TYPE_MC88000:
      return 0xffffe000;
    case BFD_MACH_O_CPU_TYPE_POWERPC:
      return 0xc0000000;
    case BFD_MACH_O_CPU_TYPE_I386:
      return 0xc0000000;
    case BFD_MACH_O_CPU_TYPE_SPARC:
      return 0xf0000000;
    case BFD_MACH_O_CPU_TYPE_I860:
      return 0;
    case BFD_MACH_O_CPU_TYPE_HPPA:
      return 0xc0000000 - 0x04000000;
    default:
      return 0;
    }
}

/* The following two tables should be kept, as far as possible, in order of
   most frequently used entries to optimize their use from gas.  */

const bfd_mach_o_xlat_name bfd_mach_o_section_type_name[] =
{
  { "regular", BFD_MACH_O_S_REGULAR},
  { "coalesced", BFD_MACH_O_S_COALESCED},
  { "zerofill", BFD_MACH_O_S_ZEROFILL},
  { "cstring_literals", BFD_MACH_O_S_CSTRING_LITERALS},
  { "4byte_literals", BFD_MACH_O_S_4BYTE_LITERALS},
  { "8byte_literals", BFD_MACH_O_S_8BYTE_LITERALS},
  { "16byte_literals", BFD_MACH_O_S_16BYTE_LITERALS},
  { "literal_pointers", BFD_MACH_O_S_LITERAL_POINTERS},
  { "mod_init_func_pointers", BFD_MACH_O_S_MOD_INIT_FUNC_POINTERS},
  { "mod_fini_func_pointers", BFD_MACH_O_S_MOD_FINI_FUNC_POINTERS},
  { "gb_zerofill", BFD_MACH_O_S_GB_ZEROFILL},
  { "interposing", BFD_MACH_O_S_INTERPOSING},
  { "dtrace_dof", BFD_MACH_O_S_DTRACE_DOF},
  { "non_lazy_symbol_pointers", BFD_MACH_O_S_NON_LAZY_SYMBOL_POINTERS},
  { "lazy_symbol_pointers", BFD_MACH_O_S_LAZY_SYMBOL_POINTERS},
  { "symbol_stubs", BFD_MACH_O_S_SYMBOL_STUBS},
  { "lazy_dylib_symbol_pointers", BFD_MACH_O_S_LAZY_DYLIB_SYMBOL_POINTERS},
  { NULL, 0}
};

const bfd_mach_o_xlat_name bfd_mach_o_section_attribute_name[] =
{
  { "pure_instructions", BFD_MACH_O_S_ATTR_PURE_INSTRUCTIONS },
  { "some_instructions", BFD_MACH_O_S_ATTR_SOME_INSTRUCTIONS },
  { "loc_reloc", BFD_MACH_O_S_ATTR_LOC_RELOC },
  { "ext_reloc", BFD_MACH_O_S_ATTR_EXT_RELOC },
  { "debug", BFD_MACH_O_S_ATTR_DEBUG },
  { "live_support", BFD_MACH_O_S_ATTR_LIVE_SUPPORT },
  { "no_dead_strip", BFD_MACH_O_S_ATTR_NO_DEAD_STRIP },
  { "strip_static_syms", BFD_MACH_O_S_ATTR_STRIP_STATIC_SYMS },
  { "no_toc", BFD_MACH_O_S_ATTR_NO_TOC },
  { "self_modifying_code", BFD_MACH_O_S_SELF_MODIFYING_CODE },
  { "modifying_code", BFD_MACH_O_S_SELF_MODIFYING_CODE },
  { NULL, 0}
};

/* Get the section type from NAME.  Return 256 if NAME is unknown.  */

unsigned int
bfd_mach_o_get_section_type_from_name (bfd *abfd, const char *name)
{
  const bfd_mach_o_xlat_name *x;
  bfd_mach_o_backend_data *bed = bfd_mach_o_get_backend_data (abfd);

  for (x = bfd_mach_o_section_type_name; x->name; x++)
    if (strcmp (x->name, name) == 0)
      {
	/* We found it... does the target support it?  */
	if (bed->bfd_mach_o_section_type_valid_for_target == NULL
	    || bed->bfd_mach_o_section_type_valid_for_target (x->val))
	  return x->val; /* OK.  */
	else
	  break; /* Not supported.  */
      }
  /* Maximum section ID = 0xff.  */
  return 256;
}

/* Get the section attribute from NAME.  Return -1 if NAME is unknown.  */

unsigned int
bfd_mach_o_get_section_attribute_from_name (const char *name)
{
  const bfd_mach_o_xlat_name *x;

  for (x = bfd_mach_o_section_attribute_name; x->name; x++)
    if (strcmp (x->name, name) == 0)
      return x->val;
  return (unsigned int)-1;
}

int
bfd_mach_o_core_fetch_environment (bfd *abfd,
				   unsigned char **rbuf,
				   unsigned int *rlen)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  unsigned long stackaddr = bfd_mach_o_stack_addr (mdata->header.cputype);
  unsigned int i = 0;

  for (i = 0; i < mdata->header.ncmds; i++)
    {
      bfd_mach_o_load_command *cur = &mdata->commands[i];
      bfd_mach_o_segment_command *seg = NULL;

      if (cur->type != BFD_MACH_O_LC_SEGMENT)
	continue;

      seg = &cur->command.segment;

      if ((seg->vmaddr + seg->vmsize) == stackaddr)
	{
	  unsigned long start = seg->fileoff;
	  unsigned long end = seg->fileoff + seg->filesize;
	  unsigned char *buf = bfd_malloc (1024);
	  unsigned long size = 1024;

	  for (;;)
	    {
	      bfd_size_type nread = 0;
	      unsigned long offset;
	      int found_nonnull = 0;

	      if (size > (end - start))
		size = (end - start);

	      buf = bfd_realloc_or_free (buf, size);
	      if (buf == NULL)
		return -1;

	      if (bfd_seek (abfd, end - size, SEEK_SET) != 0)
                {
                  free (buf);
                  return -1;
                }

	      nread = bfd_bread (buf, size, abfd);

	      if (nread != size)
		{
		  free (buf);
		  return -1;
		}

	      for (offset = 4; offset <= size; offset += 4)
		{
		  unsigned long val;

		  val = *((unsigned long *) (buf + size - offset));
		  if (! found_nonnull)
		    {
		      if (val != 0)
			found_nonnull = 1;
		    }
		  else if (val == 0x0)
		    {
		      unsigned long bottom;
		      unsigned long top;

		      bottom = seg->fileoff + seg->filesize - offset;
		      top = seg->fileoff + seg->filesize - 4;
		      *rbuf = bfd_malloc (top - bottom);
		      *rlen = top - bottom;

		      memcpy (*rbuf, buf + size - *rlen, *rlen);
		      free (buf);
		      return 0;
		    }
		}

	      if (size == (end - start))
		break;

	      size *= 2;
	    }

	  free (buf);
	}
    }

  return -1;
}

char *
bfd_mach_o_core_file_failing_command (bfd *abfd)
{
  unsigned char *buf = NULL;
  unsigned int len = 0;
  int ret = -1;

  ret = bfd_mach_o_core_fetch_environment (abfd, &buf, &len);
  if (ret < 0)
    return NULL;

  return (char *) buf;
}

int
bfd_mach_o_core_file_failing_signal (bfd *abfd ATTRIBUTE_UNUSED)
{
  return 0;
}

static bfd_mach_o_uuid_command *
bfd_mach_o_lookup_uuid_command (bfd *abfd)
{
  bfd_mach_o_load_command *uuid_cmd;
  int ncmd = bfd_mach_o_lookup_command (abfd, BFD_MACH_O_LC_UUID, &uuid_cmd);
  if (ncmd != 1)
    return FALSE;
  return &uuid_cmd->command.uuid;
}

/* Return true if ABFD is a dSYM file and its UUID matches UUID_CMD. */

static bfd_boolean
bfd_mach_o_dsym_for_uuid_p (bfd *abfd, const bfd_mach_o_uuid_command *uuid_cmd)
{
  bfd_mach_o_uuid_command *dsym_uuid_cmd;

  BFD_ASSERT (abfd);
  BFD_ASSERT (uuid_cmd);

  if (!bfd_check_format (abfd, bfd_object))
    return FALSE;

  if (bfd_get_flavour (abfd) != bfd_target_mach_o_flavour
      || bfd_mach_o_get_data (abfd) == NULL
      || bfd_mach_o_get_data (abfd)->header.filetype != BFD_MACH_O_MH_DSYM)
    return FALSE;

  dsym_uuid_cmd = bfd_mach_o_lookup_uuid_command (abfd);
  if (dsym_uuid_cmd == NULL)
    return FALSE;

  if (memcmp (uuid_cmd->uuid, dsym_uuid_cmd->uuid,
              sizeof (uuid_cmd->uuid)) != 0)
    return FALSE;

  return TRUE;
}

/* Find a BFD in DSYM_FILENAME which matches ARCH and UUID_CMD.
   The caller is responsible for closing the returned BFD object and
   its my_archive if the returned BFD is in a fat dSYM. */

static bfd *
bfd_mach_o_find_dsym (const char *dsym_filename,
                      const bfd_mach_o_uuid_command *uuid_cmd,
                      const bfd_arch_info_type *arch)
{
  bfd *base_dsym_bfd, *dsym_bfd;

  BFD_ASSERT (uuid_cmd);

  base_dsym_bfd = bfd_openr (dsym_filename, NULL);
  if (base_dsym_bfd == NULL)
    return NULL;

  dsym_bfd = bfd_mach_o_fat_extract (base_dsym_bfd, bfd_object, arch);
  if (bfd_mach_o_dsym_for_uuid_p (dsym_bfd, uuid_cmd))
    return dsym_bfd;

  bfd_close (dsym_bfd);
  if (base_dsym_bfd != dsym_bfd)
    bfd_close (base_dsym_bfd);

  return NULL;
}

/* Return a BFD created from a dSYM file for ABFD.
   The caller is responsible for closing the returned BFD object, its
   filename, and its my_archive if the returned BFD is in a fat dSYM. */

static bfd *
bfd_mach_o_follow_dsym (bfd *abfd)
{
  char *dsym_filename;
  bfd_mach_o_uuid_command *uuid_cmd;
  bfd *dsym_bfd, *base_bfd = abfd;
  const char *base_basename;

  if (abfd == NULL || bfd_get_flavour (abfd) != bfd_target_mach_o_flavour)
    return NULL;

  if (abfd->my_archive)
    base_bfd = abfd->my_archive;
  /* BFD may have been opened from a stream. */
  if (base_bfd->filename == NULL)
    {
      bfd_set_error (bfd_error_invalid_operation);
      return NULL;
    }
  base_basename = lbasename (base_bfd->filename);

  uuid_cmd = bfd_mach_o_lookup_uuid_command (abfd);
  if (uuid_cmd == NULL)
    return NULL;

  /* TODO: We assume the DWARF file has the same as the binary's.
     It seems apple's GDB checks all files in the dSYM bundle directory.
     http://opensource.apple.com/source/gdb/gdb-1708/src/gdb/macosx/macosx-tdep.c
  */
  dsym_filename = (char *)bfd_malloc (strlen (base_bfd->filename)
                                       + strlen (dsym_subdir) + 1
                                       + strlen (base_basename) + 1);
  sprintf (dsym_filename, "%s%s/%s",
           base_bfd->filename, dsym_subdir, base_basename);

  dsym_bfd = bfd_mach_o_find_dsym (dsym_filename, uuid_cmd,
                                   bfd_get_arch_info (abfd));
  if (dsym_bfd == NULL)
    free (dsym_filename);

  return dsym_bfd;
}

bfd_boolean
bfd_mach_o_find_nearest_line (bfd *abfd,
			      asection *section,
			      asymbol **symbols,
			      bfd_vma offset,
			      const char **filename_ptr,
			      const char **functionname_ptr,
			      unsigned int *line_ptr)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  if (mdata == NULL)
    return FALSE;
  switch (mdata->header.filetype)
    {
    case BFD_MACH_O_MH_OBJECT:
      break;
    case BFD_MACH_O_MH_EXECUTE:
    case BFD_MACH_O_MH_DYLIB:
    case BFD_MACH_O_MH_BUNDLE:
    case BFD_MACH_O_MH_KEXT_BUNDLE:
      if (mdata->dwarf2_find_line_info == NULL)
        {
          mdata->dsym_bfd = bfd_mach_o_follow_dsym (abfd);
          /* When we couldn't find dSYM for this binary, we look for
             the debug information in the binary itself. In this way,
             we won't try finding separated dSYM again because
             mdata->dwarf2_find_line_info will be filled. */
          if (! mdata->dsym_bfd)
            break;
          if (! _bfd_dwarf2_slurp_debug_info (abfd, mdata->dsym_bfd,
                                              dwarf_debug_sections, symbols,
                                              &mdata->dwarf2_find_line_info))
            return FALSE;
        }
      break;
    default:
      return FALSE;
    }
  if (_bfd_dwarf2_find_nearest_line (abfd, dwarf_debug_sections,
				     section, symbols, offset,
				     filename_ptr, functionname_ptr,
				     line_ptr, NULL, 0,
				     &mdata->dwarf2_find_line_info))
    return TRUE;
  return FALSE;
}

bfd_boolean
bfd_mach_o_close_and_cleanup (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  if (bfd_get_format (abfd) == bfd_object && mdata != NULL)
    {
      _bfd_dwarf2_cleanup_debug_info (abfd, &mdata->dwarf2_find_line_info);
      bfd_mach_o_free_cached_info (abfd);
      if (mdata->dsym_bfd != NULL)
        {
          bfd *fat_bfd = mdata->dsym_bfd->my_archive;
          char *dsym_filename = (char *)(fat_bfd
                                         ? fat_bfd->filename
                                         : mdata->dsym_bfd->filename);
          bfd_close (mdata->dsym_bfd);
          mdata->dsym_bfd = NULL;
          if (fat_bfd)
            bfd_close (fat_bfd);
          free (dsym_filename);
        }
    }

  if (bfd_get_format (abfd) == bfd_archive
      && abfd->xvec == &mach_o_fat_vec)
    return TRUE;
  return _bfd_generic_close_and_cleanup (abfd);
}

bfd_boolean bfd_mach_o_free_cached_info (bfd *abfd)
{
  bfd_mach_o_data_struct *mdata = bfd_mach_o_get_data (abfd);
  asection *asect;
  free (mdata->dyn_reloc_cache);
  mdata->dyn_reloc_cache = NULL;
  for (asect = abfd->sections; asect != NULL; asect = asect->next)
    {
      free (asect->relocation);
      asect->relocation = NULL;
    }

  return TRUE;
}

#define bfd_mach_o_bfd_reloc_type_lookup _bfd_norelocs_bfd_reloc_type_lookup
#define bfd_mach_o_bfd_reloc_name_lookup _bfd_norelocs_bfd_reloc_name_lookup

#define bfd_mach_o_swap_reloc_in NULL
#define bfd_mach_o_swap_reloc_out NULL
#define bfd_mach_o_print_thread NULL
#define bfd_mach_o_tgt_seg_table NULL
#define bfd_mach_o_section_type_valid_for_tgt NULL

#define TARGET_NAME 		mach_o_be_vec
#define TARGET_STRING     	"mach-o-be"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	1
#define TARGET_ARCHIVE 		0
#define TARGET_PRIORITY		1
#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE
#undef TARGET_PRIORITY

#define TARGET_NAME 		mach_o_le_vec
#define TARGET_STRING 		"mach-o-le"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	0
#define TARGET_ARCHIVE 		0
#define TARGET_PRIORITY		1

#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE
#undef TARGET_PRIORITY

/* Not yet handled: creating an archive.  */
#define bfd_mach_o_mkarchive                      _bfd_noarchive_mkarchive

/* Not used.  */
#define bfd_mach_o_read_ar_hdr                    _bfd_noarchive_read_ar_hdr
#define bfd_mach_o_write_ar_hdr                   _bfd_noarchive_write_ar_hdr
#define bfd_mach_o_slurp_armap                    _bfd_noarchive_slurp_armap
#define bfd_mach_o_slurp_extended_name_table      _bfd_noarchive_slurp_extended_name_table
#define bfd_mach_o_construct_extended_name_table  _bfd_noarchive_construct_extended_name_table
#define bfd_mach_o_truncate_arname                _bfd_noarchive_truncate_arname
#define bfd_mach_o_write_armap                    _bfd_noarchive_write_armap
#define bfd_mach_o_get_elt_at_index               _bfd_noarchive_get_elt_at_index
#define bfd_mach_o_generic_stat_arch_elt          bfd_mach_o_fat_stat_arch_elt
#define bfd_mach_o_update_armap_timestamp         _bfd_noarchive_update_armap_timestamp

#define TARGET_NAME 		mach_o_fat_vec
#define TARGET_STRING 		"mach-o-fat"
#define TARGET_ARCHITECTURE	bfd_arch_unknown
#define TARGET_BIG_ENDIAN 	1
#define TARGET_ARCHIVE 		1
#define TARGET_PRIORITY		0

#include "mach-o-target.c"

#undef TARGET_NAME
#undef TARGET_STRING
#undef TARGET_ARCHITECTURE
#undef TARGET_BIG_ENDIAN
#undef TARGET_ARCHIVE
#undef TARGET_PRIORITY
