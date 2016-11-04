/* Generic static probe support for GDB.

   Copyright (C) 2012-2016 Free Software Foundation, Inc.

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
#include "probe.h"
#include "command.h"
#include "cli/cli-cmds.h"
#include "cli/cli-utils.h"
#include "objfiles.h"
#include "symtab.h"
#include "progspace.h"
#include "filenames.h"
#include "linespec.h"
#include "gdb_regex.h"
#include "frame.h"
#include "arch-utils.h"
#include "value.h"
#include "ax.h"
#include "ax-gdb.h"
#include "location.h"
#include <ctype.h>

typedef struct bound_probe bound_probe_s;
DEF_VEC_O (bound_probe_s);



/* A helper for parse_probes that decodes a probe specification in
   SEARCH_PSPACE.  It appends matching SALs to RESULT.  */

static void
parse_probes_in_pspace (const struct probe_ops *probe_ops,
			struct program_space *search_pspace,
			const char *objfile_namestr,
			const char *provider,
			const char *name,
			struct symtabs_and_lines *result)
{
  struct objfile *objfile;

  ALL_PSPACE_OBJFILES (search_pspace, objfile)
    {
      VEC (probe_p) *probes;
      struct probe *probe;
      int ix;

      if (!objfile->sf || !objfile->sf->sym_probe_fns)
	continue;

      if (objfile_namestr
	  && FILENAME_CMP (objfile_name (objfile), objfile_namestr) != 0
	  && FILENAME_CMP (lbasename (objfile_name (objfile)),
			   objfile_namestr) != 0)
	continue;

      probes = objfile->sf->sym_probe_fns->sym_get_probes (objfile);

      for (ix = 0; VEC_iterate (probe_p, probes, ix, probe); ix++)
	{
	  struct symtab_and_line *sal;

	  if (probe_ops != &probe_ops_any && probe->pops != probe_ops)
	    continue;

	  if (provider && strcmp (probe->provider, provider) != 0)
	    continue;

	  if (strcmp (probe->name, name) != 0)
	    continue;

	  ++result->nelts;
	  result->sals = XRESIZEVEC (struct symtab_and_line, result->sals,
				     result->nelts);
	  sal = &result->sals[result->nelts - 1];

	  init_sal (sal);

	  sal->pc = get_probe_address (probe, objfile);
	  sal->explicit_pc = 1;
	  sal->section = find_pc_overlay (sal->pc);
	  sal->pspace = search_pspace;
	  sal->probe = probe;
	  sal->objfile = objfile;
	}
    }
}

/* See definition in probe.h.  */

struct symtabs_and_lines
parse_probes (const struct event_location *location,
	      struct program_space *search_pspace,
	      struct linespec_result *canonical)
{
  char *arg_end, *arg;
  char *objfile_namestr = NULL, *provider = NULL, *name, *p;
  struct cleanup *cleanup;
  struct symtabs_and_lines result;
  const struct probe_ops *probe_ops;
  const char *arg_start, *cs;

  result.sals = NULL;
  result.nelts = 0;

  gdb_assert (event_location_type (location) == PROBE_LOCATION);
  arg_start = get_probe_location (location);

  cs = arg_start;
  probe_ops = probe_linespec_to_ops (&cs);
  if (probe_ops == NULL)
    error (_("'%s' is not a probe linespec"), arg_start);

  arg = (char *) cs;
  arg = skip_spaces (arg);
  if (!*arg)
    error (_("argument to `%s' missing"), arg_start);

  arg_end = skip_to_space (arg);

  /* We make a copy here so we can write over parts with impunity.  */
  arg = savestring (arg, arg_end - arg);
  cleanup = make_cleanup (xfree, arg);

  /* Extract each word from the argument, separated by ":"s.  */
  p = strchr (arg, ':');
  if (p == NULL)
    {
      /* This is `-p name'.  */
      name = arg;
    }
  else
    {
      char *hold = p + 1;

      *p = '\0';
      p = strchr (hold, ':');
      if (p == NULL)
	{
	  /* This is `-p provider:name'.  */
	  provider = arg;
	  name = hold;
	}
      else
	{
	  /* This is `-p objfile:provider:name'.  */
	  *p = '\0';
	  objfile_namestr = arg;
	  provider = hold;
	  name = p + 1;
	}
    }

