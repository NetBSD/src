/* Functions that provide the mechanism to parse a syscall XML file
   and get its values.

   Copyright (C) 2009-2016 Free Software Foundation, Inc.

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
#include "gdbarch.h"

/* For the struct syscall definition.  */
#include "target.h"

#include "filenames.h"

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
set_xml_syscall_file_name (struct gdbarch *gdbarch, const char *name)
{
  return;
}

void
get_syscall_by_number (struct gdbarch *gdbarch,
		       int syscall_number, struct syscall *s)
{
  syscall_warn_user ();
  s->number = syscall_number;
  s->name = NULL;
}

void
get_syscall_by_name (struct gdbarch *gdbarch, const char *syscall_name,
		     struct syscall *s)
{
  syscall_warn_user ();
  s->number = UNKNOWN_SYSCALL;
  s->name = syscall_name;
}

const char **
get_syscall_names (struct gdbarch *gdbarch)
{
  syscall_warn_user ();
  return NULL;
}

struct syscall *
get_syscalls_by_group (struct gdbarch *gdbarch, const char *group)
{
  syscall_warn_user ();
  return NULL;
}

const char **
get_syscall_group_names (struct gdbarch *gdbarch)
{
  syscall_warn_user ();
  return NULL;
}

#else /* ! HAVE_LIBEXPAT */

/* Structure which describes a syscall.  */
typedef struct syscall_desc
{
  /* The syscall number.  */

  int number;

  /* The syscall name.  */

  char *name;
} *syscall_desc_p;
DEF_VEC_P(syscall_desc_p);

/* Structure of a syscall group.  */
typedef struct syscall_group_desc
{
  /* The group name.  */

  char *name;

  /* The syscalls that are part of the group.  */

  VEC(syscall_desc_p) *syscalls;
} *syscall_group_desc_p;
DEF_VEC_P(syscall_group_desc_p);

/* Structure that represents syscalls information.  */
struct syscalls_info
{
  /* The syscalls.  */

  VEC(syscall_desc_p) *syscalls;

  /* The syscall groups.  */

  VEC(syscall_group_desc_p) *groups;

  /* Variable that will hold the last known data-directory.  This is
     useful to know whether we should re-read the XML info for the
     target.  */

  char *my_gdb_datadir;
};

/* Callback data for syscall information parsing.  */
struct syscall_parsing_data
{
  /* The syscalls_info we are building.  */

  struct syscalls_info *syscalls_info;
};

static struct syscalls_info *
allocate_syscalls_info (void)
{
  return XCNEW (struct syscalls_info);
}

static void
syscalls_info_free_syscalls_desc (struct syscall_desc *sd)
{
  xfree (sd->name);
}

/* Free syscall_group_desc members but not the structure itself.  */

static void
syscalls_info_free_syscall_group_desc (struct syscall_group_desc *sd)
{
  VEC_free (syscall_desc_p, sd->syscalls);
  xfree (sd->name);
}

static void
free_syscalls_info (void *arg)
{
  struct syscalls_info *syscalls_info = (struct syscalls_info *) arg;
  struct syscall_desc *sysdesc;
  struct syscall_group_desc *groupdesc;
  int i;

  xfree (syscalls_info->my_gdb_datadir);

  if (syscalls_info->syscalls != NULL)
    {
      for (i = 0;
	   VEC_iterate (syscall_desc_p, syscalls_info->syscalls, i, sysdesc);
	   i++)
	syscalls_info_free_syscalls_desc (sysdesc);
      VEC_free (syscall_desc_p, syscalls_info->syscalls);
    }

  if (syscalls_info->groups != NULL)
    {
      for (i = 0;
	   VEC_iterate (syscall_group_desc_p,
			syscalls_info->groups, i, groupdesc);
	   i++)
	syscalls_info_free_syscall_group_desc (groupdesc);

      VEC_free (syscall_group_desc_p, syscalls_info->groups);
    }

  xfree (syscalls_info);
}

