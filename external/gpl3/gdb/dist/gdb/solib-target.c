/* Definitions for targets which report shared library events.

   Copyright (C) 2007-2015 Free Software Foundation, Inc.

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

#include "defs.h"
#include "objfiles.h"
#include "solist.h"
#include "symtab.h"
#include "symfile.h"
#include "target.h"
#include "vec.h"
#include "solib-target.h"

/* Private data for each loaded library.  */
struct lm_info
{
  /* The library's name.  The name is normally kept in the struct
     so_list; it is only here during XML parsing.  */
  char *name;

  /* The target can either specify segment bases or section bases, not
     both.  */

  /* The base addresses for each independently relocatable segment of
     this shared library.  */
  VEC(CORE_ADDR) *segment_bases;

  /* The base addresses for each independently allocatable,
     relocatable section of this shared library.  */
  VEC(CORE_ADDR) *section_bases;

  /* The cached offsets for each section of this shared library,
     determined from SEGMENT_BASES, or SECTION_BASES.  */
  struct section_offsets *offsets;
};

typedef struct lm_info *lm_info_p;
DEF_VEC_P(lm_info_p);

#if !defined(HAVE_LIBEXPAT)

static VEC(lm_info_p) *
solib_target_parse_libraries (const char *library)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML library list; XML support was disabled "
		 "at compile time"));
    }

  return NULL;
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Handle the start of a <segment> element.  */

static void
library_list_start_segment (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  VEC(lm_info_p) **list = user_data;
  struct lm_info *last = VEC_last (lm_info_p, *list);
  ULONGEST *address_p = xml_find_attribute (attributes, "address")->value;
  CORE_ADDR address = (CORE_ADDR) *address_p;

  if (last->section_bases != NULL)
    gdb_xml_error (parser,
		   _("Library list with both segments and sections"));

  VEC_safe_push (CORE_ADDR, last->segment_bases, address);
}

static void
library_list_start_section (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  VEC(lm_info_p) **list = user_data;
  struct lm_info *last = VEC_last (lm_info_p, *list);
  ULONGEST *address_p = xml_find_attribute (attributes, "address")->value;
  CORE_ADDR address = (CORE_ADDR) *address_p;

  if (last->segment_bases != NULL)
    gdb_xml_error (parser,
		   _("Library list with both segments and sections"));

  VEC_safe_push (CORE_ADDR, last->section_bases, address);
}

/* Handle the start of a <library> element.  */

static void
library_list_start_library (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  VEC(lm_info_p) **list = user_data;
  struct lm_info *item = XCNEW (struct lm_info);
  const char *name = xml_find_attribute (attributes, "name")->value;

  item->name = xstrdup (name);
  VEC_safe_push (lm_info_p, *list, item);
}

static void
library_list_end_library (struct gdb_xml_parser *parser,
			  const struct gdb_xml_element *element,
			  void *user_data, const char *body_text)
{
  VEC(lm_info_p) **list = user_data;
  struct lm_info *lm_info = VEC_last (lm_info_p, *list);

  if (lm_info->segment_bases == NULL
      && lm_info->section_bases == NULL)
    gdb_xml_error (parser,
		   _("No segment or section bases defined"));
}


/* Handle the start of a <library-list> element.  */

static void
library_list_start_list (struct gdb_xml_parser *parser,
			 const struct gdb_xml_element *element,
			 void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  char *version = xml_find_attribute (attributes, "version")->value;

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser,
		   _("Library list has unsupported version \"%s\""),
		   version);
}

/* Discard the constructed library list.  */

static void
solib_target_free_library_list (void *p)
{
  VEC(lm_info_p) **result = p;
  struct lm_info *info;
  int ix;

  for (ix = 0; VEC_iterate (lm_info_p, *result, ix, info); ix++)
    {
      xfree (info->name);
      VEC_free (CORE_ADDR, info->segment_bases);
      VEC_free (CORE_ADDR, info->section_bases);
      xfree (info);
    }
  VEC_free (lm_info_p, *result);
  *result = NULL;
}

/* The allowed elements and attributes for an XML library list.
   The root element is a <library-list>.  */