  if (*name == '\0')
    error (_("no probe name specified"));
  if (provider && *provider == '\0')
    error (_("invalid provider name"));
  if (objfile_namestr && *objfile_namestr == '\0')
    error (_("invalid objfile name"));

  if (search_pspace != NULL)
    {
      parse_probes_in_pspace (probe_ops, search_pspace, objfile_namestr,
			      provider, name, &result);
    }
  else
    {
      struct program_space *pspace;

      ALL_PSPACES (pspace)
	parse_probes_in_pspace (probe_ops, pspace, objfile_namestr,
				provider, name, &result);
    }

  if (result.nelts == 0)
    {
      throw_error (NOT_FOUND_ERROR,
		   _("No probe matching objfile=`%s', provider=`%s', name=`%s'"),
		   objfile_namestr ? objfile_namestr : _("<any>"),
		   provider ? provider : _("<any>"),
		   name);
    }

  if (canonical)
    {
      char *canon;

      canon = savestring (arg_start, arg_end - arg_start);
      make_cleanup (xfree, canon);
      canonical->special_display = 1;
      canonical->pre_expanded = 1;
      canonical->location = new_probe_location (canon);
    }

  do_cleanups (cleanup);

  return result;
}

/* See definition in probe.h.  */

VEC (probe_p) *
find_probes_in_objfile (struct objfile *objfile, const char *provider,
			const char *name)
{
  VEC (probe_p) *probes, *result = NULL;
  int ix;
  struct probe *probe;

  if (!objfile->sf || !objfile->sf->sym_probe_fns)
    return NULL;

  probes = objfile->sf->sym_probe_fns->sym_get_probes (objfile);
  for (ix = 0; VEC_iterate (probe_p, probes, ix, probe); ix++)
    {
      if (strcmp (probe->provider, provider) != 0)
	continue;

      if (strcmp (probe->name, name) != 0)
	continue;

      VEC_safe_push (probe_p, result, probe);
    }

  return result;
}

/* See definition in probe.h.  */

struct bound_probe
find_probe_by_pc (CORE_ADDR pc)
{
  struct objfile *objfile;
  struct bound_probe result;

  result.objfile = NULL;
  result.probe = NULL;

  ALL_OBJFILES (objfile)
  {
    VEC (probe_p) *probes;
    int ix;
    struct probe *probe;

    if (!objfile->sf || !objfile->sf->sym_probe_fns
	|| objfile->sect_index_text == -1)
      continue;

    /* If this proves too inefficient, we can replace with a hash.  */
    probes = objfile->sf->sym_probe_fns->sym_get_probes (objfile);
    for (ix = 0; VEC_iterate (probe_p, probes, ix, probe); ix++)
      if (get_probe_address (probe, objfile) == pc)
	{
	  result.objfile = objfile;
	  result.probe = probe;
	  return result;
	}
  }

  return result;
}



/* Make a vector of probes matching OBJNAME, PROVIDER, and PROBE_NAME.
   If POPS is not NULL, only probes of this certain probe_ops will match.
   Each argument is a regexp, or NULL, which matches anything.  */

static VEC (bound_probe_s) *
collect_probes (char *objname, char *provider, char *probe_name,
		const struct probe_ops *pops)
{
  struct objfile *objfile;
  VEC (bound_probe_s) *result = NULL;
  struct cleanup *cleanup, *cleanup_temps;
  regex_t obj_pat, prov_pat, probe_pat;

  cleanup = make_cleanup (VEC_cleanup (bound_probe_s), &result);

  cleanup_temps = make_cleanup (null_cleanup, NULL);
  if (provider != NULL)
    compile_rx_or_error (&prov_pat, provider, _("Invalid provider regexp"));
  if (probe_name != NULL)
    compile_rx_or_error (&probe_pat, probe_name, _("Invalid probe regexp"));
  if (objname != NULL)
    compile_rx_or_error (&obj_pat, objname, _("Invalid object file regexp"));

  ALL_OBJFILES (objfile)
    {
      VEC (probe_p) *probes;
      struct probe *probe;
      int ix;

      if (! objfile->sf || ! objfile->sf->sym_probe_fns)
	continue;

      if (objname)
	{
	  if (regexec (&obj_pat, objfile_name (objfile), 0, NULL, 0) != 0)
	    continue;
	}

      probes = objfile->sf->sym_probe_fns->sym_get_probes (objfile);

      for (ix = 0; VEC_iterate (probe_p, probes, ix, probe); ix++)
	{
	  struct bound_probe bound;

	  if (pops != NULL && probe->pops != pops)
	    continue;

	  if (provider
	      && regexec (&prov_pat, probe->provider, 0, NULL, 0) != 0)
	    continue;

	  if (probe_name
	      && regexec (&probe_pat, probe->name, 0, NULL, 0) != 0)
	    continue;

	  bound.objfile = objfile;
	  bound.probe = probe;
	  VEC_safe_push (bound_probe_s, result, &bound);
	}
    }

  do_cleanups (cleanup_temps);
  discard_cleanups (cleanup);
  return result;
}