static struct cleanup *
make_cleanup_free_syscalls_info (struct syscalls_info *syscalls_info)
{
  return make_cleanup (free_syscalls_info, syscalls_info);
}

/* Create a new syscall group.  Return pointer to the
   syscall_group_desc structure that represents the new group.  */

static struct syscall_group_desc *
syscall_group_create_syscall_group_desc (struct syscalls_info *syscalls_info,
					 const char *group)
{
  struct syscall_group_desc *groupdesc = XCNEW (struct syscall_group_desc);

  groupdesc->name = xstrdup (group);

  VEC_safe_push (syscall_group_desc_p, syscalls_info->groups, groupdesc);

  return groupdesc;
}

/* Add a syscall to the group.  If group doesn't exist, create it.  */

static void
syscall_group_add_syscall (struct syscalls_info *syscalls_info,
			   struct syscall_desc *syscall,
			   const char *group)
{
  struct syscall_group_desc *groupdesc = NULL;
  int i;

  /* Search for an existing group.  */
  for (i = 0;
       VEC_iterate (syscall_group_desc_p, syscalls_info->groups, i, groupdesc);
       i++)
    {
      if (strcmp (groupdesc->name, group) == 0)
	break;
    }

  if (groupdesc == NULL)
    {
      /* No group was found with this name.  We must create a new
	 one.  */
      groupdesc = syscall_group_create_syscall_group_desc (syscalls_info,
							   group);
    }

  VEC_safe_push (syscall_desc_p, groupdesc->syscalls, syscall);
}

static void
syscall_create_syscall_desc (struct syscalls_info *syscalls_info,
			     const char *name, int number,
			     char *groups)
{
  struct syscall_desc *sysdesc = XCNEW (struct syscall_desc);
  char *group;

  sysdesc->name = xstrdup (name);
  sysdesc->number = number;

  VEC_safe_push (syscall_desc_p, syscalls_info->syscalls, sysdesc);

  /*  Add syscall to its groups.  */
  if (groups != NULL)
    {
      for (group = strtok (groups, ",");
	   group != NULL;
	   group = strtok (NULL, ","))
	syscall_group_add_syscall (syscalls_info, sysdesc, group);
    }
}

/* Handle the start of a <syscall> element.  */
static void
syscall_start_syscall (struct gdb_xml_parser *parser,
                       const struct gdb_xml_element *element,
                       void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct syscall_parsing_data *data = (struct syscall_parsing_data *) user_data;
  struct gdb_xml_value *attrs = VEC_address (gdb_xml_value_s, attributes);
  int len, i;
  /* syscall info.  */
  char *name = NULL;
  int number = 0;
  char *groups = NULL;

  len = VEC_length (gdb_xml_value_s, attributes);

  for (i = 0; i < len; i++)
    {
      if (strcmp (attrs[i].name, "name") == 0)
        name = (char *) attrs[i].value;
      else if (strcmp (attrs[i].name, "number") == 0)
        number = * (ULONGEST *) attrs[i].value;
      else if (strcmp (attrs[i].name, "groups") == 0)
        groups = (char *) attrs[i].value;
      else
        internal_error (__FILE__, __LINE__,
                        _("Unknown attribute name '%s'."), attrs[i].name);
    }

  gdb_assert (name);
  syscall_create_syscall_desc (data->syscalls_info, name, number, groups);
}


