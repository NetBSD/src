/* Copyright (C) 2013-2014 Free Software Foundation, Inc.

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
#include "solib-aix.h"
#include "solist.h"
#include "inferior.h"
#include "gdb_bfd.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "symtab.h"
#include "xcoffread.h"
#include "observer.h"
#include "gdbcmd.h"

/* Variable controlling the output of the debugging traces for
   this module.  */
static int solib_aix_debug;

/* Our private data in struct so_list.  */

struct lm_info
{
  /* The name of the file mapped by the loader.  Apart from the entry
     for the main executable, this is usually a shared library (which,
     on AIX, is an archive library file, created using the "ar"
     command).  */
  char *filename;

  /* The name of the shared object file with the actual dynamic
     loading dependency.  This may be NULL (Eg. main executable).  */
  char *member_name;

  /* The address in inferior memory where the text section got mapped.  */
  CORE_ADDR text_addr;

  /* The size of the text section, obtained via the loader data.  */
  ULONGEST text_size;

  /* The address in inferior memory where the data section got mapped.  */
  CORE_ADDR data_addr;

  /* The size of the data section, obtained via the loader data.  */
  ULONGEST data_size;
};

typedef struct lm_info *lm_info_p;
DEF_VEC_P(lm_info_p);

/* Return a deep copy of the given struct lm_info object.  */

static struct lm_info *
solib_aix_new_lm_info (struct lm_info *info)
{
  struct lm_info *result = xmalloc (sizeof (struct lm_info));

  memcpy (result, info, sizeof (struct lm_info));
  result->filename = xstrdup (info->filename);
  if (info->member_name != NULL)
    result->member_name = xstrdup (info->member_name);

  return result;
}

/* Free the memory allocated for the given lm_info.  */

static void
solib_aix_xfree_lm_info (struct lm_info *info)
{
  xfree (info->filename);
  xfree (info->member_name);
  xfree (info);
}

/* This module's per-inferior data.  */

struct solib_aix_inferior_data
{
  /* The list of shared libraries.  NULL if not computed yet.

     Note that the first element of this list is always the main
     executable, which is not technically a shared library.  But
     we need that information to perform its relocation, and
     the same principles applied to shared libraries also apply
     to the main executable.  So it's simpler to keep it as part
     of this list.  */
  VEC (lm_info_p) *library_list;
};

/* Key to our per-inferior data.  */
static const struct inferior_data *solib_aix_inferior_data_handle;

/* Return this module's data for the given inferior.
   If none is found, add a zero'ed one now.  */

static struct solib_aix_inferior_data *
get_solib_aix_inferior_data (struct inferior *inf)
{
  struct solib_aix_inferior_data *data;

  data = inferior_data (inf, solib_aix_inferior_data_handle);
  if (data == NULL)
    {
      data = XZALLOC (struct solib_aix_inferior_data);
      set_inferior_data (inf, solib_aix_inferior_data_handle, data);
    }

  return data;
}

#if !defined(HAVE_LIBEXPAT)

/* Dummy implementation if XML support is not compiled in.  */

static VEC (lm_info_p) *
solib_aix_parse_libraries (const char *library)
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

/* Dummy implementation if XML support is not compiled in.  */

static void
solib_aix_free_library_list (void *p)
{
}

#else /* HAVE_LIBEXPAT */

#include "xml-support.h"

/* Handle the start of a <library> element.  */

static void
library_list_start_library (struct gdb_xml_parser *parser,
			    const struct gdb_xml_element *element,
			    void *user_data,
			    VEC (gdb_xml_value_s) *attributes)
{
  VEC (lm_info_p) **list = user_data;
  struct lm_info *item = XZALLOC (struct lm_info);
  struct gdb_xml_value *attr;

  attr = xml_find_attribute (attributes, "name");
  item->filename = xstrdup (attr->value);

  attr = xml_find_attribute (attributes, "member");
  if (attr != NULL)
    item->member_name = xstrdup (attr->value);

  attr = xml_find_attribute (attributes, "text_addr");
  item->text_addr = * (ULONGEST *) attr->value;

  attr = xml_find_attribute (attributes, "text_size");
  item->text_size = * (ULONGEST *) attr->value;

  attr = xml_find_attribute (attributes, "data_addr");
  item->data_addr = * (ULONGEST *) attr->value;

  attr = xml_find_attribute (attributes, "data_size");
  item->data_size = * (ULONGEST *) attr->value;

  VEC_safe_push (lm_info_p, *list, item);
}