/* A qsort comparison function for bound_probe_s objects.  */

static int
compare_probes (const void *a, const void *b)
{
  const struct bound_probe *pa = (const struct bound_probe *) a;
  const struct bound_probe *pb = (const struct bound_probe *) b;
  int v;

  v = strcmp (pa->probe->provider, pb->probe->provider);
  if (v)
    return v;

  v = strcmp (pa->probe->name, pb->probe->name);
  if (v)
    return v;

  if (pa->probe->address < pb->probe->address)
    return -1;
  if (pa->probe->address > pb->probe->address)
    return 1;

  return strcmp (objfile_name (pa->objfile), objfile_name (pb->objfile));
}

/* Helper function that generate entries in the ui_out table being
   crafted by `info_probes_for_ops'.  */

static void
gen_ui_out_table_header_info (VEC (bound_probe_s) *probes,
			      const struct probe_ops *p)
{
  /* `headings' refers to the names of the columns when printing `info
     probes'.  */
  VEC (info_probe_column_s) *headings = NULL;
  struct cleanup *c;
  info_probe_column_s *column;
  size_t headings_size;
  int ix;

  gdb_assert (p != NULL);

  if (p->gen_info_probes_table_header == NULL
      && p->gen_info_probes_table_values == NULL)
    return;

  gdb_assert (p->gen_info_probes_table_header != NULL
	      && p->gen_info_probes_table_values != NULL);

  c = make_cleanup (VEC_cleanup (info_probe_column_s), &headings);
  p->gen_info_probes_table_header (&headings);

  headings_size = VEC_length (info_probe_column_s, headings);

  for (ix = 0;
       VEC_iterate (info_probe_column_s, headings, ix, column);
       ++ix)
    {
      struct bound_probe *probe;
      int jx;
      size_t size_max = strlen (column->print_name);

      for (jx = 0; VEC_iterate (bound_probe_s, probes, jx, probe); ++jx)
	{
	  /* `probe_fields' refers to the values of each new field that this
	     probe will display.  */
	  VEC (const_char_ptr) *probe_fields = NULL;
	  struct cleanup *c2;
	  const char *val;
	  int kx;

	  if (probe->probe->pops != p)
	    continue;

	  c2 = make_cleanup (VEC_cleanup (const_char_ptr), &probe_fields);
	  p->gen_info_probes_table_values (probe->probe, &probe_fields);

	  gdb_assert (VEC_length (const_char_ptr, probe_fields)
		      == headings_size);

	  for (kx = 0; VEC_iterate (const_char_ptr, probe_fields, kx, val);
	       ++kx)
	    {
	      /* It is valid to have a NULL value here, which means that the
		 backend does not have something to write and this particular
		 field should be skipped.  */
	      if (val == NULL)
		continue;

	      size_max = max (strlen (val), size_max);
	    }
	  do_cleanups (c2);
	}

      ui_out_table_header (current_uiout, size_max, ui_left,
			   column->field_name, column->print_name);
    }

  do_cleanups (c);
}

/* Helper function to print not-applicable strings for all the extra
   columns defined in a probe_ops.  */

