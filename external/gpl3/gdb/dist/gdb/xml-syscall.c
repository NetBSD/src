/* Functions that provide the mechanism to parse a syscall XML file
   and get its values.

   Copyright (C) 2009-2013 Free Software Foundation, Inc.

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
#include "gdbtypes.h"
#include "xml-support.h"
#include "xml-syscall.h"

/* For the struct syscall definition.  */
#include "target.h"

#include "filenames.h"

#include "gdb_assert.h"

#ifndef HAVE_LIBEXPAT

/* Dummy functions to indicate that there's no support for fetching
   syscalls information.  */

static void
syscall_warn_user (void)
{
  static int have_warned = 0;
  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML syscalls information; XML support was "
		 "disabled at compile time."));
    }
}

void
set_xml_syscall_file_name (const char *name)
{
  return;
}

void
get_syscall_by_number (int syscall_number,
                       struct syscall *s)
{
  syscall_warn_user ();
  s->number = syscall_number;
  s->name = NULL;
}

void
get_syscall_by_name (const char *syscall_name,
                     struct syscall *s)
{
  syscall_warn_user ();
  s->number = UNKNOWN_SYSCALL;
  s->name = syscall_name;
}

const char **
get_syscall_names (void)
{
  syscall_warn_user ();
  return NULL;
}

#else /* ! HAVE_LIBEXPAT */

/* Variable that will hold the last known data-directory.  This is useful to
   know whether we should re-read the XML info for the target.  */
static char *my_gdb_datadir = NULL;

/* Structure which describes a syscall.  */
typedef struct syscall_desc
{
  /* The syscall number.  */

  int number;

  /* The syscall name.  */

  char *name;
} *syscall_desc_p;
DEF_VEC_P(syscall_desc_p);

/* Structure that represents syscalls information.  */
struct syscalls_info
{
  /* The syscalls.  */

  VEC(syscall_desc_p) *syscalls;
};

/* Callback data for syscall information parsing.  */
struct syscall_parsing_data
{
  /* The syscalls_info we are building.  */

  struct syscalls_info *sysinfo;
};

/* Structure used to store information about the available syscalls in
   the system.  */
static const struct syscalls_info *sysinfo = NULL;

/* A flag to tell if we already initialized the structure above.  */
static int have_initialized_sysinfo = 0;

/* The filename of the syscall's XML.  */
static const char *xml_syscall_file = NULL;

static struct syscalls_info *
allocate_syscalls_info (void)
{
  return XZALLOC (struct syscalls_info);
}

static void
sysinfo_free_syscalls_desc (struct syscall_desc *sd)
{
  xfree (sd->name);
}

static void
free_syscalls_info (void *arg)
{
  struct syscalls_info *sysinfo = arg;
  struct syscall_desc *sysdesc;
  int i;

  for (i = 0;
       VEC_iterate (syscall_desc_p, sysinfo->syscalls, i, sysdesc);
       i++)
    sysinfo_free_syscalls_desc (sysdesc);
  VEC_free (syscall_desc_p, sysinfo->syscalls);

  xfree (sysinfo);
}

static struct cleanup *
make_cleanup_free_syscalls_info (struct syscalls_info *sysinfo)
{
  return make_cleanup (free_syscalls_info, sysinfo);
}

static void
syscall_create_syscall_desc (struct syscalls_info *sysinfo,
                             const char *name, int number)
{
  struct syscall_desc *sysdesc = XZALLOC (struct syscall_desc);

  sysdesc->name = xstrdup (name);
  sysdesc->number = number;

  VEC_safe_push (syscall_desc_p, sysinfo->syscalls, sysdesc);
}

/* Handle the start of a <syscall> element.  */
static void
syscall_start_syscall (struct gdb_xml_parser *parser,
                       const struct gdb_xml_element *element,
                       void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct syscall_parsing_data *data = user_data;
  struct gdb_xml_value *attrs = VEC_address (gdb_xml_value_s, attributes);
  int len, i;
  /* syscall info.  */
  char *name = NULL;
  int number = 0;

  len = VEC_length (gdb_xml_value_s, attributes);

  for (i = 0; i < len; i++)
    {
      if (strcmp (attrs[i].name, "name") == 0)
        name = attrs[i].value;
      else if (strcmp (attrs[i].name, "number") == 0)
        number = * (ULONGEST *) attrs[i].value;
      else
        internal_error (__FILE__, __LINE__,
                        _("Unknown attribute name '%s'."), attrs[i].name);
    }

  gdb_assert (name);
  syscall_create_syscall_desc (data->sysinfo, name, number);
}