/* Handle the start of a <library-list-aix> element.  */

static void
library_list_start_list (struct gdb_xml_parser *parser,
                         const struct gdb_xml_element *element,
                         void *user_data, VEC (gdb_xml_value_s) *attributes)
{
  char *version = xml_find_attribute (attributes, "version")->value;

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser,
                   _("Library list has unsupported version \"%s\""),
                   version);
}

/* Discard the constructed library list.  */

static void
solib_aix_free_library_list (void *p)
{
  VEC (lm_info_p) **result = p;
  struct lm_info *info;
  int ix;

  if (solib_aix_debug)
    fprintf_unfiltered (gdb_stdlog, "DEBUG: solib_aix_free_library_list\n");

  for (ix = 0; VEC_iterate (lm_info_p, *result, ix, info); ix++)
    solib_aix_xfree_lm_info (info);
  VEC_free (lm_info_p, *result);
  *result = NULL;
}

/* The allowed elements and attributes for an AIX library list
   described in XML format.  The root element is a <library-list-aix>.  */

static const struct gdb_xml_attribute library_attributes[] =
{
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "member", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "text_addr", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "text_size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "data_addr", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "data_size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_children[] =
{
  { "library", library_attributes, NULL,
    GDB_XML_EF_REPEATABLE | GDB_XML_EF_OPTIONAL,
    library_list_start_library, NULL},
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute library_list_attributes[] =
{
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element library_list_elements[] =
{
  { "library-list-aix", library_list_attributes, library_list_children,
    GDB_XML_EF_NONE, library_list_start_list, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse LIBRARY, a string containing the loader info in XML format,
   and return an lm_info_p vector.

   Return NULL if the parsing failed.  */

static VEC (lm_info_p) *
solib_aix_parse_libraries (const char *library)
{
  VEC (lm_info_p) *result = NULL;
  struct cleanup *back_to = make_cleanup (solib_aix_free_library_list,
                                          &result);

  if (gdb_xml_parse_quick (_("aix library list"), "library-list-aix.dtd",
                           library_list_elements, library, &result) == 0)
    {
      /* Parsed successfully, keep the result.  */
      discard_cleanups (back_to);
      return result;
    }

  do_cleanups (back_to);
  return NULL;
}

#endif /* HAVE_LIBEXPAT */

/* Return the loader info for the given inferior (INF), or NULL if
   the list could not be computed.

   Cache the result in per-inferior data, so as to avoid recomputing it
   each time this function is called.

   If an error occurs while computing this list, and WARNING_MSG
   is not NULL, then print a warning including WARNING_MSG and
   a description of the error.  */

static VEC (lm_info_p) *
solib_aix_get_library_list (struct inferior *inf, const char *warning_msg)
{
  struct solib_aix_inferior_data *data;
  char *library_document;
  struct cleanup *cleanup;

  /* If already computed, return the cached value.  */
  data = get_solib_aix_inferior_data (inf);
  if (data->library_list != NULL)
    return data->library_list;

  library_document = target_read_stralloc (&current_target,
                                           TARGET_OBJECT_LIBRARIES_AIX,
                                           NULL);
  if (library_document == NULL && warning_msg != NULL)
    {
      warning (_("%s (failed to read TARGET_OBJECT_LIBRARIES_AIX)"),
	       warning_msg);
      return NULL;
    }
  cleanup = make_cleanup (xfree, library_document);

  if (solib_aix_debug)
    fprintf_unfiltered (gdb_stdlog,
			"DEBUG: TARGET_OBJECT_LIBRARIES_AIX = \n%s\n",
			library_document);

  data->library_list = solib_aix_parse_libraries (library_document);
  if (data->library_list == NULL && warning_msg != NULL)
    {
      warning (_("%s (missing XML support?)"), warning_msg);
      do_cleanups (cleanup);
      return NULL;
    }

  do_cleanups (cleanup);
  return data->library_list;
}

/* If the .bss section's VMA is set to an address located before
   the end of the .data section, causing the two sections to overlap,
   return the overlap in bytes.  Otherwise, return zero.

   Motivation:

   The GNU linker sometimes sets the start address of the .bss session
   before the end of the .data section, making the 2 sections overlap.
   The loader appears to handle this situation gracefully, by simply
   loading the bss section right after the end of the .data section.

   This means that the .data and the .bss sections are sometimes
   no longer relocated by the same amount.  The problem is that
   the ldinfo data does not contain any information regarding
   the relocation of the .bss section, assuming that it would be
   identical to the information provided for the .data section
   (this is what would normally happen if the program was linked
   correctly).

   GDB therefore needs to detect those cases, and make the corresponding
   adjustment to the .bss section offset computed from the ldinfo data
   when necessary.  This function returns the adjustment amount  (or
   zero when no adjustment is needed).  */

static CORE_ADDR
solib_aix_bss_data_overlap (bfd *abfd)
{
  struct bfd_section *data_sect, *bss_sect;

  data_sect = bfd_get_section_by_name (abfd, ".data");
  if (data_sect == NULL)
    return 0; /* No overlap possible.  */

  bss_sect = bfd_get_section_by_name (abfd, ".bss");
  if (bss_sect == NULL)
    return 0; /* No overlap possible.  */

  /* Assume the problem only occurs with linkers that place the .bss
     section after the .data section (the problem has only been
     observed when using the GNU linker, and the default linker
     script always places the .data and .bss sections in that order).  */
  if (bfd_section_vma (abfd, bss_sect)
      < bfd_section_vma (abfd, data_sect))
    return 0;

  if (bfd_section_vma (abfd, bss_sect)
      < bfd_section_vma (abfd, data_sect) + bfd_get_section_size (data_sect))
    return ((bfd_section_vma (abfd, data_sect)
	     + bfd_get_section_size (data_sect))
	    - bfd_section_vma (abfd, bss_sect));

  return 0;
}

/* Implement the "relocate_section_addresses" target_so_ops method.  */

static void
solib_aix_relocate_section_addresses (struct so_list *so,
				      struct target_section *sec)
{
  struct bfd_section *bfd_sect = sec->the_bfd_section;
  bfd *abfd = bfd_sect->owner;
  const char *section_name = bfd_section_name (abfd, bfd_sect);
  struct lm_info *info = so->lm_info;

  if (strcmp (section_name, ".text") == 0)
    {
      sec->addr = info->text_addr;
      sec->endaddr = sec->addr + info->text_size;

      /* The text address given to us by the loader contains
	 XCOFF headers, so we need to adjust by this much.  */
      sec->addr += bfd_sect->filepos;
    }
  else if (strcmp (section_name, ".data") == 0)
    {
      sec->addr = info->data_addr;
      sec->endaddr = sec->addr + info->data_size;
    }
  else if (strcmp (section_name, ".bss") == 0)
    {
      /* The information provided by the loader does not include
	 the address of the .bss section, but we know that it gets
	 relocated by the same offset as the .data section.  So,
	 compute the relocation offset for the .data section, and
	 apply it to the .bss section as well.  If the .data section
	 is not defined (which seems highly unlikely), do our best
	 by assuming no relocation.  */
      struct bfd_section *data_sect
	= bfd_get_section_by_name (abfd, ".data");
      CORE_ADDR data_offset = 0;

      if (data_sect != NULL)
	data_offset = info->data_addr - bfd_section_vma (abfd, data_sect);

      sec->addr = bfd_section_vma (abfd, bfd_sect) + data_offset;
      sec->addr += solib_aix_bss_data_overlap (abfd);
      sec->endaddr = sec->addr + bfd_section_size (abfd, bfd_sect);
    }
  else
    {
      /* All other sections should not be relocated.  */
      sec->addr = bfd_section_vma (abfd, bfd_sect);
      sec->endaddr = sec->addr + bfd_section_size (abfd, bfd_sect);
    }
}

/* Implement the "free_so" target_so_ops method.  */

static void
solib_aix_free_so (struct so_list *so)
{
  if (solib_aix_debug)
    fprintf_unfiltered (gdb_stdlog, "DEBUG: solib_aix_free_so (%s)\n",
			so->so_name);
  solib_aix_xfree_lm_info (so->lm_info);
}

/* Implement the "clear_solib" target_so_ops method.  */

static void
solib_aix_clear_solib (void)
{
  /* Nothing needed.  */
}

/* Compute and return the OBJFILE's section_offset array, using
   the associated loader info (INFO).

   The resulting array is computed on the heap and must be
   deallocated after use.  */

static struct section_offsets *
solib_aix_get_section_offsets (struct objfile *objfile,
			       struct lm_info *info)
{
  struct section_offsets *offsets;
  bfd *abfd = objfile->obfd;
  int i;

  offsets = XCALLOC (objfile->num_sections, struct section_offsets);

  /* .text */

  if (objfile->sect_index_text != -1)
    {
      struct bfd_section *sect
	= objfile->sections[objfile->sect_index_text].the_bfd_section;

      offsets->offsets[objfile->sect_index_text]
	= info->text_addr + sect->filepos - bfd_section_vma (abfd, sect);
    }

  /* .data */

  if (objfile->sect_index_data != -1)
    {
      struct bfd_section *sect
	= objfile->sections[objfile->sect_index_data].the_bfd_section;

      offsets->offsets[objfile->sect_index_data]
	= info->data_addr - bfd_section_vma (abfd, sect);
    }

  /* .bss

     The offset of the .bss section should be identical to the offset
     of the .data section.  If no .data section (which seems hard to
     believe it is possible), assume it is zero.  */

  if (objfile->sect_index_bss != -1
      && objfile->sect_index_data != -1)
    {
      offsets->offsets[objfile->sect_index_bss]
	= (offsets->offsets[objfile->sect_index_data]
	   + solib_aix_bss_data_overlap (abfd));
    }

  /* All other sections should not need relocation.  */

  return offsets;
}

/* Implement the "solib_create_inferior_hook" target_so_ops method.  */

static void
solib_aix_solib_create_inferior_hook (int from_tty)
{
  const char *warning_msg = "unable to relocate main executable";
  VEC (lm_info_p) *library_list;
  struct lm_info *exec_info;

  /* We need to relocate the main executable...  */

  library_list = solib_aix_get_library_list (current_inferior (),
					     warning_msg);
  if (library_list == NULL)
    return;  /* Warning already printed.  */

  if (VEC_length (lm_info_p, library_list) < 1)
    {
      warning (_("unable to relocate main executable (no info from loader)"));
      return;
    }

  exec_info = VEC_index (lm_info_p, library_list, 0);

  if (symfile_objfile != NULL)
    {
      struct section_offsets *offsets
	= solib_aix_get_section_offsets (symfile_objfile, exec_info);
      struct cleanup *cleanup = make_cleanup (xfree, offsets);

      objfile_relocate (symfile_objfile, offsets);
      do_cleanups (cleanup);
    }
}

/* Implement the "special_symbol_handling" target_so_ops method.  */

static void
solib_aix_special_symbol_handling (void)
{
  /* Nothing needed.  */
}

/* Implement the "current_sos" target_so_ops method.  */

static struct so_list *
solib_aix_current_sos (void)
{
  struct so_list *start = NULL, *last = NULL;
  VEC (lm_info_p) *library_list;
  struct lm_info *info;
  int ix;

  library_list = solib_aix_get_library_list (current_inferior (), NULL);
  if (library_list == NULL)
    return NULL;

  /* Build a struct so_list for each entry on the list.
     We skip the first entry, since this is the entry corresponding
     to the main executable, not a shared library.  */
  for (ix = 1; VEC_iterate (lm_info_p, library_list, ix, info); ix++)
    {
      struct so_list *new_solib = XZALLOC (struct so_list);
      char *so_name;

      if (info->member_name == NULL)
	{
	 /* INFO->FILENAME is probably not an archive, but rather
	    a shared object.  Unusual, but it should be possible
	    to link a program against a shared object directory,
	    without having to put it in an archive first.  */
	 so_name = xstrdup (info->filename);
	}
      else
	{
	 /* This is the usual case on AIX, where the shared object
	    is a member of an archive.  Create a synthetic so_name
	    that follows the same convention as AIX's ldd tool
	    (Eg: "/lib/libc.a(shr.o)").  */
	 so_name = xstrprintf ("%s(%s)", info->filename, info->member_name);
	}
      strncpy (new_solib->so_original_name, so_name,
	       SO_NAME_MAX_PATH_SIZE - 1);
      new_solib->so_name[SO_NAME_MAX_PATH_SIZE - 1] = '\0';
      memcpy (new_solib->so_name, new_solib->so_original_name,
	      SO_NAME_MAX_PATH_SIZE);
      new_solib->lm_info = solib_aix_new_lm_info (info);

      /* Add it to the list.  */
      if (!start)
        last = start = new_solib;
      else
        {
          last->next = new_solib;
          last = new_solib;
        }
    }

  return start;
}

/* Implement the "open_symbol_file_object" target_so_ops method.  */

static int
solib_aix_open_symbol_file_object (void *from_ttyp)
{
  return 0;
}

/* Implement the "in_dynsym_resolve_code" target_so_ops method.  */

static int
solib_aix_in_dynsym_resolve_code (CORE_ADDR pc)
{
  return 0;
}

/* Implement the "bfd_open" target_so_ops method.  */

static bfd *
solib_aix_bfd_open (char *pathname)
{
  /* The pathname is actually a synthetic filename with the following
     form: "/path/to/sharedlib(member.o)" (double-quotes excluded).
     split this into archive name and member name.

     FIXME: This is a little hacky.  Perhaps we should provide access
     to the solib's lm_info here?  */
  const int path_len = strlen (pathname);
  char *sep;
  char *filename;
  int filename_len;
  char *member_name;
  bfd *archive_bfd, *object_bfd;
  struct cleanup *cleanup;

  if (pathname[path_len - 1] != ')')
    return solib_bfd_open (pathname);

  /* Search for the associated parens.  */
  sep = strrchr (pathname, '(');
  if (sep == NULL)
    {
      /* Should never happen, but recover as best as we can (trying
	 to open pathname without decoding, possibly leading to
	 a failure), rather than triggering an assert failure).  */
      warning (_("missing '(' in shared object pathname: %s"), pathname);
      return solib_bfd_open (pathname);
    }
  filename_len = sep - pathname;

  filename = xstrprintf ("%.*s", filename_len, pathname);
  cleanup = make_cleanup (xfree, filename);
  member_name = xstrprintf ("%.*s", path_len - filename_len - 2, sep + 1);
  make_cleanup (xfree, member_name);

  archive_bfd = gdb_bfd_open (filename, gnutarget, -1);
  if (archive_bfd == NULL)
    {
      warning (_("Could not open `%s' as an executable file: %s"),
	       filename, bfd_errmsg (bfd_get_error ()));
      do_cleanups (cleanup);
      return NULL;
    }

  if (bfd_check_format (archive_bfd, bfd_object))
    {
      do_cleanups (cleanup);
      return archive_bfd;
    }

  if (! bfd_check_format (archive_bfd, bfd_archive))
    {
      warning (_("\"%s\": not in executable format: %s."),
	       filename, bfd_errmsg (bfd_get_error ()));
      gdb_bfd_unref (archive_bfd);
      do_cleanups (cleanup);
      return NULL;
    }

  object_bfd = gdb_bfd_openr_next_archived_file (archive_bfd, NULL);
  while (object_bfd != NULL)
    {
      bfd *next;

      if (strcmp (member_name, object_bfd->filename) == 0)
	break;

      next = gdb_bfd_openr_next_archived_file (archive_bfd, object_bfd);
      gdb_bfd_unref (object_bfd);
      object_bfd = next;
    }

  if (object_bfd == NULL)
    {
      warning (_("\"%s\": member \"%s\" missing."), filename, member_name);
      gdb_bfd_unref (archive_bfd);
      do_cleanups (cleanup);
      return NULL;
    }

  if (! bfd_check_format (object_bfd, bfd_object))
    {
      warning (_("%s(%s): not in object format: %s."),
	       filename, member_name, bfd_errmsg (bfd_get_error ()));
      gdb_bfd_unref (archive_bfd);
      gdb_bfd_unref (object_bfd);
      do_cleanups (cleanup);
      return NULL;
    }

  /* Override the returned bfd's name with our synthetic name in order
     to allow commands listing all shared libraries to display that
     synthetic name.  Otherwise, we would only be displaying the name
     of the archive member object.  */
  xfree (bfd_get_filename (object_bfd));
  object_bfd->filename = xstrdup (pathname);

  gdb_bfd_unref (archive_bfd);
  do_cleanups (cleanup);
  return object_bfd;
}

/* Return the obj_section corresponding to OBJFILE's data section,
   or NULL if not found.  */
/* FIXME: Define in a more general location? */

static struct obj_section *
data_obj_section_from_objfile (struct objfile *objfile)
{
  struct obj_section *osect;

  ALL_OBJFILE_OSECTIONS (objfile, osect)
    if (strcmp (bfd_section_name (objfile->obfd, osect->the_bfd_section),
		".data") == 0)
      return osect;

  return NULL;
}

/* Return the TOC value corresponding to the given PC address,
   or raise an error if the value could not be determined.  */

CORE_ADDR
solib_aix_get_toc_value (CORE_ADDR pc)
{
  struct obj_section *pc_osect = find_pc_section (pc);
  struct obj_section *data_osect;
  CORE_ADDR result;

  if (pc_osect == NULL)
    error (_("unable to find TOC entry for pc %s "
	     "(no section contains this PC)"),
	   core_addr_to_string (pc));

  data_osect = data_obj_section_from_objfile (pc_osect->objfile);
  if (data_osect == NULL)
    error (_("unable to find TOC entry for pc %s "
	     "(%s has no data section)"),
	   core_addr_to_string (pc), objfile_name (pc_osect->objfile));

  result = (obj_section_addr (data_osect)
	    + xcoff_get_toc_offset (pc_osect->objfile));
  if (solib_aix_debug)
    fprintf_unfiltered (gdb_stdlog,
			"DEBUG: solib_aix_get_toc_value (pc=%s) -> %s\n",
			core_addr_to_string (pc),
			core_addr_to_string (result));

  return result;
}

/* This module's normal_stop observer.  */

static void
solib_aix_normal_stop_observer (struct bpstats *unused_1, int unused_2)
{
  struct solib_aix_inferior_data *data
    = get_solib_aix_inferior_data (current_inferior ());

  /* The inferior execution has been resumed, and it just stopped
     again.  This means that the list of shared libraries may have
     evolved.  Reset our cached value.  */
  solib_aix_free_library_list (&data->library_list);
}

/* Implements the "show debug aix-solib" command.  */

static void
show_solib_aix_debug (struct ui_file *file, int from_tty,
		      struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("solib-aix debugging is %s.\n"), value);
}

/* The target_so_ops for AIX targets.  */
struct target_so_ops solib_aix_so_ops;

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_solib_aix;

void
_initialize_solib_aix (void)
{
  solib_aix_so_ops.relocate_section_addresses
    = solib_aix_relocate_section_addresses;
  solib_aix_so_ops.free_so = solib_aix_free_so;
  solib_aix_so_ops.clear_solib = solib_aix_clear_solib;
  solib_aix_so_ops.solib_create_inferior_hook
    = solib_aix_solib_create_inferior_hook;
  solib_aix_so_ops.special_symbol_handling
    = solib_aix_special_symbol_handling;
  solib_aix_so_ops.current_sos = solib_aix_current_sos;
  solib_aix_so_ops.open_symbol_file_object
    = solib_aix_open_symbol_file_object;
  solib_aix_so_ops.in_dynsym_resolve_code
    = solib_aix_in_dynsym_resolve_code;
  solib_aix_so_ops.bfd_open = solib_aix_bfd_open;

  solib_aix_inferior_data_handle = register_inferior_data ();

  observer_attach_normal_stop (solib_aix_normal_stop_observer);

  /* Debug this file's internals.  */
  add_setshow_boolean_cmd ("aix-solib", class_maintenance,
			   &solib_aix_debug, _("\
Control the debugging traces for the solib-aix module."), _("\
Show whether solib-aix debugging traces are enabled."), _("\
When on, solib-aix debugging traces are enabled."),
                            NULL,
                            show_solib_aix_debug,
                            &setdebuglist, &showdebuglist);
}