/* The elements and attributes of an XML syscall document.  */
static const struct gdb_xml_attribute syscall_attr[] = {
  { "number", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "groups", GDB_XML_AF_OPTIONAL, NULL, NULL },
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

  data.syscalls_info = allocate_syscalls_info ();
  result_cleanup = make_cleanup_free_syscalls_info (data.syscalls_info);

  if (gdb_xml_parse_quick (_("syscalls info"), NULL,
			   syselements, document, &data) == 0)
    {
      /* Parsed successfully.  */
      discard_cleanups (result_cleanup);
      return data.syscalls_info;
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
static struct syscalls_info *
xml_init_syscalls_info (const char *filename)
{
  char *full_file;
  char *dirname;
  struct syscalls_info *syscalls_info;
  struct cleanup *back_to;

  full_file = xml_fetch_content_from_file (filename, gdb_datadir);
  if (full_file == NULL)
    return NULL;

  back_to = make_cleanup (xfree, full_file);

  dirname = ldirname (filename);
  if (dirname != NULL)
    make_cleanup (xfree, dirname);

  syscalls_info = syscall_parse_xml (full_file,
				     xml_fetch_content_from_file, dirname);
  do_cleanups (back_to);

  return syscalls_info;
}

/* Initializes the syscalls_info structure according to the
   architecture.  */
static void
init_syscalls_info (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  const char *xml_syscall_file = gdbarch_xml_syscall_file (gdbarch);

  /* Should we re-read the XML info for this target?  */
  if (syscalls_info != NULL && syscalls_info->my_gdb_datadir != NULL
      && filename_cmp (syscalls_info->my_gdb_datadir, gdb_datadir) != 0)
    {
      /* The data-directory changed from the last time we used it.
	 It means that we have to re-read the XML info.  */
      free_syscalls_info (syscalls_info);
      syscalls_info = NULL;
      set_gdbarch_syscalls_info (gdbarch, NULL);
    }

  /* Did we succeed at initializing this?  */
  if (syscalls_info != NULL)
    return;

  syscalls_info = xml_init_syscalls_info (xml_syscall_file);

  /* If there was some error reading the XML file, we initialize
     gdbarch->syscalls_info anyway, in order to store information
     about our attempt.  */
  if (syscalls_info == NULL)
    syscalls_info = allocate_syscalls_info ();

  if (syscalls_info->syscalls == NULL)
    {
      if (xml_syscall_file != NULL)
	warning (_("Could not load the syscall XML file `%s/%s'."),
		 gdb_datadir, xml_syscall_file);
      else
	warning (_("There is no XML file to open."));

      warning (_("GDB will not be able to display "
		 "syscall names nor to verify if\n"
		 "any provided syscall numbers are valid."));
    }

  /* Saving the data-directory used to read this XML info.  */
  syscalls_info->my_gdb_datadir = xstrdup (gdb_datadir);

  set_gdbarch_syscalls_info (gdbarch, syscalls_info);
}

/* Search for a syscall group by its name.  Return syscall_group_desc
   structure for the group if found or NULL otherwise.  */

static struct syscall_group_desc *
syscall_group_get_group_by_name (const struct syscalls_info *syscalls_info,
				 const char *group)
{
  struct syscall_group_desc *groupdesc;
  int i;

  if (syscalls_info == NULL)
    return NULL;

  if (group == NULL)
    return NULL;

   /* Search for existing group.  */
  for (i = 0;
       VEC_iterate (syscall_group_desc_p, syscalls_info->groups, i, groupdesc);
       i++)
    {
      if (strcmp (groupdesc->name, group) == 0)
	return groupdesc;
    }

  return NULL;
}

static int
xml_get_syscall_number (struct gdbarch *gdbarch,
                        const char *syscall_name)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_desc *sysdesc;
  int i;

  if (syscalls_info == NULL
      || syscall_name == NULL)
    return UNKNOWN_SYSCALL;

  for (i = 0;
       VEC_iterate(syscall_desc_p, syscalls_info->syscalls, i, sysdesc);
       i++)
    if (strcmp (sysdesc->name, syscall_name) == 0)
      return sysdesc->number;

  return UNKNOWN_SYSCALL;
}

static const char *
xml_get_syscall_name (struct gdbarch *gdbarch,
                      int syscall_number)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_desc *sysdesc;
  int i;

  if (syscalls_info == NULL
      || syscall_number < 0)
    return NULL;

  for (i = 0;
       VEC_iterate(syscall_desc_p, syscalls_info->syscalls, i, sysdesc);
       i++)
    if (sysdesc->number == syscall_number)
      return sysdesc->name;

  return NULL;
}