/* The elements and attributes of an XML syscall document.  */
static const struct gdb_xml_attribute syscall_attr[] = {
  { "number", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element syscalls_info_children[] = {
  { "syscall", syscall_attr, NULL,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    syscall_start_syscall, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element syselements[] = {
  { "syscalls_info", NULL, syscalls_info_children,
    GDB_XML_EF_NONE, NULL, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static struct syscalls_info *
syscall_parse_xml (const char *document, xml_fetch_another fetcher,
                   void *fetcher_baton)
{
  struct cleanup *result_cleanup;
  struct syscall_parsing_data data;

  data.sysinfo = allocate_syscalls_info ();
  result_cleanup = make_cleanup_free_syscalls_info (data.sysinfo);

  if (gdb_xml_parse_quick (_("syscalls info"), NULL,
			   syselements, document, &data) == 0)
    {
      /* Parsed successfully.  */
      discard_cleanups (result_cleanup);
      return data.sysinfo;
    }
  else
    {
      warning (_("Could not load XML syscalls info; ignoring"));
      do_cleanups (result_cleanup);
      return NULL;
    }
}

/* Function responsible for initializing the information
   about the syscalls.  It reads the XML file and fills the
   struct syscalls_info with the values.
   
   Returns the struct syscalls_info if the file is valid, NULL otherwise.  */
static const struct syscalls_info *
xml_init_syscalls_info (const char *filename)
{
  char *full_file;
  char *dirname;
  struct syscalls_info *sysinfo;
  struct cleanup *back_to;

  full_file = xml_fetch_content_from_file (filename, gdb_datadir);
  if (full_file == NULL)
    return NULL;

  back_to = make_cleanup (xfree, full_file);

  dirname = ldirname (filename);
  if (dirname != NULL)
    make_cleanup (xfree, dirname);

  sysinfo = syscall_parse_xml (full_file,
			       xml_fetch_content_from_file, dirname);
  do_cleanups (back_to);

  return sysinfo;
}

/* Initializes the syscalls_info structure according to the
   architecture.  */
static void
init_sysinfo (void)
{
  /* Should we re-read the XML info for this target?  */
  if (my_gdb_datadir && filename_cmp (my_gdb_datadir, gdb_datadir) != 0)
    {
      /* The data-directory changed from the last time we used it.
	 It means that we have to re-read the XML info.  */
      have_initialized_sysinfo = 0;
      xfree (my_gdb_datadir);
      my_gdb_datadir = NULL;
      if (sysinfo)
	free_syscalls_info ((void *) sysinfo);
    }

  /* Did we already try to initialize the structure?  */
  if (have_initialized_sysinfo)
    return;

  sysinfo = xml_init_syscalls_info (xml_syscall_file);

  have_initialized_sysinfo = 1;

  if (sysinfo == NULL)
    {
      if (xml_syscall_file)
	warning (_("Could not load the syscall XML file `%s/%s'."),
		 gdb_datadir, xml_syscall_file);
      else
	warning (_("There is no XML file to open."));

      warning (_("GDB will not be able to display "
		 "syscall names nor to verify if\n"
		 "any provided syscall numbers are valid."));
    }

  /* Saving the data-directory used to read this XML info.  */
  my_gdb_datadir = xstrdup (gdb_datadir);
}

static int
xml_get_syscall_number (const struct syscalls_info *sysinfo,
                        const char *syscall_name)
{
  struct syscall_desc *sysdesc;
  int i;

  if (sysinfo == NULL
      || syscall_name == NULL)
    return UNKNOWN_SYSCALL;

  for (i = 0;
       VEC_iterate(syscall_desc_p, sysinfo->syscalls, i, sysdesc);
       i++)
    if (strcmp (sysdesc->name, syscall_name) == 0)
      return sysdesc->number;

  return UNKNOWN_SYSCALL;
}

static const char *
xml_get_syscall_name (const struct syscalls_info *sysinfo,
                      int syscall_number)
{
  struct syscall_desc *sysdesc;
  int i;

  if (sysinfo == NULL
      || syscall_number < 0)
    return NULL;

  for (i = 0;
       VEC_iterate(syscall_desc_p, sysinfo->syscalls, i, sysdesc);
       i++)
    if (sysdesc->number == syscall_number)
      return sysdesc->name;

  return NULL;
}

static const char **
xml_list_of_syscalls (const struct syscalls_info *sysinfo)
{
  struct syscall_desc *sysdesc;
  const char **names = NULL;
  int nsyscalls;
  int i;

  if (sysinfo == NULL)
    return NULL;

  nsyscalls = VEC_length (syscall_desc_p, sysinfo->syscalls);
  names = xmalloc ((nsyscalls + 1) * sizeof (char *));

  for (i = 0;
       VEC_iterate (syscall_desc_p, sysinfo->syscalls, i, sysdesc);
       i++)
    names[i] = sysdesc->name;

  names[i] = NULL;

  return names;
}

void
set_xml_syscall_file_name (const char *name)
{
  xml_syscall_file = name;
}

void
get_syscall_by_number (int syscall_number,
                       struct syscall *s)
{
  init_sysinfo ();

  s->number = syscall_number;
  s->name = xml_get_syscall_name (sysinfo, syscall_number);
}

void
get_syscall_by_name (const char *syscall_name,
                     struct syscall *s)
{
  init_sysinfo ();

  s->number = xml_get_syscall_number (sysinfo, syscall_name);
  s->name = syscall_name;
}

const char **
get_syscall_names (void)
{
  init_sysinfo ();

  return xml_list_of_syscalls (sysinfo);
}

#endif /* ! HAVE_LIBEXPAT */