static const struct gdb_xml_attribute segment_attributes[] = {
  { "address", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute section_attributes[] = {
  { "address", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_children[] = {
  { "segment", segment_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_segment, NULL },
  { "section", section_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_section, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute library_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_children[] = {
  { "library", library_attributes, library_children,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_library, library_list_end_library },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute library_list_attributes[] = {
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_elements[] = {
  { "library-list", library_list_attributes, library_list_children,
    GDB_XML_EF_NONE, library_list_start_list, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static VEC(lm_info_p) *
solib_target_parse_libraries (const char *library)
{
  VEC(lm_info_p) *result = NULL;
  struct cleanup *back_to = make_cleanup (solib_target_free_library_list,
					  &result);

  if (gdb_xml_parse_quick (_("target library list"), "library-list.dtd",
			   library_list_elements, library, &result) == 0)
    {
      /* Parsed successfully, keep the result.  */
      discard_cleanups (back_to);
      return result;
    }

  do_cleanups (back_to);
  return NULL;
}
#endif

static struct so_list *
solib_target_current_sos (void)
{
  struct so_list *new_solib, *start = NULL, *last = NULL;
  char *library_document;
  struct cleanup *old_chain;
  VEC(lm_info_p) *library_list;
  struct lm_info *info;
  int ix;

  /* Fetch the list of shared libraries.  */
  library_document = target_read_stralloc (&current_target,
					   TARGET_OBJECT_LIBRARIES,
					   NULL);
  if (library_document == NULL)
    return NULL;

  /* solib_target_parse_libraries may throw, so we use a cleanup.  */
  old_chain = make_cleanup (xfree, library_document);

  /* Parse the list.  */
  library_list = solib_target_parse_libraries (library_document);

  /* library_document string is not needed behind this point.  */
  do_cleanups (old_chain);

  if (library_list == NULL)
    return NULL;

  /* Build a struct so_list for each entry on the list.  */
  for (ix = 0; VEC_iterate (lm_info_p, library_list, ix, info); ix++)
    {
      new_solib = XCNEW (struct so_list);
      strncpy (new_solib->so_name, info->name, SO_NAME_MAX_PATH_SIZE - 1);
      new_solib->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      strncpy (new_solib->so_original_name, info->name,
	       SO_NAME_MAX_PATH_SIZE - 1);
      new_solib->so_original_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      new_solib->lm_info = info;

      /* We no longer need this copy of the name.  */
      xfree (info->name);
      info->name = NULL;

      /* Add it to the list.  */
      if (!start)
	last = start = new_solib;
      else
	{
	  last->next = new_solib;
	  last = new_solib;
	}
    }

  /* Free the library list, but not its members.  */
  VEC_free (lm_info_p, library_list);

  return start;
}

static void
solib_target_special_symbol_handling (void)
{
  /* Nothing needed.  */
}

static void
solib_target_solib_create_inferior_hook (int from_tty)
{
  /* Nothing needed.  */
}

static void
solib_target_clear_solib (void)
{
  /* Nothing needed.  */
}

static void
solib_target_free_so (struct so_list *so)
{
  gdb_assert (so->lm_info->name == NULL);
  xfree (so->lm_info->offsets);
  VEC_free (CORE_ADDR, so->lm_info->segment_bases);
  xfree (so->lm_info);
}

static void
solib_target_relocate_section_addresses (struct so_list *so,
					 struct target_section *sec)
{
  CORE_ADDR offset;

  /* Build the offset table only once per object file.  We can not do
     it any earlier, since we need to open the file first.  */
  if (so->lm_info->offsets == NULL)
    {
      int num_sections = gdb_bfd_count_sections (so->abfd);

      so->lm_info->offsets = xzalloc (SIZEOF_N_SECTION_OFFSETS (num_sections));

      if (so->lm_info->section_bases)
	{
	  int i;
	  asection *sect;
	  int num_section_bases
	    = VEC_length (CORE_ADDR, so->lm_info->section_bases);
	  int num_alloc_sections = 0;

	  for (i = 0, sect = so->abfd->sections;
	       sect != NULL;
	       i++, sect = sect->next)
	    if ((bfd_get_section_flags (so->abfd, sect) & SEC_ALLOC))
	      num_alloc_sections++;

	  if (num_alloc_sections != num_section_bases)
	    warning (_("\
Could not relocate shared library \"%s\": wrong number of ALLOC sections"),
		     so->so_name);
	  else
	    {
	      int bases_index = 0;
	      int found_range = 0;
	      CORE_ADDR *section_bases;

	      section_bases = VEC_address (CORE_ADDR,
					   so->lm_info->section_bases);

	      so->addr_low = ~(CORE_ADDR) 0;
	      so->addr_high = 0;
	      for (i = 0, sect = so->abfd->sections;
		   sect != NULL;
		   i++, sect = sect->next)
		{
		  if (!(bfd_get_section_flags (so->abfd, sect) & SEC_ALLOC))
		    continue;
		  if (bfd_section_size (so->abfd, sect) > 0)
		    {
		      CORE_ADDR low, high;

		      low = section_bases[i];
		      high = low + bfd_section_size (so->abfd, sect) - 1;

		      if (low < so->addr_low)
			so->addr_low = low;
		      if (high > so->addr_high)
			so->addr_high = high;
		      gdb_assert (so->addr_low <= so->addr_high);
		      found_range = 1;
		    }
		  so->lm_info->offsets->offsets[i]
		    = section_bases[bases_index];
		  bases_index++;
		}
	      if (!found_range)
		so->addr_low = so->addr_high = 0;
	      gdb_assert (so->addr_low <= so->addr_high);
	    }
	}
      else if (so->lm_info->segment_bases)
	{
	  struct symfile_segment_data *data;

	  data = get_symfile_segment_data (so->abfd);
	  if (data == NULL)
	    warning (_("\
Could not relocate shared library \"%s\": no segments"), so->so_name);
	  else
	    {
	      ULONGEST orig_delta;
	      int i;
	      int num_bases;
	      CORE_ADDR *segment_bases;

	      num_bases = VEC_length (CORE_ADDR, so->lm_info->segment_bases);
	      segment_bases = VEC_address (CORE_ADDR,
					   so->lm_info->segment_bases);

	      if (!symfile_map_offsets_to_segments (so->abfd, data,
						    so->lm_info->offsets,
						    num_bases, segment_bases))
		warning (_("\
Could not relocate shared library \"%s\": bad offsets"), so->so_name);

	      /* Find the range of addresses to report for this library in
		 "info sharedlibrary".  Report any consecutive segments
		 which were relocated as a single unit.  */
	      gdb_assert (num_bases > 0);
	      orig_delta = segment_bases[0] - data->segment_bases[0];

	      for (i = 1; i < data->num_segments; i++)
		{
		  /* If we have run out of offsets, assume all
		     remaining segments have the same offset.  */
		  if (i >= num_bases)
		    continue;

		  /* If this segment does not have the same offset, do
		     not include it in the library's range.  */
		  if (segment_bases[i] - data->segment_bases[i] != orig_delta)
		    break;
		}

	      so->addr_low = segment_bases[0];
	      so->addr_high = (data->segment_bases[i - 1]
			       + data->segment_sizes[i - 1]
			       + orig_delta);
	      gdb_assert (so->addr_low <= so->addr_high);

	      free_symfile_segment_data (data);
	    }
	}
    }

  offset = so->lm_info->offsets->offsets[gdb_bfd_section_index
					 (sec->the_bfd_section->owner,
					  sec->the_bfd_section)];
  sec->addr += offset;
  sec->endaddr += offset;
}

static int
solib_target_open_symbol_file_object (void *from_ttyp)
{
  /* We can't locate the main symbol file based on the target's
     knowledge; the user has to specify it.  */
  return 0;
}

static int
solib_target_in_dynsym_resolve_code (CORE_ADDR pc)
{
  /* We don't have a range of addresses for the dynamic linker; there
     may not be one in the program's address space.  So only report
     PLT entries (which may be import stubs).  */
  return in_plt_section (pc);
}

struct target_so_ops solib_target_so_ops;

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_solib_target;

void
_initialize_solib_target (void)
{
  solib_target_so_ops.relocate_section_addresses
    = solib_target_relocate_section_addresses;
  solib_target_so_ops.free_so = solib_target_free_so;
  solib_target_so_ops.clear_solib = solib_target_clear_solib;
  solib_target_so_ops.solib_create_inferior_hook
    = solib_target_solib_create_inferior_hook;
  solib_target_so_ops.special_symbol_handling
    = solib_target_special_symbol_handling;
  solib_target_so_ops.current_sos = solib_target_current_sos;
  solib_target_so_ops.open_symbol_file_object
    = solib_target_open_symbol_file_object;
  solib_target_so_ops.in_dynsym_resolve_code
    = solib_target_in_dynsym_resolve_code;
  solib_target_so_ops.bfd_open = solib_bfd_open;

  /* Set current_target_so_ops to solib_target_so_ops if not already
     set.  */
  if (current_target_so_ops == 0)
    current_target_so_ops = &solib_target_so_ops;
}