static void
print_ui_out_not_applicables (const struct probe_ops *pops)
{
  struct cleanup *c;
  VEC (info_probe_column_s) *headings = NULL;
  info_probe_column_s *column;
  int ix;

  if (pops->gen_info_probes_table_header == NULL)
    return;

  c = make_cleanup (VEC_cleanup (info_probe_column_s), &headings);
  pops->gen_info_probes_table_header (&headings);

  for (ix = 0;
       VEC_iterate (info_probe_column_s, headings, ix, column);
       ++ix)
    ui_out_field_string (current_uiout, column->field_name, _("n/a"));

  do_cleanups (c);
}

/* Helper function to print extra information about a probe and an objfile
   represented by PROBE.  */

static void
print_ui_out_info (struct probe *probe)
{
  int ix;
  int j = 0;
  /* `values' refers to the actual values of each new field in the output
     of `info probe'.  `headings' refers to the names of each new field.  */
  VEC (const_char_ptr) *values = NULL;
  VEC (info_probe_column_s) *headings = NULL;
  info_probe_column_s *column;
  struct cleanup *c;

  gdb_assert (probe != NULL);
  gdb_assert (probe->pops != NULL);

  if (probe->pops->gen_info_probes_table_header == NULL
      && probe->pops->gen_info_probes_table_values == NULL)
    return;

  gdb_assert (probe->pops->gen_info_probes_table_header != NULL
	      && probe->pops->gen_info_probes_table_values != NULL);

  c = make_cleanup (VEC_cleanup (info_probe_column_s), &headings);
  make_cleanup (VEC_cleanup (const_char_ptr), &values);

  probe->pops->gen_info_probes_table_header (&headings);
  probe->pops->gen_info_probes_table_values (probe, &values);

  gdb_assert (VEC_length (info_probe_column_s, headings)
	      == VEC_length (const_char_ptr, values));

  for (ix = 0;
       VEC_iterate (info_probe_column_s, headings, ix, column);
       ++ix)
    {
      const char *val = VEC_index (const_char_ptr, values, j++);

      if (val == NULL)
	ui_out_field_skip (current_uiout, column->field_name);
      else
	ui_out_field_string (current_uiout, column->field_name, val);
    }

  do_cleanups (c);
}

/* Helper function that returns the number of extra fields which POPS will
   need.  */

static int
get_number_extra_fields (const struct probe_ops *pops)
{
  VEC (info_probe_column_s) *headings = NULL;
  struct cleanup *c;
  int n;

  if (pops->gen_info_probes_table_header == NULL)
    return 0;

  c = make_cleanup (VEC_cleanup (info_probe_column_s), &headings);
  pops->gen_info_probes_table_header (&headings);

  n = VEC_length (info_probe_column_s, headings);

  do_cleanups (c);

  return n;
}

/* Helper function that returns 1 if there is a probe in PROBES
   featuring the given POPS.  It returns 0 otherwise.  */

static int
exists_probe_with_pops (VEC (bound_probe_s) *probes,
			const struct probe_ops *pops)
{
  struct bound_probe *probe;
  int ix;

  for (ix = 0; VEC_iterate (bound_probe_s, probes, ix, probe); ++ix)
    if (probe->probe->pops == pops)
      return 1;

  return 0;
}

/* Helper function that parses a probe linespec of the form [PROVIDER
   [PROBE [OBJNAME]]] from the provided string STR.  */

static void
parse_probe_linespec (const char *str, char **provider,
		      char **probe_name, char **objname)
{
  *probe_name = *objname = NULL;

  *provider = extract_arg_const (&str);
  if (*provider != NULL)
    {
      *probe_name = extract_arg_const (&str);
      if (*probe_name != NULL)
	*objname = extract_arg_const (&str);
    }
}

/* See comment in probe.h.  */