static const char **
xml_list_of_syscalls (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_desc *sysdesc;
  const char **names = NULL;
  int nsyscalls;
  int i;

  if (syscalls_info == NULL)
    return NULL;

  nsyscalls = VEC_length (syscall_desc_p, syscalls_info->syscalls);
  names = XNEWVEC (const char *, nsyscalls + 1);

  for (i = 0;
       VEC_iterate (syscall_desc_p, syscalls_info->syscalls, i, sysdesc);
       i++)
    names[i] = sysdesc->name;

  names[i] = NULL;

  return names;
}

/* Iterate over the syscall_group_desc element to return a list of
   syscalls that are part of the given group, terminated by an empty
   element.  If the syscall group doesn't exist, return NULL.  */

static struct syscall *
xml_list_syscalls_by_group (struct gdbarch *gdbarch, const char *group)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_group_desc *groupdesc;
  struct syscall_desc *sysdesc;
  struct syscall *syscalls = NULL;
  int nsyscalls;
  int i;

  if (syscalls_info == NULL)
    return NULL;

  groupdesc = syscall_group_get_group_by_name (syscalls_info, group);
  if (groupdesc == NULL)
    return NULL;

  nsyscalls = VEC_length (syscall_desc_p, groupdesc->syscalls);
  syscalls = (struct syscall*) xmalloc ((nsyscalls + 1)
					* sizeof (struct syscall));

  for (i = 0;
       VEC_iterate (syscall_desc_p, groupdesc->syscalls, i, sysdesc);
       i++)
    {
      syscalls[i].name = sysdesc->name;
      syscalls[i].number = sysdesc->number;
    }

  /* Add final element marker.  */
  syscalls[i].name = NULL;
  syscalls[i].number = 0;

  return syscalls;
}

/* Return a NULL terminated list of syscall groups or an empty list, if
   no syscall group is available.  Return NULL, if there is no syscall
   information available.  */

static const char **
xml_list_of_groups (struct gdbarch *gdbarch)
{
  struct syscalls_info *syscalls_info = gdbarch_syscalls_info (gdbarch);
  struct syscall_group_desc *groupdesc;
  const char **names = NULL;
  int i;
  int ngroups;

  if (syscalls_info == NULL)
    return NULL;

  ngroups = VEC_length (syscall_group_desc_p, syscalls_info->groups);
  names = (const char**) xmalloc ((ngroups + 1) * sizeof (char *));

  for (i = 0;
       VEC_iterate (syscall_group_desc_p, syscalls_info->groups, i, groupdesc);
       i++)
    names[i] = groupdesc->name;

  names[i] = NULL;

  return names;
}

void
set_xml_syscall_file_name (struct gdbarch *gdbarch, const char *name)
{
  set_gdbarch_xml_syscall_file (gdbarch, name);
}

void
get_syscall_by_number (struct gdbarch *gdbarch,
		       int syscall_number, struct syscall *s)
{
  init_syscalls_info (gdbarch);

  s->number = syscall_number;
  s->name = xml_get_syscall_name (gdbarch, syscall_number);
}

void
get_syscall_by_name (struct gdbarch *gdbarch,
		     const char *syscall_name, struct syscall *s)
{
  init_syscalls_info (gdbarch);

  s->number = xml_get_syscall_number (gdbarch, syscall_name);
  s->name = syscall_name;
}

const char **
get_syscall_names (struct gdbarch *gdbarch)
{
  init_syscalls_info (gdbarch);

  return xml_list_of_syscalls (gdbarch);
}

/* See comment in xml-syscall.h.  */

struct syscall *
get_syscalls_by_group (struct gdbarch *gdbarch, const char *group)
{
  init_syscalls_info (gdbarch);

  return xml_list_syscalls_by_group (gdbarch, group);
}

/* See comment in xml-syscall.h.  */

const char **
get_syscall_group_names (struct gdbarch *gdbarch)
{
  init_syscalls_info (gdbarch);

  return xml_list_of_groups (gdbarch);
}

#endif /* ! HAVE_LIBEXPAT */
