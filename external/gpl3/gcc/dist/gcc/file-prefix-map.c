/* Implementation of file prefix remapping support (-f*-prefix-map options).
   Copyright (C) 2017 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 3, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "diagnostic.h"
#include "file-prefix-map.h"

/* Structure recording the mapping from source file and directory names at
   compile time to those to be embedded in the compilation result (debug
   information, the __FILE__ macro expansion, etc).  */
struct file_prefix_map
{
  const char *old_prefix;
  const char *new_prefix;
  size_t old_len;
  size_t new_len;
  struct file_prefix_map *next;
};

/* Record a file prefix mapping in the specified map.  ARG is the argument to
   -f*-prefix-map and must be of the form OLD=NEW.  OPT is the option name
   for diagnostics.  */
static void
add_prefix_map (file_prefix_map *&maps, const char *arg, const char *opt)
{
  file_prefix_map *map;
  const char *p;

  /* Note: looking for the last '='. The thinking is we can control the paths
     inside our projects but not where the users build them.  */
  p = strrchr (arg, '=');
  if (!p)
    {
      error ("invalid argument %qs to %qs", arg, opt);
      return;
    }
  map = XNEW (file_prefix_map);
  map->old_prefix = xstrndup (arg, p - arg);
  map->old_len = p - arg;
  p++;
  map->new_prefix = xstrdup (p);
  map->new_len = strlen (p);
  map->next = maps;
  maps = map;
}

/* Perform user-specified mapping of filename prefixes.  Return the
   GC-allocated new name corresponding to FILENAME or FILENAME if no
   remapping was performed.  */

static const char *
remap_filename (file_prefix_map *maps, const char *filename)
{
  file_prefix_map *map;
  char *s;
  const char *name;
  size_t name_len;

  for (map = maps; map; map = map->next)
    if (filename_ncmp (filename, map->old_prefix, map->old_len) == 0)
      break;
  if (!map)
    return filename;
  name = filename + map->old_len;
  name_len = strlen (name) + 1;

  s = (char *) ggc_alloc_atomic (name_len + map->new_len);
  memcpy (s, map->new_prefix, map->new_len);
  memcpy (s + map->new_len, name, name_len);
  return s;
}

/* NOTE: if adding another -f*-prefix-map option then don't forget to
   ignore it in DW_AT_producer (dwarf2out.c).  */

/* Linked lists of file_prefix_map structures.  */
static file_prefix_map *macro_prefix_maps; /* -fmacro-prefix-map  */
static file_prefix_map *debug_prefix_maps; /* -fdebug-prefix-map  */

/* Record a file prefix mapping for -fmacro-prefix-map.  */
void
add_macro_prefix_map (const char *arg)
{
  add_prefix_map (macro_prefix_maps, arg, "-fmacro-prefix-map");
}

/* Record a file prefix mapping for -fdebug-prefix-map.  */
void
add_debug_prefix_map (const char *arg)
{
  add_prefix_map (debug_prefix_maps, arg, "-fdebug-prefix-map");
}

/* Record a file prefix mapping for all -f*-prefix-map.  */
void
add_file_prefix_map (const char *arg)
{
  add_prefix_map (macro_prefix_maps, arg, "-ffile-prefix-map");
  add_prefix_map (debug_prefix_maps, arg, "-ffile-prefix-map");
}

/* Remap using -fmacro-prefix-map.  Return the GC-allocated new name
   corresponding to FILENAME or FILENAME if no remapping was performed.  */
const char *
remap_macro_filename (const char *filename)
{
  return remap_filename (macro_prefix_maps, filename);
}

/* Original GCC version disabled. The NetBSD version handles regex */
#if 0
/* Remap using -fdebug-prefix-map.  Return the GC-allocated new name
   corresponding to FILENAME or FILENAME if no remapping was performed.  */
const char *
remap_debug_filename (const char *filename)
{
  return remap_filename (debug_prefix_maps, filename);
}
#endif

/*****
 ***** The following code is a NetBSD extension that allows regex and
 ***** \[0-9] substitutition arguments.
 *****/

/* Perform user-specified mapping of debug filename prefixes.  Return
   the new name corresponding to FILENAME.  */

static const char *
remap_debug_prefix_filename (const char *filename)
{
  file_prefix_map *map;
  char *s;
  const char *name;
  size_t name_len;

  for (map = debug_prefix_maps; map; map = map->next)
    if (filename_ncmp (filename, map->old_prefix, map->old_len) == 0)
      break;
  if (!map)
    return filename;
  name = filename + map->old_len;
  name_len = strlen (name) + 1;
  s = (char *) alloca (name_len + map->new_len);
  memcpy (s, map->new_prefix, map->new_len);
  memcpy (s + map->new_len, name, name_len);
  return ggc_strdup (s);
}

#include <regex.h>

typedef struct debug_regex_map
{
  regex_t re;
  const char *sub;
  struct debug_regex_map *next;
} debug_regex_map;

/* Linked list of such structures.  */
debug_regex_map *debug_regex_maps;


/* Record a debug file regex mapping.  ARG is the argument to
   -fdebug-regex-map and must be of the form OLD=NEW.  */

void
add_debug_regex_map (const char *arg)
{
  debug_regex_map *map;
  const char *p;
  char *old;
  char buf[1024];
  regex_t re;
  int e;

  p = strchr (arg, '=');
  if (!p)
    {
      error ("invalid argument %qs to -fdebug-regex-map", arg);
      return;
    }
  
  old = xstrndup (arg, p - arg);
  if ((e = regcomp(&re, old, REG_EXTENDED)) != 0)
    {
      regerror(e, &re, buf, sizeof(buf));
      warning (0, "regular expression compilation for %qs in argument to "
	       "-fdebug-regex-map failed: %qs", old, buf);
      free(old);
      return;
    }
  free(old);

  map = XNEW (debug_regex_map);
  map->re = re;
  p++;
  map->sub = xstrdup (p);
  map->next = debug_regex_maps;
  debug_regex_maps = map;
}

extern "C" ssize_t regasub(char **, const char *,
  const regmatch_t *rm, const char *);

/* Perform user-specified mapping of debug filename regular expressions.  Return
   the new name corresponding to FILENAME.  */

static const char *
remap_debug_regex_filename (const char *filename)
{
  debug_regex_map *map;
  char *s;
  regmatch_t rm[10];

  for (map = debug_regex_maps; map; map = map->next)
    if (regexec (&map->re, filename, 10, rm, 0) == 0
       && regasub (&s, map->sub, rm, filename) >= 0)
      {
	 const char *name = ggc_strdup(s);
	 free(s);
	 return name;
      }
  return filename;
}

const char *
remap_debug_filename (const char *filename)
{
   return remap_debug_regex_filename (remap_debug_prefix_filename (filename));
}