void
info_probes_for_ops (const char *arg, int from_tty,
		     const struct probe_ops *pops)
{
  char *provider, *probe_name = NULL, *objname = NULL;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
  VEC (bound_probe_s) *probes;
  int i, any_found;
  int ui_out_extra_fields = 0;
  size_t size_addr;
  size_t size_name = strlen ("Name");
  size_t size_objname = strlen ("Object");
  size_t size_provider = strlen ("Provider");
  size_t size_type = strlen ("Type");
  struct bound_probe *probe;
  struct gdbarch *gdbarch = get_current_arch ();

  parse_probe_linespec (arg, &provider, &probe_name, &objname);
  make_cleanup (xfree, provider);
  make_cleanup (xfree, probe_name);
  make_cleanup (xfree, objname);

  probes = collect_probes (objname, provider, probe_name, pops);
  make_cleanup (VEC_cleanup (probe_p), &probes);

  if (pops == NULL)
    {
      const struct probe_ops *po;
      int ix;

      /* If the probe_ops is NULL, it means the user has requested a "simple"
	 `info probes', i.e., she wants to print all information about all
	 probes.  For that, we have to identify how many extra fields we will
	 need to add in the ui_out table.

	 To do that, we iterate over all probe_ops, querying each one about
	 its extra fields, and incrementing `ui_out_extra_fields' to reflect
	 that number.  But note that we ignore the probe_ops for which no probes
	 are defined with the given search criteria.  */

      for (ix = 0; VEC_iterate (probe_ops_cp, all_probe_ops, ix, po); ++ix)
	if (exists_probe_with_pops (probes, po))
	  ui_out_extra_fields += get_number_extra_fields (po);
    }
  else
    ui_out_extra_fields = get_number_extra_fields (pops);

  make_cleanup_ui_out_table_begin_end (current_uiout,
				       5 + ui_out_extra_fields,
				       VEC_length (bound_probe_s, probes),
				       "StaticProbes");

  if (!VEC_empty (bound_probe_s, probes))
    qsort (VEC_address (bound_probe_s, probes),
	   VEC_length (bound_probe_s, probes),
	   sizeof (bound_probe_s), compare_probes);

  /* What's the size of an address in our architecture?  */
  size_addr = gdbarch_addr_bit (gdbarch) == 64 ? 18 : 10;

  /* Determining the maximum size of each field (`type', `provider',
     `name' and `objname').  */
  for (i = 0; VEC_iterate (bound_probe_s, probes, i, probe); ++i)
    {
      const char *probe_type = probe->probe->pops->type_name (probe->probe);

      size_type = max (strlen (probe_type), size_type);
      size_name = max (strlen (probe->probe->name), size_name);
      size_provider = max (strlen (probe->probe->provider), size_provider);
      size_objname = max (strlen (objfile_name (probe->objfile)), size_objname);
    }

  ui_out_table_header (current_uiout, size_type, ui_left, "type", _("Type"));
  ui_out_table_header (current_uiout, size_provider, ui_left, "provider",
		       _("Provider"));
  ui_out_table_header (current_uiout, size_name, ui_left, "name", _("Name"));
  ui_out_table_header (current_uiout, size_addr, ui_left, "addr", _("Where"));

  if (pops == NULL)
    {
      const struct probe_ops *po;
      int ix;

      /* We have to generate the table header for each new probe type
	 that we will print.  Note that this excludes probe types not
	 having any defined probe with the search criteria.  */
      for (ix = 0; VEC_iterate (probe_ops_cp, all_probe_ops, ix, po); ++ix)
	if (exists_probe_with_pops (probes, po))
	  gen_ui_out_table_header_info (probes, po);
    }
  else
    gen_ui_out_table_header_info (probes, pops);

  ui_out_table_header (current_uiout, size_objname, ui_left, "object",
		       _("Object"));
  ui_out_table_body (current_uiout);

  for (i = 0; VEC_iterate (bound_probe_s, probes, i, probe); ++i)
    {
      struct cleanup *inner;
      const char *probe_type = probe->probe->pops->type_name (probe->probe);

      inner = make_cleanup_ui_out_tuple_begin_end (current_uiout, "probe");

      ui_out_field_string (current_uiout, "type",probe_type);
      ui_out_field_string (current_uiout, "provider", probe->probe->provider);
      ui_out_field_string (current_uiout, "name", probe->probe->name);
      ui_out_field_core_addr (current_uiout, "addr",
			      probe->probe->arch,
			      get_probe_address (probe->probe, probe->objfile));

      if (pops == NULL)
	{
	  const struct probe_ops *po;
	  int ix;

	  for (ix = 0; VEC_iterate (probe_ops_cp, all_probe_ops, ix, po);
	       ++ix)
	    if (probe->probe->pops == po)
	      print_ui_out_info (probe->probe);
	    else if (exists_probe_with_pops (probes, po))
	      print_ui_out_not_applicables (po);
	}
      else
	print_ui_out_info (probe->probe);

      ui_out_field_string (current_uiout, "object",
			   objfile_name (probe->objfile));
      ui_out_text (current_uiout, "\n");

      do_cleanups (inner);
    }

  any_found = !VEC_empty (bound_probe_s, probes);
  do_cleanups (cleanup);

  if (!any_found)
    ui_out_message (current_uiout, 0, _("No probes matched.\n"));
}

/* Implementation of the `info probes' command.  */

static void
info_probes_command (char *arg, int from_tty)
{
  info_probes_for_ops (arg, from_tty, NULL);
}

/* Implementation of the `enable probes' command.  */

static void
enable_probes_command (char *arg, int from_tty)
{
  char *provider, *probe_name = NULL, *objname = NULL;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
  VEC (bound_probe_s) *probes;
  struct bound_probe *probe;
  int i;

  parse_probe_linespec ((const char *) arg, &provider, &probe_name, &objname);
  make_cleanup (xfree, provider);
  make_cleanup (xfree, probe_name);
  make_cleanup (xfree, objname);

  probes = collect_probes (objname, provider, probe_name, NULL);
  if (VEC_empty (bound_probe_s, probes))
    {
      ui_out_message (current_uiout, 0, _("No probes matched.\n"));
      do_cleanups (cleanup);
      return;
    }

  /* Enable the selected probes, provided their backends support the
     notion of enabling a probe.  */
  for (i = 0; VEC_iterate (bound_probe_s, probes, i, probe); ++i)
    {
      const struct probe_ops *pops = probe->probe->pops;

      if (pops->enable_probe != NULL)
	{
	  pops->enable_probe (probe->probe);
	  ui_out_message (current_uiout, 0,
			  _("Probe %s:%s enabled.\n"),
			  probe->probe->provider, probe->probe->name);
	}
      else
	ui_out_message (current_uiout, 0,
			_("Probe %s:%s cannot be enabled.\n"),
			probe->probe->provider, probe->probe->name);
    }

  do_cleanups (cleanup);
}

/* Implementation of the `disable probes' command.  */

static void
disable_probes_command (char *arg, int from_tty)
{
  char *provider, *probe_name = NULL, *objname = NULL;
  struct cleanup *cleanup = make_cleanup (null_cleanup, NULL);
  VEC (bound_probe_s) *probes;
  struct bound_probe *probe;
  int i;

  parse_probe_linespec ((const char *) arg, &provider, &probe_name, &objname);
  make_cleanup (xfree, provider);
  make_cleanup (xfree, probe_name);
  make_cleanup (xfree, objname);

  probes = collect_probes (objname, provider, probe_name, NULL /* pops */);
  if (VEC_empty (bound_probe_s, probes))
    {
      ui_out_message (current_uiout, 0, _("No probes matched.\n"));
      do_cleanups (cleanup);
      return;
    }

  /* Disable the selected probes, provided their backends support the
     notion of enabling a probe.  */
  for (i = 0; VEC_iterate (bound_probe_s, probes, i, probe); ++i)
    {
      const struct probe_ops *pops = probe->probe->pops;

      if (pops->disable_probe != NULL)
	{
	  pops->disable_probe (probe->probe);
	  ui_out_message (current_uiout, 0,
			  _("Probe %s:%s disabled.\n"),
			  probe->probe->provider, probe->probe->name);
	}
      else
	ui_out_message (current_uiout, 0,
			_("Probe %s:%s cannot be disabled.\n"),
			probe->probe->provider, probe->probe->name);
    }

  do_cleanups (cleanup);
}

/* See comments in probe.h.  */

CORE_ADDR
get_probe_address (struct probe *probe, struct objfile *objfile)
{
  return probe->pops->get_probe_address (probe, objfile);
}

/* See comments in probe.h.  */

unsigned
get_probe_argument_count (struct probe *probe, struct frame_info *frame)
{
  return probe->pops->get_probe_argument_count (probe, frame);
}

/* See comments in probe.h.  */

int
can_evaluate_probe_arguments (struct probe *probe)
{
  return probe->pops->can_evaluate_probe_arguments (probe);
}

/* See comments in probe.h.  */

struct value *
evaluate_probe_argument (struct probe *probe, unsigned n,
			 struct frame_info *frame)
{
  return probe->pops->evaluate_probe_argument (probe, n, frame);
}

/* See comments in probe.h.  */

struct value *
probe_safe_evaluate_at_pc (struct frame_info *frame, unsigned n)
{
  struct bound_probe probe;
  unsigned n_args;

  probe = find_probe_by_pc (get_frame_pc (frame));
  if (!probe.probe)
    return NULL;

  n_args = get_probe_argument_count (probe.probe, frame);
  if (n >= n_args)
    return NULL;

  return evaluate_probe_argument (probe.probe, n, frame);
}

/* See comment in probe.h.  */

const struct probe_ops *
probe_linespec_to_ops (const char **linespecp)
{
  int ix;
  const struct probe_ops *probe_ops;

  for (ix = 0; VEC_iterate (probe_ops_cp, all_probe_ops, ix, probe_ops); ix++)
    if (probe_ops->is_linespec (linespecp))
      return probe_ops;

  return NULL;
}

/* See comment in probe.h.  */

int
probe_is_linespec_by_keyword (const char **linespecp, const char *const *keywords)
{
  const char *s = *linespecp;
  const char *const *csp;

  for (csp = keywords; *csp; csp++)
    {
      const char *keyword = *csp;
      size_t len = strlen (keyword);

      if (strncmp (s, keyword, len) == 0 && isspace (s[len]))
	{
	  *linespecp += len + 1;
	  return 1;
	}
    }

  return 0;
}

/* Implementation of `is_linespec' method for `struct probe_ops'.  */

static int
probe_any_is_linespec (const char **linespecp)
{
  static const char *const keywords[] = { "-p", "-probe", NULL };

  return probe_is_linespec_by_keyword (linespecp, keywords);
}

/* Dummy method used for `probe_ops_any'.  */

static void
probe_any_get_probes (VEC (probe_p) **probesp, struct objfile *objfile)
{
  /* No probes can be provided by this dummy backend.  */
}

/* Operations associated with a generic probe.  */

const struct probe_ops probe_ops_any =
{
  probe_any_is_linespec,
  probe_any_get_probes,
};

/* See comments in probe.h.  */

struct cmd_list_element **
info_probes_cmdlist_get (void)
{
  static struct cmd_list_element *info_probes_cmdlist;

  if (info_probes_cmdlist == NULL)
    add_prefix_cmd ("probes", class_info, info_probes_command,
		    _("\
Show available static probes.\n\
Usage: info probes [all|TYPE [ARGS]]\n\
TYPE specifies the type of the probe, and can be one of the following:\n\
  - stap\n\
If you specify TYPE, there may be additional arguments needed by the\n\
subcommand.\n\
If you do not specify any argument, or specify `all', then the command\n\
will show information about all types of probes."),
		    &info_probes_cmdlist, "info probes ",
		    0/*allow-unknown*/, &infolist);

  return &info_probes_cmdlist;
}



/* This is called to compute the value of one of the $_probe_arg*
   convenience variables.  */

static struct value *
compute_probe_arg (struct gdbarch *arch, struct internalvar *ivar,
		   void *data)
{
  struct frame_info *frame = get_selected_frame (_("No frame selected"));
  CORE_ADDR pc = get_frame_pc (frame);
  int sel = (int) (uintptr_t) data;
  struct bound_probe pc_probe;
  const struct sym_probe_fns *pc_probe_fns;
  unsigned n_args;

  /* SEL == -1 means "_probe_argc".  */
  gdb_assert (sel >= -1);

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe.probe == NULL)
    error (_("No probe at PC %s"), core_addr_to_string (pc));

  n_args = get_probe_argument_count (pc_probe.probe, frame);
  if (sel == -1)
    return value_from_longest (builtin_type (arch)->builtin_int, n_args);

  if (sel >= n_args)
    error (_("Invalid probe argument %d -- probe has %u arguments available"),
	   sel, n_args);

  return evaluate_probe_argument (pc_probe.probe, sel, frame);
}

/* This is called to compile one of the $_probe_arg* convenience
   variables into an agent expression.  */

static void
compile_probe_arg (struct internalvar *ivar, struct agent_expr *expr,
		   struct axs_value *value, void *data)
{
  CORE_ADDR pc = expr->scope;
  int sel = (int) (uintptr_t) data;
  struct bound_probe pc_probe;
  const struct sym_probe_fns *pc_probe_fns;
  int n_args;
  struct frame_info *frame = get_selected_frame (NULL);

  /* SEL == -1 means "_probe_argc".  */
  gdb_assert (sel >= -1);

  pc_probe = find_probe_by_pc (pc);
  if (pc_probe.probe == NULL)
    error (_("No probe at PC %s"), core_addr_to_string (pc));

  n_args = get_probe_argument_count (pc_probe.probe, frame);

  if (sel == -1)
    {
      value->kind = axs_rvalue;
      value->type = builtin_type (expr->gdbarch)->builtin_int;
      ax_const_l (expr, n_args);
      return;
    }

  gdb_assert (sel >= 0);
  if (sel >= n_args)
    error (_("Invalid probe argument %d -- probe has %d arguments available"),
	   sel, n_args);

  pc_probe.probe->pops->compile_to_ax (pc_probe.probe, expr, value, sel);
}

static const struct internalvar_funcs probe_funcs =
{
  compute_probe_arg,
  compile_probe_arg,
  NULL
};


VEC (probe_ops_cp) *all_probe_ops;

void _initialize_probe (void);

void
_initialize_probe (void)
{
  VEC_safe_push (probe_ops_cp, all_probe_ops, &probe_ops_any);

  create_internalvar_type_lazy ("_probe_argc", &probe_funcs,
				(void *) (uintptr_t) -1);
  create_internalvar_type_lazy ("_probe_arg0", &probe_funcs,
				(void *) (uintptr_t) 0);
  create_internalvar_type_lazy ("_probe_arg1", &probe_funcs,
				(void *) (uintptr_t) 1);
  create_internalvar_type_lazy ("_probe_arg2", &probe_funcs,
				(void *) (uintptr_t) 2);
  create_internalvar_type_lazy ("_probe_arg3", &probe_funcs,
				(void *) (uintptr_t) 3);
  create_internalvar_type_lazy ("_probe_arg4", &probe_funcs,
				(void *) (uintptr_t) 4);
  create_internalvar_type_lazy ("_probe_arg5", &probe_funcs,
				(void *) (uintptr_t) 5);
  create_internalvar_type_lazy ("_probe_arg6", &probe_funcs,
				(void *) (uintptr_t) 6);
  create_internalvar_type_lazy ("_probe_arg7", &probe_funcs,
				(void *) (uintptr_t) 7);
  create_internalvar_type_lazy ("_probe_arg8", &probe_funcs,
				(void *) (uintptr_t) 8);
  create_internalvar_type_lazy ("_probe_arg9", &probe_funcs,
				(void *) (uintptr_t) 9);
  create_internalvar_type_lazy ("_probe_arg10", &probe_funcs,
				(void *) (uintptr_t) 10);
  create_internalvar_type_lazy ("_probe_arg11", &probe_funcs,
				(void *) (uintptr_t) 11);

  add_cmd ("all", class_info, info_probes_command,
	   _("\
Show information about all type of probes."),
	   info_probes_cmdlist_get ());

  add_cmd ("probes", class_breakpoint, enable_probes_command, _("\
Enable probes.\n\
Usage: enable probes [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name.\n\
If you do not specify any argument then the command will enable\n\
all defined probes."),
	   &enablelist);

  add_cmd ("probes", class_breakpoint, disable_probes_command, _("\
Disable probes.\n\
Usage: disable probes [PROVIDER [NAME [OBJECT]]]\n\
Each argument is a regular expression, used to select probes.\n\
PROVIDER matches probe provider names.\n\
NAME matches the probe names.\n\
OBJECT matches the executable or shared library name.\n\
If you do not specify any argument then the command will disable\n\
all defined probes."),
	   &disablelist);

}
