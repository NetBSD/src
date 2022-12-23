/* Linker command language support.
   Copyright (C) 1991-2020 Free Software Foundation, Inc.

   This file is part of the GNU Binutils.

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
#include <limits.h>
#include "bfd.h"
#include "libiberty.h"
#include "filenames.h"
#include "safe-ctype.h"
#include "obstack.h"
#include "bfdlink.h"
#include "ctf-api.h"

#include "ld.h"
#include "ldmain.h"
#include "ldexp.h"
#include "ldlang.h"
#include <ldgram.h>
#include "ldlex.h"
#include "ldmisc.h"
#include "ldctor.h"
#include "ldfile.h"
#include "ldemul.h"
#include "fnmatch.h"
#include "demangle.h"
#include "hashtab.h"
#include "elf-bfd.h"
#ifdef ENABLE_PLUGINS
#include "plugin.h"
#endif /* ENABLE_PLUGINS */

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & (((TYPE*) 0)->MEMBER))
#endif

/* Convert between addresses in bytes and sizes in octets.
   For currently supported targets, octets_per_byte is always a power
   of two, so we can use shifts.  */
#define TO_ADDR(X) ((X) >> opb_shift)
#define TO_SIZE(X) ((X) << opb_shift)

/* Local variables.  */
static struct obstack stat_obstack;
static struct obstack map_obstack;

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free
static const char *entry_symbol_default = "start";
static bfd_boolean map_head_is_link_order = FALSE;
static lang_output_section_statement_type *default_common_section;
static bfd_boolean map_option_f;
static bfd_vma print_dot;
static lang_input_statement_type *first_file;
static const char *current_target;
/* Header for list of statements corresponding to any files involved in the
   link, either specified from the command-line or added implicitely (eg.
   archive member used to resolved undefined symbol, wildcard statement from
   linker script, etc.).  Next pointer is in next field of a
   lang_statement_header_type (reached via header field in a
   lang_statement_union).  */
static lang_statement_list_type statement_list;
static lang_statement_list_type *stat_save[10];
static lang_statement_list_type **stat_save_ptr = &stat_save[0];
static struct unique_sections *unique_section_list;
static struct asneeded_minfo *asneeded_list_head;
static unsigned int opb_shift = 0;

/* Forward declarations.  */
static void exp_init_os (etree_type *);
static lang_input_statement_type *lookup_name (const char *);
static void insert_undefined (const char *);
static bfd_boolean sort_def_symbol (struct bfd_link_hash_entry *, void *);
static void print_statement (lang_statement_union_type *,
			     lang_output_section_statement_type *);
static void print_statement_list (lang_statement_union_type *,
				  lang_output_section_statement_type *);
static void print_statements (void);
static void print_input_section (asection *, bfd_boolean);
static bfd_boolean lang_one_common (struct bfd_link_hash_entry *, void *);
static void lang_record_phdrs (void);
static void lang_do_version_exports_section (void);
static void lang_finalize_version_expr_head
  (struct bfd_elf_version_expr_head *);
static void lang_do_memory_regions (void);

/* Exported variables.  */
const char *output_target;
lang_output_section_statement_type *abs_output_section;
lang_statement_list_type lang_os_list;
lang_statement_list_type *stat_ptr = &statement_list;
/* Header for list of statements corresponding to files used in the final
   executable.  This can be either object file specified on the command-line
   or library member resolving an undefined reference.  Next pointer is in next
   field of a lang_input_statement_type (reached via input_statement field in a
   lang_statement_union).  */
lang_statement_list_type file_chain = { NULL, NULL };
/* Header for list of statements corresponding to files specified on the
   command-line for linking.  It thus contains real object files and archive
   but not archive members.  Next pointer is in next_real_file field of a
   lang_input_statement_type statement (reached via input_statement field in a
   lang_statement_union).  */
lang_statement_list_type input_file_chain;
struct bfd_sym_chain entry_symbol = { NULL, NULL };
const char *entry_section = ".text";
struct lang_input_statement_flags input_flags;
bfd_boolean entry_from_cmdline;
bfd_boolean undef_from_cmdline;
bfd_boolean lang_has_input_file = FALSE;
bfd_boolean had_output_filename = FALSE;
bfd_boolean lang_float_flag = FALSE;
bfd_boolean delete_output_file_on_failure = FALSE;
struct lang_phdr *lang_phdr_list;
struct lang_nocrossrefs *nocrossref_list;
struct asneeded_minfo **asneeded_list_tail;
static ctf_file_t *ctf_output;

 /* Functions that traverse the linker script and might evaluate
    DEFINED() need to increment this at the start of the traversal.  */
int lang_statement_iteration = 0;

/* Return TRUE if the PATTERN argument is a wildcard pattern.
   Although backslashes are treated specially if a pattern contains
   wildcards, we do not consider the mere presence of a backslash to
   be enough to cause the pattern to be treated as a wildcard.
   That lets us handle DOS filenames more naturally.  */
#define wildcardp(pattern) (strpbrk ((pattern), "?*[") != NULL)

#define new_stat(x, y) \
  (x##_type *) new_statement (x##_enum, sizeof (x##_type), y)

#define outside_section_address(q) \
  ((q)->output_offset + (q)->output_section->vma)

#define outside_symbol_address(q) \
  ((q)->value + outside_section_address (q->section))

#define SECTION_NAME_MAP_LENGTH (16)

/* CTF sections smaller than this are not compressed: compression of
   dictionaries this small doesn't gain much, and this lets consumers mmap the
   sections directly out of the ELF file and use them with no decompression
   overhead if they want to.  */
#define CTF_COMPRESSION_THRESHOLD 4096

void *
stat_alloc (size_t size)
{
  return obstack_alloc (&stat_obstack, size);
}

static int
name_match (const char *pattern, const char *name)
{
  if (wildcardp (pattern))
    return fnmatch (pattern, name, 0);
  return strcmp (pattern, name);
}

/* If PATTERN is of the form archive:file, return a pointer to the
   separator.  If not, return NULL.  */

static char *
archive_path (const char *pattern)
{
  char *p = NULL;

  if (link_info.path_separator == 0)
    return p;

  p = strchr (pattern, link_info.path_separator);
#ifdef HAVE_DOS_BASED_FILE_SYSTEM
  if (p == NULL || link_info.path_separator != ':')
    return p;

  /* Assume a match on the second char is part of drive specifier,
     as in "c:\silly.dos".  */
  if (p == pattern + 1 && ISALPHA (*pattern))
    p = strchr (p + 1, link_info.path_separator);
#endif
  return p;
}

/* Given that FILE_SPEC results in a non-NULL SEP result from archive_path,
   return whether F matches FILE_SPEC.  */

static bfd_boolean
input_statement_is_archive_path (const char *file_spec, char *sep,
				 lang_input_statement_type *f)
{
  bfd_boolean match = FALSE;

  if ((*(sep + 1) == 0
       || name_match (sep + 1, f->filename) == 0)
      && ((sep != file_spec)
	  == (f->the_bfd != NULL && f->the_bfd->my_archive != NULL)))
    {
      match = TRUE;

      if (sep != file_spec)
	{
	  const char *aname = f->the_bfd->my_archive->filename;
	  *sep = 0;
	  match = name_match (file_spec, aname) == 0;
	  *sep = link_info.path_separator;
	}
    }
  return match;
}

static bfd_boolean
unique_section_p (const asection *sec,
		  const lang_output_section_statement_type *os)
{
  struct unique_sections *unam;
  const char *secnam;

  if (!link_info.resolve_section_groups
      && sec->owner != NULL
      && bfd_is_group_section (sec->owner, sec))
    return !(os != NULL
	     && strcmp (os->name, DISCARD_SECTION_NAME) == 0);

  secnam = sec->name;
  for (unam = unique_section_list; unam; unam = unam->next)
    if (name_match (unam->name, secnam) == 0)
      return TRUE;

  return FALSE;
}

/* Generic traversal routines for finding matching sections.  */

/* Return true if FILE matches a pattern in EXCLUDE_LIST, otherwise return
   false.  */

static bfd_boolean
walk_wild_file_in_exclude_list (struct name_list *exclude_list,
				lang_input_statement_type *file)
{
  struct name_list *list_tmp;

  for (list_tmp = exclude_list;
       list_tmp;
       list_tmp = list_tmp->next)
    {
      char *p = archive_path (list_tmp->name);

      if (p != NULL)
	{
	  if (input_statement_is_archive_path (list_tmp->name, p, file))
	    return TRUE;
	}

      else if (name_match (list_tmp->name, file->filename) == 0)
	return TRUE;

      /* FIXME: Perhaps remove the following at some stage?  Matching
	 unadorned archives like this was never documented and has
	 been superceded by the archive:path syntax.  */
      else if (file->the_bfd != NULL
	       && file->the_bfd->my_archive != NULL
	       && name_match (list_tmp->name,
			      file->the_bfd->my_archive->filename) == 0)
	return TRUE;
    }

  return FALSE;
}

/* Try processing a section against a wildcard.  This just calls
   the callback unless the filename exclusion list is present
   and excludes the file.  It's hardly ever present so this
   function is very fast.  */

static void
walk_wild_consider_section (lang_wild_statement_type *ptr,
			    lang_input_statement_type *file,
			    asection *s,
			    struct wildcard_list *sec,
			    callback_t callback,
			    void *data)
{
  /* Don't process sections from files which were excluded.  */
  if (walk_wild_file_in_exclude_list (sec->spec.exclude_name_list, file))
    return;

  (*callback) (ptr, sec, s, ptr->section_flag_list, file, data);
}

/* Lowest common denominator routine that can handle everything correctly,
   but slowly.  */

static void
walk_wild_section_general (lang_wild_statement_type *ptr,
			   lang_input_statement_type *file,
			   callback_t callback,
			   void *data)
{
  asection *s;
  struct wildcard_list *sec;

  for (s = file->the_bfd->sections; s != NULL; s = s->next)
    {
      sec = ptr->section_list;
      if (sec == NULL)
	(*callback) (ptr, sec, s, ptr->section_flag_list, file, data);

      while (sec != NULL)
	{
	  bfd_boolean skip = FALSE;

	  if (sec->spec.name != NULL)
	    {
	      const char *sname = bfd_section_name (s);

	      skip = name_match (sec->spec.name, sname) != 0;
	    }

	  if (!skip)
	    walk_wild_consider_section (ptr, file, s, sec, callback, data);

	  sec = sec->next;
	}
    }
}

/* Routines to find a single section given its name.  If there's more
   than one section with that name, we report that.  */

typedef struct
{
  asection *found_section;
  bfd_boolean multiple_sections_found;
} section_iterator_callback_data;

static bfd_boolean
section_iterator_callback (bfd *abfd ATTRIBUTE_UNUSED, asection *s, void *data)
{
  section_iterator_callback_data *d = (section_iterator_callback_data *) data;

  if (d->found_section != NULL)
    {
      d->multiple_sections_found = TRUE;
      return TRUE;
    }

  d->found_section = s;
  return FALSE;
}

static asection *
find_section (lang_input_statement_type *file,
	      struct wildcard_list *sec,
	      bfd_boolean *multiple_sections_found)
{
  section_iterator_callback_data cb_data = { NULL, FALSE };

  bfd_get_section_by_name_if (file->the_bfd, sec->spec.name,
			      section_iterator_callback, &cb_data);
  *multiple_sections_found = cb_data.multiple_sections_found;
  return cb_data.found_section;
}

/* Code for handling simple wildcards without going through fnmatch,
   which can be expensive because of charset translations etc.  */

/* A simple wild is a literal string followed by a single '*',
   where the literal part is at least 4 characters long.  */

static bfd_boolean
is_simple_wild (const char *name)
{
  size_t len = strcspn (name, "*?[");
  return len >= 4 && name[len] == '*' && name[len + 1] == '\0';
}

static bfd_boolean
match_simple_wild (const char *pattern, const char *name)
{
  /* The first four characters of the pattern are guaranteed valid
     non-wildcard characters.  So we can go faster.  */
  if (pattern[0] != name[0] || pattern[1] != name[1]
      || pattern[2] != name[2] || pattern[3] != name[3])
    return FALSE;

  pattern += 4;
  name += 4;
  while (*pattern != '*')
    if (*name++ != *pattern++)
      return FALSE;

  return TRUE;
}

/* Return the numerical value of the init_priority attribute from
   section name NAME.  */

static int
get_init_priority (const asection *sec)
{
  const char *name = bfd_section_name (sec);
  const char *dot;

  /* GCC uses the following section names for the init_priority
     attribute with numerical values 101 to 65535 inclusive. A
     lower value means a higher priority.

     1: .init_array.NNNNN/.fini_array.NNNNN: Where NNNNN is the
	decimal numerical value of the init_priority attribute.
	The order of execution in .init_array is forward and
	.fini_array is backward.
     2: .ctors.NNNNN/.dtors.NNNNN: Where NNNNN is 65535 minus the
	decimal numerical value of the init_priority attribute.
	The order of execution in .ctors is backward and .dtors
	is forward.

     .init_array.NNNNN sections would normally be placed in an output
     .init_array section, .fini_array.NNNNN in .fini_array,
     .ctors.NNNNN in .ctors, and .dtors.NNNNN in .dtors.  This means
     we should sort by increasing number (and could just use
     SORT_BY_NAME in scripts).  However if .ctors.NNNNN sections are
     being placed in .init_array (which may also contain
     .init_array.NNNNN sections) or .dtors.NNNNN sections are being
     placed in .fini_array then we need to extract the init_priority
     attribute and sort on that.  */
  dot = strrchr (name, '.');
  if (dot != NULL && ISDIGIT (dot[1]))
    {
      char *end;
      unsigned long init_priority = strtoul (dot + 1, &end, 10);
      if (*end == 0)
	{
	  if (dot == name + 6
	      && (strncmp (name, ".ctors", 6) == 0
		  || strncmp (name, ".dtors", 6) == 0))
	    init_priority = 65535 - init_priority;
	  if (init_priority <= INT_MAX)
	    return init_priority;
	}
    }
  return -1;
}

/* Compare sections ASEC and BSEC according to SORT.  */

static int
compare_section (sort_type sort, asection *asec, asection *bsec)
{
  int ret;
  int a_priority, b_priority;

  switch (sort)
    {
    default:
      abort ();

    case by_init_priority:
      a_priority = get_init_priority (asec);
      b_priority = get_init_priority (bsec);
      if (a_priority < 0 || b_priority < 0)
	goto sort_by_name;
      ret = a_priority - b_priority;
      if (ret)
	break;
      else
	goto sort_by_name;

    case by_alignment_name:
      ret = bfd_section_alignment (bsec) - bfd_section_alignment (asec);
      if (ret)
	break;
      /* Fall through.  */

    case by_name:
sort_by_name:
      ret = strcmp (bfd_section_name (asec), bfd_section_name (bsec));
      break;

    case by_name_alignment:
      ret = strcmp (bfd_section_name (asec), bfd_section_name (bsec));
      if (ret)
	break;
      /* Fall through.  */

    case by_alignment:
      ret = bfd_section_alignment (bsec) - bfd_section_alignment (asec);
      break;
    }

  return ret;
}

/* Build a Binary Search Tree to sort sections, unlike insertion sort
   used in wild_sort(). BST is considerably faster if the number of
   of sections are large.  */

static lang_section_bst_type **
wild_sort_fast (lang_wild_statement_type *wild,
		struct wildcard_list *sec,
		lang_input_statement_type *file ATTRIBUTE_UNUSED,
		asection *section)
{
  lang_section_bst_type **tree;

  tree = &wild->tree;
  if (!wild->filenames_sorted
      && (sec == NULL || sec->spec.sorted == none))
    {
      /* Append at the right end of tree.  */
      while (*tree)
	tree = &((*tree)->right);
      return tree;
    }

  while (*tree)
    {
      /* Find the correct node to append this section.  */
      if (compare_section (sec->spec.sorted, section, (*tree)->section) < 0)
	tree = &((*tree)->left);
      else
	tree = &((*tree)->right);
    }

  return tree;
}

/* Use wild_sort_fast to build a BST to sort sections.  */

static void
output_section_callback_fast (lang_wild_statement_type *ptr,
			      struct wildcard_list *sec,
			      asection *section,
			      struct flag_info *sflag_list ATTRIBUTE_UNUSED,
			      lang_input_statement_type *file,
			      void *output)
{
  lang_section_bst_type *node;
  lang_section_bst_type **tree;
  lang_output_section_statement_type *os;

  os = (lang_output_section_statement_type *) output;

  if (unique_section_p (section, os))
    return;

  node = (lang_section_bst_type *) xmalloc (sizeof (lang_section_bst_type));
  node->left = 0;
  node->right = 0;
  node->section = section;

  tree = wild_sort_fast (ptr, sec, file, section);
  if (tree != NULL)
    *tree = node;
}

/* Convert a sorted sections' BST back to list form.  */

static void
output_section_callback_tree_to_list (lang_wild_statement_type *ptr,
				      lang_section_bst_type *tree,
				      void *output)
{
  if (tree->left)
    output_section_callback_tree_to_list (ptr, tree->left, output);

  lang_add_section (&ptr->children, tree->section, NULL,
		    (lang_output_section_statement_type *) output);

  if (tree->right)
    output_section_callback_tree_to_list (ptr, tree->right, output);

  free (tree);
}

/* Specialized, optimized routines for handling different kinds of
   wildcards */

static void
walk_wild_section_specs1_wild0 (lang_wild_statement_type *ptr,
				lang_input_statement_type *file,
				callback_t callback,
				void *data)
{
  /* We can just do a hash lookup for the section with the right name.
     But if that lookup discovers more than one section with the name
     (should be rare), we fall back to the general algorithm because
     we would otherwise have to sort the sections to make sure they
     get processed in the bfd's order.  */
  bfd_boolean multiple_sections_found;
  struct wildcard_list *sec0 = ptr->handler_data[0];
  asection *s0 = find_section (file, sec0, &multiple_sections_found);

  if (multiple_sections_found)
    walk_wild_section_general (ptr, file, callback, data);
  else if (s0)
    walk_wild_consider_section (ptr, file, s0, sec0, callback, data);
}

static void
walk_wild_section_specs1_wild1 (lang_wild_statement_type *ptr,
				lang_input_statement_type *file,
				callback_t callback,
				void *data)
{
  asection *s;
  struct wildcard_list *wildsec0 = ptr->handler_data[0];

  for (s = file->the_bfd->sections; s != NULL; s = s->next)
    {
      const char *sname = bfd_section_name (s);
      bfd_boolean skip = !match_simple_wild (wildsec0->spec.name, sname);

      if (!skip)
	walk_wild_consider_section (ptr, file, s, wildsec0, callback, data);
    }
}

static void
walk_wild_section_specs2_wild1 (lang_wild_statement_type *ptr,
				lang_input_statement_type *file,
				callback_t callback,
				void *data)
{
  asection *s;
  struct wildcard_list *sec0 = ptr->handler_data[0];
  struct wildcard_list *wildsec1 = ptr->handler_data[1];
  bfd_boolean multiple_sections_found;
  asection *s0 = find_section (file, sec0, &multiple_sections_found);

  if (multiple_sections_found)
    {
      walk_wild_section_general (ptr, file, callback, data);
      return;
    }

  /* Note that if the section was not found, s0 is NULL and
     we'll simply never succeed the s == s0 test below.  */
  for (s = file->the_bfd->sections; s != NULL; s = s->next)
    {
      /* Recall that in this code path, a section cannot satisfy more
	 than one spec, so if s == s0 then it cannot match
	 wildspec1.  */
      if (s == s0)
	walk_wild_consider_section (ptr, file, s, sec0, callback, data);
      else
	{
	  const char *sname = bfd_section_name (s);
	  bfd_boolean skip = !match_simple_wild (wildsec1->spec.name, sname);

	  if (!skip)
	    walk_wild_consider_section (ptr, file, s, wildsec1, callback,
					data);
	}
    }
}

static void
walk_wild_section_specs3_wild2 (lang_wild_statement_type *ptr,
				lang_input_statement_type *file,
				callback_t callback,
				void *data)
{
  asection *s;
  struct wildcard_list *sec0 = ptr->handler_data[0];
  struct wildcard_list *wildsec1 = ptr->handler_data[1];
  struct wildcard_list *wildsec2 = ptr->handler_data[2];
  bfd_boolean multiple_sections_found;
  asection *s0 = find_section (file, sec0, &multiple_sections_found);

  if (multiple_sections_found)
    {
      walk_wild_section_general (ptr, file, callback, data);
      return;
    }

  for (s = file->the_bfd->sections; s != NULL; s = s->next)
    {
      if (s == s0)
	walk_wild_consider_section (ptr, file, s, sec0, callback, data);
      else
	{
	  const char *sname = bfd_section_name (s);
	  bfd_boolean skip = !match_simple_wild (wildsec1->spec.name, sname);

	  if (!skip)
	    walk_wild_consider_section (ptr, file, s, wildsec1, callback, data);
	  else
	    {
	      skip = !match_simple_wild (wildsec2->spec.name, sname);
	      if (!skip)
		walk_wild_consider_section (ptr, file, s, wildsec2, callback,
					    data);
	    }
	}
    }
}

static void
walk_wild_section_specs4_wild2 (lang_wild_statement_type *ptr,
				lang_input_statement_type *file,
				callback_t callback,
				void *data)
{
  asection *s;
  struct wildcard_list *sec0 = ptr->handler_data[0];
  struct wildcard_list *sec1 = ptr->handler_data[1];
  struct wildcard_list *wildsec2 = ptr->handler_data[2];
  struct wildcard_list *wildsec3 = ptr->handler_data[3];
  bfd_boolean multiple_sections_found;
  asection *s0 = find_section (file, sec0, &multiple_sections_found), *s1;

  if (multiple_sections_found)
    {
      walk_wild_section_general (ptr, file, callback, data);
      return;
    }

  s1 = find_section (file, sec1, &multiple_sections_found);
  if (multiple_sections_found)
    {
      walk_wild_section_general (ptr, file, callback, data);
      return;
    }

  for (s = file->the_bfd->sections; s != NULL; s = s->next)
    {
      if (s == s0)
	walk_wild_consider_section (ptr, file, s, sec0, callback, data);
      else
	if (s == s1)
	  walk_wild_consider_section (ptr, file, s, sec1, callback, data);
	else
	  {
	    const char *sname = bfd_section_name (s);
	    bfd_boolean skip = !match_simple_wild (wildsec2->spec.name,
						   sname);

	    if (!skip)
	      walk_wild_consider_section (ptr, file, s, wildsec2, callback,
					  data);
	    else
	      {
		skip = !match_simple_wild (wildsec3->spec.name, sname);
		if (!skip)
		  walk_wild_consider_section (ptr, file, s, wildsec3,
					      callback, data);
	      }
	  }
    }
}

static void
walk_wild_section (lang_wild_statement_type *ptr,
		   lang_input_statement_type *file,
		   callback_t callback,
		   void *data)
{
  if (file->flags.just_syms)
    return;

  (*ptr->walk_wild_section_handler) (ptr, file, callback, data);
}

/* Returns TRUE when name1 is a wildcard spec that might match
   something name2 can match.  We're conservative: we return FALSE
   only if the prefixes of name1 and name2 are different up to the
   first wildcard character.  */

static bfd_boolean
wild_spec_can_overlap (const char *name1, const char *name2)
{
  size_t prefix1_len = strcspn (name1, "?*[");
  size_t prefix2_len = strcspn (name2, "?*[");
  size_t min_prefix_len;

  /* Note that if there is no wildcard character, then we treat the
     terminating 0 as part of the prefix.  Thus ".text" won't match
     ".text." or ".text.*", for example.  */
  if (name1[prefix1_len] == '\0')
    prefix1_len++;
  if (name2[prefix2_len] == '\0')
    prefix2_len++;

  min_prefix_len = prefix1_len < prefix2_len ? prefix1_len : prefix2_len;

  return memcmp (name1, name2, min_prefix_len) == 0;
}

/* Select specialized code to handle various kinds of wildcard
   statements.  */

static void
analyze_walk_wild_section_handler (lang_wild_statement_type *ptr)
{
  int sec_count = 0;
  int wild_name_count = 0;
  struct wildcard_list *sec;
  int signature;
  int data_counter;

  ptr->walk_wild_section_handler = walk_wild_section_general;
  ptr->handler_data[0] = NULL;
  ptr->handler_data[1] = NULL;
  ptr->handler_data[2] = NULL;
  ptr->handler_data[3] = NULL;
  ptr->tree = NULL;

  /* Count how many wildcard_specs there are, and how many of those
     actually use wildcards in the name.  Also, bail out if any of the
     wildcard names are NULL. (Can this actually happen?
     walk_wild_section used to test for it.)  And bail out if any
     of the wildcards are more complex than a simple string
     ending in a single '*'.  */
  for (sec = ptr->section_list; sec != NULL; sec = sec->next)
    {
      ++sec_count;
      if (sec->spec.name == NULL)
	return;
      if (wildcardp (sec->spec.name))
	{
	  ++wild_name_count;
	  if (!is_simple_wild (sec->spec.name))
	    return;
	}
    }

  /* The zero-spec case would be easy to optimize but it doesn't
     happen in practice.  Likewise, more than 4 specs doesn't
     happen in practice.  */
  if (sec_count == 0 || sec_count > 4)
    return;

  /* Check that no two specs can match the same section.  */
  for (sec = ptr->section_list; sec != NULL; sec = sec->next)
    {
      struct wildcard_list *sec2;
      for (sec2 = sec->next; sec2 != NULL; sec2 = sec2->next)
	{
	  if (wild_spec_can_overlap (sec->spec.name, sec2->spec.name))
	    return;
	}
    }

  signature = (sec_count << 8) + wild_name_count;
  switch (signature)
    {
    case 0x0100:
      ptr->walk_wild_section_handler = walk_wild_section_specs1_wild0;
      break;
    case 0x0101:
      ptr->walk_wild_section_handler = walk_wild_section_specs1_wild1;
      break;
    case 0x0201:
      ptr->walk_wild_section_handler = walk_wild_section_specs2_wild1;
      break;
    case 0x0302:
      ptr->walk_wild_section_handler = walk_wild_section_specs3_wild2;
      break;
    case 0x0402:
      ptr->walk_wild_section_handler = walk_wild_section_specs4_wild2;
      break;
    default:
      return;
    }

  /* Now fill the data array with pointers to the specs, first the
     specs with non-wildcard names, then the specs with wildcard
     names.  It's OK to process the specs in different order from the
     given order, because we've already determined that no section
     will match more than one spec.  */
  data_counter = 0;
  for (sec = ptr->section_list; sec != NULL; sec = sec->next)
    if (!wildcardp (sec->spec.name))
      ptr->handler_data[data_counter++] = sec;
  for (sec = ptr->section_list; sec != NULL; sec = sec->next)
    if (wildcardp (sec->spec.name))
      ptr->handler_data[data_counter++] = sec;
}

/* Handle a wild statement for a single file F.  */

static void
walk_wild_file (lang_wild_statement_type *s,
		lang_input_statement_type *f,
		callback_t callback,
		void *data)
{
  if (walk_wild_file_in_exclude_list (s->exclude_name_list, f))
    return;

  if (f->the_bfd == NULL
      || !bfd_check_format (f->the_bfd, bfd_archive))
    walk_wild_section (s, f, callback, data);
  else
    {
      bfd *member;

      /* This is an archive file.  We must map each member of the
	 archive separately.  */
      member = bfd_openr_next_archived_file (f->the_bfd, NULL);
      while (member != NULL)
	{
	  /* When lookup_name is called, it will call the add_symbols
	     entry point for the archive.  For each element of the
	     archive which is included, BFD will call ldlang_add_file,
	     which will set the usrdata field of the member to the
	     lang_input_statement.  */
	  if (bfd_usrdata (member) != NULL)
	    walk_wild_section (s, bfd_usrdata (member), callback, data);

	  member = bfd_openr_next_archived_file (f->the_bfd, member);
	}
    }
}

static void
walk_wild (lang_wild_statement_type *s, callback_t callback, void *data)
{
  const char *file_spec = s->filename;
  char *p;

  if (file_spec == NULL)
    {
      /* Perform the iteration over all files in the list.  */
      LANG_FOR_EACH_INPUT_STATEMENT (f)
	{
	  walk_wild_file (s, f, callback, data);
	}
    }
  else if ((p = archive_path (file_spec)) != NULL)
    {
      LANG_FOR_EACH_INPUT_STATEMENT (f)
	{
	  if (input_statement_is_archive_path (file_spec, p, f))
	    walk_wild_file (s, f, callback, data);
	}
    }
  else if (wildcardp (file_spec))
    {
      LANG_FOR_EACH_INPUT_STATEMENT (f)
	{
	  if (fnmatch (file_spec, f->filename, 0) == 0)
	    walk_wild_file (s, f, callback, data);
	}
    }
  else
    {
      lang_input_statement_type *f;

      /* Perform the iteration over a single file.  */
      f = lookup_name (file_spec);
      if (f)
	walk_wild_file (s, f, callback, data);
    }
}

/* lang_for_each_statement walks the parse tree and calls the provided
   function for each node, except those inside output section statements
   with constraint set to -1.  */

void
lang_for_each_statement_worker (void (*func) (lang_statement_union_type *),
				lang_statement_union_type *s)
{
  for (; s != NULL; s = s->header.next)
    {
      func (s);

      switch (s->header.type)
	{
	case lang_constructors_statement_enum:
	  lang_for_each_statement_worker (func, constructor_list.head);
	  break;
	case lang_output_section_statement_enum:
	  if (s->output_section_statement.constraint != -1)
	    lang_for_each_statement_worker
	      (func, s->output_section_statement.children.head);
	  break;
	case lang_wild_statement_enum:
	  lang_for_each_statement_worker (func,
					  s->wild_statement.children.head);
	  break;
	case lang_group_statement_enum:
	  lang_for_each_statement_worker (func,
					  s->group_statement.children.head);
	  break;
	case lang_data_statement_enum:
	case lang_reloc_statement_enum:
	case lang_object_symbols_statement_enum:
	case lang_output_statement_enum:
	case lang_target_statement_enum:
	case lang_input_section_enum:
	case lang_input_statement_enum:
	case lang_assignment_statement_enum:
	case lang_padding_statement_enum:
	case lang_address_statement_enum:
	case lang_fill_statement_enum:
	case lang_insert_statement_enum:
	  break;
	default:
	  FAIL ();
	  break;
	}
    }
}

void
lang_for_each_statement (void (*func) (lang_statement_union_type *))
{
  lang_for_each_statement_worker (func, statement_list.head);
}

/*----------------------------------------------------------------------*/

void
lang_list_init (lang_statement_list_type *list)
{
  list->head = NULL;
  list->tail = &list->head;
}

static void
lang_statement_append (lang_statement_list_type *list,
		       void *element,
		       void *field)
{
  *(list->tail) = element;
  list->tail = field;
}

void
push_stat_ptr (lang_statement_list_type *new_ptr)
{
  if (stat_save_ptr >= stat_save + sizeof (stat_save) / sizeof (stat_save[0]))
    abort ();
  *stat_save_ptr++ = stat_ptr;
  stat_ptr = new_ptr;
}

void
pop_stat_ptr (void)
{
  if (stat_save_ptr <= stat_save)
    abort ();
  stat_ptr = *--stat_save_ptr;
}

/* Build a new statement node for the parse tree.  */

static lang_statement_union_type *
new_statement (enum statement_enum type,
	       size_t size,
	       lang_statement_list_type *list)
{
  lang_statement_union_type *new_stmt;

  new_stmt = stat_alloc (size);
  new_stmt->header.type = type;
  new_stmt->header.next = NULL;
  lang_statement_append (list, new_stmt, &new_stmt->header.next);
  return new_stmt;
}

/* Build a new input file node for the language.  There are several
   ways in which we treat an input file, eg, we only look at symbols,
   or prefix it with a -l etc.

   We can be supplied with requests for input files more than once;
   they may, for example be split over several lines like foo.o(.text)
   foo.o(.data) etc, so when asked for a file we check that we haven't
   got it already so we don't duplicate the bfd.  */

static lang_input_statement_type *
new_afile (const char *name,
	   lang_input_file_enum_type file_type,
	   const char *target)
{
  lang_input_statement_type *p;

  lang_has_input_file = TRUE;

  p = new_stat (lang_input_statement, stat_ptr);
  memset (&p->the_bfd, 0,
	  sizeof (*p) - offsetof (lang_input_statement_type, the_bfd));
  p->target = target;
  p->flags.dynamic = input_flags.dynamic;
  p->flags.add_DT_NEEDED_for_dynamic = input_flags.add_DT_NEEDED_for_dynamic;
  p->flags.add_DT_NEEDED_for_regular = input_flags.add_DT_NEEDED_for_regular;
  p->flags.whole_archive = input_flags.whole_archive;
  p->flags.sysrooted = input_flags.sysrooted;

  switch (file_type)
    {
    case lang_input_file_is_symbols_only_enum:
      p->filename = name;
      p->local_sym_name = name;
      p->flags.real = TRUE;
      p->flags.just_syms = TRUE;
      break;
    case lang_input_file_is_fake_enum:
      p->filename = name;
      p->local_sym_name = name;
      break;
    case lang_input_file_is_l_enum:
      if (name[0] == ':' && name[1] != '\0')
	{
	  p->filename = name + 1;
	  p->flags.full_name_provided = TRUE;
	}
      else
	p->filename = name;
      p->local_sym_name = concat ("-l", name, (const char *) NULL);
      p->flags.maybe_archive = TRUE;
      p->flags.real = TRUE;
      p->flags.search_dirs = TRUE;
      break;
    case lang_input_file_is_marker_enum:
      p->filename = name;
      p->local_sym_name = name;
      p->flags.search_dirs = TRUE;
      break;
    case lang_input_file_is_search_file_enum:
      p->filename = name;
      p->local_sym_name = name;
      p->flags.real = TRUE;
      p->flags.search_dirs = TRUE;
      break;
    case lang_input_file_is_file_enum:
      p->filename = name;
      p->local_sym_name = name;
      p->flags.real = TRUE;
      break;
    default:
      FAIL ();
    }

  lang_statement_append (&input_file_chain, p, &p->next_real_file);
  return p;
}

lang_input_statement_type *
lang_add_input_file (const char *name,
		     lang_input_file_enum_type file_type,
		     const char *target)
{
  if (name != NULL
      && (*name == '=' || CONST_STRNEQ (name, "$SYSROOT")))
    {
      lang_input_statement_type *ret;
      char *sysrooted_name
	= concat (ld_sysroot,
		  name + (*name == '=' ? 1 : strlen ("$SYSROOT")),
		  (const char *) NULL);

      /* We've now forcibly prepended the sysroot, making the input
	 file independent of the context.  Therefore, temporarily
	 force a non-sysrooted context for this statement, so it won't
	 get the sysroot prepended again when opened.  (N.B. if it's a
	 script, any child nodes with input files starting with "/"
	 will be handled as "sysrooted" as they'll be found to be
	 within the sysroot subdirectory.)  */
      unsigned int outer_sysrooted = input_flags.sysrooted;
      input_flags.sysrooted = 0;
      ret = new_afile (sysrooted_name, file_type, target);
      input_flags.sysrooted = outer_sysrooted;
      return ret;
    }

  return new_afile (name, file_type, target);
}

struct out_section_hash_entry
{
  struct bfd_hash_entry root;
  lang_statement_union_type s;
};

/* The hash table.  */

static struct bfd_hash_table output_section_statement_table;

/* Support routines for the hash table used by lang_output_section_find,
   initialize the table, fill in an entry and remove the table.  */

static struct bfd_hash_entry *
output_section_statement_newfunc (struct bfd_hash_entry *entry,
				  struct bfd_hash_table *table,
				  const char *string)
{
  lang_output_section_statement_type **nextp;
  struct out_section_hash_entry *ret;

  if (entry == NULL)
    {
      entry = (struct bfd_hash_entry *) bfd_hash_allocate (table,
							   sizeof (*ret));
      if (entry == NULL)
	return entry;
    }

  entry = bfd_hash_newfunc (entry, table, string);
  if (entry == NULL)
    return entry;

  ret = (struct out_section_hash_entry *) entry;
  memset (&ret->s, 0, sizeof (ret->s));
  ret->s.header.type = lang_output_section_statement_enum;
  ret->s.output_section_statement.subsection_alignment = NULL;
  ret->s.output_section_statement.section_alignment = NULL;
  ret->s.output_section_statement.block_value = 1;
  lang_list_init (&ret->s.output_section_statement.children);
  lang_statement_append (stat_ptr, &ret->s, &ret->s.header.next);

  /* For every output section statement added to the list, except the
     first one, lang_os_list.tail points to the "next"
     field of the last element of the list.  */
  if (lang_os_list.head != NULL)
    ret->s.output_section_statement.prev
      = ((lang_output_section_statement_type *)
	 ((char *) lang_os_list.tail
	  - offsetof (lang_output_section_statement_type, next)));

  /* GCC's strict aliasing rules prevent us from just casting the
     address, so we store the pointer in a variable and cast that
     instead.  */
  nextp = &ret->s.output_section_statement.next;
  lang_statement_append (&lang_os_list, &ret->s, nextp);
  return &ret->root;
}

static void
output_section_statement_table_init (void)
{
  if (!bfd_hash_table_init_n (&output_section_statement_table,
			      output_section_statement_newfunc,
			      sizeof (struct out_section_hash_entry),
			      61))
    einfo (_("%F%P: can not create hash table: %E\n"));
}

static void
output_section_statement_table_free (void)
{
  bfd_hash_table_free (&output_section_statement_table);
}

/* Build enough state so that the parser can build its tree.  */

void
lang_init (void)
{
  obstack_begin (&stat_obstack, 1000);

  stat_ptr = &statement_list;

  output_section_statement_table_init ();

  lang_list_init (stat_ptr);

  lang_list_init (&input_file_chain);
  lang_list_init (&lang_os_list);
  lang_list_init (&file_chain);
  first_file = lang_add_input_file (NULL, lang_input_file_is_marker_enum,
				    NULL);
  abs_output_section =
    lang_output_section_statement_lookup (BFD_ABS_SECTION_NAME, 0, TRUE);

  abs_output_section->bfd_section = bfd_abs_section_ptr;

  asneeded_list_head = NULL;
  asneeded_list_tail = &asneeded_list_head;
}

void
lang_finish (void)
{
  output_section_statement_table_free ();
}

/*----------------------------------------------------------------------
  A region is an area of memory declared with the
  MEMORY {  name:org=exp, len=exp ... }
  syntax.

  We maintain a list of all the regions here.

  If no regions are specified in the script, then the default is used
  which is created when looked up to be the entire data space.

  If create is true we are creating a region inside a MEMORY block.
  In this case it is probably an error to create a region that has
  already been created.  If we are not inside a MEMORY block it is
  dubious to use an undeclared region name (except DEFAULT_MEMORY_REGION)
  and so we issue a warning.

  Each region has at least one name.  The first name is either
  DEFAULT_MEMORY_REGION or the name given in the MEMORY block.  You can add
  alias names to an existing region within a script with
  REGION_ALIAS (alias, region_name).  Each name corresponds to at most one
  region.  */

static lang_memory_region_type *lang_memory_region_list;
static lang_memory_region_type **lang_memory_region_list_tail
  = &lang_memory_region_list;

lang_memory_region_type *
lang_memory_region_lookup (const char *const name, bfd_boolean create)
{
  lang_memory_region_name *n;
  lang_memory_region_type *r;
  lang_memory_region_type *new_region;

  /* NAME is NULL for LMA memspecs if no region was specified.  */
  if (name == NULL)
    return NULL;

  for (r = lang_memory_region_list; r != NULL; r = r->next)
    for (n = &r->name_list; n != NULL; n = n->next)
      if (strcmp (n->name, name) == 0)
	{
	  if (create)
	    einfo (_("%P:%pS: warning: redeclaration of memory region `%s'\n"),
		   NULL, name);
	  return r;
	}

  if (!create && strcmp (name, DEFAULT_MEMORY_REGION))
    einfo (_("%P:%pS: warning: memory region `%s' not declared\n"),
	   NULL, name);

  new_region = stat_alloc (sizeof (lang_memory_region_type));

  new_region->name_list.name = xstrdup (name);
  new_region->name_list.next = NULL;
  new_region->next = NULL;
  new_region->origin_exp = NULL;
  new_region->origin = 0;
  new_region->length_exp = NULL;
  new_region->length = ~(bfd_size_type) 0;
  new_region->current = 0;
  new_region->last_os = NULL;
  new_region->flags = 0;
  new_region->not_flags = 0;
  new_region->had_full_message = FALSE;

  *lang_memory_region_list_tail = new_region;
  lang_memory_region_list_tail = &new_region->next;

  return new_region;
}

void
lang_memory_region_alias (const char *alias, const char *region_name)
{
  lang_memory_region_name *n;
  lang_memory_region_type *r;
  lang_memory_region_type *region;

  /* The default region must be unique.  This ensures that it is not necessary
     to iterate through the name list if someone wants the check if a region is
     the default memory region.  */
  if (strcmp (region_name, DEFAULT_MEMORY_REGION) == 0
      || strcmp (alias, DEFAULT_MEMORY_REGION) == 0)
    einfo (_("%F%P:%pS: error: alias for default memory region\n"), NULL);

  /* Look for the target region and check if the alias is not already
     in use.  */
  region = NULL;
  for (r = lang_memory_region_list; r != NULL; r = r->next)
    for (n = &r->name_list; n != NULL; n = n->next)
      {
	if (region == NULL && strcmp (n->name, region_name) == 0)
	  region = r;
	if (strcmp (n->name, alias) == 0)
	  einfo (_("%F%P:%pS: error: redefinition of memory region "
		   "alias `%s'\n"),
		 NULL, alias);
      }

  /* Check if the target region exists.  */
  if (region == NULL)
    einfo (_("%F%P:%pS: error: memory region `%s' "
	     "for alias `%s' does not exist\n"),
	   NULL, region_name, alias);

  /* Add alias to region name list.  */
  n = stat_alloc (sizeof (lang_memory_region_name));
  n->name = xstrdup (alias);
  n->next = region->name_list.next;
  region->name_list.next = n;
}

static lang_memory_region_type *
lang_memory_default (asection *section)
{
  lang_memory_region_type *p;

  flagword sec_flags = section->flags;

  /* Override SEC_DATA to mean a writable section.  */
  if ((sec_flags & (SEC_ALLOC | SEC_READONLY | SEC_CODE)) == SEC_ALLOC)
    sec_flags |= SEC_DATA;

  for (p = lang_memory_region_list; p != NULL; p = p->next)
    {
      if ((p->flags & sec_flags) != 0
	  && (p->not_flags & sec_flags) == 0)
	{
	  return p;
	}
    }
  return lang_memory_region_lookup (DEFAULT_MEMORY_REGION, FALSE);
}

/* Get the output section statement directly from the userdata.  */

lang_output_section_statement_type *
lang_output_section_get (const asection *output_section)
{
  return bfd_section_userdata (output_section);
}

/* Find or create an output_section_statement with the given NAME.
   If CONSTRAINT is non-zero match one with that constraint, otherwise
   match any non-negative constraint.  If CREATE, always make a
   new output_section_statement for SPECIAL CONSTRAINT.  */

lang_output_section_statement_type *
lang_output_section_statement_lookup (const char *name,
				      int constraint,
				      bfd_boolean create)
{
  struct out_section_hash_entry *entry;

  entry = ((struct out_section_hash_entry *)
	   bfd_hash_lookup (&output_section_statement_table, name,
			    create, FALSE));
  if (entry == NULL)
    {
      if (create)
	einfo (_("%F%P: failed creating section `%s': %E\n"), name);
      return NULL;
    }

  if (entry->s.output_section_statement.name != NULL)
    {
      /* We have a section of this name, but it might not have the correct
	 constraint.  */
      struct out_section_hash_entry *last_ent;

      name = entry->s.output_section_statement.name;
      if (create && constraint == SPECIAL)
	/* Not traversing to the end reverses the order of the second
	   and subsequent SPECIAL sections in the hash table chain,
	   but that shouldn't matter.  */
	last_ent = entry;
      else
	do
	  {
	    if (constraint == entry->s.output_section_statement.constraint
		|| (constraint == 0
		    && entry->s.output_section_statement.constraint >= 0))
	      return &entry->s.output_section_statement;
	    last_ent = entry;
	    entry = (struct out_section_hash_entry *) entry->root.next;
	  }
	while (entry != NULL
	       && name == entry->s.output_section_statement.name);

      if (!create)
	return NULL;

      entry
	= ((struct out_section_hash_entry *)
	   output_section_statement_newfunc (NULL,
					     &output_section_statement_table,
					     name));
      if (entry == NULL)
	{
	  einfo (_("%F%P: failed creating section `%s': %E\n"), name);
	  return NULL;
	}
      entry->root = last_ent->root;
      last_ent->root.next = &entry->root;
    }

  entry->s.output_section_statement.name = name;
  entry->s.output_section_statement.constraint = constraint;
  return &entry->s.output_section_statement;
}

/* Find the next output_section_statement with the same name as OS.
   If CONSTRAINT is non-zero, find one with that constraint otherwise
   match any non-negative constraint.  */

lang_output_section_statement_type *
next_matching_output_section_statement (lang_output_section_statement_type *os,
					int constraint)
{
  /* All output_section_statements are actually part of a
     struct out_section_hash_entry.  */
  struct out_section_hash_entry *entry = (struct out_section_hash_entry *)
    ((char *) os
     - offsetof (struct out_section_hash_entry, s.output_section_statement));
  const char *name = os->name;

  ASSERT (name == entry->root.string);
  do
    {
      entry = (struct out_section_hash_entry *) entry->root.next;
      if (entry == NULL
	  || name != entry->s.output_section_statement.name)
	return NULL;
    }
  while (constraint != entry->s.output_section_statement.constraint
	 && (constraint != 0
	     || entry->s.output_section_statement.constraint < 0));

  return &entry->s.output_section_statement;
}

/* A variant of lang_output_section_find used by place_orphan.
   Returns the output statement that should precede a new output
   statement for SEC.  If an exact match is found on certain flags,
   sets *EXACT too.  */

lang_output_section_statement_type *
lang_output_section_find_by_flags (const asection *sec,
				   flagword sec_flags,
				   lang_output_section_statement_type **exact,
				   lang_match_sec_type_func match_type)
{
  lang_output_section_statement_type *first, *look, *found;
  flagword look_flags, differ;

  /* We know the first statement on this list is *ABS*.  May as well
     skip it.  */
  first = (void *) lang_os_list.head;
  first = first->next;

  /* First try for an exact match.  */
  found = NULL;
  for (look = first; look; look = look->next)
    {
      look_flags = look->flags;
      if (look->bfd_section != NULL)
	{
	  look_flags = look->bfd_section->flags;
	  if (match_type && !match_type (link_info.output_bfd,
					 look->bfd_section,
					 sec->owner, sec))
	    continue;
	}
      differ = look_flags ^ sec_flags;
      if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD | SEC_READONLY
		      | SEC_CODE | SEC_SMALL_DATA | SEC_THREAD_LOCAL)))
	found = look;
    }
  if (found != NULL)
    {
      if (exact != NULL)
	*exact = found;
      return found;
    }

  if ((sec_flags & SEC_CODE) != 0
      && (sec_flags & SEC_ALLOC) != 0)
    {
      /* Try for a rw code section.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    {
	      look_flags = look->bfd_section->flags;
	      if (match_type && !match_type (link_info.output_bfd,
					     look->bfd_section,
					     sec->owner, sec))
		continue;
	    }
	  differ = look_flags ^ sec_flags;
	  if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD
			  | SEC_CODE | SEC_SMALL_DATA | SEC_THREAD_LOCAL)))
	    found = look;
	}
    }
  else if ((sec_flags & SEC_READONLY) != 0
	   && (sec_flags & SEC_ALLOC) != 0)
    {
      /* .rodata can go after .text, .sdata2 after .rodata.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    {
	      look_flags = look->bfd_section->flags;
	      if (match_type && !match_type (link_info.output_bfd,
					     look->bfd_section,
					     sec->owner, sec))
		continue;
	    }
	  differ = look_flags ^ sec_flags;
	  if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD
			  | SEC_READONLY | SEC_SMALL_DATA))
	      || (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD
			      | SEC_READONLY))
		  && !(look_flags & SEC_SMALL_DATA)))
	    found = look;
	}
    }
  else if ((sec_flags & SEC_THREAD_LOCAL) != 0
	   && (sec_flags & SEC_ALLOC) != 0)
    {
      /* .tdata can go after .data, .tbss after .tdata.  Treat .tbss
	 as if it were a loaded section, and don't use match_type.  */
      bfd_boolean seen_thread_local = FALSE;

      match_type = NULL;
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    look_flags = look->bfd_section->flags;

	  differ = look_flags ^ (sec_flags | SEC_LOAD | SEC_HAS_CONTENTS);
	  if (!(differ & (SEC_THREAD_LOCAL | SEC_ALLOC)))
	    {
	      /* .tdata and .tbss must be adjacent and in that order.  */
	      if (!(look_flags & SEC_LOAD)
		  && (sec_flags & SEC_LOAD))
		/* ..so if we're at a .tbss section and we're placing
		   a .tdata section stop looking and return the
		   previous section.  */
		break;
	      found = look;
	      seen_thread_local = TRUE;
	    }
	  else if (seen_thread_local)
	    break;
	  else if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD)))
	    found = look;
	}
    }
  else if ((sec_flags & SEC_SMALL_DATA) != 0
	   && (sec_flags & SEC_ALLOC) != 0)
    {
      /* .sdata goes after .data, .sbss after .sdata.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    {
	      look_flags = look->bfd_section->flags;
	      if (match_type && !match_type (link_info.output_bfd,
					     look->bfd_section,
					     sec->owner, sec))
		continue;
	    }
	  differ = look_flags ^ sec_flags;
	  if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD
			  | SEC_THREAD_LOCAL))
	      || ((look_flags & SEC_SMALL_DATA)
		  && !(sec_flags & SEC_HAS_CONTENTS)))
	    found = look;
	}
    }
  else if ((sec_flags & SEC_HAS_CONTENTS) != 0
	   && (sec_flags & SEC_ALLOC) != 0)
    {
      /* .data goes after .rodata.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    {
	      look_flags = look->bfd_section->flags;
	      if (match_type && !match_type (link_info.output_bfd,
					     look->bfd_section,
					     sec->owner, sec))
		continue;
	    }
	  differ = look_flags ^ sec_flags;
	  if (!(differ & (SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD
			  | SEC_SMALL_DATA | SEC_THREAD_LOCAL)))
	    found = look;
	}
    }
  else if ((sec_flags & SEC_ALLOC) != 0)
    {
      /* .bss goes after any other alloc section.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    {
	      look_flags = look->bfd_section->flags;
	      if (match_type && !match_type (link_info.output_bfd,
					     look->bfd_section,
					     sec->owner, sec))
		continue;
	    }
	  differ = look_flags ^ sec_flags;
	  if (!(differ & SEC_ALLOC))
	    found = look;
	}
    }
  else
    {
      /* non-alloc go last.  */
      for (look = first; look; look = look->next)
	{
	  look_flags = look->flags;
	  if (look->bfd_section != NULL)
	    look_flags = look->bfd_section->flags;
	  differ = look_flags ^ sec_flags;
	  if (!(differ & SEC_DEBUGGING))
	    found = look;
	}
      return found;
    }

  if (found || !match_type)
    return found;

  return lang_output_section_find_by_flags (sec, sec_flags, NULL, NULL);
}

/* Find the last output section before given output statement.
   Used by place_orphan.  */

static asection *
output_prev_sec_find (lang_output_section_statement_type *os)
{
  lang_output_section_statement_type *lookup;

  for (lookup = os->prev; lookup != NULL; lookup = lookup->prev)
    {
      if (lookup->constraint < 0)
	continue;

      if (lookup->bfd_section != NULL && lookup->bfd_section->owner != NULL)
	return lookup->bfd_section;
    }

  return NULL;
}

/* Look for a suitable place for a new output section statement.  The
   idea is to skip over anything that might be inside a SECTIONS {}
   statement in a script, before we find another output section
   statement.  Assignments to "dot" before an output section statement
   are assumed to belong to it, except in two cases;  The first
   assignment to dot, and assignments before non-alloc sections.
   Otherwise we might put an orphan before . = . + SIZEOF_HEADERS or
   similar assignments that set the initial address, or we might
   insert non-alloc note sections among assignments setting end of
   image symbols.  */

static lang_statement_union_type **
insert_os_after (lang_output_section_statement_type *after)
{
  lang_statement_union_type **where;
  lang_statement_union_type **assign = NULL;
  bfd_boolean ignore_first;

  ignore_first = after == (void *) lang_os_list.head;

  for (where = &after->header.next;
       *where != NULL;
       where = &(*where)->header.next)
    {
      switch ((*where)->header.type)
	{
	case lang_assignment_statement_enum:
	  if (assign == NULL)
	    {
	      lang_assignment_statement_type *ass;

	      ass = &(*where)->assignment_statement;
	      if (ass->exp->type.node_class != etree_assert
		  && ass->exp->assign.dst[0] == '.'
		  && ass->exp->assign.dst[1] == 0)
		{
		  if (!ignore_first)
		    assign = where;
		  ignore_first = FALSE;
		}
	    }
	  continue;
	case lang_wild_statement_enum:
	case lang_input_section_enum:
	case lang_object_symbols_statement_enum:
	case lang_fill_statement_enum:
	case lang_data_statement_enum:
	case lang_reloc_statement_enum:
	case lang_padding_statement_enum:
	case lang_constructors_statement_enum:
	  assign = NULL;
	  ignore_first = FALSE;
	  continue;
	case lang_output_section_statement_enum:
	  if (assign != NULL)
	    {
	      asection *s = (*where)->output_section_statement.bfd_section;

	      if (s == NULL
		  || s->map_head.s == NULL
		  || (s->flags & SEC_ALLOC) != 0)
		where = assign;
	    }
	  break;
	case lang_input_statement_enum:
	case lang_address_statement_enum:
	case lang_target_statement_enum:
	case lang_output_statement_enum:
	case lang_group_statement_enum:
	case lang_insert_statement_enum:
	  continue;
	}
      break;
    }

  return where;
}

lang_output_section_statement_type *
lang_insert_orphan (asection *s,
		    const char *secname,
		    int constraint,
		    lang_output_section_statement_type *after,
		    struct orphan_save *place,
		    etree_type *address,
		    lang_statement_list_type *add_child)
{
  lang_statement_list_type add;
  lang_output_section_statement_type *os;
  lang_output_section_statement_type **os_tail;

  /* If we have found an appropriate place for the output section
     statements for this orphan, add them to our own private list,
     inserting them later into the global statement list.  */
  if (after != NULL)
    {
      lang_list_init (&add);
      push_stat_ptr (&add);
    }

  if (bfd_link_relocatable (&link_info)
      || (s->flags & (SEC_LOAD | SEC_ALLOC)) == 0)
    address = exp_intop (0);

  os_tail = (lang_output_section_statement_type **) lang_os_list.tail;
  os = lang_enter_output_section_statement (secname, address, normal_section,
					    NULL, NULL, NULL, constraint, 0);

  if (add_child == NULL)
    add_child = &os->children;
  lang_add_section (add_child, s, NULL, os);

  if (after && (s->flags & (SEC_LOAD | SEC_ALLOC)) != 0)
    {
      const char *region = (after->region
			    ? after->region->name_list.name
			    : DEFAULT_MEMORY_REGION);
      const char *lma_region = (after->lma_region
				? after->lma_region->name_list.name
				: NULL);
      lang_leave_output_section_statement (NULL, region, after->phdrs,
					   lma_region);
    }
  else
    lang_leave_output_section_statement (NULL, DEFAULT_MEMORY_REGION, NULL,
					 NULL);

  /* Restore the global list pointer.  */
  if (after != NULL)
    pop_stat_ptr ();

  if (after != NULL && os->bfd_section != NULL)
    {
      asection *snew, *as;
      bfd_boolean place_after = place->stmt == NULL;
      bfd_boolean insert_after = TRUE;

      snew = os->bfd_section;

      /* Shuffle the bfd section list to make the output file look
	 neater.  This is really only cosmetic.  */
      if (place->section == NULL
	  && after != (void *) lang_os_list.head)
	{
	  asection *bfd_section = after->bfd_section;

	  /* If the output statement hasn't been used to place any input
	     sections (and thus doesn't have an output bfd_section),
	     look for the closest prior output statement having an
	     output section.  */
	  if (bfd_section == NULL)
	    bfd_section = output_prev_sec_find (after);

	  if (bfd_section != NULL && bfd_section != snew)
	    place->section = &bfd_section->next;
	}

      if (place->section == NULL)
	place->section = &link_info.output_bfd->sections;

      as = *place->section;

      if (!as)
	{
	  /* Put the section at the end of the list.  */

	  /* Unlink the section.  */
	  bfd_section_list_remove (link_info.output_bfd, snew);

	  /* Now tack it back on in the right place.  */
	  bfd_section_list_append (link_info.output_bfd, snew);
	}
      else if ((bfd_get_flavour (link_info.output_bfd)
		== bfd_target_elf_flavour)
	       && (bfd_get_flavour (s->owner)
		   == bfd_target_elf_flavour)
	       && ((elf_section_type (s) == SHT_NOTE
		    && (s->flags & SEC_LOAD) != 0)
		   || (elf_section_type (as) == SHT_NOTE
		       && (as->flags & SEC_LOAD) != 0)))
	{
	  /* Make sure that output note sections are grouped and sorted
	     by alignments when inserting a note section or insert a
	     section after a note section,  */
	  asection *sec;
	  /* A specific section after which the output note section
	     should be placed.  */
	  asection *after_sec;
	  /* True if we need to insert the orphan section after a
	     specific section to maintain output note section order.  */
	  bfd_boolean after_sec_note = FALSE;

	  static asection *first_orphan_note = NULL;

	  /* Group and sort output note section by alignments in
	     ascending order.  */
	  after_sec = NULL;
	  if (elf_section_type (s) == SHT_NOTE
	      && (s->flags & SEC_LOAD) != 0)
	    {
	      /* Search from the beginning for the last output note
		 section with equal or larger alignments.  NB: Don't
		 place orphan note section after non-note sections.  */

	      first_orphan_note = NULL;
	      for (sec = link_info.output_bfd->sections;
		   (sec != NULL
		    && !bfd_is_abs_section (sec));
		   sec = sec->next)
		if (sec != snew
		    && elf_section_type (sec) == SHT_NOTE
		    && (sec->flags & SEC_LOAD) != 0)
		  {
		    if (!first_orphan_note)
		      first_orphan_note = sec;
		    if (sec->alignment_power >= s->alignment_power)
		      after_sec = sec;
		  }
		else if (first_orphan_note)
		  {
		    /* Stop if there is non-note section after the first
		       orphan note section.  */
		    break;
		  }

	      /* If this will be the first orphan note section, it can
		 be placed at the default location.  */
	      after_sec_note = first_orphan_note != NULL;
	      if (after_sec == NULL && after_sec_note)
		{
		  /* If all output note sections have smaller
		     alignments, place the section before all
		     output orphan note sections.  */
		  after_sec = first_orphan_note;
		  insert_after = FALSE;
		}
	    }
	  else if (first_orphan_note)
	    {
	      /* Don't place non-note sections in the middle of orphan
	         note sections.  */
	      after_sec_note = TRUE;
	      after_sec = as;
	      for (sec = as->next;
		   (sec != NULL
		    && !bfd_is_abs_section (sec));
		   sec = sec->next)
		if (elf_section_type (sec) == SHT_NOTE
		    && (sec->flags & SEC_LOAD) != 0)
		  after_sec = sec;
	    }

	  if (after_sec_note)
	    {
	      if (after_sec)
		{
		  /* Search forward to insert OS after AFTER_SEC output
		     statement.  */
		  lang_output_section_statement_type *stmt, *next;
		  bfd_boolean found = FALSE;
		  for (stmt = after; stmt != NULL; stmt = next)
		    {
		      next = stmt->next;
		      if (insert_after)
			{
			  if (stmt->bfd_section == after_sec)
			    {
			      place_after = TRUE;
			      found = TRUE;
			      after = stmt;
			      break;
			    }
			}
		      else
			{
			  /* If INSERT_AFTER is FALSE, place OS before
			     AFTER_SEC output statement.  */
			  if (next && next->bfd_section == after_sec)
			    {
			      place_after = TRUE;
			      found = TRUE;
			      after = stmt;
			      break;
			    }
			}
		    }

		  /* Search backward to insert OS after AFTER_SEC output
		     statement.  */
		  if (!found)
		    for (stmt = after; stmt != NULL; stmt = stmt->prev)
		      {
			if (insert_after)
			  {
			    if (stmt->bfd_section == after_sec)
			      {
				place_after = TRUE;
				after = stmt;
				break;
			      }
			  }
			else
			  {
			    /* If INSERT_AFTER is FALSE, place OS before
			       AFTER_SEC output statement.  */
			    if (stmt->next->bfd_section == after_sec)
			      {
				place_after = TRUE;
				after = stmt;
				break;
			      }
			  }
		      }
		}

	      if (after_sec == NULL
		  || (insert_after && after_sec->next != snew)
		  || (!insert_after && after_sec->prev != snew))
		{
		  /* Unlink the section.  */
		  bfd_section_list_remove (link_info.output_bfd, snew);

		  /* Place SNEW after AFTER_SEC.  If AFTER_SEC is NULL,
		     prepend SNEW.  */
		  if (after_sec)
		    {
		      if (insert_after)
			bfd_section_list_insert_after (link_info.output_bfd,
						       after_sec, snew);
		      else
			bfd_section_list_insert_before (link_info.output_bfd,
						       after_sec, snew);
		    }
		  else
		    bfd_section_list_prepend (link_info.output_bfd, snew);
		}
	    }
	  else if (as != snew && as->prev != snew)
	    {
	      /* Unlink the section.  */
	      bfd_section_list_remove (link_info.output_bfd, snew);

	      /* Now tack it back on in the right place.  */
	      bfd_section_list_insert_before (link_info.output_bfd,
					      as, snew);
	    }
	}
      else if (as != snew && as->prev != snew)
	{
	  /* Unlink the section.  */
	  bfd_section_list_remove (link_info.output_bfd, snew);

	  /* Now tack it back on in the right place.  */
	  bfd_section_list_insert_before (link_info.output_bfd, as, snew);
	}

      /* Save the end of this list.  Further ophans of this type will
	 follow the one we've just added.  */
      place->section = &snew->next;

      /* The following is non-cosmetic.  We try to put the output
	 statements in some sort of reasonable order here, because they
	 determine the final load addresses of the orphan sections.
	 In addition, placing output statements in the wrong order may
	 require extra segments.  For instance, given a typical
	 situation of all read-only sections placed in one segment and
	 following that a segment containing all the read-write
	 sections, we wouldn't want to place an orphan read/write
	 section before or amongst the read-only ones.  */
      if (add.head != NULL)
	{
	  lang_output_section_statement_type *newly_added_os;

	  /* Place OS after AFTER if AFTER_NOTE is TRUE.  */
	  if (place_after)
	    {
	      lang_statement_union_type **where = insert_os_after (after);

	      *add.tail = *where;
	      *where = add.head;

	      place->os_tail = &after->next;
	    }
	  else
	    {
	      /* Put it after the last orphan statement we added.  */
	      *add.tail = *place->stmt;
	      *place->stmt = add.head;
	    }

	  /* Fix the global list pointer if we happened to tack our
	     new list at the tail.  */
	  if (*stat_ptr->tail == add.head)
	    stat_ptr->tail = add.tail;

	  /* Save the end of this list.  */
	  place->stmt = add.tail;

	  /* Do the same for the list of output section statements.  */
	  newly_added_os = *os_tail;
	  *os_tail = NULL;
	  newly_added_os->prev = (lang_output_section_statement_type *)
	    ((char *) place->os_tail
	     - offsetof (lang_output_section_statement_type, next));
	  newly_added_os->next = *place->os_tail;
	  if (newly_added_os->next != NULL)
	    newly_added_os->next->prev = newly_added_os;
	  *place->os_tail = newly_added_os;
	  place->os_tail = &newly_added_os->next;

	  /* Fixing the global list pointer here is a little different.
	     We added to the list in lang_enter_output_section_statement,
	     trimmed off the new output_section_statment above when
	     assigning *os_tail = NULL, but possibly added it back in
	     the same place when assigning *place->os_tail.  */
	  if (*os_tail == NULL)
	    lang_os_list.tail = (lang_statement_union_type **) os_tail;
	}
    }
  return os;
}

static void
lang_print_asneeded (void)
{
  struct asneeded_minfo *m;

  if (asneeded_list_head == NULL)
    return;

  minfo (_("\nAs-needed library included to satisfy reference by file (symbol)\n\n"));

  for (m = asneeded_list_head; m != NULL; m = m->next)
    {
      size_t len;

      minfo ("%s", m->soname);
      len = strlen (m->soname);

      if (len >= 29)
	{
	  print_nl ();
	  len = 0;
	}
      while (len < 30)
	{
	  print_space ();
	  ++len;
	}

      if (m->ref != NULL)
	minfo ("%pB ", m->ref);
      minfo ("(%pT)\n", m->name);
    }
}

static void
lang_map_flags (flagword flag)
{
  if (flag & SEC_ALLOC)
    minfo ("a");

  if (flag & SEC_CODE)
    minfo ("x");

  if (flag & SEC_READONLY)
    minfo ("r");

  if (flag & SEC_DATA)
    minfo ("w");

  if (flag & SEC_LOAD)
    minfo ("l");
}

void
lang_map (void)
{
  lang_memory_region_type *m;
  bfd_boolean dis_header_printed = FALSE;

  LANG_FOR_EACH_INPUT_STATEMENT (file)
    {
      asection *s;

      if ((file->the_bfd->flags & (BFD_LINKER_CREATED | DYNAMIC)) != 0
	  || file->flags.just_syms)
	continue;

      if (config.print_map_discarded)
	for (s = file->the_bfd->sections; s != NULL; s = s->next)
	  if ((s->output_section == NULL
	       || s->output_section->owner != link_info.output_bfd)
	      && (s->flags & (SEC_LINKER_CREATED | SEC_KEEP)) == 0)
	    {
	      if (! dis_header_printed)
		{
		  fprintf (config.map_file, _("\nDiscarded input sections\n\n"));
		  dis_header_printed = TRUE;
		}

	      print_input_section (s, TRUE);
	    }
    }

  minfo (_("\nMemory Configuration\n\n"));
  fprintf (config.map_file, "%-16s %-18s %-18s %s\n",
	   _("Name"), _("Origin"), _("Length"), _("Attributes"));

  for (m = lang_memory_region_list; m != NULL; m = m->next)
    {
      char buf[100];
      int len;

      fprintf (config.map_file, "%-16s ", m->name_list.name);

      sprintf_vma (buf, m->origin);
      minfo ("0x%s ", buf);
      len = strlen (buf);
      while (len < 16)
	{
	  print_space ();
	  ++len;
	}

      minfo ("0x%V", m->length);
      if (m->flags || m->not_flags)
	{
#ifndef BFD64
	  minfo ("        ");
#endif
	  if (m->flags)
	    {
	      print_space ();
	      lang_map_flags (m->flags);
	    }

	  if (m->not_flags)
	    {
	      minfo (" !");
	      lang_map_flags (m->not_flags);
	    }
	}

      print_nl ();
    }

  fprintf (config.map_file, _("\nLinker script and memory map\n\n"));

  if (!link_info.reduce_memory_overheads)
    {
      obstack_begin (&map_obstack, 1000);
      bfd_link_hash_traverse (link_info.hash, sort_def_symbol, 0);
    }
  expld.phase = lang_fixed_phase_enum;
  lang_statement_iteration++;
  print_statements ();

  ldemul_extra_map_file_text (link_info.output_bfd, &link_info,
			      config.map_file);
}

static bfd_boolean
sort_def_symbol (struct bfd_link_hash_entry *hash_entry,
		 void *info ATTRIBUTE_UNUSED)
{
  if ((hash_entry->type == bfd_link_hash_defined
       || hash_entry->type == bfd_link_hash_defweak)
      && hash_entry->u.def.section->owner != link_info.output_bfd
      && hash_entry->u.def.section->owner != NULL)
    {
      input_section_userdata_type *ud;
      struct map_symbol_def *def;

      ud = bfd_section_userdata (hash_entry->u.def.section);
      if (!ud)
	{
	  ud = stat_alloc (sizeof (*ud));
	  bfd_set_section_userdata (hash_entry->u.def.section, ud);
	  ud->map_symbol_def_tail = &ud->map_symbol_def_head;
	  ud->map_symbol_def_count = 0;
	}
      else if (!ud->map_symbol_def_tail)
	ud->map_symbol_def_tail = &ud->map_symbol_def_head;

      def = (struct map_symbol_def *) obstack_alloc (&map_obstack, sizeof *def);
      def->entry = hash_entry;
      *(ud->map_symbol_def_tail) = def;
      ud->map_symbol_def_tail = &def->next;
      ud->map_symbol_def_count++;
    }
  return TRUE;
}

/* Initialize an output section.  */

static void
init_os (lang_output_section_statement_type *s, flagword flags)
{
  if (strcmp (s->name, DISCARD_SECTION_NAME) == 0)
    einfo (_("%F%P: illegal use of `%s' section\n"), DISCARD_SECTION_NAME);

  if (s->constraint != SPECIAL)
    s->bfd_section = bfd_get_section_by_name (link_info.output_bfd, s->name);
  if (s->bfd_section == NULL)
    s->bfd_section = bfd_make_section_anyway_with_flags (link_info.output_bfd,
							 s->name, flags);
  if (s->bfd_section == NULL)
    {
      einfo (_("%F%P: output format %s cannot represent section"
	       " called %s: %E\n"),
	     link_info.output_bfd->xvec->name, s->name);
    }
  s->bfd_section->output_section = s->bfd_section;
  s->bfd_section->output_offset = 0;

  /* Set the userdata of the output section to the output section
     statement to avoid lookup.  */
  bfd_set_section_userdata (s->bfd_section, s);

  /* If there is a base address, make sure that any sections it might
     mention are initialized.  */
  if (s->addr_tree != NULL)
    exp_init_os (s->addr_tree);

  if (s->load_base != NULL)
    exp_init_os (s->load_base);

  /* If supplied an alignment, set it.  */
  if (s->section_alignment != NULL)
    s->bfd_section->alignment_power = exp_get_power (s->section_alignment,
						     "section alignment");
}

/* Make sure that all output sections mentioned in an expression are
   initialized.  */

static void
exp_init_os (etree_type *exp)
{
  switch (exp->type.node_class)
    {
    case etree_assign:
    case etree_provide:
    case etree_provided:
      exp_init_os (exp->assign.src);
      break;

    case etree_binary:
      exp_init_os (exp->binary.lhs);
      exp_init_os (exp->binary.rhs);
      break;

    case etree_trinary:
      exp_init_os (exp->trinary.cond);
      exp_init_os (exp->trinary.lhs);
      exp_init_os (exp->trinary.rhs);
      break;

    case etree_assert:
      exp_init_os (exp->assert_s.child);
      break;

    case etree_unary:
      exp_init_os (exp->unary.child);
      break;

    case etree_name:
      switch (exp->type.node_code)
	{
	case ADDR:
	case LOADADDR:
	case SIZEOF:
	  {
	    lang_output_section_statement_type *os;

	    os = lang_output_section_find (exp->name.name);
	    if (os != NULL && os->bfd_section == NULL)
	      init_os (os, 0);
	  }
	}
      break;

    default:
      break;
    }
}

static void
section_already_linked (bfd *abfd, asection *sec, void *data)
{
  lang_input_statement_type *entry = (lang_input_statement_type *) data;

  /* If we are only reading symbols from this object, then we want to
     discard all sections.  */
  if (entry->flags.just_syms)
    {
      bfd_link_just_syms (abfd, sec, &link_info);
      return;
    }

  /* Deal with SHF_EXCLUDE ELF sections.  */
  if (!bfd_link_relocatable (&link_info)
      && (abfd->flags & BFD_PLUGIN) == 0
      && (sec->flags & (SEC_GROUP | SEC_KEEP | SEC_EXCLUDE)) == SEC_EXCLUDE)
    sec->output_section = bfd_abs_section_ptr;

  if (!(abfd->flags & DYNAMIC))
    bfd_section_already_linked (abfd, sec, &link_info);
}


/* Returns true if SECTION is one we know will be discarded based on its
   section flags, otherwise returns false.  */

static bfd_boolean
lang_discard_section_p (asection *section)
{
  bfd_boolean discard;
  flagword flags = section->flags;

  /* Discard sections marked with SEC_EXCLUDE.  */
  discard = (flags & SEC_EXCLUDE) != 0;

  /* Discard the group descriptor sections when we're finally placing the
     sections from within the group.  */
  if ((flags & SEC_GROUP) != 0
      && link_info.resolve_section_groups)
    discard = TRUE;

  /* Discard debugging sections if we are stripping debugging
     information.  */
  if ((link_info.strip == strip_debugger || link_info.strip == strip_all)
      && (flags & SEC_DEBUGGING) != 0)
    discard = TRUE;

  return discard;
}

/* The wild routines.

   These expand statements like *(.text) and foo.o to a list of
   explicit actions, like foo.o(.text), bar.o(.text) and
   foo.o(.text, .data).  */

/* Add SECTION to the output section OUTPUT.  Do this by creating a
   lang_input_section statement which is placed at PTR.  */

void
lang_add_section (lang_statement_list_type *ptr,
		  asection *section,
		  struct flag_info *sflag_info,
		  lang_output_section_statement_type *output)
{
  flagword flags = section->flags;

  bfd_boolean discard;
  lang_input_section_type *new_section;
  bfd *abfd = link_info.output_bfd;

  /* Is this section one we know should be discarded?  */
  discard = lang_discard_section_p (section);

  /* Discard input sections which are assigned to a section named
     DISCARD_SECTION_NAME.  */
  if (strcmp (output->name, DISCARD_SECTION_NAME) == 0)
    discard = TRUE;

  if (discard)
    {
      if (section->output_section == NULL)
	{
	  /* This prevents future calls from assigning this section.  */
	  section->output_section = bfd_abs_section_ptr;
	}
      return;
    }

  if (sflag_info)
    {
      bfd_boolean keep;

      keep = bfd_lookup_section_flags (&link_info, sflag_info, section);
      if (!keep)
	return;
    }

  if (section->output_section != NULL)
    return;

  /* We don't copy the SEC_NEVER_LOAD flag from an input section
     to an output section, because we want to be able to include a
     SEC_NEVER_LOAD section in the middle of an otherwise loaded
     section (I don't know why we want to do this, but we do).
     build_link_order in ldwrite.c handles this case by turning
     the embedded SEC_NEVER_LOAD section into a fill.  */
  flags &= ~ SEC_NEVER_LOAD;

  /* If final link, don't copy the SEC_LINK_ONCE flags, they've
     already been processed.  One reason to do this is that on pe
     format targets, .text$foo sections go into .text and it's odd
     to see .text with SEC_LINK_ONCE set.  */
  if ((flags & (SEC_LINK_ONCE | SEC_GROUP)) == (SEC_LINK_ONCE | SEC_GROUP))
    {
      if (link_info.resolve_section_groups)
	flags &= ~(SEC_LINK_ONCE | SEC_LINK_DUPLICATES | SEC_RELOC);
      else
	flags &= ~(SEC_LINK_DUPLICATES | SEC_RELOC);
    }
  else if (!bfd_link_relocatable (&link_info))
    flags &= ~(SEC_LINK_ONCE | SEC_LINK_DUPLICATES | SEC_RELOC);

  switch (output->sectype)
    {
    case normal_section:
    case overlay_section:
    case first_overlay_section:
      break;
    case noalloc_section:
      flags &= ~SEC_ALLOC;
      break;
    case noload_section:
      flags &= ~SEC_LOAD;
      flags |= SEC_NEVER_LOAD;
      /* Unfortunately GNU ld has managed to evolve two different
	 meanings to NOLOAD in scripts.  ELF gets a .bss style noload,
	 alloc, no contents section.  All others get a noload, noalloc
	 section.  */
      if (bfd_get_flavour (link_info.output_bfd) == bfd_target_elf_flavour)
	flags &= ~SEC_HAS_CONTENTS;
      else
	flags &= ~SEC_ALLOC;
      break;
    }

  if (output->bfd_section == NULL)
    init_os (output, flags);

  /* If SEC_READONLY is not set in the input section, then clear
     it from the output section.  */
  output->bfd_section->flags &= flags | ~SEC_READONLY;

  if (output->bfd_section->linker_has_input)
    {
      /* Only set SEC_READONLY flag on the first input section.  */
      flags &= ~ SEC_READONLY;

      /* Keep SEC_MERGE and SEC_STRINGS only if they are the same.  */
      if ((output->bfd_section->flags & (SEC_MERGE | SEC_STRINGS))
	  != (flags & (SEC_MERGE | SEC_STRINGS))
	  || ((flags & SEC_MERGE) != 0
	      && output->bfd_section->entsize != section->entsize))
	{
	  output->bfd_section->flags &= ~ (SEC_MERGE | SEC_STRINGS);
	  flags &= ~ (SEC_MERGE | SEC_STRINGS);
	}
    }
  output->bfd_section->flags |= flags;

  if (!output->bfd_section->linker_has_input)
    {
      output->bfd_section->linker_has_input = 1;
      /* This must happen after flags have been updated.  The output
	 section may have been created before we saw its first input
	 section, eg. for a data statement.  */
      bfd_init_private_section_data (section->owner, section,
				     link_info.output_bfd,
				     output->bfd_section,
				     &link_info);
      if ((flags & SEC_MERGE) != 0)
	output->bfd_section->entsize = section->entsize;
    }

  if ((flags & SEC_TIC54X_BLOCK) != 0
      && bfd_get_arch (section->owner) == bfd_arch_tic54x)
    {
      /* FIXME: This value should really be obtained from the bfd...  */
      output->block_value = 128;
    }

  if (section->alignment_power > output->bfd_section->alignment_power)
    output->bfd_section->alignment_power = section->alignment_power;

  section->output_section = output->bfd_section;

  if (!map_head_is_link_order)
    {
      asection *s = output->bfd_section->map_tail.s;
      output->bfd_section->map_tail.s = section;
      section->map_head.s = NULL;
      section->map_tail.s = s;
      if (s != NULL)
	s->map_head.s = section;
      else
	output->bfd_section->map_head.s = section;
    }

  /* Add a section reference to the list.  */
  new_section = new_stat (lang_input_section, ptr);
  new_section->section = section;
}

/* Handle wildcard sorting.  This returns the lang_input_section which
   should follow the one we are going to create for SECTION and FILE,
   based on the sorting requirements of WILD.  It returns NULL if the
   new section should just go at the end of the current list.  */

static lang_statement_union_type *
wild_sort (lang_wild_statement_type *wild,
	   struct wildcard_list *sec,
	   lang_input_statement_type *file,
	   asection *section)
{
  lang_statement_union_type *l;

  if (!wild->filenames_sorted
      && (sec == NULL || sec->spec.sorted == none))
    return NULL;

  for (l = wild->children.head; l != NULL; l = l->header.next)
    {
      lang_input_section_type *ls;

      if (l->header.type != lang_input_section_enum)
	continue;
      ls = &l->input_section;

      /* Sorting by filename takes precedence over sorting by section
	 name.  */

      if (wild->filenames_sorted)
	{
	  const char *fn, *ln;
	  bfd_boolean fa, la;
	  int i;

	  /* The PE support for the .idata section as generated by
	     dlltool assumes that files will be sorted by the name of
	     the archive and then the name of the file within the
	     archive.  */

	  if (file->the_bfd != NULL
	      && file->the_bfd->my_archive != NULL)
	    {
	      fn = bfd_get_filename (file->the_bfd->my_archive);
	      fa = TRUE;
	    }
	  else
	    {
	      fn = file->filename;
	      fa = FALSE;
	    }

	  if (ls->section->owner->my_archive != NULL)
	    {
	      ln = bfd_get_filename (ls->section->owner->my_archive);
	      la = TRUE;
	    }
	  else
	    {
	      ln = ls->section->owner->filename;
	      la = FALSE;
	    }

	  i = filename_cmp (fn, ln);
	  if (i > 0)
	    continue;
	  else if (i < 0)
	    break;

	  if (fa || la)
	    {
	      if (fa)
		fn = file->filename;
	      if (la)
		ln = ls->section->owner->filename;

	      i = filename_cmp (fn, ln);
	      if (i > 0)
		continue;
	      else if (i < 0)
		break;
	    }
	}

      /* Here either the files are not sorted by name, or we are
	 looking at the sections for this file.  */

      if (sec != NULL
	  && sec->spec.sorted != none
	  && sec->spec.sorted != by_none)
	if (compare_section (sec->spec.sorted, section, ls->section) < 0)
	  break;
    }

  return l;
}

/* Expand a wild statement for a particular FILE.  SECTION may be
   NULL, in which case it is a wild card.  */

static void
output_section_callback (lang_wild_statement_type *ptr,
			 struct wildcard_list *sec,
			 asection *section,
			 struct flag_info *sflag_info,
			 lang_input_statement_type *file,
			 void *output)
{
  lang_statement_union_type *before;
  lang_output_section_statement_type *os;

  os = (lang_output_section_statement_type *) output;

  /* Exclude sections that match UNIQUE_SECTION_LIST.  */
  if (unique_section_p (section, os))
    return;

  before = wild_sort (ptr, sec, file, section);

  /* Here BEFORE points to the lang_input_section which
     should follow the one we are about to add.  If BEFORE
     is NULL, then the section should just go at the end
     of the current list.  */

  if (before == NULL)
    lang_add_section (&ptr->children, section, sflag_info, os);
  else
    {
      lang_statement_list_type list;
      lang_statement_union_type **pp;

      lang_list_init (&list);
      lang_add_section (&list, section, sflag_info, os);

      /* If we are discarding the section, LIST.HEAD will
	 be NULL.  */
      if (list.head != NULL)
	{
	  ASSERT (list.head->header.next == NULL);

	  for (pp = &ptr->children.head;
	       *pp != before;
	       pp = &(*pp)->header.next)
	    ASSERT (*pp != NULL);

	  list.head->header.next = *pp;
	  *pp = list.head;
	}
    }
}

/* Check if all sections in a wild statement for a particular FILE
   are readonly.  */

static void
check_section_callback (lang_wild_statement_type *ptr ATTRIBUTE_UNUSED,
			struct wildcard_list *sec ATTRIBUTE_UNUSED,
			asection *section,
			struct flag_info *sflag_info ATTRIBUTE_UNUSED,
			lang_input_statement_type *file ATTRIBUTE_UNUSED,
			void *output)
{
  lang_output_section_statement_type *os;

  os = (lang_output_section_statement_type *) output;

  /* Exclude sections that match UNIQUE_SECTION_LIST.  */
  if (unique_section_p (section, os))
    return;

  if (section->output_section == NULL && (section->flags & SEC_READONLY) == 0)
    os->all_input_readonly = FALSE;
}

/* This is passed a file name which must have been seen already and
   added to the statement tree.  We will see if it has been opened
   already and had its symbols read.  If not then we'll read it.  */

static lang_input_statement_type *
lookup_name (const char *name)
{
  lang_input_statement_type *search;

  for (search = (void *) input_file_chain.head;
       search != NULL;
       search = search->next_real_file)
    {
      /* Use the local_sym_name as the name of the file that has
	 already been loaded as filename might have been transformed
	 via the search directory lookup mechanism.  */
      const char *filename = search->local_sym_name;

      if (filename != NULL
	  && filename_cmp (filename, name) == 0)
	break;
    }

  if (search == NULL)
    {
      /* Arrange to splice the input statement added by new_afile into
	 statement_list after the current input_file_chain tail.
	 We know input_file_chain is not an empty list, and that
	 lookup_name was called via open_input_bfds.  Later calls to
	 lookup_name should always match an existing input_statement.  */
      lang_statement_union_type **tail = stat_ptr->tail;
      lang_statement_union_type **after
	= (void *) ((char *) input_file_chain.tail
		    - offsetof (lang_input_statement_type, next_real_file)
		    + offsetof (lang_input_statement_type, header.next));
      lang_statement_union_type *rest = *after;
      stat_ptr->tail = after;
      search = new_afile (name, lang_input_file_is_search_file_enum,
			  default_target);
      *stat_ptr->tail = rest;
      if (*tail == NULL)
	stat_ptr->tail = tail;
    }

  /* If we have already added this file, or this file is not real
     don't add this file.  */
  if (search->flags.loaded || !search->flags.real)
    return search;

  if (!load_symbols (search, NULL))
    return NULL;

  return search;
}

/* Save LIST as a list of libraries whose symbols should not be exported.  */

struct excluded_lib
{
  char *name;
  struct excluded_lib *next;
};
static struct excluded_lib *excluded_libs;

void
add_excluded_libs (const char *list)
{
  const char *p = list, *end;

  while (*p != '\0')
    {
      struct excluded_lib *entry;
      end = strpbrk (p, ",:");
      if (end == NULL)
	end = p + strlen (p);
      entry = (struct excluded_lib *) xmalloc (sizeof (*entry));
      entry->next = excluded_libs;
      entry->name = (char *) xmalloc (end - p + 1);
      memcpy (entry->name, p, end - p);
      entry->name[end - p] = '\0';
      excluded_libs = entry;
      if (*end == '\0')
	break;
      p = end + 1;
    }
}

static void
check_excluded_libs (bfd *abfd)
{
  struct excluded_lib *lib = excluded_libs;

  while (lib)
    {
      int len = strlen (lib->name);
      const char *filename = lbasename (abfd->filename);

      if (strcmp (lib->name, "ALL") == 0)
	{
	  abfd->no_export = TRUE;
	  return;
	}

      if (filename_ncmp (lib->name, filename, len) == 0
	  && (filename[len] == '\0'
	      || (filename[len] == '.' && filename[len + 1] == 'a'
		  && filename[len + 2] == '\0')))
	{
	  abfd->no_export = TRUE;
	  return;
	}

      lib = lib->next;
    }
}

/* Get the symbols for an input file.  */

bfd_boolean
load_symbols (lang_input_statement_type *entry,
	      lang_statement_list_type *place)
{
  char **matching;

  if (entry->flags.loaded)
    return TRUE;

  ldfile_open_file (entry);

  /* Do not process further if the file was missing.  */
  if (entry->flags.missing_file)
    return TRUE;

  if (trace_files || verbose)
    info_msg ("%pI\n", entry);

  if (!bfd_check_format (entry->the_bfd, bfd_archive)
      && !bfd_check_format_matches (entry->the_bfd, bfd_object, &matching))
    {
      bfd_error_type err;
      struct lang_input_statement_flags save_flags;
      extern FILE *yyin;

      err = bfd_get_error ();

      /* See if the emulation has some special knowledge.  */
      if (ldemul_unrecognized_file (entry))
	return TRUE;

      if (err == bfd_error_file_ambiguously_recognized)
	{
	  char **p;

	  einfo (_("%P: %pB: file not recognized: %E;"
		   " matching formats:"), entry->the_bfd);
	  for (p = matching; *p != NULL; p++)
	    einfo (" %s", *p);
	  einfo ("%F\n");
	}
      else if (err != bfd_error_file_not_recognized
	       || place == NULL)
	einfo (_("%F%P: %pB: file not recognized: %E\n"), entry->the_bfd);

      bfd_close (entry->the_bfd);
      entry->the_bfd = NULL;

      /* Try to interpret the file as a linker script.  */
      save_flags = input_flags;
      ldfile_open_command_file (entry->filename);

      push_stat_ptr (place);
      input_flags.add_DT_NEEDED_for_regular
	= entry->flags.add_DT_NEEDED_for_regular;
      input_flags.add_DT_NEEDED_for_dynamic
	= entry->flags.add_DT_NEEDED_for_dynamic;
      input_flags.whole_archive = entry->flags.whole_archive;
      input_flags.dynamic = entry->flags.dynamic;

      ldfile_assumed_script = TRUE;
      parser_input = input_script;
      yyparse ();
      ldfile_assumed_script = FALSE;

      /* missing_file is sticky.  sysrooted will already have been
	 restored when seeing EOF in yyparse, but no harm to restore
	 again.  */
      save_flags.missing_file |= input_flags.missing_file;
      input_flags = save_flags;
      pop_stat_ptr ();
      fclose (yyin);
      yyin = NULL;
      entry->flags.loaded = TRUE;

      return TRUE;
    }

  if (ldemul_recognized_file (entry))
    return TRUE;

  /* We don't call ldlang_add_file for an archive.  Instead, the
     add_symbols entry point will call ldlang_add_file, via the
     add_archive_element callback, for each element of the archive
     which is used.  */
  switch (bfd_get_format (entry->the_bfd))
    {
    default:
      break;

    case bfd_object:
      if (!entry->flags.reload)
	ldlang_add_file (entry);
      break;

    case bfd_archive:
      check_excluded_libs (entry->the_bfd);

      bfd_set_usrdata (entry->the_bfd, entry);
      if (entry->flags.whole_archive)
	{
	  bfd *member = NULL;
	  bfd_boolean loaded = TRUE;

	  for (;;)
	    {
	      bfd *subsbfd;
	      member = bfd_openr_next_archived_file (entry->the_bfd, member);

	      if (member == NULL)
		break;

	      if (!bfd_check_format (member, bfd_object))
		{
		  einfo (_("%F%P: %pB: member %pB in archive is not an object\n"),
			 entry->the_bfd, member);
		  loaded = FALSE;
		}

	      subsbfd = member;
	      if (!(*link_info.callbacks
		    ->add_archive_element) (&link_info, member,
					    "--whole-archive", &subsbfd))
		abort ();

	      /* Potentially, the add_archive_element hook may have set a
		 substitute BFD for us.  */
	      if (!bfd_link_add_symbols (subsbfd, &link_info))
		{
		  einfo (_("%F%P: %pB: error adding symbols: %E\n"), member);
		  loaded = FALSE;
		}
	    }

	  entry->flags.loaded = loaded;
	  return loaded;
	}
      break;
    }

  if (bfd_link_add_symbols (entry->the_bfd, &link_info))
    entry->flags.loaded = TRUE;
  else
    einfo (_("%F%P: %pB: error adding symbols: %E\n"), entry->the_bfd);

  return entry->flags.loaded;
}

/* Handle a wild statement.  S->FILENAME or S->SECTION_LIST or both
   may be NULL, indicating that it is a wildcard.  Separate
   lang_input_section statements are created for each part of the
   expansion; they are added after the wild statement S.  OUTPUT is
   the output section.  */

static void
wild (lang_wild_statement_type *s,
      const char *target ATTRIBUTE_UNUSED,
      lang_output_section_statement_type *output)
{
  struct wildcard_list *sec;

  if (s->handler_data[0]
      && s->handler_data[0]->spec.sorted == by_name
      && !s->filenames_sorted)
    {
      lang_section_bst_type *tree;

      walk_wild (s, output_section_callback_fast, output);

      tree = s->tree;
      if (tree)
	{
	  output_section_callback_tree_to_list (s, tree, output);
	  s->tree = NULL;
	}
    }
  else
    walk_wild (s, output_section_callback, output);

  if (default_common_section == NULL)
    for (sec = s->section_list; sec != NULL; sec = sec->next)
      if (sec->spec.name != NULL && strcmp (sec->spec.name, "COMMON") == 0)
	{
	  /* Remember the section that common is going to in case we
	     later get something which doesn't know where to put it.  */
	  default_common_section = output;
	  break;
	}
}

/* Return TRUE iff target is the sought target.  */

static int
get_target (const bfd_target *target, void *data)
{
  const char *sought = (const char *) data;

  return strcmp (target->name, sought) == 0;
}

/* Like strcpy() but convert to lower case as well.  */

static void
stricpy (char *dest, const char *src)
{
  char c;

  while ((c = *src++) != 0)
    *dest++ = TOLOWER (c);

  *dest = 0;
}

/* Remove the first occurrence of needle (if any) in haystack
   from haystack.  */

static void
strcut (char *haystack, const char *needle)
{
  haystack = strstr (haystack, needle);

  if (haystack)
    {
      char *src;

      for (src = haystack + strlen (needle); *src;)
	*haystack++ = *src++;

      *haystack = 0;
    }
}

/* Compare two target format name strings.
   Return a value indicating how "similar" they are.  */

static int
name_compare (const char *first, const char *second)
{
  char *copy1;
  char *copy2;
  int result;

  copy1 = (char *) xmalloc (strlen (first) + 1);
  copy2 = (char *) xmalloc (strlen (second) + 1);

  /* Convert the names to lower case.  */
  stricpy (copy1, first);
  stricpy (copy2, second);

  /* Remove size and endian strings from the name.  */
  strcut (copy1, "big");
  strcut (copy1, "little");
  strcut (copy2, "big");
  strcut (copy2, "little");

  /* Return a value based on how many characters match,
     starting from the beginning.   If both strings are
     the same then return 10 * their length.  */
  for (result = 0; copy1[result] == copy2[result]; result++)
    if (copy1[result] == 0)
      {
	result *= 10;
	break;
      }

  free (copy1);
  free (copy2);

  return result;
}

/* Set by closest_target_match() below.  */
static const bfd_target *winner;

/* Scan all the valid bfd targets looking for one that has the endianness
   requirement that was specified on the command line, and is the nearest
   match to the original output target.  */

static int
closest_target_match (const bfd_target *target, void *data)
{
  const bfd_target *original = (const bfd_target *) data;

  if (command_line.endian == ENDIAN_BIG
      && target->byteorder != BFD_ENDIAN_BIG)
    return 0;

  if (command_line.endian == ENDIAN_LITTLE
      && target->byteorder != BFD_ENDIAN_LITTLE)
    return 0;

  /* Must be the same flavour.  */
  if (target->flavour != original->flavour)
    return 0;

  /* Ignore generic big and little endian elf vectors.  */
  if (strcmp (target->name, "elf32-big") == 0
      || strcmp (target->name, "elf64-big") == 0
      || strcmp (target->name, "elf32-little") == 0
      || strcmp (target->name, "elf64-little") == 0)
    return 0;

  /* If we have not found a potential winner yet, then record this one.  */
  if (winner == NULL)
    {
      winner = target;
      return 0;
    }

  /* Oh dear, we now have two potential candidates for a successful match.
     Compare their names and choose the better one.  */
  if (name_compare (target->name, original->name)
      > name_compare (winner->name, original->name))
    winner = target;

  /* Keep on searching until wqe have checked them all.  */
  return 0;
}

/* Return the BFD target format of the first input file.  */

static const char *
get_first_input_target (void)
{
  const char *target = NULL;

  LANG_FOR_EACH_INPUT_STATEMENT (s)
    {
      if (s->header.type == lang_input_statement_enum
	  && s->flags.real)
	{
	  ldfile_open_file (s);

	  if (s->the_bfd != NULL
	      && bfd_check_format (s->the_bfd, bfd_object))
	    {
	      target = bfd_get_target (s->the_bfd);

	      if (target != NULL)
		break;
	    }
	}
    }

  return target;
}

const char *
lang_get_output_target (void)
{
  const char *target;

  /* Has the user told us which output format to use?  */
  if (output_target != NULL)
    return output_target;

  /* No - has the current target been set to something other than
     the default?  */
  if (current_target != default_target && current_target != NULL)
    return current_target;

  /* No - can we determine the format of the first input file?  */
  target = get_first_input_target ();
  if (target != NULL)
    return target;

  /* Failed - use the default output target.  */
  return default_target;
}

/* Open the output file.  */

static void
open_output (const char *name)
{
  output_target = lang_get_output_target ();

  /* Has the user requested a particular endianness on the command
     line?  */
  if (command_line.endian != ENDIAN_UNSET)
    {
      /* Get the chosen target.  */
      const bfd_target *target
	= bfd_iterate_over_targets (get_target, (void *) output_target);

      /* If the target is not supported, we cannot do anything.  */
      if (target != NULL)
	{
	  enum bfd_endian desired_endian;

	  if (command_line.endian == ENDIAN_BIG)
	    desired_endian = BFD_ENDIAN_BIG;
	  else
	    desired_endian = BFD_ENDIAN_LITTLE;

	  /* See if the target has the wrong endianness.  This should
	     not happen if the linker script has provided big and
	     little endian alternatives, but some scrips don't do
	     this.  */
	  if (target->byteorder != desired_endian)
	    {
	      /* If it does, then see if the target provides
		 an alternative with the correct endianness.  */
	      if (target->alternative_target != NULL
		  && (target->alternative_target->byteorder == desired_endian))
		output_target = target->alternative_target->name;
	      else
		{
		  /* Try to find a target as similar as possible to
		     the default target, but which has the desired
		     endian characteristic.  */
		  bfd_iterate_over_targets (closest_target_match,
					    (void *) target);

		  /* Oh dear - we could not find any targets that
		     satisfy our requirements.  */
		  if (winner == NULL)
		    einfo (_("%P: warning: could not find any targets"
			     " that match endianness requirement\n"));
		  else
		    output_target = winner->name;
		}
	    }
	}
    }

  link_info.output_bfd = bfd_openw (name, output_target);

  if (link_info.output_bfd == NULL)
    {
      if (bfd_get_error () == bfd_error_invalid_target)
	einfo (_("%F%P: target %s not found\n"), output_target);

      einfo (_("%F%P: cannot open output file %s: %E\n"), name);
    }

  delete_output_file_on_failure = TRUE;

  if (!bfd_set_format (link_info.output_bfd, bfd_object))
    einfo (_("%F%P: %s: can not make object file: %E\n"), name);
  if (!bfd_set_arch_mach (link_info.output_bfd,
			   ldfile_output_architecture,
			   ldfile_output_machine))
    einfo (_("%F%P: %s: can not set architecture: %E\n"), name);

  link_info.hash = bfd_link_hash_table_create (link_info.output_bfd);
  if (link_info.hash == NULL)
    einfo (_("%F%P: can not create hash table: %E\n"));

  bfd_set_gp_size (link_info.output_bfd, g_switch_value);
}

static void
ldlang_open_output (lang_statement_union_type *statement)
{
  switch (statement->header.type)
    {
    case lang_output_statement_enum:
      ASSERT (link_info.output_bfd == NULL);
      open_output (statement->output_statement.name);
      ldemul_set_output_arch ();
      if (config.magic_demand_paged
	  && !bfd_link_relocatable (&link_info))
	link_info.output_bfd->flags |= D_PAGED;
      else
	link_info.output_bfd->flags &= ~D_PAGED;
      if (config.text_read_only)
	link_info.output_bfd->flags |= WP_TEXT;
      else
	link_info.output_bfd->flags &= ~WP_TEXT;
      if (link_info.traditional_format)
	link_info.output_bfd->flags |= BFD_TRADITIONAL_FORMAT;
      else
	link_info.output_bfd->flags &= ~BFD_TRADITIONAL_FORMAT;
      break;

    case lang_target_statement_enum:
      current_target = statement->target_statement.target;
      break;
    default:
      break;
    }
}

static void
init_opb (asection *s)
{
  unsigned int x;

  opb_shift = 0;
  if (bfd_get_flavour (link_info.output_bfd) == bfd_target_elf_flavour
      && s != NULL
      && (s->flags & SEC_ELF_OCTETS) != 0)
    return;

  x = bfd_arch_mach_octets_per_byte (ldfile_output_architecture,
				     ldfile_output_machine);
  if (x > 1)
    while ((x & 1) == 0)
      {
	x >>= 1;
	++opb_shift;
      }
  ASSERT (x == 1);
}

/* Open all the input files.  */

enum open_bfd_mode
  {
    OPEN_BFD_NORMAL = 0,
    OPEN_BFD_FORCE = 1,
    OPEN_BFD_RESCAN = 2
  };
#ifdef ENABLE_PLUGINS
static lang_input_statement_type *plugin_insert = NULL;
static struct bfd_link_hash_entry *plugin_undefs = NULL;
#endif

static void
open_input_bfds (lang_statement_union_type *s, enum open_bfd_mode mode)
{
  for (; s != NULL; s = s->header.next)
    {
      switch (s->header.type)
	{
	case lang_constructors_statement_enum:
	  open_input_bfds (constructor_list.head, mode);
	  break;
	case lang_output_section_statement_enum:
	  open_input_bfds (s->output_section_statement.children.head, mode);
	  break;
	case lang_wild_statement_enum:
	  /* Maybe we should load the file's symbols.  */
	  if ((mode & OPEN_BFD_RESCAN) == 0
	      && s->wild_statement.filename
	      && !wildcardp (s->wild_statement.filename)
	      && !archive_path (s->wild_statement.filename))
	    lookup_name (s->wild_statement.filename);
	  open_input_bfds (s->wild_statement.children.head, mode);
	  break;
	case lang_group_statement_enum:
	  {
	    struct bfd_link_hash_entry *undefs;
#ifdef ENABLE_PLUGINS
	    lang_input_statement_type *plugin_insert_save;
#endif

	    /* We must continually search the entries in the group
	       until no new symbols are added to the list of undefined
	       symbols.  */

	    do
	      {
#ifdef ENABLE_PLUGINS
		plugin_insert_save = plugin_insert;
#endif
		undefs = link_info.hash->undefs_tail;
		open_input_bfds (s->group_statement.children.head,
				 mode | OPEN_BFD_FORCE);
	      }
	    while (undefs != link_info.hash->undefs_tail
#ifdef ENABLE_PLUGINS
		   /* Objects inserted by a plugin, which are loaded
		      before we hit this loop, may have added new
		      undefs.  */
		   || (plugin_insert != plugin_insert_save && plugin_undefs)
#endif
		   );
	  }
	  break;
	case lang_target_statement_enum:
	  current_target = s->target_statement.target;
	  break;
	case lang_input_statement_enum:
	  if (s->input_statement.flags.real)
	    {
	      lang_statement_union_type **os_tail;
	      lang_statement_list_type add;
	      bfd *abfd;

	      s->input_statement.target = current_target;

	      /* If we are being called from within a group, and this
		 is an archive which has already been searched, then
		 force it to be researched unless the whole archive
		 has been loaded already.  Do the same for a rescan.
		 Likewise reload --as-needed shared libs.  */
	      if (mode != OPEN_BFD_NORMAL
#ifdef ENABLE_PLUGINS
		  && ((mode & OPEN_BFD_RESCAN) == 0
		      || plugin_insert == NULL)
#endif
		  && s->input_statement.flags.loaded
		  && (abfd = s->input_statement.the_bfd) != NULL
		  && ((bfd_get_format (abfd) == bfd_archive
		       && !s->input_statement.flags.whole_archive)
		      || (bfd_get_format (abfd) == bfd_object
			  && ((abfd->flags) & DYNAMIC) != 0
			  && s->input_statement.flags.add_DT_NEEDED_for_regular
			  && bfd_get_flavour (abfd) == bfd_target_elf_flavour
			  && (elf_dyn_lib_class (abfd) & DYN_AS_NEEDED) != 0)))
		{
		  s->input_statement.flags.loaded = FALSE;
		  s->input_statement.flags.reload = TRUE;
		}

	      os_tail = lang_os_list.tail;
	      lang_list_init (&add);

	      if (!load_symbols (&s->input_statement, &add))
		config.make_executable = FALSE;

	      if (add.head != NULL)
		{
		  /* If this was a script with output sections then
		     tack any added statements on to the end of the
		     list.  This avoids having to reorder the output
		     section statement list.  Very likely the user
		     forgot -T, and whatever we do here will not meet
		     naive user expectations.  */
		  if (os_tail != lang_os_list.tail)
		    {
		      einfo (_("%P: warning: %s contains output sections;"
			       " did you forget -T?\n"),
			     s->input_statement.filename);
		      *stat_ptr->tail = add.head;
		      stat_ptr->tail = add.tail;
		    }
		  else
		    {
		      *add.tail = s->header.next;
		      s->header.next = add.head;
		    }
		}
	    }
#ifdef ENABLE_PLUGINS
	  /* If we have found the point at which a plugin added new
	     files, clear plugin_insert to enable archive rescan.  */
	  if (&s->input_statement == plugin_insert)
	    plugin_insert = NULL;
#endif
	  break;
	case lang_assignment_statement_enum:
	  if (s->assignment_statement.exp->type.node_class != etree_assert)
	    exp_fold_tree_no_dot (s->assignment_statement.exp);
	  break;
	default:
	  break;
	}
    }

  /* Exit if any of the files were missing.  */
  if (input_flags.missing_file)
    einfo ("%F");
}

/* Open the CTF sections in the input files with libctf: if any were opened,
   create a fake input file that we'll write the merged CTF data to later
   on.  */

static void
ldlang_open_ctf (void)
{
  int any_ctf = 0;
  int err;

  LANG_FOR_EACH_INPUT_STATEMENT (file)
    {
      asection *sect;

      /* Incoming files from the compiler have a single ctf_file_t in them
	 (which is presented to us by the libctf API in a ctf_archive_t
	 wrapper): files derived from a previous relocatable link have a CTF
	 archive containing possibly many CTF files.  */

      if ((file->the_ctf = ctf_bfdopen (file->the_bfd, &err)) == NULL)
	{
	  if (err != ECTF_NOCTFDATA)
	    einfo (_("%P: warning: CTF section in `%pI' not loaded: "
		     "its types will be discarded: `%s'\n"), file,
		     ctf_errmsg (err));
	  continue;
	}

      /* Prevent the contents of this section from being written, while
	 requiring the section itself to be duplicated in the output.  */
      /* This section must exist if ctf_bfdopen() succeeded.  */
      sect = bfd_get_section_by_name (file->the_bfd, ".ctf");
      sect->size = 0;
      sect->flags |= SEC_NEVER_LOAD | SEC_HAS_CONTENTS | SEC_LINKER_CREATED;

      any_ctf = 1;
    }

  if (!any_ctf)
    {
      ctf_output = NULL;
      return;
    }

  if ((ctf_output = ctf_create (&err)) != NULL)
    return;

  einfo (_("%P: warning: CTF output not created: `s'\n"),
	 ctf_errmsg (err));

  LANG_FOR_EACH_INPUT_STATEMENT (errfile)
    ctf_close (errfile->the_ctf);
}

/* Merge together CTF sections.  After this, only the symtab-dependent
   function and data object sections need adjustment.  */

static void
lang_merge_ctf (void)
{
  asection *output_sect;

  if (!ctf_output)
    return;

  output_sect = bfd_get_section_by_name (link_info.output_bfd, ".ctf");

  /* If the section was discarded, don't waste time merging.  */
  if (output_sect == NULL)
    {
      ctf_file_close (ctf_output);
      ctf_output = NULL;

      LANG_FOR_EACH_INPUT_STATEMENT (file)
	{
	  ctf_close (file->the_ctf);
	  file->the_ctf = NULL;
	}
      return;
    }

  LANG_FOR_EACH_INPUT_STATEMENT (file)
    {
      if (!file->the_ctf)
	continue;

      /* Takes ownership of file->u.the_ctfa.  */
      if (ctf_link_add_ctf (ctf_output, file->the_ctf, file->filename) < 0)
	{
	  einfo (_("%F%P: cannot link with CTF in %pB: %s\n"), file->the_bfd,
		 ctf_errmsg (ctf_errno (ctf_output)));
	  ctf_close (file->the_ctf);
	  file->the_ctf = NULL;
	  continue;
	}
    }

  if (ctf_link (ctf_output, CTF_LINK_SHARE_UNCONFLICTED) < 0)
    {
      einfo (_("%F%P: CTF linking failed; output will have no CTF section: %s\n"),
	     ctf_errmsg (ctf_errno (ctf_output)));
      if (output_sect)
	{
	  output_sect->size = 0;
	  output_sect->flags |= SEC_EXCLUDE;
	}
    }
}

/* Let the emulation examine the symbol table and strtab to help it optimize the
   CTF, if supported.  */

void
ldlang_ctf_apply_strsym (struct elf_sym_strtab *syms, bfd_size_type symcount,
			 struct elf_strtab_hash *symstrtab)
{
  ldemul_examine_strtab_for_ctf (ctf_output, syms, symcount, symstrtab);
}

/* Write out the CTF section.  Called early, if the emulation isn't going to
   need to dedup against the strtab and symtab, then possibly called from the
   target linker code if the dedup has happened.  */
static void
lang_write_ctf (int late)
{
  size_t output_size;
  asection *output_sect;

  if (!ctf_output)
    return;

  if (late)
    {
      /* Emit CTF late if this emulation says it can do so.  */
      if (ldemul_emit_ctf_early ())
	return;
    }
  else
    {
      if (!ldemul_emit_ctf_early ())
	return;
    }

  /* Emit CTF.  */

  output_sect = bfd_get_section_by_name (link_info.output_bfd, ".ctf");
  if (output_sect)
    {
      output_sect->contents = ctf_link_write (ctf_output, &output_size,
					      CTF_COMPRESSION_THRESHOLD);
      output_sect->size = output_size;
      output_sect->flags |= SEC_IN_MEMORY | SEC_KEEP;

      if (!output_sect->contents)
	{
	  einfo (_("%F%P: CTF section emission failed; output will have no "
		   "CTF section: %s\n"), ctf_errmsg (ctf_errno (ctf_output)));
	  output_sect->size = 0;
	  output_sect->flags |= SEC_EXCLUDE;
	}
    }

  /* This also closes every CTF input file used in the link.  */
  ctf_file_close (ctf_output);
  ctf_output = NULL;

  LANG_FOR_EACH_INPUT_STATEMENT (file)
    file->the_ctf = NULL;
}

/* Write out the CTF section late, if the emulation needs that.  */

void
ldlang_write_ctf_late (void)
{
  /* Trigger a "late call", if the emulation needs one.  */

  lang_write_ctf (1);
}

/* Add the supplied name to the symbol table as an undefined reference.
   This is a two step process as the symbol table doesn't even exist at
   the time the ld command line is processed.  First we put the name
   on a list, then, once the output file has been opened, transfer the
   name to the symbol table.  */

typedef struct bfd_sym_chain ldlang_undef_chain_list_type;

#define ldlang_undef_chain_list_head entry_symbol.next

void
ldlang_add_undef (const char *const name, bfd_boolean cmdline)
{
  ldlang_undef_chain_list_type *new_undef;

  undef_from_cmdline = undef_from_cmdline || cmdline;
  new_undef = stat_alloc (sizeof (*new_undef));
  new_undef->next = ldlang_undef_chain_list_head;
  ldlang_undef_chain_list_head = new_undef;

  new_undef->name = xstrdup (name);

  if (link_info.output_bfd != NULL)
    insert_undefined (new_undef->name);
}

/* Insert NAME as undefined in the symbol table.  */

static void
insert_undefined (const char *name)
{
  struct bfd_link_hash_entry *h;

  h = bfd_link_hash_lookup (link_info.hash, name, TRUE, FALSE, TRUE);
  if (h == NULL)
    einfo (_("%F%P: bfd_link_hash_lookup failed: %E\n"));
  if (h->type == bfd_link_hash_new)
    {
      h->type = bfd_link_hash_undefined;
      h->u.undef.abfd = NULL;
      h->non_ir_ref_regular = TRUE;
      if (is_elf_hash_table (link_info.hash))
	((struct elf_link_hash_entry *) h)->mark = 1;
      bfd_link_add_undef (link_info.hash, h);
    }
}

/* Run through the list of undefineds created above and place them
   into the linker hash table as undefined symbols belonging to the
   script file.  */

static void
lang_place_undefineds (void)
{
  ldlang_undef_chain_list_type *ptr;

  for (ptr = ldlang_undef_chain_list_head; ptr != NULL; ptr = ptr->next)
    insert_undefined (ptr->name);
}

/* Structure used to build the list of symbols that the user has required
   be defined.  */

struct require_defined_symbol
{
  const char *name;
  struct require_defined_symbol *next;
};

/* The list of symbols that the user has required be defined.  */

static struct require_defined_symbol *require_defined_symbol_list;

/* Add a new symbol NAME to the list of symbols that are required to be
   defined.  */

void
ldlang_add_require_defined (const char *const name)
{
  struct require_defined_symbol *ptr;

  ldlang_add_undef (name, TRUE);
  ptr = stat_alloc (sizeof (*ptr));
  ptr->next = require_defined_symbol_list;
  ptr->name = strdup (name);
  require_defined_symbol_list = ptr;
}

/* Check that all symbols the user required to be defined, are defined,
   raise an error if we find a symbol that is not defined.  */

static void
ldlang_check_require_defined_symbols (void)
{
  struct require_defined_symbol *ptr;

  for (ptr = require_defined_symbol_list; ptr != NULL; ptr = ptr->next)
    {
      struct bfd_link_hash_entry *h;

      h = bfd_link_hash_lookup (link_info.hash, ptr->name,
				FALSE, FALSE, TRUE);
      if (h == NULL
	  || (h->type != bfd_link_hash_defined
	      && h->type != bfd_link_hash_defweak))
	einfo(_("%X%P: required symbol `%s' not defined\n"), ptr->name);
    }
}

/* Check for all readonly or some readwrite sections.  */

static void
check_input_sections
  (lang_statement_union_type *s,
   lang_output_section_statement_type *output_section_statement)
{
  for (; s != NULL; s = s->header.next)
    {
      switch (s->header.type)
	{
	case lang_wild_statement_enum:
	  walk_wild (&s->wild_statement, check_section_callback,
		     output_section_statement);
	  if (!output_section_statement->all_input_readonly)
	    return;
	  break;
	case lang_constructors_statement_enum:
	  check_input_sections (constructor_list.head,
				output_section_statement);
	  if (!output_section_statement->all_input_readonly)
	    return;
	  break;
	case lang_group_statement_enum:
	  check_input_sections (s->group_statement.children.head,
				output_section_statement);
	  if (!output_section_statement->all_input_readonly)
	    return;
	  break;
	default:
	  break;
	}
    }
}

/* Update wildcard statements if needed.  */

static void
update_wild_statements (lang_statement_union_type *s)
{
  struct wildcard_list *sec;

  switch (sort_section)
    {
    default:
      FAIL ();

    case none:
      break;

    case by_name:
    case by_alignment:
      for (; s != NULL; s = s->header.next)
	{
	  switch (s->header.type)
	    {
	    default:
	      break;

	    case lang_wild_statement_enum:
	      for (sec = s->wild_statement.section_list; sec != NULL;
		   sec = sec->next)
		/* Don't sort .init/.fini sections.  */
		if (strcmp (sec->spec.name, ".init") != 0
		    && strcmp (sec->spec.name, ".fini") != 0)
		  switch (sec->spec.sorted)
		    {
		    case none:
		      sec->spec.sorted = sort_section;
		      break;
		    case by_name:
		      if (sort_section == by_alignment)
			sec->spec.sorted = by_name_alignment;
		      break;
		    case by_alignment:
		      if (sort_section == by_name)
			sec->spec.sorted = by_alignment_name;
		      break;
		    default:
		      break;
		    }
	      break;

	    case lang_constructors_statement_enum:
	      update_wild_statements (constructor_list.head);
	      break;

	    case lang_output_section_statement_enum:
	      update_wild_statements
		(s->output_section_statement.children.head);
	      break;

	    case lang_group_statement_enum:
	      update_wild_statements (s->group_statement.children.head);
	      break;
	    }
	}
      break;
    }
}

/* Open input files and attach to output sections.  */

static void
map_input_to_output_sections
  (lang_statement_union_type *s, const char *target,
   lang_output_section_statement_type *os)
{
  for (; s != NULL; s = s->header.next)
    {
      lang_output_section_statement_type *tos;
      flagword flags;

      switch (s->header.type)
	{
	case lang_wild_statement_enum:
	  wild (&s->wild_statement, target, os);
	  break;
	case lang_constructors_statement_enum:
	  map_input_to_output_sections (constructor_list.head,
					target,
					os);
	  break;
	case lang_output_section_statement_enum:
	  tos = &s->output_section_statement;
	  if (tos->constraint != 0)
	    {
	      if (tos->constraint != ONLY_IF_RW
		  && tos->constraint != ONLY_IF_RO)
		break;
	      tos->all_input_readonly = TRUE;
	      check_input_sections (tos->children.head, tos);
	      if (tos->all_input_readonly != (tos->constraint == ONLY_IF_RO))
		{
		  tos->constraint = -1;
		  break;
		}
	    }
	  map_input_to_output_sections (tos->children.head,
					target,
					tos);
	  break;
	case lang_output_statement_enum:
	  break;
	case lang_target_statement_enum:
	  target = s->target_statement.target;
	  break;
	case lang_group_statement_enum:
	  map_input_to_output_sections (s->group_statement.children.head,
					target,
					os);
	  break;
	case lang_data_statement_enum:
	  /* Make sure that any sections mentioned in the expression
	     are initialized.  */
	  exp_init_os (s->data_statement.exp);
	  /* The output section gets CONTENTS, ALLOC and LOAD, but
	     these may be overridden by the script.  */
	  flags = SEC_HAS_CONTENTS | SEC_ALLOC | SEC_LOAD;
	  switch (os->sectype)
	    {
	    case normal_section:
	    case overlay_section:
	    case first_overlay_section:
	      break;
	    case noalloc_section:
	      flags = SEC_HAS_CONTENTS;
	      break;
	    case noload_section:
	      if (bfd_get_flavour (link_info.output_bfd)
		  == bfd_target_elf_flavour)
		flags = SEC_NEVER_LOAD | SEC_ALLOC;
	      else
		flags = SEC_NEVER_LOAD | SEC_HAS_CONTENTS;
	      break;
	    }
	  if (os->bfd_section == NULL)
	    init_os (os, flags);
	  else
	    os->bfd_section->flags |= flags;
	  break;
	case lang_input_section_enum:
	  break;
	case lang_fill_statement_enum:
	case lang_object_symbols_statement_enum:
	case lang_reloc_statement_enum:
	case lang_padding_statement_enum:
	case lang_input_statement_enum:
	  if (os != NULL && os->bfd_section == NULL)
	    init_os (os, 0);
	  break;
	case lang_assignment_statement_enum:
	  if (os != NULL && os->bfd_section == NULL)
	    init_os (os, 0);

	  /* Make sure that any sections mentioned in the assignment
	     are initialized.  */
	  exp_init_os (s->assignment_statement.exp);
	  break;
	case lang_address_statement_enum:
	  /* Mark the specified section with the supplied address.
	     If this section was actually a segment marker, then the
	     directive is ignored if the linker script explicitly
	     processed the segment marker.  Originally, the linker
	     treated segment directives (like -Ttext on the
	     command-line) as section directives.  We honor the
	     section directive semantics for backwards compatibility;
	     linker scripts that do not specifically check for
	     SEGMENT_START automatically get the old semantics.  */
	  if (!s->address_statement.segment
	      || !s->address_statement.segment->used)
	    {
	      const char *name = s->address_statement.section_name;

	      /* Create the output section statement here so that
		 orphans with a set address will be placed after other
		 script sections.  If we let the orphan placement code
		 place them in amongst other sections then the address
		 will affect following script sections, which is
		 likely to surprise naive users.  */
	      tos = lang_output_section_statement_lookup (name, 0, TRUE);
	      tos->addr_tree = s->address_statement.address;
	      if (tos->bfd_section == NULL)
		init_os (tos, 0);
	    }
	  break;
	case lang_insert_statement_enum:
	  break;
	}
    }
}

/* An insert statement snips out all the linker statements from the
   start of the list and places them after the output section
   statement specified by the insert.  This operation is complicated
   by the fact that we keep a doubly linked list of output section
   statements as well as the singly linked list of all statements.
   FIXME someday: Twiddling with the list not only moves statements
   from the user's script but also input and group statements that are
   built from command line object files and --start-group.  We only
   get away with this because the list pointers used by file_chain
   and input_file_chain are not reordered, and processing via
   statement_list after this point mostly ignores input statements.
   One exception is the map file, where LOAD and START GROUP/END GROUP
   can end up looking odd.  */

static void
process_insert_statements (lang_statement_union_type **start)
{
  lang_statement_union_type **s;
  lang_output_section_statement_type *first_os = NULL;
  lang_output_section_statement_type *last_os = NULL;
  lang_output_section_statement_type *os;

  s = start;
  while (*s != NULL)
    {
      if ((*s)->header.type == lang_output_section_statement_enum)
	{
	  /* Keep pointers to the first and last output section
	     statement in the sequence we may be about to move.  */
	  os = &(*s)->output_section_statement;

	  ASSERT (last_os == NULL || last_os->next == os);
	  last_os = os;

	  /* Set constraint negative so that lang_output_section_find
	     won't match this output section statement.  At this
	     stage in linking constraint has values in the range
	     [-1, ONLY_IN_RW].  */
	  last_os->constraint = -2 - last_os->constraint;
	  if (first_os == NULL)
	    first_os = last_os;
	}
      else if ((*s)->header.type == lang_group_statement_enum)
	{
	  /* A user might put -T between --start-group and
	     --end-group.  One way this odd construct might arise is
	     from a wrapper around ld to change library search
	     behaviour.  For example:
	     #! /bin/sh
	     exec real_ld --start-group "$@" --end-group
	     This isn't completely unreasonable so go looking inside a
	     group statement for insert statements.  */
	  process_insert_statements (&(*s)->group_statement.children.head);
	}
      else if ((*s)->header.type == lang_insert_statement_enum)
	{
	  lang_insert_statement_type *i = &(*s)->insert_statement;
	  lang_output_section_statement_type *where;
	  lang_statement_union_type **ptr;
	  lang_statement_union_type *first;

	  where = lang_output_section_find (i->where);
	  if (where != NULL && i->is_before)
	    {
	      do
		where = where->prev;
	      while (where != NULL && where->constraint < 0);
	    }
	  if (where == NULL)
	    {
	      einfo (_("%F%P: %s not found for insert\n"), i->where);
	      return;
	    }

	  /* Deal with reordering the output section statement list.  */
	  if (last_os != NULL)
	    {
	      asection *first_sec, *last_sec;
	      struct lang_output_section_statement_struct **next;

	      /* Snip out the output sections we are moving.  */
	      first_os->prev->next = last_os->next;
	      if (last_os->next == NULL)
		{
		  next = &first_os->prev->next;
		  lang_os_list.tail = (lang_statement_union_type **) next;
		}
	      else
		last_os->next->prev = first_os->prev;
	      /* Add them in at the new position.  */
	      last_os->next = where->next;
	      if (where->next == NULL)
		{
		  next = &last_os->next;
		  lang_os_list.tail = (lang_statement_union_type **) next;
		}
	      else
		where->next->prev = last_os;
	      first_os->prev = where;
	      where->next = first_os;

	      /* Move the bfd sections in the same way.  */
	      first_sec = NULL;
	      last_sec = NULL;
	      for (os = first_os; os != NULL; os = os->next)
		{
		  os->constraint = -2 - os->constraint;
		  if (os->bfd_section != NULL
		      && os->bfd_section->owner != NULL)
		    {
		      last_sec = os->bfd_section;
		      if (first_sec == NULL)
			first_sec = last_sec;
		    }
		  if (os == last_os)
		    break;
		}
	      if (last_sec != NULL)
		{
		  asection *sec = where->bfd_section;
		  if (sec == NULL)
		    sec = output_prev_sec_find (where);

		  /* The place we want to insert must come after the
		     sections we are moving.  So if we find no
		     section or if the section is the same as our
		     last section, then no move is needed.  */
		  if (sec != NULL && sec != last_sec)
		    {
		      /* Trim them off.  */
		      if (first_sec->prev != NULL)
			first_sec->prev->next = last_sec->next;
		      else
			link_info.output_bfd->sections = last_sec->next;
		      if (last_sec->next != NULL)
			last_sec->next->prev = first_sec->prev;
		      else
			link_info.output_bfd->section_last = first_sec->prev;
		      /* Add back.  */
		      last_sec->next = sec->next;
		      if (sec->next != NULL)
			sec->next->prev = last_sec;
		      else
			link_info.output_bfd->section_last = last_sec;
		      first_sec->prev = sec;
		      sec->next = first_sec;
		    }
		}

	      first_os = NULL;
	      last_os = NULL;
	    }

	  ptr = insert_os_after (where);
	  /* Snip everything from the start of the list, up to and
	     including the insert statement we are currently processing.  */
	  first = *start;
	  *start = (*s)->header.next;
	  /* Add them back where they belong, minus the insert.  */
	  *s = *ptr;
	  if (*s == NULL)
	    statement_list.tail = s;
	  *ptr = first;
	  s = start;
	  continue;
	}
      s = &(*s)->header.next;
    }

  /* Undo constraint twiddling.  */
  for (os = first_os; os != NULL; os = os->next)
    {
      os->constraint = -2 - os->constraint;
      if (os == last_os)
	break;
    }
}

/* An output section might have been removed after its statement was
   added.  For example, ldemul_before_allocation can remove dynamic
   sections if they turn out to be not needed.  Clean them up here.  */

void
strip_excluded_output_sections (void)
{
  lang_output_section_statement_type *os;

  /* Run lang_size_sections (if not already done).  */
  if (expld.phase != lang_mark_phase_enum)
    {
      expld.phase = lang_mark_phase_enum;
      expld.dataseg.phase = exp_seg_none;
      one_lang_size_sections_pass (NULL, FALSE);
      lang_reset_memory_regions ();
    }

  for (os = (void *) lang_os_list.head;
       os != NULL;
       os = os->next)
    {
      asection *output_section;
      bfd_boolean exclude;

      if (os->constraint < 0)
	continue;

      output_section = os->bfd_section;
      if (output_section == NULL)
	continue;

      exclude = (output_section->rawsize == 0
		 && (output_section->flags & SEC_KEEP) == 0
		 && !bfd_section_removed_from_list (link_info.output_bfd,
						    output_section));

      /* Some sections have not yet been sized, notably .gnu.version,
	 .dynsym, .dynstr and .hash.  These all have SEC_LINKER_CREATED
	 input sections, so don't drop output sections that have such
	 input sections unless they are also marked SEC_EXCLUDE.  */
      if (exclude && output_section->map_head.s != NULL)
	{
	  asection *s;

	  for (s = output_section->map_head.s; s != NULL; s = s->map_head.s)
	    if ((s->flags & SEC_EXCLUDE) == 0
		&& ((s->flags & SEC_LINKER_CREATED) != 0
		    || link_info.emitrelocations))
	      {
		exclude = FALSE;
		break;
	      }
	}

      if (exclude)
	{
	  /* We don't set bfd_section to NULL since bfd_section of the
	     removed output section statement may still be used.  */
	  if (!os->update_dot)
	    os->ignored = TRUE;
	  output_section->flags |= SEC_EXCLUDE;
	  bfd_section_list_remove (link_info.output_bfd, output_section);
	  link_info.output_bfd->section_count--;
	}
    }
}

/* Called from ldwrite to clear out asection.map_head and
   asection.map_tail for use as link_orders in ldwrite.  */

void
lang_clear_os_map (void)
{
  lang_output_section_statement_type *os;

  if (map_head_is_link_order)
    return;

  for (os = (void *) lang_os_list.head;
       os != NULL;
       os = os->next)
    {
      asection *output_section;

      if (os->constraint < 0)
	continue;

      output_section = os->bfd_section;
      if (output_section == NULL)
	continue;

      /* TODO: Don't just junk map_head.s, turn them into link_orders.  */
      output_section->map_head.link_order = NULL;
      output_section->map_tail.link_order = NULL;
    }

  /* Stop future calls to lang_add_section from messing with map_head
     and map_tail link_order fields.  */
  map_head_is_link_order = TRUE;
}

static void
print_output_section_statement
  (lang_output_section_statement_type *output_section_statement)
{
  asection *section = output_section_statement->bfd_section;
  int len;

  if (output_section_statement != abs_output_section)
    {
      minfo ("\n%s", output_section_statement->name);

      if (section != NULL)
	{
	  print_dot = section->vma;

	  len = strlen (output_section_statement->name);
	  if (len >= SECTION_NAME_MAP_LENGTH - 1)
	    {
	      print_nl ();
	      len = 0;
	    }
	  while (len < SECTION_NAME_MAP_LENGTH)
	    {
	      print_space ();
	      ++len;
	    }

	  minfo ("0x%V %W", section->vma, TO_ADDR (section->size));

	  if (section->vma != section->lma)
	    minfo (_(" load address 0x%V"), section->lma);

	  if (output_section_statement->update_dot_tree != NULL)
	    exp_fold_tree (output_section_statement->update_dot_tree,
			   bfd_abs_section_ptr, &print_dot);
	}

      print_nl ();
    }

  print_statement_list (output_section_statement->children.head,
			output_section_statement);
}

static void
print_assignment (lang_assignment_statement_type *assignment,
		  lang_output_section_statement_type *output_section)
{
  unsigned int i;
  bfd_boolean is_dot;
  etree_type *tree;
  asection *osec;

  for (i = 0; i < SECTION_NAME_MAP_LENGTH; i++)
    print_space ();

  if (assignment->exp->type.node_class == etree_assert)
    {
      is_dot = FALSE;
      tree = assignment->exp->assert_s.child;
    }
  else
    {
      const char *dst = assignment->exp->assign.dst;

      is_dot = (dst[0] == '.' && dst[1] == 0);
      tree = assignment->exp;
    }

  osec = output_section->bfd_section;
  if (osec == NULL)
    osec = bfd_abs_section_ptr;

  if (assignment->exp->type.node_class != etree_provide)
    exp_fold_tree (tree, osec, &print_dot);
  else
    expld.result.valid_p = FALSE;

  if (expld.result.valid_p)
    {
      bfd_vma value;

      if (assignment->exp->type.node_class == etree_assert
	  || is_dot
	  || expld.assign_name != NULL)
	{
	  value = expld.result.value;

	  if (expld.result.section != NULL)
	    value += expld.result.section->vma;

	  minfo ("0x%V", value);
	  if (is_dot)
	    print_dot = value;
	}
      else
	{
	  struct bfd_link_hash_entry *h;

	  h = bfd_link_hash_lookup (link_info.hash, assignment->exp->assign.dst,
				    FALSE, FALSE, TRUE);
	  if (h != NULL
	      && (h->type == bfd_link_hash_defined
		  || h->type == bfd_link_hash_defweak))
	    {
	      value = h->u.def.value;
	      value += h->u.def.section->output_section->vma;
	      value += h->u.def.section->output_offset;

	      minfo ("[0x%V]", value);
	    }
	  else
	    minfo ("[unresolved]");
	}
    }
  else
    {
      if (assignment->exp->type.node_class == etree_provide)
	minfo ("[!provide]");
      else
	minfo ("*undef*   ");
#ifdef BFD64
      minfo ("        ");
#endif
    }
  expld.assign_name = NULL;

  minfo ("                ");
  exp_print_tree (assignment->exp);
  print_nl ();
}

static void
print_input_statement (lang_input_statement_type *statm)
{
  if (statm->filename != NULL)
    fprintf (config.map_file, "LOAD %s\n", statm->filename);
}

/* Print all symbols defined in a particular section.  This is called
   via bfd_link_hash_traverse, or by print_all_symbols.  */

static bfd_boolean
print_one_symbol (struct bfd_link_hash_entry *hash_entry, void *ptr)
{
  asection *sec = (asection *) ptr;

  if ((hash_entry->type == bfd_link_hash_defined
       || hash_entry->type == bfd_link_hash_defweak)
      && sec == hash_entry->u.def.section)
    {
      int i;

      for (i = 0; i < SECTION_NAME_MAP_LENGTH; i++)
	print_space ();
      minfo ("0x%V   ",
	     (hash_entry->u.def.value
	      + hash_entry->u.def.section->output_offset
	      + hash_entry->u.def.section->output_section->vma));

      minfo ("             %pT\n", hash_entry->root.string);
    }

  return TRUE;
}

static int
hash_entry_addr_cmp (const void *a, const void *b)
{
  const struct bfd_link_hash_entry *l = *(const struct bfd_link_hash_entry **)a;
  const struct bfd_link_hash_entry *r = *(const struct bfd_link_hash_entry **)b;

  if (l->u.def.value < r->u.def.value)
    return -1;
  else if (l->u.def.value > r->u.def.value)
    return 1;
  else
    return 0;
}

static void
print_all_symbols (asection *sec)
{
  input_section_userdata_type *ud = bfd_section_userdata (sec);
  struct map_symbol_def *def;
  struct bfd_link_hash_entry **entries;
  unsigned int i;

  if (!ud)
    return;

  *ud->map_symbol_def_tail = 0;

  /* Sort the symbols by address.  */
  entries = (struct bfd_link_hash_entry **)
      obstack_alloc (&map_obstack,
		     ud->map_symbol_def_count * sizeof (*entries));

  for (i = 0, def = ud->map_symbol_def_head; def; def = def->next, i++)
    entries[i] = def->entry;

  qsort (entries, ud->map_symbol_def_count, sizeof (*entries),
	 hash_entry_addr_cmp);

  /* Print the symbols.  */
  for (i = 0; i < ud->map_symbol_def_count; i++)
    print_one_symbol (entries[i], sec);

  obstack_free (&map_obstack, entries);
}

/* Print information about an input section to the map file.  */

static void
print_input_section (asection *i, bfd_boolean is_discarded)
{
  bfd_size_type size = i->size;
  int len;
  bfd_vma addr;

  init_opb (i);

  print_space ();
  minfo ("%s", i->name);

  len = 1 + strlen (i->name);
  if (len >= SECTION_NAME_MAP_LENGTH - 1)
    {
      print_nl ();
      len = 0;
    }
  while (len < SECTION_NAME_MAP_LENGTH)
    {
      print_space ();
      ++len;
    }

  if (i->output_section != NULL
      && i->output_section->owner == link_info.output_bfd)
    addr = i->output_section->vma + i->output_offset;
  else
    {
      addr = print_dot;
      if (!is_discarded)
	size = 0;
    }

  minfo ("0x%V %W %pB\n", addr, TO_ADDR (size), i->owner);

  if (size != i->rawsize && i->rawsize != 0)
    {
      len = SECTION_NAME_MAP_LENGTH + 3;
#ifdef BFD64
      len += 16;
#else
      len += 8;
#endif
      while (len > 0)
	{
	  print_space ();
	  --len;
	}

      minfo (_("%W (size before relaxing)\n"), TO_ADDR (i->rawsize));
    }

  if (i->output_section != NULL
      && i->output_section->owner == link_info.output_bfd)
    {
      if (link_info.reduce_memory_overheads)
	bfd_link_hash_traverse (link_info.hash, print_one_symbol, i);
      else
	print_all_symbols (i);

      /* Update print_dot, but make sure that we do not move it
	 backwards - this could happen if we have overlays and a
	 later overlay is shorter than an earier one.  */
      if (addr + TO_ADDR (size) > print_dot)
	print_dot = addr + TO_ADDR (size);
    }
}

static void
print_fill_statement (lang_fill_statement_type *fill)
{
  size_t size;
  unsigned char *p;
  fputs (" FILL mask 0x", config.map_file);
  for (p = fill->fill->data, size = fill->fill->size; size != 0; p++, size--)
    fprintf (config.map_file, "%02x", *p);
  fputs ("\n", config.map_file);
}

static void
print_data_statement (lang_data_statement_type *data)
{
  int i;
  bfd_vma addr;
  bfd_size_type size;
  const char *name;

  init_opb (data->output_section);
  for (i = 0; i < SECTION_NAME_MAP_LENGTH; i++)
    print_space ();

  addr = data->output_offset;
  if (data->output_section != NULL)
    addr += data->output_section->vma;

  switch (data->type)
    {
    default:
      abort ();
    case BYTE:
      size = BYTE_SIZE;
      name = "BYTE";
      break;
    case SHORT:
      size = SHORT_SIZE;
      name = "SHORT";
      break;
    case LONG:
      size = LONG_SIZE;
      name = "LONG";
      break;
    case QUAD:
      size = QUAD_SIZE;
      name = "QUAD";
      break;
    case SQUAD:
      size = QUAD_SIZE;
      name = "SQUAD";
      break;
    }

  if (size < TO_SIZE ((unsigned) 1))
    size = TO_SIZE ((unsigned) 1);
  minfo ("0x%V %W %s 0x%v", addr, TO_ADDR (size), name, data->value);

  if (data->exp->type.node_class != etree_value)
    {
      print_space ();
      exp_print_tree (data->exp);
    }

  print_nl ();

  print_dot = addr + TO_ADDR (size);
}

/* Print an address statement.  These are generated by options like
   -Ttext.  */

static void
print_address_statement (lang_address_statement_type *address)
{
  minfo (_("Address of section %s set to "), address->section_name);
  exp_print_tree (address->address);
  print_nl ();
}

/* Print a reloc statement.  */

static void
print_reloc_statement (lang_reloc_statement_type *reloc)
{
  int i;
  bfd_vma addr;
  bfd_size_type size;

  init_opb (reloc->output_section);
  for (i = 0; i < SECTION_NAME_MAP_LENGTH; i++)
    print_space ();

  addr = reloc->output_offset;
  if (reloc->output_section != NULL)
    addr += reloc->output_section->vma;

  size = bfd_get_reloc_size (reloc->howto);

  minfo ("0x%V %W RELOC %s ", addr, TO_ADDR (size), reloc->howto->name);

  if (reloc->name != NULL)
    minfo ("%s+", reloc->name);
  else
    minfo ("%s+", reloc->section->name);

  exp_print_tree (reloc->addend_exp);

  print_nl ();

  print_dot = addr + TO_ADDR (size);
}

static void
print_padding_statement (lang_padding_statement_type *s)
{
  int len;
  bfd_vma addr;

  init_opb (s->output_section);
  minfo (" *fill*");

  len = sizeof " *fill*" - 1;
  while (len < SECTION_NAME_MAP_LENGTH)
    {
      print_space ();
      ++len;
    }

  addr = s->output_offset;
  if (s->output_section != NULL)
    addr += s->output_section->vma;
  minfo ("0x%V %W ", addr, TO_ADDR (s->size));

  if (s->fill->size != 0)
    {
      size_t size;
      unsigned char *p;
      for (p = s->fill->data, size = s->fill->size; size != 0; p++, size--)
	fprintf (config.map_file, "%02x", *p);
    }

  print_nl ();

  print_dot = addr + TO_ADDR (s->size);
}

static void
print_wild_statement (lang_wild_statement_type *w,
		      lang_output_section_statement_type *os)
{
  struct wildcard_list *sec;

  print_space ();

  if (w->exclude_name_list)
    {
      name_list *tmp;
      minfo ("EXCLUDE_FILE(%s", w->exclude_name_list->name);
      for (tmp = w->exclude_name_list->next; tmp; tmp = tmp->next)
	minfo (" %s", tmp->name);
      minfo (") ");
    }

  if (w->filenames_sorted)
    minfo ("SORT_BY_NAME(");
  if (w->filename != NULL)
    minfo ("%s", w->filename);
  else
    minfo ("*");
  if (w->filenames_sorted)
    minfo (")");

  minfo ("(");
  for (sec = w->section_list; sec; sec = sec->next)
    {
      int closing_paren = 0;

      switch (sec->spec.sorted)
	{
	case none:
	  break;

	case by_name:
	  minfo ("SORT_BY_NAME(");
	  closing_paren = 1;
	  break;

	case by_alignment:
	  minfo ("SORT_BY_ALIGNMENT(");
	  closing_paren = 1;
	  break;

	case by_name_alignment:
	  minfo ("SORT_BY_NAME(SORT_BY_ALIGNMENT(");
	  closing_paren = 2;
	  break;

	case by_alignment_name:
	  minfo ("SORT_BY_ALIGNMENT(SORT_BY_NAME(");
	  closing_paren = 2;
	  break;

	case by_none:
	  minfo ("SORT_NONE(");
	  closing_paren = 1;
	  break;

	case by_init_priority:
	  minfo ("SORT_BY_INIT_PRIORITY(");
	  closing_paren = 1;
	  break;
	}

      if (sec->spec.exclude_name_list != NULL)
	{
	  name_list *tmp;
	  minfo ("EXCLUDE_FILE(%s", sec->spec.exclude_name_list->name);
	  for (tmp = sec->spec.exclude_name_list->next; tmp; tmp = tmp->next)
	    minfo (" %s", tmp->name);
	  minfo (") ");
	}
      if (sec->spec.name != NULL)
	minfo ("%s", sec->spec.name);
      else
	minfo ("*");
      for (;closing_paren > 0; closing_paren--)
	minfo (")");
      if (sec->next)
	minfo (" ");
    }
  minfo (")");

  print_nl ();

  print_statement_list (w->children.head, os);
}

/* Print a group statement.  */

static void
print_group (lang_group_statement_type *s,
	     lang_output_section_statement_type *os)
{
  fprintf (config.map_file, "START GROUP\n");
  print_statement_list (s->children.head, os);
  fprintf (config.map_file, "END GROUP\n");
}

/* Print the list of statements in S.
   This can be called for any statement type.  */

static void
print_statement_list (lang_statement_union_type *s,
		      lang_output_section_statement_type *os)
{
  while (s != NULL)
    {
      print_statement (s, os);
      s = s->header.next;
    }
}

/* Print the first statement in statement list S.
   This can be called for any statement type.  */

static void
print_statement (lang_statement_union_type *s,
		 lang_output_section_statement_type *os)
{
  switch (s->header.type)
    {
    default:
      fprintf (config.map_file, _("Fail with %d\n"), s->header.type);
      FAIL ();
      break;
    case lang_constructors_statement_enum:
      if (constructor_list.head != NULL)
	{
	  if (constructors_sorted)
	    minfo (" SORT (CONSTRUCTORS)\n");
	  else
	    minfo (" CONSTRUCTORS\n");
	  print_statement_list (constructor_list.head, os);
	}
      break;
    case lang_wild_statement_enum:
      print_wild_statement (&s->wild_statement, os);
      break;
    case lang_address_statement_enum:
      print_address_statement (&s->address_statement);
      break;
    case lang_object_symbols_statement_enum:
      minfo (" CREATE_OBJECT_SYMBOLS\n");
      break;
    case lang_fill_statement_enum:
      print_fill_statement (&s->fill_statement);
      break;
    case lang_data_statement_enum:
      print_data_statement (&s->data_statement);
      break;
    case lang_reloc_statement_enum:
      print_reloc_statement (&s->reloc_statement);
      break;
    case lang_input_section_enum:
      print_input_section (s->input_section.section, FALSE);
      break;
    case lang_padding_statement_enum:
      print_padding_statement (&s->padding_statement);
      break;
    case lang_output_section_statement_enum:
      print_output_section_statement (&s->output_section_statement);
      break;
    case lang_assignment_statement_enum:
      print_assignment (&s->assignment_statement, os);
      break;
    case lang_target_statement_enum:
      fprintf (config.map_file, "TARGET(%s)\n", s->target_statement.target);
      break;
    case lang_output_statement_enum:
      minfo ("OUTPUT(%s", s->output_statement.name);
      if (output_target != NULL)
	minfo (" %s", output_target);
      minfo (")\n");
      break;
    case lang_input_statement_enum:
      print_input_statement (&s->input_statement);
      break;
    case lang_group_statement_enum:
      print_group (&s->group_statement, os);
      break;
    case lang_insert_statement_enum:
      minfo ("INSERT %s %s\n",
	     s->insert_statement.is_before ? "BEFORE" : "AFTER",
	     s->insert_statement.where);
      break;
    }
}

static void
print_statements (void)
{
  print_statement_list (statement_list.head, abs_output_section);
}

/* Print the first N statements in statement list S to STDERR.
   If N == 0, nothing is printed.
   If N < 0, the entire list is printed.
   Intended to be called from GDB.  */

void
dprint_statement (lang_statement_union_type *s, int n)
{
  FILE *map_save = config.map_file;

  config.map_file = stderr;

  if (n < 0)
    print_statement_list (s, abs_output_section);
  else
    {
      while (s && --n >= 0)
	{
	  print_statement (s, abs_output_section);
	  s = s->header.next;
	}
    }

  config.map_file = map_save;
}

static void
insert_pad (lang_statement_union_type **ptr,
	    fill_type *fill,
	    bfd_size_type alignment_needed,
	    asection *output_section,
	    bfd_vma dot)
{
  static fill_type zero_fill;
  lang_statement_union_type *pad = NULL;

  if (ptr != &statement_list.head)
    pad = ((lang_statement_union_type *)
	   ((char *) ptr - offsetof (lang_statement_union_type, header.next)));
  if (pad != NULL
      && pad->header.type == lang_padding_statement_enum
      && pad->padding_statement.output_section == output_section)
    {
      /* Use the existing pad statement.  */
    }
  else if ((pad = *ptr) != NULL
	   && pad->header.type == lang_padding_statement_enum
	   && pad->padding_statement.output_section == output_section)
    {
      /* Use the existing pad statement.  */
    }
  else
    {
      /* Make a new padding statement, linked into existing chain.  */
      pad = stat_alloc (sizeof (lang_padding_statement_type));
      pad->header.next = *ptr;
      *ptr = pad;
      pad->header.type = lang_padding_statement_enum;
      pad->padding_statement.output_section = output_section;
      if (fill == NULL)
	fill = &zero_fill;
      pad->padding_statement.fill = fill;
    }
  pad->padding_statement.output_offset = dot - output_section->vma;
  pad->padding_statement.size = alignment_needed;
  if (!(output_section->flags & SEC_FIXED_SIZE))
    output_section->size = TO_SIZE (dot + TO_ADDR (alignment_needed)
				    - output_section->vma);
}

/* Work out how much this section will move the dot point.  */

static bfd_vma
size_input_section
  (lang_statement_union_type **this_ptr,
   lang_output_section_statement_type *output_section_statement,
   fill_type *fill,
   bfd_vma dot)
{
  lang_input_section_type *is = &((*this_ptr)->input_section);
  asection *i = is->section;
  asection *o = output_section_statement->bfd_section;

  if (i->sec_info_type == SEC_INFO_TYPE_JUST_SYMS)
    i->output_offset = i->vma - o->vma;
  else if (((i->flags & SEC_EXCLUDE) != 0)
	   || output_section_statement->ignored)
    i->output_offset = dot - o->vma;
  else
    {
      bfd_size_type alignment_needed;

      /* Align this section first to the input sections requirement,
	 then to the output section's requirement.  If this alignment
	 is greater than any seen before, then record it too.  Perform
	 the alignment by inserting a magic 'padding' statement.  */

      if (output_section_statement->subsection_alignment != NULL)
	i->alignment_power
	  = exp_get_power (output_section_statement->subsection_alignment,
			   "subsection alignment");

      if (o->alignment_power < i->alignment_power)
	o->alignment_power = i->alignment_power;

      alignment_needed = align_power (dot, i->alignment_power) - dot;

      if (alignment_needed != 0)
	{
	  insert_pad (this_ptr, fill, TO_SIZE (alignment_needed), o, dot);
	  dot += alignment_needed;
	}

      /* Remember where in the output section this input section goes.  */
      i->output_offset = dot - o->vma;

      /* Mark how big the output section must be to contain this now.  */
      dot += TO_ADDR (i->size);
      if (!(o->flags & SEC_FIXED_SIZE))
	o->size = TO_SIZE (dot - o->vma);
    }

  return dot;
}

struct check_sec
{
  asection *sec;
  bfd_boolean warned;
};

static int
sort_sections_by_lma (const void *arg1, const void *arg2)
{
  const asection *sec1 = ((const struct check_sec *) arg1)->sec;
  const asection *sec2 = ((const struct check_sec *) arg2)->sec;

  if (sec1->lma < sec2->lma)
    return -1;
  else if (sec1->lma > sec2->lma)
    return 1;
  else if (sec1->id < sec2->id)
    return -1;
  else if (sec1->id > sec2->id)
    return 1;

  return 0;
}

static int
sort_sections_by_vma (const void *arg1, const void *arg2)
{
  const asection *sec1 = ((const struct check_sec *) arg1)->sec;
  const asection *sec2 = ((const struct check_sec *) arg2)->sec;

  if (sec1->vma < sec2->vma)
    return -1;
  else if (sec1->vma > sec2->vma)
    return 1;
  else if (sec1->id < sec2->id)
    return -1;
  else if (sec1->id > sec2->id)
    return 1;

  return 0;
}

#define IS_TBSS(s) \
  ((s->flags & (SEC_LOAD | SEC_THREAD_LOCAL)) == SEC_THREAD_LOCAL)

#define IGNORE_SECTION(s) \
  ((s->flags & SEC_ALLOC) == 0 || IS_TBSS (s))

/* Check to see if any allocated sections overlap with other allocated
   sections.  This can happen if a linker script specifies the output
   section addresses of the two sections.  Also check whether any memory
   region has overflowed.  */

static void
lang_check_section_addresses (void)
{
  asection *s, *p;
  struct check_sec *sections;
  size_t i, count;
  bfd_vma addr_mask;
  bfd_vma s_start;
  bfd_vma s_end;
  bfd_vma p_start = 0;
  bfd_vma p_end = 0;
  lang_memory_region_type *m;
  bfd_boolean overlays;

  /* Detect address space overflow on allocated sections.  */
  addr_mask = ((bfd_vma) 1 <<
	       (bfd_arch_bits_per_address (link_info.output_bfd) - 1)) - 1;
  addr_mask = (addr_mask << 1) + 1;
  for (s = link_info.output_bfd->sections; s != NULL; s = s->next)
    if ((s->flags & SEC_ALLOC) != 0)
      {
	s_end = (s->vma + s->size) & addr_mask;
	if (s_end != 0 && s_end < (s->vma & addr_mask))
	  einfo (_("%X%P: section %s VMA wraps around address space\n"),
		 s->name);
	else
	  {
	    s_end = (s->lma + s->size) & addr_mask;
	    if (s_end != 0 && s_end < (s->lma & addr_mask))
	      einfo (_("%X%P: section %s LMA wraps around address space\n"),
		     s->name);
	  }
      }

  if (bfd_count_sections (link_info.output_bfd) <= 1)
    return;

  count = bfd_count_sections (link_info.output_bfd);
  sections = XNEWVEC (struct check_sec, count);

  /* Scan all sections in the output list.  */
  count = 0;
  for (s = link_info.output_bfd->sections; s != NULL; s = s->next)
    {
      if (IGNORE_SECTION (s)
	  || s->size == 0)
	continue;

      sections[count].sec = s;
      sections[count].warned = FALSE;
      count++;
    }

  if (count <= 1)
    {
      free (sections);
      return;
    }

  qsort (sections, count, sizeof (*sections), sort_sections_by_lma);

  /* First check section LMAs.  There should be no overlap of LMAs on
     loadable sections, even with overlays.  */
  for (p = NULL, i = 0; i < count; i++)
    {
      s = sections[i].sec;
      init_opb (s);
      if ((s->flags & SEC_LOAD) != 0)
	{
	  s_start = s->lma;
	  s_end = s_start + TO_ADDR (s->size) - 1;

	  /* Look for an overlap.  We have sorted sections by lma, so
	     we know that s_start >= p_start.  Besides the obvious
	     case of overlap when the current section starts before
	     the previous one ends, we also must have overlap if the
	     previous section wraps around the address space.  */
	  if (p != NULL
	      && (s_start <= p_end
		  || p_end < p_start))
	    {
	      einfo (_("%X%P: section %s LMA [%V,%V]"
		       " overlaps section %s LMA [%V,%V]\n"),
		     s->name, s_start, s_end, p->name, p_start, p_end);
	      sections[i].warned = TRUE;
	    }
	  p = s;
	  p_start = s_start;
	  p_end = s_end;
	}
    }

  /* If any non-zero size allocated section (excluding tbss) starts at
     exactly the same VMA as another such section, then we have
     overlays.  Overlays generated by the OVERLAY keyword will have
     this property.  It is possible to intentionally generate overlays
     that fail this test, but it would be unusual.  */
  qsort (sections, count, sizeof (*sections), sort_sections_by_vma);
  overlays = FALSE;
  p_start = sections[0].sec->vma;
  for (i = 1; i < count; i++)
    {
      s_start = sections[i].sec->vma;
      if (p_start == s_start)
	{
	  overlays = TRUE;
	  break;
	}
      p_start = s_start;
    }

  /* Now check section VMAs if no overlays were detected.  */
  if (!overlays)
    {
      for (p = NULL, i = 0; i < count; i++)
	{
	  s = sections[i].sec;
	  init_opb (s);
	  s_start = s->vma;
	  s_end = s_start + TO_ADDR (s->size) - 1;

	  if (p != NULL
	      && !sections[i].warned
	      && (s_start <= p_end
		  || p_end < p_start))
	    einfo (_("%X%P: section %s VMA [%V,%V]"
		     " overlaps section %s VMA [%V,%V]\n"),
		   s->name, s_start, s_end, p->name, p_start, p_end);
	  p = s;
	  p_start = s_start;
	  p_end = s_end;
	}
    }

  free (sections);

  /* If any memory region has overflowed, report by how much.
     We do not issue this diagnostic for regions that had sections
     explicitly placed outside their bounds; os_region_check's
     diagnostics are adequate for that case.

     FIXME: It is conceivable that m->current - (m->origin + m->length)
     might overflow a 32-bit integer.  There is, alas, no way to print
     a bfd_vma quantity in decimal.  */
  for (m = lang_memory_region_list; m; m = m->next)
    if (m->had_full_message)
      {
	unsigned long over = m->current - (m->origin + m->length);
	einfo (ngettext ("%X%P: region `%s' overflowed by %lu byte\n",
			 "%X%P: region `%s' overflowed by %lu bytes\n",
			 over),
	       m->name_list.name, over);
      }
}

/* Make sure the new address is within the region.  We explicitly permit the
   current address to be at the exact end of the region when the address is
   non-zero, in case the region is at the end of addressable memory and the
   calculation wraps around.  */

static void
os_region_check (lang_output_section_statement_type *os,
		 lang_memory_region_type *region,
		 etree_type *tree,
		 bfd_vma rbase)
{
  if ((region->current < region->origin
       || (region->current - region->origin > region->length))
      && ((region->current != region->origin + region->length)
	  || rbase == 0))
    {
      if (tree != NULL)
	{
	  einfo (_("%X%P: address 0x%v of %pB section `%s'"
		   " is not within region `%s'\n"),
		 region->current,
		 os->bfd_section->owner,
		 os->bfd_section->name,
		 region->name_list.name);
	}
      else if (!region->had_full_message)
	{
	  region->had_full_message = TRUE;

	  einfo (_("%X%P: %pB section `%s' will not fit in region `%s'\n"),
		 os->bfd_section->owner,
		 os->bfd_section->name,
		 region->name_list.name);
	}
    }
}

static void
ldlang_check_relro_region (lang_statement_union_type *s,
			   seg_align_type *seg)
{
  if (seg->relro == exp_seg_relro_start)
    {
      if (!seg->relro_start_stat)
	seg->relro_start_stat = s;
      else
	{
	  ASSERT (seg->relro_start_stat == s);
	}
    }
  else if (seg->relro == exp_seg_relro_end)
    {
      if (!seg->relro_end_stat)
	seg->relro_end_stat = s;
      else
	{
	  ASSERT (seg->relro_end_stat == s);
	}
    }
}

/* Set the sizes for all the output sections.  */

static bfd_vma
lang_size_sections_1
  (lang_statement_union_type **prev,
   lang_output_section_statement_type *output_section_statement,
   fill_type *fill,
   bfd_vma dot,
   bfd_boolean *relax,
   bfd_boolean check_regions)
{
  lang_statement_union_type *s;

  /* Size up the sections from their constituent parts.  */
  for (s = *prev; s != NULL; s = s->header.next)
    {
      switch (s->header.type)
	{
	case lang_output_section_statement_enum:
	  {
	    bfd_vma newdot, after, dotdelta;
	    lang_output_section_statement_type *os;
	    lang_memory_region_type *r;
	    int section_alignment = 0;

	    os = &s->output_section_statement;
	    init_opb (os->bfd_section);
	    if (os->constraint == -1)
	      break;

	    /* FIXME: We shouldn't need to zero section vmas for ld -r
	       here, in lang_insert_orphan, or in the default linker scripts.
	       This is covering for coff backend linker bugs.  See PR6945.  */
	    if (os->addr_tree == NULL
		&& bfd_link_relocatable (&link_info)
		&& (bfd_get_flavour (link_info.output_bfd)
		    == bfd_target_coff_flavour))
	      os->addr_tree = exp_intop (0);
	    if (os->addr_tree != NULL)
	      {
		os->processed_vma = FALSE;
		exp_fold_tree (os->addr_tree, bfd_abs_section_ptr, &dot);

		if (expld.result.valid_p)
		  {
		    dot = expld.result.value;
		    if (expld.result.section != NULL)
		      dot += expld.result.section->vma;
		  }
		else if (expld.phase != lang_mark_phase_enum)
		  einfo (_("%F%P:%pS: non constant or forward reference"
			   " address expression for section %s\n"),
			 os->addr_tree, os->name);
	      }

	    if (os->bfd_section == NULL)
	      /* This section was removed or never actually created.  */
	      break;

	    /* If this is a COFF shared library section, use the size and
	       address from the input section.  FIXME: This is COFF
	       specific; it would be cleaner if there were some other way
	       to do this, but nothing simple comes to mind.  */
	    if (((bfd_get_flavour (link_info.output_bfd)
		  == bfd_target_ecoff_flavour)
		 || (bfd_get_flavour (link_info.output_bfd)
		     == bfd_target_coff_flavour))
		&& (os->bfd_section->flags & SEC_COFF_SHARED_LIBRARY) != 0)
	      {
		asection *input;

		if (os->children.head == NULL
		    || os->children.head->header.next != NULL
		    || (os->children.head->header.type
			!= lang_input_section_enum))
		  einfo (_("%X%P: internal error on COFF shared library"
			   " section %s\n"), os->name);

		input = os->children.head->input_section.section;
		bfd_set_section_vma (os->bfd_section,
				     bfd_section_vma (input));
		if (!(os->bfd_section->flags & SEC_FIXED_SIZE))
		  os->bfd_section->size = input->size;
		break;
	      }

	    newdot = dot;
	    dotdelta = 0;
	    if (bfd_is_abs_section (os->bfd_section))
	      {
		/* No matter what happens, an abs section starts at zero.  */
		ASSERT (os->bfd_section->vma == 0);
	      }
	    else
	      {
		if (os->addr_tree == NULL)
		  {
		    /* No address specified for this section, get one
		       from the region specification.  */
		    if (os->region == NULL
			|| ((os->bfd_section->flags & (SEC_ALLOC | SEC_LOAD))
			    && os->region->name_list.name[0] == '*'
			    && strcmp (os->region->name_list.name,
				       DEFAULT_MEMORY_REGION) == 0))
		      {
			os->region = lang_memory_default (os->bfd_section);
		      }

		    /* If a loadable section is using the default memory
		       region, and some non default memory regions were
		       defined, issue an error message.  */
		    if (!os->ignored
			&& !IGNORE_SECTION (os->bfd_section)
			&& !bfd_link_relocatable (&link_info)
			&& check_regions
			&& strcmp (os->region->name_list.name,
				   DEFAULT_MEMORY_REGION) == 0
			&& lang_memory_region_list != NULL
			&& (strcmp (lang_memory_region_list->name_list.name,
				    DEFAULT_MEMORY_REGION) != 0
			    || lang_memory_region_list->next != NULL)
			&& expld.phase != lang_mark_phase_enum)
		      {
			/* By default this is an error rather than just a
			   warning because if we allocate the section to the
			   default memory region we can end up creating an
			   excessively large binary, or even seg faulting when
			   attempting to perform a negative seek.  See
			   sources.redhat.com/ml/binutils/2003-04/msg00423.html
			   for an example of this.  This behaviour can be
			   overridden by the using the --no-check-sections
			   switch.  */
			if (command_line.check_section_addresses)
			  einfo (_("%F%P: error: no memory region specified"
				   " for loadable section `%s'\n"),
				 bfd_section_name (os->bfd_section));
			else
			  einfo (_("%P: warning: no memory region specified"
				   " for loadable section `%s'\n"),
				 bfd_section_name (os->bfd_section));
		      }

		    newdot = os->region->current;
		    section_alignment = os->bfd_section->alignment_power;
		  }
		else
		  section_alignment = exp_get_power (os->section_alignment,
						     "section alignment");

		/* Align to what the section needs.  */
		if (section_alignment > 0)
		  {
		    bfd_vma savedot = newdot;
		    newdot = align_power (newdot, section_alignment);

		    dotdelta = newdot - savedot;
		    if (dotdelta != 0
			&& (config.warn_section_align
			    || os->addr_tree != NULL)
			&& expld.phase != lang_mark_phase_enum)
		      einfo (ngettext ("%P: warning: changing start of "
				       "section %s by %lu byte\n",
				       "%P: warning: changing start of "
				       "section %s by %lu bytes\n",
				       (unsigned long) dotdelta),
			     os->name, (unsigned long) dotdelta);
		  }

		bfd_set_section_vma (os->bfd_section, newdot);

		os->bfd_section->output_offset = 0;
	      }

	    lang_size_sections_1 (&os->children.head, os,
				  os->fill, newdot, relax, check_regions);

	    os->processed_vma = TRUE;

	    if (bfd_is_abs_section (os->bfd_section) || os->ignored)
	      /* Except for some special linker created sections,
		 no output section should change from zero size
		 after strip_excluded_output_sections.  A non-zero
		 size on an ignored section indicates that some
		 input section was not sized early enough.  */
	      ASSERT (os->bfd_section->size == 0);
	    else
	      {
		dot = os->bfd_section->vma;

		/* Put the section within the requested block size, or
		   align at the block boundary.  */
		after = ((dot
			  + TO_ADDR (os->bfd_section->size)
			  + os->block_value - 1)
			 & - (bfd_vma) os->block_value);

		if (!(os->bfd_section->flags & SEC_FIXED_SIZE))
		  os->bfd_section->size = TO_SIZE (after
						   - os->bfd_section->vma);
	      }

	    /* Set section lma.  */
	    r = os->region;
	    if (r == NULL)
	      r = lang_memory_region_lookup (DEFAULT_MEMORY_REGION, FALSE);

	    if (os->load_base)
	      {
		bfd_vma lma = exp_get_abs_int (os->load_base, 0, "load base");
		os->bfd_section->lma = lma;
	      }
	    else if (os->lma_region != NULL)
	      {
		bfd_vma lma = os->lma_region->current;

		if (os->align_lma_with_input)
		  lma += dotdelta;
		else
		  {
		    /* When LMA_REGION is the same as REGION, align the LMA
		       as we did for the VMA, possibly including alignment
		       from the bfd section.  If a different region, then
		       only align according to the value in the output
		       statement.  */
		    if (os->lma_region != os->region)
		      section_alignment = exp_get_power (os->section_alignment,
							 "section alignment");
		    if (section_alignment > 0)
		      lma = align_power (lma, section_alignment);
		  }
		os->bfd_section->lma = lma;
	      }
	    else if (r->last_os != NULL
		     && (os->bfd_section->flags & SEC_ALLOC) != 0)
	      {
		bfd_vma lma;
		asection *last;

		last = r->last_os->output_section_statement.bfd_section;

		/* A backwards move of dot should be accompanied by
		   an explicit assignment to the section LMA (ie.
		   os->load_base set) because backwards moves can
		   create overlapping LMAs.  */
		if (dot < last->vma
		    && os->bfd_section->size != 0
		    && dot + TO_ADDR (os->bfd_section->size) <= last->vma)
		  {
		    /* If dot moved backwards then leave lma equal to
		       vma.  This is the old default lma, which might
		       just happen to work when the backwards move is
		       sufficiently large.  Nag if this changes anything,
		       so people can fix their linker scripts.  */

		    if (last->vma != last->lma)
		      einfo (_("%P: warning: dot moved backwards "
			       "before `%s'\n"), os->name);
		  }
		else
		  {
		    /* If this is an overlay, set the current lma to that
		       at the end of the previous section.  */
		    if (os->sectype == overlay_section)
		      lma = last->lma + TO_ADDR (last->size);

		    /* Otherwise, keep the same lma to vma relationship
		       as the previous section.  */
		    else
		      lma = os->bfd_section->vma + last->lma - last->vma;

		    if (section_alignment > 0)
		      lma = align_power (lma, section_alignment);
		    os->bfd_section->lma = lma;
		  }
	      }
	    os->processed_lma = TRUE;

	    /* Keep track of normal sections using the default
	       lma region.  We use this to set the lma for
	       following sections.  Overlays or other linker
	       script assignment to lma might mean that the
	       default lma == vma is incorrect.
	       To avoid warnings about dot moving backwards when using
	       -Ttext, don't start tracking sections until we find one
	       of non-zero size or with lma set differently to vma.
	       Do this tracking before we short-cut the loop so that we
	       track changes for the case where the section size is zero,
	       but the lma is set differently to the vma.  This is
	       important, if an orphan section is placed after an
	       otherwise empty output section that has an explicit lma
	       set, we want that lma reflected in the orphans lma.  */
	    if (((!IGNORE_SECTION (os->bfd_section)
		  && (os->bfd_section->size != 0
		      || (r->last_os == NULL
			  && os->bfd_section->vma != os->bfd_section->lma)
		      || (r->last_os != NULL
			  && dot >= (r->last_os->output_section_statement
				     .bfd_section->vma))))
		 || os->sectype == first_overlay_section)
		&& os->lma_region == NULL
		&& !bfd_link_relocatable (&link_info))
	      r->last_os = s;

	    if (bfd_is_abs_section (os->bfd_section) || os->ignored)
	      break;

	    /* .tbss sections effectively have zero size.  */
	    if (!IS_TBSS (os->bfd_section)
		|| bfd_link_relocatable (&link_info))
	      dotdelta = TO_ADDR (os->bfd_section->size);
	    else
	      dotdelta = 0;
	    dot += dotdelta;

	    if (os->update_dot_tree != 0)
	      exp_fold_tree (os->update_dot_tree, bfd_abs_section_ptr, &dot);

	    /* Update dot in the region ?
	       We only do this if the section is going to be allocated,
	       since unallocated sections do not contribute to the region's
	       overall size in memory.  */
	    if (os->region != NULL
		&& (os->bfd_section->flags & (SEC_ALLOC | SEC_LOAD)))
	      {
		os->region->current = dot;

		if (check_regions)
		  /* Make sure the new address is within the region.  */
		  os_region_check (os, os->region, os->addr_tree,
				   os->bfd_section->vma);

		if (os->lma_region != NULL && os->lma_region != os->region
		    && ((os->bfd_section->flags & SEC_LOAD)
			|| os->align_lma_with_input))
		  {
		    os->lma_region->current = os->bfd_section->lma + dotdelta;

		    if (check_regions)
		      os_region_check (os, os->lma_region, NULL,
				       os->bfd_section->lma);
		  }
	      }
	  }
	  break;

	case lang_constructors_statement_enum:
	  dot = lang_size_sections_1 (&constructor_list.head,
				      output_section_statement,
				      fill, dot, relax, check_regions);
	  break;

	case lang_data_statement_enum:
	  {
	    unsigned int size = 0;

	    s->data_statement.output_offset =
	      dot - output_section_statement->bfd_section->vma;
	    s->data_statement.output_section =
	      output_section_statement->bfd_section;

	    /* We might refer to provided symbols in the expression, and
	       need to mark them as needed.  */
	    exp_fold_tree (s->data_statement.exp, bfd_abs_section_ptr, &dot);

	    switch (s->data_statement.type)
	      {
	      default:
		abort ();
	      case QUAD:
	      case SQUAD:
		size = QUAD_SIZE;
		break;
	      case LONG:
		size = LONG_SIZE;
		break;
	      case SHORT:
		size = SHORT_SIZE;
		break;
	      case BYTE:
		size = BYTE_SIZE;
		break;
	      }
	    if (size < TO_SIZE ((unsigned) 1))
	      size = TO_SIZE ((unsigned) 1);
	    dot += TO_ADDR (size);
	    if (!(output_section_statement->bfd_section->flags
		  & SEC_FIXED_SIZE))
	      output_section_statement->bfd_section->size
		= TO_SIZE (dot - output_section_statement->bfd_section->vma);

	  }
	  break;

	case lang_reloc_statement_enum:
	  {
	    int size;

	    s->reloc_statement.output_offset =
	      dot - output_section_statement->bfd_section->vma;
	    s->reloc_statement.output_section =
	      output_section_statement->bfd_section;
	    size = bfd_get_reloc_size (s->reloc_statement.howto);
	    dot += TO_ADDR (size);
	    if (!(output_section_statement->bfd_section->flags
		  & SEC_FIXED_SIZE))
	      output_section_statement->bfd_section->size
		= TO_SIZE (dot - output_section_statement->bfd_section->vma);
	  }
	  break;

	case lang_wild_statement_enum:
	  dot = lang_size_sections_1 (&s->wild_statement.children.head,
				      output_section_statement,
				      fill, dot, relax, check_regions);
	  break;

	case lang_object_symbols_statement_enum:
	  link_info.create_object_symbols_section
	    = output_section_statement->bfd_section;
	  output_section_statement->bfd_section->flags |= SEC_KEEP;
	  break;

	case lang_output_statement_enum:
	case lang_target_statement_enum:
	  break;

	case lang_input_section_enum:
	  {
	    asection *i;

	    i = s->input_section.section;
	    if (relax)
	      {
		bfd_boolean again;

		if (!bfd_relax_section (i->owner, i, &link_info, &again))
		  einfo (_("%F%P: can't relax section: %E\n"));
		if (again)
		  *relax = TRUE;
	      }
	    dot = size_input_section (prev, output_section_statement,
				      fill, dot);
	  }
	  break;

	case lang_input_statement_enum:
	  break;

	case lang_fill_statement_enum:
	  s->fill_statement.output_section =
	    output_section_statement->bfd_section;

	  fill = s->fill_statement.fill;
	  break;

	case lang_assignment_statement_enum:
	  {
	    bfd_vma newdot = dot;
	    etree_type *tree = s->assignment_statement.exp;

	    expld.dataseg.relro = exp_seg_relro_none;

	    exp_fold_tree (tree,
			   output_section_statement->bfd_section,
			   &newdot);

	    ldlang_check_relro_region (s, &expld.dataseg);

	    expld.dataseg.relro = exp_seg_relro_none;

	    /* This symbol may be relative to this section.  */
	    if ((tree->type.node_class == etree_provided
		 || tree->type.node_class == etree_assign)
		&& (tree->assign.dst [0] != '.'
		    || tree->assign.dst [1] != '\0'))
	      output_section_statement->update_dot = 1;

	    if (!output_section_statement->ignored)
	      {
		if (output_section_statement == abs_output_section)
		  {
		    /* If we don't have an output section, then just adjust
		       the default memory address.  */
		    lang_memory_region_lookup (DEFAULT_MEMORY_REGION,
					       FALSE)->current = newdot;
		  }
		else if (newdot != dot)
		  {
		    /* Insert a pad after this statement.  We can't
		       put the pad before when relaxing, in case the
		       assignment references dot.  */
		    insert_pad (&s->header.next, fill, TO_SIZE (newdot - dot),
				output_section_statement->bfd_section, dot);

		    /* Don't neuter the pad below when relaxing.  */
		    s = s->header.next;

		    /* If dot is advanced, this implies that the section
		       should have space allocated to it, unless the
		       user has explicitly stated that the section
		       should not be allocated.  */
		    if (output_section_statement->sectype != noalloc_section
			&& (output_section_statement->sectype != noload_section
			    || (bfd_get_flavour (link_info.output_bfd)
				== bfd_target_elf_flavour)))
		      output_section_statement->bfd_section->flags |= SEC_ALLOC;
		  }
		dot = newdot;
	      }
	  }
	  break;

	case lang_padding_statement_enum:
	  /* If this is the first time lang_size_sections is called,
	     we won't have any padding statements.  If this is the
	     second or later passes when relaxing, we should allow
	     padding to shrink.  If padding is needed on this pass, it
	     will be added back in.  */
	  s->padding_statement.size = 0;

	  /* Make sure output_offset is valid.  If relaxation shrinks
	     the section and this pad isn't needed, it's possible to
	     have output_offset larger than the final size of the
	     section.  bfd_set_section_contents will complain even for
	     a pad size of zero.  */
	  s->padding_statement.output_offset
	    = dot - output_section_statement->bfd_section->vma;
	  break;

	case lang_group_statement_enum:
	  dot = lang_size_sections_1 (&s->group_statement.children.head,
				      output_section_statement,
				      fill, dot, relax, check_regions);
	  break;

	case lang_insert_statement_enum:
	  break;

	  /* We can only get here when relaxing is turned on.  */
	case lang_address_statement_enum:
	  break;

	default:
	  FAIL ();
	  break;
	}
      prev = &s->header.next;
    }
  return dot;
}

/* Callback routine that is used in _bfd_elf_map_sections_to_segments.
   The BFD library has set NEW_SEGMENT to TRUE iff it thinks that
   CURRENT_SECTION and PREVIOUS_SECTION ought to be placed into different
   segments.  We are allowed an opportunity to override this decision.  */

bfd_boolean
ldlang_override_segment_assignment (struct bfd_link_info *info ATTRIBUTE_UNUSED,
				    bfd *abfd ATTRIBUTE_UNUSED,
				    asection *current_section,
				    asection *previous_section,
				    bfd_boolean new_segment)
{
  lang_output_section_statement_type *cur;
  lang_output_section_statement_type *prev;

  /* The checks below are only necessary when the BFD library has decided
     that the two sections ought to be placed into the same segment.  */
  if (new_segment)
    return TRUE;

  /* Paranoia checks.  */
  if (current_section == NULL || previous_section == NULL)
    return new_segment;

  /* If this flag is set, the target never wants code and non-code
     sections comingled in the same segment.  */
  if (config.separate_code
      && ((current_section->flags ^ previous_section->flags) & SEC_CODE))
    return TRUE;

  /* Find the memory regions associated with the two sections.
     We call lang_output_section_find() here rather than scanning the list
     of output sections looking for a matching section pointer because if
     we have a large number of sections then a hash lookup is faster.  */
  cur  = lang_output_section_find (current_section->name);
  prev = lang_output_section_find (previous_section->name);

  /* More paranoia.  */
  if (cur == NULL || prev == NULL)
    return new_segment;

  /* If the regions are different then force the sections to live in
     different segments.  See the email thread starting at the following
     URL for the reasons why this is necessary:
     http://sourceware.org/ml/binutils/2007-02/msg00216.html  */
  return cur->region != prev->region;
}

void
one_lang_size_sections_pass (bfd_boolean *relax, bfd_boolean check_regions)
{
  lang_statement_iteration++;
  lang_size_sections_1 (&statement_list.head, abs_output_section,
			0, 0, relax, check_regions);
}

static bfd_boolean
lang_size_segment (seg_align_type *seg)
{
  /* If XXX_SEGMENT_ALIGN XXX_SEGMENT_END pair was seen, check whether
     a page could be saved in the data segment.  */
  bfd_vma first, last;

  first = -seg->base & (seg->pagesize - 1);
  last = seg->end & (seg->pagesize - 1);
  if (first && last
      && ((seg->base & ~(seg->pagesize - 1))
	  != (seg->end & ~(seg->pagesize - 1)))
      && first + last <= seg->pagesize)
    {
      seg->phase = exp_seg_adjust;
      return TRUE;
    }

  seg->phase = exp_seg_done;
  return FALSE;
}

static bfd_vma
lang_size_relro_segment_1 (seg_align_type *seg)
{
  bfd_vma relro_end, desired_end;
  asection *sec;

  /* Compute the expected PT_GNU_RELRO/PT_LOAD segment end.  */
  relro_end = ((seg->relro_end + seg->pagesize - 1)
	       & ~(seg->pagesize - 1));

  /* Adjust by the offset arg of XXX_SEGMENT_RELRO_END.  */
  desired_end = relro_end - seg->relro_offset;

  /* For sections in the relro segment..  */
  for (sec = link_info.output_bfd->section_last; sec; sec = sec->prev)
    if ((sec->flags & SEC_ALLOC) != 0
	&& sec->vma >= seg->base
	&& sec->vma < seg->relro_end - seg->relro_offset)
      {
	/* Where do we want to put this section so that it ends as
	   desired?  */
	bfd_vma start, end, bump;

	end = start = sec->vma;
	if (!IS_TBSS (sec))
	  end += TO_ADDR (sec->size);
	bump = desired_end - end;
	/* We'd like to increase START by BUMP, but we must heed
	   alignment so the increase might be less than optimum.  */
	start += bump;
	start &= ~(((bfd_vma) 1 << sec->alignment_power) - 1);
	/* This is now the desired end for the previous section.  */
	desired_end = start;
      }

  seg->phase = exp_seg_relro_adjust;
  ASSERT (desired_end >= seg->base);
  seg->base = desired_end;
  return relro_end;
}

static bfd_boolean
lang_size_relro_segment (bfd_boolean *relax, bfd_boolean check_regions)
{
  bfd_boolean do_reset = FALSE;
  bfd_boolean do_data_relro;
  bfd_vma data_initial_base, data_relro_end;

  if (link_info.relro && expld.dataseg.relro_end)
    {
      do_data_relro = TRUE;
      data_initial_base = expld.dataseg.base;
      data_relro_end = lang_size_relro_segment_1 (&expld.dataseg);
    }
  else
    {
      do_data_relro = FALSE;
      data_initial_base = data_relro_end = 0;
    }

  if (do_data_relro)
    {
      lang_reset_memory_regions ();
      one_lang_size_sections_pass (relax, check_regions);

      /* Assignments to dot, or to output section address in a user
	 script have increased padding over the original.  Revert.  */
      if (do_data_relro && expld.dataseg.relro_end > data_relro_end)
	{
	  expld.dataseg.base = data_initial_base;;
	  do_reset = TRUE;
	}
    }

  if (!do_data_relro && lang_size_segment (&expld.dataseg))
    do_reset = TRUE;

  return do_reset;
}

void
lang_size_sections (bfd_boolean *relax, bfd_boolean check_regions)
{
  expld.phase = lang_allocating_phase_enum;
  expld.dataseg.phase = exp_seg_none;

  one_lang_size_sections_pass (relax, check_regions);

  if (expld.dataseg.phase != exp_seg_end_seen)
    expld.dataseg.phase = exp_seg_done;

  if (expld.dataseg.phase == exp_seg_end_seen)
    {
      bfd_boolean do_reset
	= lang_size_relro_segment (relax, check_regions);

      if (do_reset)
	{
	  lang_reset_memory_regions ();
	  one_lang_size_sections_pass (relax, check_regions);
	}

      if (link_info.relro && expld.dataseg.relro_end)
	{
	  link_info.relro_start = expld.dataseg.base;
	  link_info.relro_end = expld.dataseg.relro_end;
	}
    }
}

static lang_output_section_statement_type *current_section;
static lang_assignment_statement_type *current_assign;
static bfd_boolean prefer_next_section;

/* Worker function for lang_do_assignments.  Recursiveness goes here.  */

static bfd_vma
lang_do_assignments_1 (lang_statement_union_type *s,
		       lang_output_section_statement_type *current_os,
		       fill_type *fill,
		       bfd_vma dot,
		       bfd_boolean *found_end)
{
  for (; s != NULL; s = s->header.next)
    {
      switch (s->header.type)
	{
	case lang_constructors_statement_enum:
	  dot = lang_do_assignments_1 (constructor_list.head,
				       current_os, fill, dot, found_end);
	  break;

	case lang_output_section_statement_enum:
	  {
	    lang_output_section_statement_type *os;
	    bfd_vma newdot;

	    os = &(s->output_section_statement);
	    os->after_end = *found_end;
	    init_opb (os->bfd_section);
	    if (os->bfd_section != NULL && !os->ignored)
	      {
		if ((os->bfd_section->flags & SEC_ALLOC) != 0)
		  {
		    current_section = os;
		    prefer_next_section = FALSE;
		  }
		dot = os->bfd_section->vma;
	      }
	    newdot = lang_do_assignments_1 (os->children.head,
					    os, os->fill, dot, found_end);
	    if (!os->ignored)
	      {
		if (os->bfd_section != NULL)
		  {
		    /* .tbss sections effectively have zero size.  */
		    if (!IS_TBSS (os->bfd_section)
			|| bfd_link_relocatable (&link_info))
		      dot += TO_ADDR (os->bfd_section->size);

		    if (os->update_dot_tree != NULL)
		      exp_fold_tree (os->update_dot_tree,
				     bfd_abs_section_ptr, &dot);
		  }
		else
		  dot = newdot;
	      }
	  }
	  break;

	case lang_wild_statement_enum:

	  dot = lang_do_assignments_1 (s->wild_statement.children.head,
				       current_os, fill, dot, found_end);
	  break;

	case lang_object_symbols_statement_enum:
	case lang_output_statement_enum:
	case lang_target_statement_enum:
	  break;

	case lang_data_statement_enum:
	  exp_fold_tree (s->data_statement.exp, bfd_abs_section_ptr, &dot);
	  if (expld.result.valid_p)
	    {
	      s->data_statement.value = expld.result.value;
	      if (expld.result.section != NULL)
		s->data_statement.value += expld.result.section->vma;
	    }
	  else if (expld.phase == lang_final_phase_enum)
	    einfo (_("%F%P: invalid data statement\n"));
	  {
	    unsigned int size;
	    switch (s->data_statement.type)
	      {
	      default:
		abort ();
	      case QUAD:
	      case SQUAD:
		size = QUAD_SIZE;
		break;
	      case LONG:
		size = LONG_SIZE;
		break;
	      case SHORT:
		size = SHORT_SIZE;
		break;
	      case BYTE:
		size = BYTE_SIZE;
		break;
	      }
	    if (size < TO_SIZE ((unsigned) 1))
	      size = TO_SIZE ((unsigned) 1);
	    dot += TO_ADDR (size);
	  }
	  break;

	case lang_reloc_statement_enum:
	  exp_fold_tree (s->reloc_statement.addend_exp,
			 bfd_abs_section_ptr, &dot);
	  if (expld.result.valid_p)
	    s->reloc_statement.addend_value = expld.result.value;
	  else if (expld.phase == lang_final_phase_enum)
	    einfo (_("%F%P: invalid reloc statement\n"));
	  dot += TO_ADDR (bfd_get_reloc_size (s->reloc_statement.howto));
	  break;

	case lang_input_section_enum:
	  {
	    asection *in = s->input_section.section;

	    if ((in->flags & SEC_EXCLUDE) == 0)
	      dot += TO_ADDR (in->size);
	  }
	  break;

	case lang_input_statement_enum:
	  break;

	case lang_fill_statement_enum:
	  fill = s->fill_statement.fill;
	  break;

	case lang_assignment_statement_enum:
	  current_assign = &s->assignment_statement;
	  if (current_assign->exp->type.node_class != etree_assert)
	    {
	      const char *p = current_assign->exp->assign.dst;

	      if (current_os == abs_output_section && p[0] == '.' && p[1] == 0)
		prefer_next_section = TRUE;

	      while (*p == '_')
		++p;
	      if (strcmp (p, "end") == 0)
		*found_end = TRUE;
	    }
	  exp_fold_tree (s->assignment_statement.exp,
			 (current_os->bfd_section != NULL
			  ? current_os->bfd_section : bfd_und_section_ptr),
			 &dot);
	  break;

	case lang_padding_statement_enum:
	  dot += TO_ADDR (s->padding_statement.size);
	  break;

	case lang_group_statement_enum:
	  dot = lang_do_assignments_1 (s->group_statement.children.head,
				       current_os, fill, dot, found_end);
	  break;

	case lang_insert_statement_enum:
	  break;

	case lang_address_statement_enum:
	  break;

	default:
	  FAIL ();
	  break;
	}
    }
  return dot;
}

void
lang_do_assignments (lang_phase_type phase)
{
  bfd_boolean found_end = FALSE;

  current_section = NULL;
  prefer_next_section = FALSE;
  expld.phase = phase;
  lang_statement_iteration++;
  lang_do_assignments_1 (statement_list.head,
			 abs_output_section, NULL, 0, &found_end);
}

/* For an assignment statement outside of an output section statement,
   choose the best of neighbouring output sections to use for values
   of "dot".  */

asection *
section_for_dot (void)
{
  asection *s;

  /* Assignments belong to the previous output section, unless there
     has been an assignment to "dot", in which case following
     assignments belong to the next output section.  (The assumption
     is that an assignment to "dot" is setting up the address for the
     next output section.)  Except that past the assignment to "_end"
     we always associate with the previous section.  This exception is
     for targets like SH that define an alloc .stack or other
     weirdness after non-alloc sections.  */
  if (current_section == NULL || prefer_next_section)
    {
      lang_statement_union_type *stmt;
      lang_output_section_statement_type *os;

      for (stmt = (lang_statement_union_type *) current_assign;
	   stmt != NULL;
	   stmt = stmt->header.next)
	if (stmt->header.type == lang_output_section_statement_enum)
	  break;

      os = &stmt->output_section_statement;
      while (os != NULL
	     && !os->after_end
	     && (os->bfd_section == NULL
		 || (os->bfd_section->flags & SEC_EXCLUDE) != 0
		 || bfd_section_removed_from_list (link_info.output_bfd,
						   os->bfd_section)))
	os = os->next;

      if (current_section == NULL || os == NULL || !os->after_end)
	{
	  if (os != NULL)
	    s = os->bfd_section;
	  else
	    s = link_info.output_bfd->section_last;
	  while (s != NULL
		 && ((s->flags & SEC_ALLOC) == 0
		     || (s->flags & SEC_THREAD_LOCAL) != 0))
	    s = s->prev;
	  if (s != NULL)
	    return s;

	  return bfd_abs_section_ptr;
	}
    }

  s = current_section->bfd_section;

  /* The section may have been stripped.  */
  while (s != NULL
	 && ((s->flags & SEC_EXCLUDE) != 0
	     || (s->flags & SEC_ALLOC) == 0
	     || (s->flags & SEC_THREAD_LOCAL) != 0
	     || bfd_section_removed_from_list (link_info.output_bfd, s)))
    s = s->prev;
  if (s == NULL)
    s = link_info.output_bfd->sections;
  while (s != NULL
	 && ((s->flags & SEC_ALLOC) == 0
	     || (s->flags & SEC_THREAD_LOCAL) != 0))
    s = s->next;
  if (s != NULL)
    return s;

  return bfd_abs_section_ptr;
}

/* Array of __start/__stop/.startof./.sizeof/ symbols.  */

static struct bfd_link_hash_entry **start_stop_syms;
static size_t start_stop_count = 0;
static size_t start_stop_alloc = 0;

/* Give start/stop SYMBOL for SEC a preliminary definition, and add it
   to start_stop_syms.  */

static void
lang_define_start_stop (const char *symbol, asection *sec)
{
  struct bfd_link_hash_entry *h;

  h = bfd_define_start_stop (link_info.output_bfd, &link_info, symbol, sec);
  if (h != NULL)
    {
      if (start_stop_count == start_stop_alloc)
	{
	  start_stop_alloc = 2 * start_stop_alloc + 10;
	  start_stop_syms
	    = xrealloc (start_stop_syms,
			start_stop_alloc * sizeof (*start_stop_syms));
	}
      start_stop_syms[start_stop_count++] = h;
    }
}

/* Check for input sections whose names match references to
   __start_SECNAME or __stop_SECNAME symbols.  Give the symbols
   preliminary definitions.  */

static void
lang_init_start_stop (void)
{
  bfd *abfd;
  asection *s;
  char leading_char = bfd_get_symbol_leading_char (link_info.output_bfd);

  for (abfd = link_info.input_bfds; abfd != NULL; abfd = abfd->link.next)
    for (s = abfd->sections; s != NULL; s = s->next)
      {
	const char *ps;
	const char *secname = s->name;

	for (ps = secname; *ps != '\0'; ps++)
	  if (!ISALNUM ((unsigned char) *ps) && *ps != '_')
	    break;
	if (*ps == '\0')
	  {
	    char *symbol = (char *) xmalloc (10 + strlen (secname));

	    symbol[0] = leading_char;
	    sprintf (symbol + (leading_char != 0), "__start_%s", secname);
	    lang_define_start_stop (symbol, s);

	    symbol[1] = leading_char;
	    memcpy (symbol + 1 + (leading_char != 0), "__stop", 6);
	    lang_define_start_stop (symbol + 1, s);

	    free (symbol);
	  }
      }
}

/* Iterate over start_stop_syms.  */

static void
foreach_start_stop (void (*func) (struct bfd_link_hash_entry *))
{
  size_t i;

  for (i = 0; i < start_stop_count; ++i)
    func (start_stop_syms[i]);
}

/* __start and __stop symbols are only supposed to be defined by the
   linker for orphan sections, but we now extend that to sections that
   map to an output section of the same name.  The symbols were
   defined early for --gc-sections, before we mapped input to output
   sections, so undo those that don't satisfy this rule.  */

static void
undef_start_stop (struct bfd_link_hash_entry *h)
{
  if (h->ldscript_def)
    return;

  if (h->u.def.section->output_section == NULL
      || h->u.def.section->output_section->owner != link_info.output_bfd
      || strcmp (h->u.def.section->name,
		 h->u.def.section->output_section->name) != 0)
    {
      asection *sec = bfd_get_section_by_name (link_info.output_bfd,
					       h->u.def.section->name);
      if (sec != NULL)
	{
	  /* When there are more than one input sections with the same
	     section name, SECNAME, linker picks the first one to define
	     __start_SECNAME and __stop_SECNAME symbols.  When the first
	     input section is removed by comdat group, we need to check
	     if there is still an output section with section name
	     SECNAME.  */
	  asection *i;
	  for (i = sec->map_head.s; i != NULL; i = i->map_head.s)
	    if (strcmp (h->u.def.section->name, i->name) == 0)
	      {
		h->u.def.section = i;
		return;
	      }
	}
      h->type = bfd_link_hash_undefined;
      h->u.undef.abfd = NULL;
    }
}

static void
lang_undef_start_stop (void)
{
  foreach_start_stop (undef_start_stop);
}

/* Check for output sections whose names match references to
   .startof.SECNAME or .sizeof.SECNAME symbols.  Give the symbols
   preliminary definitions.  */

static void
lang_init_startof_sizeof (void)
{
  asection *s;

  for (s = link_info.output_bfd->sections; s != NULL; s = s->next)
    {
      const char *secname = s->name;
      char *symbol = (char *) xmalloc (10 + strlen (secname));

      sprintf (symbol, ".startof.%s", secname);
      lang_define_start_stop (symbol, s);

      memcpy (symbol + 1, ".size", 5);
      lang_define_start_stop (symbol + 1, s);
      free (symbol);
    }
}

/* Set .startof., .sizeof., __start and __stop symbols final values.  */

static void
set_start_stop (struct bfd_link_hash_entry *h)
{
  if (h->ldscript_def
      || h->type != bfd_link_hash_defined)
    return;

  if (h->root.string[0] == '.')
    {
      /* .startof. or .sizeof. symbol.
	 .startof. already has final value.  */
      if (h->root.string[2] == 'i')
	{
	  /* .sizeof.  */
	  h->u.def.value = TO_ADDR (h->u.def.section->size);
	  h->u.def.section = bfd_abs_section_ptr;
	}
    }
  else
    {
      /* __start or __stop symbol.  */
      int has_lead = bfd_get_symbol_leading_char (link_info.output_bfd) != 0;

      h->u.def.section = h->u.def.section->output_section;
      if (h->root.string[4 + has_lead] == 'o')
	{
	  /* __stop_ */
	  h->u.def.value = TO_ADDR (h->u.def.section->size);
	}
    }
}

static void
lang_finalize_start_stop (void)
{
  foreach_start_stop (set_start_stop);
}

static void
lang_end (void)
{
  struct bfd_link_hash_entry *h;
  bfd_boolean warn;

  if ((bfd_link_relocatable (&link_info) && !link_info.gc_sections)
      || bfd_link_dll (&link_info))
    warn = entry_from_cmdline;
  else
    warn = TRUE;

  /* Force the user to specify a root when generating a relocatable with
     --gc-sections, unless --gc-keep-exported was also given.  */
  if (bfd_link_relocatable (&link_info)
      && link_info.gc_sections
      && !link_info.gc_keep_exported
      && !(entry_from_cmdline || undef_from_cmdline))
    einfo (_("%F%P: gc-sections requires either an entry or "
	     "an undefined symbol\n"));

  if (entry_symbol.name == NULL)
    {
      /* No entry has been specified.  Look for the default entry, but
	 don't warn if we don't find it.  */
      entry_symbol.name = entry_symbol_default;
      warn = FALSE;
    }

  h = bfd_link_hash_lookup (link_info.hash, entry_symbol.name,
			    FALSE, FALSE, TRUE);
  if (h != NULL
      && (h->type == bfd_link_hash_defined
	  || h->type == bfd_link_hash_defweak)
      && h->u.def.section->output_section != NULL)
    {
      bfd_vma val;

      val = (h->u.def.value
	     + bfd_section_vma (h->u.def.section->output_section)
	     + h->u.def.section->output_offset);
      if (!bfd_set_start_address (link_info.output_bfd, val))
	einfo (_("%F%P: %s: can't set start address\n"), entry_symbol.name);
    }
  else
    {
      bfd_vma val;
      const char *send;

      /* We couldn't find the entry symbol.  Try parsing it as a
	 number.  */
      val = bfd_scan_vma (entry_symbol.name, &send, 0);
      if (*send == '\0')
	{
	  if (!bfd_set_start_address (link_info.output_bfd, val))
	    einfo (_("%F%P: can't set start address\n"));
	}
      else
	{
	  asection *ts;

	  /* Can't find the entry symbol, and it's not a number.  Use
	     the first address in the text section.  */
	  ts = bfd_get_section_by_name (link_info.output_bfd, entry_section);
	  if (ts != NULL)
	    {
	      if (warn)
		einfo (_("%P: warning: cannot find entry symbol %s;"
			 " defaulting to %V\n"),
		       entry_symbol.name,
		       bfd_section_vma (ts));
	      if (!bfd_set_start_address (link_info.output_bfd,
					  bfd_section_vma (ts)))
		einfo (_("%F%P: can't set start address\n"));
	    }
	  else
	    {
	      if (warn)
		einfo (_("%P: warning: cannot find entry symbol %s;"
			 " not setting start address\n"),
		       entry_symbol.name);
	    }
	}
    }
}

/* This is a small function used when we want to ignore errors from
   BFD.  */

static void
ignore_bfd_errors (const char *fmt ATTRIBUTE_UNUSED,
		   va_list ap ATTRIBUTE_UNUSED)
{
  /* Don't do anything.  */
}

/* Check that the architecture of all the input files is compatible
   with the output file.  Also call the backend to let it do any
   other checking that is needed.  */

static void
lang_check (void)
{
  lang_input_statement_type *file;
  bfd *input_bfd;
  const bfd_arch_info_type *compatible;

  for (file = (void *) file_chain.head;
       file != NULL;
       file = file->next)
    {
#ifdef ENABLE_PLUGINS
      /* Don't check format of files claimed by plugin.  */
      if (file->flags.claimed)
	continue;
#endif /* ENABLE_PLUGINS */
      input_bfd = file->the_bfd;
      compatible
	= bfd_arch_get_compatible (input_bfd, link_info.output_bfd,
				   command_line.accept_unknown_input_arch);

      /* In general it is not possible to perform a relocatable
	 link between differing object formats when the input
	 file has relocations, because the relocations in the
	 input format may not have equivalent representations in
	 the output format (and besides BFD does not translate
	 relocs for other link purposes than a final link).  */
      if ((bfd_link_relocatable (&link_info)
	   || link_info.emitrelocations)
	  && (compatible == NULL
	      || (bfd_get_flavour (input_bfd)
		  != bfd_get_flavour (link_info.output_bfd)))
	  && (bfd_get_file_flags (input_bfd) & HAS_RELOC) != 0)
	{
	  einfo (_("%F%P: relocatable linking with relocations from"
		   " format %s (%pB) to format %s (%pB) is not supported\n"),
		 bfd_get_target (input_bfd), input_bfd,
		 bfd_get_target (link_info.output_bfd), link_info.output_bfd);
	  /* einfo with %F exits.  */
	}

      if (compatible == NULL)
	{
	  if (command_line.warn_mismatch)
	    einfo (_("%X%P: %s architecture of input file `%pB'"
		     " is incompatible with %s output\n"),
		   bfd_printable_name (input_bfd), input_bfd,
		   bfd_printable_name (link_info.output_bfd));
	}
      else if (bfd_count_sections (input_bfd))
	{
	  /* If the input bfd has no contents, it shouldn't set the
	     private data of the output bfd.  */

	  bfd_error_handler_type pfn = NULL;

	  /* If we aren't supposed to warn about mismatched input
	     files, temporarily set the BFD error handler to a
	     function which will do nothing.  We still want to call
	     bfd_merge_private_bfd_data, since it may set up
	     information which is needed in the output file.  */
	  if (!command_line.warn_mismatch)
	    pfn = bfd_set_error_handler (ignore_bfd_errors);
	  if (!bfd_merge_private_bfd_data (input_bfd, &link_info))
	    {
	      if (command_line.warn_mismatch)
		einfo (_("%X%P: failed to merge target specific data"
			 " of file %pB\n"), input_bfd);
	    }
	  if (!command_line.warn_mismatch)
	    bfd_set_error_handler (pfn);
	}
    }
}

/* Look through all the global common symbols and attach them to the
   correct section.  The -sort-common command line switch may be used
   to roughly sort the entries by alignment.  */

static void
lang_common (void)
{
  if (link_info.inhibit_common_definition)
    return;
  if (bfd_link_relocatable (&link_info)
      && !command_line.force_common_definition)
    return;

  if (!config.sort_common)
    bfd_link_hash_traverse (link_info.hash, lang_one_common, NULL);
  else
    {
      unsigned int power;

      if (config.sort_common == sort_descending)
	{
	  for (power = 4; power > 0; power--)
	    bfd_link_hash_traverse (link_info.hash, lang_one_common, &power);

	  power = 0;
	  bfd_link_hash_traverse (link_info.hash, lang_one_common, &power);
	}
      else
	{
	  for (power = 0; power <= 4; power++)
	    bfd_link_hash_traverse (link_info.hash, lang_one_common, &power);

	  power = (unsigned int) -1;
	  bfd_link_hash_traverse (link_info.hash, lang_one_common, &power);
	}
    }
}

/* Place one common symbol in the correct section.  */

static bfd_boolean
lang_one_common (struct bfd_link_hash_entry *h, void *info)
{
  unsigned int power_of_two;
  bfd_vma size;
  asection *section;

  if (h->type != bfd_link_hash_common)
    return TRUE;

  size = h->u.c.size;
  power_of_two = h->u.c.p->alignment_power;

  if (config.sort_common == sort_descending
      && power_of_two < *(unsigned int *) info)
    return TRUE;
  else if (config.sort_common == sort_ascending
	   && power_of_two > *(unsigned int *) info)
    return TRUE;

  section = h->u.c.p->section;
  if (!bfd_define_common_symbol (link_info.output_bfd, &link_info, h))
    einfo (_("%F%P: could not define common symbol `%pT': %E\n"),
	   h->root.string);

  if (config.map_file != NULL)
    {
      static bfd_boolean header_printed;
      int len;
      char *name;
      char buf[50];

      if (!header_printed)
	{
	  minfo (_("\nAllocating common symbols\n"));
	  minfo (_("Common symbol       size              file\n\n"));
	  header_printed = TRUE;
	}

      name = bfd_demangle (link_info.output_bfd, h->root.string,
			   DMGL_ANSI | DMGL_PARAMS);
      if (name == NULL)
	{
	  minfo ("%s", h->root.string);
	  len = strlen (h->root.string);
	}
      else
	{
	  minfo ("%s", name);
	  len = strlen (name);
	  free (name);
	}

      if (len >= 19)
	{
	  print_nl ();
	  len = 0;
	}
      while (len < 20)
	{
	  print_space ();
	  ++len;
	}

      minfo ("0x");
      if (size <= 0xffffffff)
	sprintf (buf, "%lx", (unsigned long) size);
      else
	sprintf_vma (buf, size);
      minfo ("%s", buf);
      len = strlen (buf);

      while (len < 16)
	{
	  print_space ();
	  ++len;
	}

      minfo ("%pB\n", section->owner);
    }

  return TRUE;
}

/* Handle a single orphan section S, placing the orphan into an appropriate
   output section.  The effects of the --orphan-handling command line
   option are handled here.  */

static void
ldlang_place_orphan (asection *s)
{
  if (config.orphan_handling == orphan_handling_discard)
    {
      lang_output_section_statement_type *os;
      os = lang_output_section_statement_lookup (DISCARD_SECTION_NAME, 0,
						 TRUE);
      if (os->addr_tree == NULL
	  && (bfd_link_relocatable (&link_info)
	      || (s->flags & (SEC_LOAD | SEC_ALLOC)) == 0))
	os->addr_tree = exp_intop (0);
      lang_add_section (&os->children, s, NULL, os);
    }
  else
    {
      lang_output_section_statement_type *os;
      const char *name = s->name;
      int constraint = 0;

      if (config.orphan_handling == orphan_handling_error)
	einfo (_("%X%P: error: unplaced orphan section `%pA' from `%pB'\n"),
	       s, s->owner);

      if (config.unique_orphan_sections || unique_section_p (s, NULL))
	constraint = SPECIAL;

      os = ldemul_place_orphan (s, name, constraint);
      if (os == NULL)
	{
	  os = lang_output_section_statement_lookup (name, constraint, TRUE);
	  if (os->addr_tree == NULL
	      && (bfd_link_relocatable (&link_info)
		  || (s->flags & (SEC_LOAD | SEC_ALLOC)) == 0))
	    os->addr_tree = exp_intop (0);
	  lang_add_section (&os->children, s, NULL, os);
	}

      if (config.orphan_handling == orphan_handling_warn)
	einfo (_("%P: warning: orphan section `%pA' from `%pB' being "
		 "placed in section `%s'\n"),
	       s, s->owner, os->name);
    }
}

/* Run through the input files and ensure that every input section has
   somewhere to go.  If one is found without a destination then create
   an input request and place it into the statement tree.  */

static void
lang_place_orphans (void)
{
  LANG_FOR_EACH_INPUT_STATEMENT (file)
    {
      asection *s;

      for (s = file->the_bfd->sections; s != NULL; s = s->next)
	{
	  if (s->output_section == NULL)
	    {
	      /* This section of the file is not attached, root
		 around for a sensible place for it to go.  */

	      if (file->flags.just_syms)
		bfd_link_just_syms (file->the_bfd, s, &link_info);
	      else if (lang_discard_section_p (s))
		s->output_section = bfd_abs_section_ptr;
	      else if (strcmp (s->name, "COMMON") == 0)
		{
		  /* This is a lonely common section which must have
		     come from an archive.  We attach to the section
		     with the wildcard.  */
		  if (!bfd_link_relocatable (&link_info)
		      || command_line.force_common_definition)
		    {
		      if (default_common_section == NULL)
			default_common_section
			  = lang_output_section_statement_lookup (".bss", 0,
								  TRUE);
		      lang_add_section (&default_common_section->children, s,
					NULL, default_common_section);
		    }
		}
	      else
		ldlang_place_orphan (s);
	    }
	}
    }
}

void
lang_set_flags (lang_memory_region_type *ptr, const char *flags, int invert)
{
  flagword *ptr_flags;

  ptr_flags = invert ? &ptr->not_flags : &ptr->flags;

  while (*flags)
    {
      switch (*flags)
	{
	  /* PR 17900: An exclamation mark in the attributes reverses
	     the sense of any of the attributes that follow.  */
	case '!':
	  invert = !invert;
	  ptr_flags = invert ? &ptr->not_flags : &ptr->flags;
	  break;

	case 'A': case 'a':
	  *ptr_flags |= SEC_ALLOC;
	  break;

	case 'R': case 'r':
	  *ptr_flags |= SEC_READONLY;
	  break;

	case 'W': case 'w':
	  *ptr_flags |= SEC_DATA;
	  break;

	case 'X': case 'x':
	  *ptr_flags |= SEC_CODE;
	  break;

	case 'L': case 'l':
	case 'I': case 'i':
	  *ptr_flags |= SEC_LOAD;
	  break;

	default:
	  einfo (_("%F%P: invalid character %c (%d) in flags\n"),
		 *flags, *flags);
	  break;
	}
      flags++;
    }
}

/* Call a function on each real input file.  This function will be
   called on an archive, but not on the elements.  */

void
lang_for_each_input_file (void (*func) (lang_input_statement_type *))
{
  lang_input_statement_type *f;

  for (f = (void *) input_file_chain.head;
       f != NULL;
       f = f->next_real_file)
    if (f->flags.real)
      func (f);
}

/* Call a function on each real file.  The function will be called on
   all the elements of an archive which are included in the link, but
   will not be called on the archive file itself.  */

void
lang_for_each_file (void (*func) (lang_input_statement_type *))
{
  LANG_FOR_EACH_INPUT_STATEMENT (f)
    {
      if (f->flags.real)
	func (f);
    }
}

void
ldlang_add_file (lang_input_statement_type *entry)
{
  lang_statement_append (&file_chain, entry, &entry->next);

  /* The BFD linker needs to have a list of all input BFDs involved in
     a link.  */
  ASSERT (entry->the_bfd->link.next == NULL);
  ASSERT (entry->the_bfd != link_info.output_bfd);

  *link_info.input_bfds_tail = entry->the_bfd;
  link_info.input_bfds_tail = &entry->the_bfd->link.next;
  bfd_set_usrdata (entry->the_bfd, entry);
  bfd_set_gp_size (entry->the_bfd, g_switch_value);

  /* Look through the sections and check for any which should not be
     included in the link.  We need to do this now, so that we can
     notice when the backend linker tries to report multiple
     definition errors for symbols which are in sections we aren't
     going to link.  FIXME: It might be better to entirely ignore
     symbols which are defined in sections which are going to be
     discarded.  This would require modifying the backend linker for
     each backend which might set the SEC_LINK_ONCE flag.  If we do
     this, we should probably handle SEC_EXCLUDE in the same way.  */

  bfd_map_over_sections (entry->the_bfd, section_already_linked, entry);
}

void
lang_add_output (const char *name, int from_script)
{
  /* Make -o on command line override OUTPUT in script.  */
  if (!had_output_filename || !from_script)
    {
      output_filename = name;
      had_output_filename = TRUE;
    }
}

lang_output_section_statement_type *
lang_enter_output_section_statement (const char *output_section_statement_name,
				     etree_type *address_exp,
				     enum section_type sectype,
				     etree_type *align,
				     etree_type *subalign,
				     etree_type *ebase,
				     int constraint,
				     int align_with_input)
{
  lang_output_section_statement_type *os;

  os = lang_output_section_statement_lookup (output_section_statement_name,
					     constraint, TRUE);
  current_section = os;

  if (os->addr_tree == NULL)
    {
      os->addr_tree = address_exp;
    }
  os->sectype = sectype;
  if (sectype != noload_section)
    os->flags = SEC_NO_FLAGS;
  else
    os->flags = SEC_NEVER_LOAD;
  os->block_value = 1;

  /* Make next things chain into subchain of this.  */
  push_stat_ptr (&os->children);

  os->align_lma_with_input = align_with_input == ALIGN_WITH_INPUT;
  if (os->align_lma_with_input && align != NULL)
    einfo (_("%F%P:%pS: error: align with input and explicit align specified\n"),
	   NULL);

  os->subsection_alignment = subalign;
  os->section_alignment = align;

  os->load_base = ebase;
  return os;
}

void
lang_final (void)
{
  lang_output_statement_type *new_stmt;

  new_stmt = new_stat (lang_output_statement, stat_ptr);
  new_stmt->name = output_filename;
}

/* Reset the current counters in the regions.  */

void
lang_reset_memory_regions (void)
{
  lang_memory_region_type *p = lang_memory_region_list;
  asection *o;
  lang_output_section_statement_type *os;

  for (p = lang_memory_region_list; p != NULL; p = p->next)
    {
      p->current = p->origin;
      p->last_os = NULL;
    }

  for (os = (void *) lang_os_list.head;
       os != NULL;
       os = os->next)
    {
      os->processed_vma = FALSE;
      os->processed_lma = FALSE;
    }

  for (o = link_info.output_bfd->sections; o != NULL; o = o->next)
    {
      /* Save the last size for possible use by bfd_relax_section.  */
      o->rawsize = o->size;
      if (!(o->flags & SEC_FIXED_SIZE))
	o->size = 0;
    }
}

/* Worker for lang_gc_sections_1.  */

static void
gc_section_callback (lang_wild_statement_type *ptr,
		     struct wildcard_list *sec ATTRIBUTE_UNUSED,
		     asection *section,
		     struct flag_info *sflag_info ATTRIBUTE_UNUSED,
		     lang_input_statement_type *file ATTRIBUTE_UNUSED,
		     void *data ATTRIBUTE_UNUSED)
{
  /* If the wild pattern was marked KEEP, the member sections
     should be as well.  */
  if (ptr->keep_sections)
    section->flags |= SEC_KEEP;
}

/* Iterate over sections marking them against GC.  */

static void
lang_gc_sections_1 (lang_statement_union_type *s)
{
  for (; s != NULL; s = s->header.next)
    {
      switch (s->header.type)
	{
	case lang_wild_statement_enum:
	  walk_wild (&s->wild_statement, gc_section_callback, NULL);
	  break;
	case lang_constructors_statement_enum:
	  lang_gc_sections_1 (constructor_list.head);
	  break;
	case lang_output_section_statement_enum:
	  lang_gc_sections_1 (s->output_section_statement.children.head);
	  break;
	case lang_group_statement_enum:
	  lang_gc_sections_1 (s->group_statement.children.head);
	  break;
	default:
	  break;
	}
    }
}

static void
lang_gc_sections (void)
{
  /* Keep all sections so marked in the link script.  */
  lang_gc_sections_1 (statement_list.head);

  /* SEC_EXCLUDE is ignored when doing a relocatable link, except in
     the special case of debug info.  (See bfd/stabs.c)
     Twiddle the flag here, to simplify later linker code.  */
  if (bfd_link_relocatable (&link_info))
    {
      LANG_FOR_EACH_INPUT_STATEMENT (f)
	{
	  asection *sec;
#ifdef ENABLE_PLUGINS
	  if (f->flags.claimed)
	    continue;
#endif
	  for (sec = f->the_bfd->sections; sec != NULL; sec = sec->next)
	    if ((sec->flags & SEC_DEBUGGING) == 0)
	      sec->flags &= ~SEC_EXCLUDE;
	}
    }

  if (link_info.gc_sections)
    bfd_gc_sections (link_info.output_bfd, &link_info);
}

/* Worker for lang_find_relro_sections_1.  */

static void
find_relro_section_callback (lang_wild_statement_type *ptr ATTRIBUTE_UNUSED,
			     struct wildcard_list *sec ATTRIBUTE_UNUSED,
			     asection *section,
			     struct flag_info *sflag_info ATTRIBUTE_UNUSED,
			     lang_input_statement_type *file ATTRIBUTE_UNUSED,
			     void *data)
{
  /* Discarded, excluded and ignored sections effectively have zero
     size.  */
  if (section->output_section != NULL
      && section->output_section->owner == link_info.output_bfd
      && (section->output_section->flags & SEC_EXCLUDE) == 0
      && !IGNORE_SECTION (section)
      && section->size != 0)
    {
      bfd_boolean *has_relro_section = (bfd_boolean *) data;
      *has_relro_section = TRUE;
    }
}

/* Iterate over sections for relro sections.  */

static void
lang_find_relro_sections_1 (lang_statement_union_type *s,
			    seg_align_type *seg,
			    bfd_boolean *has_relro_section)
{
  if (*has_relro_section)
    return;

  for (; s != NULL; s = s->header.next)
    {
      if (s == seg->relro_end_stat)
	break;

      switch (s->header.type)
	{
	case lang_wild_statement_enum:
	  walk_wild (&s->wild_statement,
		     find_relro_section_callback,
		     has_relro_section);
	  break;
	case lang_constructors_statement_enum:
	  lang_find_relro_sections_1 (constructor_list.head,
				      seg, has_relro_section);
	  break;
	case lang_output_section_statement_enum:
	  lang_find_relro_sections_1 (s->output_section_statement.children.head,
				      seg, has_relro_section);
	  break;
	case lang_group_statement_enum:
	  lang_find_relro_sections_1 (s->group_statement.children.head,
				      seg, has_relro_section);
	  break;
	default:
	  break;
	}
    }
}

static void
lang_find_relro_sections (void)
{
  bfd_boolean has_relro_section = FALSE;

  /* Check all sections in the link script.  */

  lang_find_relro_sections_1 (expld.dataseg.relro_start_stat,
			      &expld.dataseg, &has_relro_section);

  if (!has_relro_section)
    link_info.relro = FALSE;
}

/* Relax all sections until bfd_relax_section gives up.  */

void
lang_relax_sections (bfd_boolean need_layout)
{
  if (RELAXATION_ENABLED)
    {
      /* We may need more than one relaxation pass.  */
      int i = link_info.relax_pass;

      /* The backend can use it to determine the current pass.  */
      link_info.relax_pass = 0;

      while (i--)
	{
	  /* Keep relaxing until bfd_relax_section gives up.  */
	  bfd_boolean relax_again;

	  link_info.relax_trip = -1;
	  do
	    {
	      link_info.relax_trip++;

	      /* Note: pe-dll.c does something like this also.  If you find
		 you need to change this code, you probably need to change
		 pe-dll.c also.  DJ  */

	      /* Do all the assignments with our current guesses as to
		 section sizes.  */
	      lang_do_assignments (lang_assigning_phase_enum);

	      /* We must do this after lang_do_assignments, because it uses
		 size.  */
	      lang_reset_memory_regions ();

	      /* Perform another relax pass - this time we know where the
		 globals are, so can make a better guess.  */
	      relax_again = FALSE;
	      lang_size_sections (&relax_again, FALSE);
	    }
	  while (relax_again);

	  link_info.relax_pass++;
	}
      need_layout = TRUE;
    }

  if (need_layout)
    {
      /* Final extra sizing to report errors.  */
      lang_do_assignments (lang_assigning_phase_enum);
      lang_reset_memory_regions ();
      lang_size_sections (NULL, TRUE);
    }
}

#ifdef ENABLE_PLUGINS
/* Find the insert point for the plugin's replacement files.  We
   place them after the first claimed real object file, or if the
   first claimed object is an archive member, after the last real
   object file immediately preceding the archive.  In the event
   no objects have been claimed at all, we return the first dummy
   object file on the list as the insert point; that works, but
   the callee must be careful when relinking the file_chain as it
   is not actually on that chain, only the statement_list and the
   input_file list; in that case, the replacement files must be
   inserted at the head of the file_chain.  */

static lang_input_statement_type *
find_replacements_insert_point (bfd_boolean *before)
{
  lang_input_statement_type *claim1, *lastobject;
  lastobject = (void *) input_file_chain.head;
  for (claim1 = (void *) file_chain.head;
       claim1 != NULL;
       claim1 = claim1->next)
    {
      if (claim1->flags.claimed)
	{
	  *before = claim1->flags.claim_archive;
	  return claim1->flags.claim_archive ? lastobject : claim1;
	}
      /* Update lastobject if this is a real object file.  */
      if (claim1->the_bfd != NULL && claim1->the_bfd->my_archive == NULL)
	lastobject = claim1;
    }
  /* No files were claimed by the plugin.  Choose the last object
     file found on the list (maybe the first, dummy entry) as the
     insert point.  */
  *before = FALSE;
  return lastobject;
}

/* Find where to insert ADD, an archive element or shared library
   added during a rescan.  */

static lang_input_statement_type **
find_rescan_insertion (lang_input_statement_type *add)
{
  bfd *add_bfd = add->the_bfd;
  lang_input_statement_type *f;
  lang_input_statement_type *last_loaded = NULL;
  lang_input_statement_type *before = NULL;
  lang_input_statement_type **iter = NULL;

  if (add_bfd->my_archive != NULL)
    add_bfd = add_bfd->my_archive;

  /* First look through the input file chain, to find an object file
     before the one we've rescanned.  Normal object files always
     appear on both the input file chain and the file chain, so this
     lets us get quickly to somewhere near the correct place on the
     file chain if it is full of archive elements.  Archives don't
     appear on the file chain, but if an element has been extracted
     then their input_statement->next points at it.  */
  for (f = (void *) input_file_chain.head;
       f != NULL;
       f = f->next_real_file)
    {
      if (f->the_bfd == add_bfd)
	{
	  before = last_loaded;
	  if (f->next != NULL)
	    return &f->next->next;
	}
      if (f->the_bfd != NULL && f->next != NULL)
	last_loaded = f;
    }

  for (iter = before ? &before->next : &file_chain.head->input_statement.next;
       *iter != NULL;
       iter = &(*iter)->next)
    if (!(*iter)->flags.claim_archive
	&& (*iter)->the_bfd->my_archive == NULL)
      break;

  return iter;
}

/* Insert SRCLIST into DESTLIST after given element by chaining
   on FIELD as the next-pointer.  (Counterintuitively does not need
   a pointer to the actual after-node itself, just its chain field.)  */

static void
lang_list_insert_after (lang_statement_list_type *destlist,
			lang_statement_list_type *srclist,
			lang_statement_union_type **field)
{
  *(srclist->tail) = *field;
  *field = srclist->head;
  if (destlist->tail == field)
    destlist->tail = srclist->tail;
}

/* Detach new nodes added to DESTLIST since the time ORIGLIST
   was taken as a copy of it and leave them in ORIGLIST.  */

static void
lang_list_remove_tail (lang_statement_list_type *destlist,
		       lang_statement_list_type *origlist)
{
  union lang_statement_union **savetail;
  /* Check that ORIGLIST really is an earlier state of DESTLIST.  */
  ASSERT (origlist->head == destlist->head);
  savetail = origlist->tail;
  origlist->head = *(savetail);
  origlist->tail = destlist->tail;
  destlist->tail = savetail;
  *savetail = NULL;
}

static lang_statement_union_type **
find_next_input_statement (lang_statement_union_type **s)
{
  for ( ; *s; s = &(*s)->header.next)
    {
      lang_statement_union_type **t;
      switch ((*s)->header.type)
	{
	case lang_input_statement_enum:
	  return s;
	case lang_wild_statement_enum:
	  t = &(*s)->wild_statement.children.head;
	  break;
	case lang_group_statement_enum:
	  t = &(*s)->group_statement.children.head;
	  break;
	case lang_output_section_statement_enum:
	  t = &(*s)->output_section_statement.children.head;
	  break;
	default:
	  continue;
	}
      t = find_next_input_statement (t);
      if (*t)
	return t;
    }
  return s;
}
#endif /* ENABLE_PLUGINS */

/* Add NAME to the list of garbage collection entry points.  */

void
lang_add_gc_name (const char *name)
{
  struct bfd_sym_chain *sym;

  if (name == NULL)
    return;

  sym = stat_alloc (sizeof (*sym));

  sym->next = link_info.gc_sym_list;
  sym->name = name;
  link_info.gc_sym_list = sym;
}

/* Check relocations.  */

static void
lang_check_relocs (void)
{
  if (link_info.check_relocs_after_open_input)
    {
      bfd *abfd;

      for (abfd = link_info.input_bfds;
	   abfd != (bfd *) NULL; abfd = abfd->link.next)
	if (!bfd_link_check_relocs (abfd, &link_info))
	  {
	    /* No object output, fail return.  */
	    config.make_executable = FALSE;
	    /* Note: we do not abort the loop, but rather
	       continue the scan in case there are other
	       bad relocations to report.  */
	  }
    }
}

/* Look through all output sections looking for places where we can
   propagate forward the lma region.  */

static void
lang_propagate_lma_regions (void)
{
  lang_output_section_statement_type *os;

  for (os = (void *) lang_os_list.head;
       os != NULL;
       os = os->next)
    {
      if (os->prev != NULL
	  && os->lma_region == NULL
	  && os->load_base == NULL
	  && os->addr_tree == NULL
	  && os->region == os->prev->region)
	os->lma_region = os->prev->lma_region;
    }
}

void
lang_process (void)
{
  /* Finalize dynamic list.  */
  if (link_info.dynamic_list)
    lang_finalize_version_expr_head (&link_info.dynamic_list->head);

  current_target = default_target;

  /* Open the output file.  */
  lang_for_each_statement (ldlang_open_output);
  init_opb (NULL);

  ldemul_create_output_section_statements ();

  /* Add to the hash table all undefineds on the command line.  */
  lang_place_undefineds ();

  if (!bfd_section_already_linked_table_init ())
    einfo (_("%F%P: can not create hash table: %E\n"));

  /* Create a bfd for each input file.  */
  current_target = default_target;
  lang_statement_iteration++;
  open_input_bfds (statement_list.head, OPEN_BFD_NORMAL);
  /* open_input_bfds also handles assignments, so we can give values
     to symbolic origin/length now.  */
  lang_do_memory_regions ();

#ifdef ENABLE_PLUGINS
  if (link_info.lto_plugin_active)
    {
      lang_statement_list_type added;
      lang_statement_list_type files, inputfiles;

      /* Now all files are read, let the plugin(s) decide if there
	 are any more to be added to the link before we call the
	 emulation's after_open hook.  We create a private list of
	 input statements for this purpose, which we will eventually
	 insert into the global statement list after the first claimed
	 file.  */
      added = *stat_ptr;
      /* We need to manipulate all three chains in synchrony.  */
      files = file_chain;
      inputfiles = input_file_chain;
      if (plugin_call_all_symbols_read ())
	einfo (_("%F%P: %s: plugin reported error after all symbols read\n"),
	       plugin_error_plugin ());
      /* Open any newly added files, updating the file chains.  */
      plugin_undefs = link_info.hash->undefs_tail;
      open_input_bfds (*added.tail, OPEN_BFD_NORMAL);
      if (plugin_undefs == link_info.hash->undefs_tail)
	plugin_undefs = NULL;
      /* Restore the global list pointer now they have all been added.  */
      lang_list_remove_tail (stat_ptr, &added);
      /* And detach the fresh ends of the file lists.  */
      lang_list_remove_tail (&file_chain, &files);
      lang_list_remove_tail (&input_file_chain, &inputfiles);
      /* Were any new files added?  */
      if (added.head != NULL)
	{
	  /* If so, we will insert them into the statement list immediately
	     after the first input file that was claimed by the plugin,
	     unless that file was an archive in which case it is inserted
	     immediately before.  */
	  bfd_boolean before;
	  lang_statement_union_type **prev;
	  plugin_insert = find_replacements_insert_point (&before);
	  /* If a plugin adds input files without having claimed any, we
	     don't really have a good idea where to place them.  Just putting
	     them at the start or end of the list is liable to leave them
	     outside the crtbegin...crtend range.  */
	  ASSERT (plugin_insert != NULL);
	  /* Splice the new statement list into the old one.  */
	  prev = &plugin_insert->header.next;
	  if (before)
	    {
	      prev = find_next_input_statement (prev);
	      if (*prev != (void *) plugin_insert->next_real_file)
		{
		  /* We didn't find the expected input statement.
		     Fall back to adding after plugin_insert.  */
		  prev = &plugin_insert->header.next;
		}
	    }
	  lang_list_insert_after (stat_ptr, &added, prev);
	  /* Likewise for the file chains.  */
	  lang_list_insert_after (&input_file_chain, &inputfiles,
				  (void *) &plugin_insert->next_real_file);
	  /* We must be careful when relinking file_chain; we may need to
	     insert the new files at the head of the list if the insert
	     point chosen is the dummy first input file.  */
	  if (plugin_insert->filename)
	    lang_list_insert_after (&file_chain, &files,
				    (void *) &plugin_insert->next);
	  else
	    lang_list_insert_after (&file_chain, &files, &file_chain.head);

	  /* Rescan archives in case new undefined symbols have appeared.  */
	  files = file_chain;
	  lang_statement_iteration++;
	  open_input_bfds (statement_list.head, OPEN_BFD_RESCAN);
	  lang_list_remove_tail (&file_chain, &files);
	  while (files.head != NULL)
	    {
	      lang_input_statement_type **insert;
	      lang_input_statement_type **iter, *temp;
	      bfd *my_arch;

	      insert = find_rescan_insertion (&files.head->input_statement);
	      /* All elements from an archive can be added at once.  */
	      iter = &files.head->input_statement.next;
	      my_arch = files.head->input_statement.the_bfd->my_archive;
	      if (my_arch != NULL)
		for (; *iter != NULL; iter = &(*iter)->next)
		  if ((*iter)->the_bfd->my_archive != my_arch)
		    break;
	      temp = *insert;
	      *insert = &files.head->input_statement;
	      files.head = (lang_statement_union_type *) *iter;
	      *iter = temp;
	      if (my_arch != NULL)
		{
		  lang_input_statement_type *parent = bfd_usrdata (my_arch);
		  if (parent != NULL)
		    parent->next = (lang_input_statement_type *)
		      ((char *) iter
		       - offsetof (lang_input_statement_type, next));
		}
	    }
	}
    }
#endif /* ENABLE_PLUGINS */

  /* Make sure that nobody has tried to add a symbol to this list
     before now.  */
  ASSERT (link_info.gc_sym_list == NULL);

  link_info.gc_sym_list = &entry_symbol;

  if (entry_symbol.name == NULL)
    {
      link_info.gc_sym_list = ldlang_undef_chain_list_head;

      /* entry_symbol is normally initialied by a ENTRY definition in the
	 linker script or the -e command line option.  But if neither of
	 these have been used, the target specific backend may still have
	 provided an entry symbol via a call to lang_default_entry().
	 Unfortunately this value will not be processed until lang_end()
	 is called, long after this function has finished.  So detect this
	 case here and add the target's entry symbol to the list of starting
	 points for garbage collection resolution.  */
      lang_add_gc_name (entry_symbol_default);
    }

  lang_add_gc_name (link_info.init_function);
  lang_add_gc_name (link_info.fini_function);

  ldemul_after_open ();
  if (config.map_file != NULL)
    lang_print_asneeded ();

  ldlang_open_ctf ();

  bfd_section_already_linked_table_free ();

  /* Make sure that we're not mixing architectures.  We call this
     after all the input files have been opened, but before we do any
     other processing, so that any operations merge_private_bfd_data
     does on the output file will be known during the rest of the
     link.  */
  lang_check ();

  /* Handle .exports instead of a version script if we're told to do so.  */
  if (command_line.version_exports_section)
    lang_do_version_exports_section ();

  /* Build all sets based on the information gathered from the input
     files.  */
  ldctor_build_sets ();

  /* Give initial values for __start and __stop symbols, so that  ELF
     gc_sections will keep sections referenced by these symbols.  Must
     be done before lang_do_assignments below.  */
  if (config.build_constructors)
    lang_init_start_stop ();

  /* PR 13683: We must rerun the assignments prior to running garbage
     collection in order to make sure that all symbol aliases are resolved.  */
  lang_do_assignments (lang_mark_phase_enum);
  expld.phase = lang_first_phase_enum;

  /* Size up the common data.  */
  lang_common ();

  /* Remove unreferenced sections if asked to.  */
  lang_gc_sections ();

  /* Check relocations.  */
  lang_check_relocs ();

  ldemul_after_check_relocs ();

  /* Update wild statements.  */
  update_wild_statements (statement_list.head);

  /* Run through the contours of the script and attach input sections
     to the correct output sections.  */
  lang_statement_iteration++;
  map_input_to_output_sections (statement_list.head, NULL, NULL);

  /* Start at the statement immediately after the special abs_section
     output statement, so that it isn't reordered.  */
  process_insert_statements (&lang_os_list.head->header.next);

  /* Find any sections not attached explicitly and handle them.  */
  lang_place_orphans ();

  if (!bfd_link_relocatable (&link_info))
    {
      asection *found;

      /* Merge SEC_MERGE sections.  This has to be done after GC of
	 sections, so that GCed sections are not merged, but before
	 assigning dynamic symbols, since removing whole input sections
	 is hard then.  */
      bfd_merge_sections (link_info.output_bfd, &link_info);

      /* Look for a text section and set the readonly attribute in it.  */
      found = bfd_get_section_by_name (link_info.output_bfd, ".text");

      if (found != NULL)
	{
	  if (config.text_read_only)
	    found->flags |= SEC_READONLY;
	  else
	    found->flags &= ~SEC_READONLY;
	}
    }

  /* Merge together CTF sections.  After this, only the symtab-dependent
     function and data object sections need adjustment.  */
  lang_merge_ctf ();

  /* Emit the CTF, iff the emulation doesn't need to do late emission after
     examining things laid out late, like the strtab.  */
  lang_write_ctf (0);

  /* Copy forward lma regions for output sections in same lma region.  */
  lang_propagate_lma_regions ();

  /* Defining __start/__stop symbols early for --gc-sections to work
     around a glibc build problem can result in these symbols being
     defined when they should not be.  Fix them now.  */
  if (config.build_constructors)
    lang_undef_start_stop ();

  /* Define .startof./.sizeof. symbols with preliminary values before
     dynamic symbols are created.  */
  if (!bfd_link_relocatable (&link_info))
    lang_init_startof_sizeof ();

  /* Do anything special before sizing sections.  This is where ELF
     and other back-ends size dynamic sections.  */
  ldemul_before_allocation ();

  /* We must record the program headers before we try to fix the
     section positions, since they will affect SIZEOF_HEADERS.  */
  lang_record_phdrs ();

  /* Check relro sections.  */
  if (link_info.relro && !bfd_link_relocatable (&link_info))
    lang_find_relro_sections ();

  /* Size up the sections.  */
  lang_size_sections (NULL, !RELAXATION_ENABLED);

  /* See if anything special should be done now we know how big
     everything is.  This is where relaxation is done.  */
  ldemul_after_allocation ();

  /* Fix any __start, __stop, .startof. or .sizeof. symbols.  */
  lang_finalize_start_stop ();

  /* Do all the assignments again, to report errors.  Assignment
     statements are processed multiple times, updating symbols; In
     open_input_bfds, lang_do_assignments, and lang_size_sections.
     Since lang_relax_sections calls lang_do_assignments, symbols are
     also updated in ldemul_after_allocation.  */
  lang_do_assignments (lang_final_phase_enum);

  ldemul_finish ();

  /* Convert absolute symbols to section relative.  */
  ldexp_finalize_syms ();

  /* Make sure that the section addresses make sense.  */
  if (command_line.check_section_addresses)
    lang_check_section_addresses ();

  /* Check any required symbols are known.  */
  ldlang_check_require_defined_symbols ();

  lang_end ();
}

/* EXPORTED TO YACC */

void
lang_add_wild (struct wildcard_spec *filespec,
	       struct wildcard_list *section_list,
	       bfd_boolean keep_sections)
{
  struct wildcard_list *curr, *next;
  lang_wild_statement_type *new_stmt;

  /* Reverse the list as the parser puts it back to front.  */
  for (curr = section_list, section_list = NULL;
       curr != NULL;
       section_list = curr, curr = next)
    {
      next = curr->next;
      curr->next = section_list;
    }

  if (filespec != NULL && filespec->name != NULL)
    {
      if (strcmp (filespec->name, "*") == 0)
	filespec->name = NULL;
      else if (!wildcardp (filespec->name))
	lang_has_input_file = TRUE;
    }

  new_stmt = new_stat (lang_wild_statement, stat_ptr);
  new_stmt->filename = NULL;
  new_stmt->filenames_sorted = FALSE;
  new_stmt->section_flag_list = NULL;
  new_stmt->exclude_name_list = NULL;
  if (filespec != NULL)
    {
      new_stmt->filename = filespec->name;
      new_stmt->filenames_sorted = filespec->sorted == by_name;
      new_stmt->section_flag_list = filespec->section_flag_list;
      new_stmt->exclude_name_list = filespec->exclude_name_list;
    }
  new_stmt->section_list = section_list;
  new_stmt->keep_sections = keep_sections;
  lang_list_init (&new_stmt->children);
  analyze_walk_wild_section_handler (new_stmt);
}

void
lang_section_start (const char *name, etree_type *address,
		    const segment_type *segment)
{
  lang_address_statement_type *ad;

  ad = new_stat (lang_address_statement, stat_ptr);
  ad->section_name = name;
  ad->address = address;
  ad->segment = segment;
}

/* Set the start symbol to NAME.  CMDLINE is nonzero if this is called
   because of a -e argument on the command line, or zero if this is
   called by ENTRY in a linker script.  Command line arguments take
   precedence.  */

void
lang_add_entry (const char *name, bfd_boolean cmdline)
{
  if (entry_symbol.name == NULL
      || cmdline
      || !entry_from_cmdline)
    {
      entry_symbol.name = name;
      entry_from_cmdline = cmdline;
    }
}

/* Set the default start symbol to NAME.  .em files should use this,
   not lang_add_entry, to override the use of "start" if neither the
   linker script nor the command line specifies an entry point.  NAME
   must be permanently allocated.  */
void
lang_default_entry (const char *name)
{
  entry_symbol_default = name;
}

void
lang_add_target (const char *name)
{
  lang_target_statement_type *new_stmt;

  new_stmt = new_stat (lang_target_statement, stat_ptr);
  new_stmt->target = name;
}

void
lang_add_map (const char *name)
{
  while (*name)
    {
      switch (*name)
	{
	case 'F':
	  map_option_f = TRUE;
	  break;
	}
      name++;
    }
}

void
lang_add_fill (fill_type *fill)
{
  lang_fill_statement_type *new_stmt;

  new_stmt = new_stat (lang_fill_statement, stat_ptr);
  new_stmt->fill = fill;
}

void
lang_add_data (int type, union etree_union *exp)
{
  lang_data_statement_type *new_stmt;

  new_stmt = new_stat (lang_data_statement, stat_ptr);
  new_stmt->exp = exp;
  new_stmt->type = type;
}

/* Create a new reloc statement.  RELOC is the BFD relocation type to
   generate.  HOWTO is the corresponding howto structure (we could
   look this up, but the caller has already done so).  SECTION is the
   section to generate a reloc against, or NAME is the name of the
   symbol to generate a reloc against.  Exactly one of SECTION and
   NAME must be NULL.  ADDEND is an expression for the addend.  */

void
lang_add_reloc (bfd_reloc_code_real_type reloc,
		reloc_howto_type *howto,
		asection *section,
		const char *name,
		union etree_union *addend)
{
  lang_reloc_statement_type *p = new_stat (lang_reloc_statement, stat_ptr);

  p->reloc = reloc;
  p->howto = howto;
  p->section = section;
  p->name = name;
  p->addend_exp = addend;

  p->addend_value = 0;
  p->output_section = NULL;
  p->output_offset = 0;
}

lang_assignment_statement_type *
lang_add_assignment (etree_type *exp)
{
  lang_assignment_statement_type *new_stmt;

  new_stmt = new_stat (lang_assignment_statement, stat_ptr);
  new_stmt->exp = exp;
  return new_stmt;
}

void
lang_add_attribute (enum statement_enum attribute)
{
  new_statement (attribute, sizeof (lang_statement_header_type), stat_ptr);
}

void
lang_startup (const char *name)
{
  if (first_file->filename != NULL)
    {
      einfo (_("%F%P: multiple STARTUP files\n"));
    }
  first_file->filename = name;
  first_file->local_sym_name = name;
  first_file->flags.real = TRUE;
}

void
lang_float (bfd_boolean maybe)
{
  lang_float_flag = maybe;
}


/* Work out the load- and run-time regions from a script statement, and
   store them in *LMA_REGION and *REGION respectively.

   MEMSPEC is the name of the run-time region, or the value of
   DEFAULT_MEMORY_REGION if the statement didn't specify one.
   LMA_MEMSPEC is the name of the load-time region, or null if the
   statement didn't specify one.HAVE_LMA_P is TRUE if the statement
   had an explicit load address.

   It is an error to specify both a load region and a load address.  */

static void
lang_get_regions (lang_memory_region_type **region,
		  lang_memory_region_type **lma_region,
		  const char *memspec,
		  const char *lma_memspec,
		  bfd_boolean have_lma,
		  bfd_boolean have_vma)
{
  *lma_region = lang_memory_region_lookup (lma_memspec, FALSE);

  /* If no runtime region or VMA has been specified, but the load region
     has been specified, then use the load region for the runtime region
     as well.  */
  if (lma_memspec != NULL
      && !have_vma
      && strcmp (memspec, DEFAULT_MEMORY_REGION) == 0)
    *region = *lma_region;
  else
    *region = lang_memory_region_lookup (memspec, FALSE);

  if (have_lma && lma_memspec != 0)
    einfo (_("%X%P:%pS: section has both a load address and a load region\n"),
	   NULL);
}

void
lang_leave_output_section_statement (fill_type *fill, const char *memspec,
				     lang_output_section_phdr_list *phdrs,
				     const char *lma_memspec)
{
  lang_get_regions (&current_section->region,
		    &current_section->lma_region,
		    memspec, lma_memspec,
		    current_section->load_base != NULL,
		    current_section->addr_tree != NULL);

  current_section->fill = fill;
  current_section->phdrs = phdrs;
  pop_stat_ptr ();
}

/* Set the output format type.  -oformat overrides scripts.  */

void
lang_add_output_format (const char *format,
			const char *big,
			const char *little,
			int from_script)
{
  if (output_target == NULL || !from_script)
    {
      if (command_line.endian == ENDIAN_BIG
	  && big != NULL)
	format = big;
      else if (command_line.endian == ENDIAN_LITTLE
	       && little != NULL)
	format = little;

      output_target = format;
    }
}

void
lang_add_insert (const char *where, int is_before)
{
  lang_insert_statement_type *new_stmt;

  new_stmt = new_stat (lang_insert_statement, stat_ptr);
  new_stmt->where = where;
  new_stmt->is_before = is_before;
  saved_script_handle = previous_script_handle;
}

/* Enter a group.  This creates a new lang_group_statement, and sets
   stat_ptr to build new statements within the group.  */

void
lang_enter_group (void)
{
  lang_group_statement_type *g;

  g = new_stat (lang_group_statement, stat_ptr);
  lang_list_init (&g->children);
  push_stat_ptr (&g->children);
}

/* Leave a group.  This just resets stat_ptr to start writing to the
   regular list of statements again.  Note that this will not work if
   groups can occur inside anything else which can adjust stat_ptr,
   but currently they can't.  */

void
lang_leave_group (void)
{
  pop_stat_ptr ();
}

/* Add a new program header.  This is called for each entry in a PHDRS
   command in a linker script.  */

void
lang_new_phdr (const char *name,
	       etree_type *type,
	       bfd_boolean filehdr,
	       bfd_boolean phdrs,
	       etree_type *at,
	       etree_type *flags)
{
  struct lang_phdr *n, **pp;
  bfd_boolean hdrs;

  n = stat_alloc (sizeof (struct lang_phdr));
  n->next = NULL;
  n->name = name;
  n->type = exp_get_vma (type, 0, "program header type");
  n->filehdr = filehdr;
  n->phdrs = phdrs;
  n->at = at;
  n->flags = flags;

  hdrs = n->type == 1 && (phdrs || filehdr);

  for (pp = &lang_phdr_list; *pp != NULL; pp = &(*pp)->next)
    if (hdrs
	&& (*pp)->type == 1
	&& !((*pp)->filehdr || (*pp)->phdrs))
      {
	einfo (_("%X%P:%pS: PHDRS and FILEHDR are not supported"
		 " when prior PT_LOAD headers lack them\n"), NULL);
	hdrs = FALSE;
      }

  *pp = n;
}

/* Record the program header information in the output BFD.  FIXME: We
   should not be calling an ELF specific function here.  */

static void
lang_record_phdrs (void)
{
  unsigned int alc;
  asection **secs;
  lang_output_section_phdr_list *last;
  struct lang_phdr *l;
  lang_output_section_statement_type *os;

  alc = 10;
  secs = (asection **) xmalloc (alc * sizeof (asection *));
  last = NULL;

  for (l = lang_phdr_list; l != NULL; l = l->next)
    {
      unsigned int c;
      flagword flags;
      bfd_vma at;

      c = 0;
      for (os = (void *) lang_os_list.head;
	   os != NULL;
	   os = os->next)
	{
	  lang_output_section_phdr_list *pl;

	  if (os->constraint < 0)
	    continue;

	  pl = os->phdrs;
	  if (pl != NULL)
	    last = pl;
	  else
	    {
	      if (os->sectype == noload_section
		  || os->bfd_section == NULL
		  || (os->bfd_section->flags & SEC_ALLOC) == 0)
		continue;

	      /* Don't add orphans to PT_INTERP header.  */
	      if (l->type == 3)
		continue;

	      if (last == NULL)
		{
		  lang_output_section_statement_type *tmp_os;

		  /* If we have not run across a section with a program
		     header assigned to it yet, then scan forwards to find
		     one.  This prevents inconsistencies in the linker's
		     behaviour when a script has specified just a single
		     header and there are sections in that script which are
		     not assigned to it, and which occur before the first
		     use of that header. See here for more details:
		     http://sourceware.org/ml/binutils/2007-02/msg00291.html  */
		  for (tmp_os = os; tmp_os; tmp_os = tmp_os->next)
		    if (tmp_os->phdrs)
		      {
			last = tmp_os->phdrs;
			break;
		      }
		  if (last == NULL)
		    einfo (_("%F%P: no sections assigned to phdrs\n"));
		}
	      pl = last;
	    }

	  if (os->bfd_section == NULL)
	    continue;

	  for (; pl != NULL; pl = pl->next)
	    {
	      if (strcmp (pl->name, l->name) == 0)
		{
		  if (c >= alc)
		    {
		      alc *= 2;
		      secs = (asection **) xrealloc (secs,
						     alc * sizeof (asection *));
		    }
		  secs[c] = os->bfd_section;
		  ++c;
		  pl->used = TRUE;
		}
	    }
	}

      if (l->flags == NULL)
	flags = 0;
      else
	flags = exp_get_vma (l->flags, 0, "phdr flags");

      if (l->at == NULL)
	at = 0;
      else
	at = exp_get_vma (l->at, 0, "phdr load address");

      if (!bfd_record_phdr (link_info.output_bfd, l->type,
			    l->flags != NULL, flags, l->at != NULL,
			    at, l->filehdr, l->phdrs, c, secs))
	einfo (_("%F%P: bfd_record_phdr failed: %E\n"));
    }

  free (secs);

  /* Make sure all the phdr assignments succeeded.  */
  for (os = (void *) lang_os_list.head;
       os != NULL;
       os = os->next)
    {
      lang_output_section_phdr_list *pl;

      if (os->constraint < 0
	  || os->bfd_section == NULL)
	continue;

      for (pl = os->phdrs;
	   pl != NULL;
	   pl = pl->next)
	if (!pl->used && strcmp (pl->name, "NONE") != 0)
	  einfo (_("%X%P: section `%s' assigned to non-existent phdr `%s'\n"),
		 os->name, pl->name);
    }
}

/* Record a list of sections which may not be cross referenced.  */

void
lang_add_nocrossref (lang_nocrossref_type *l)
{
  struct lang_nocrossrefs *n;

  n = (struct lang_nocrossrefs *) xmalloc (sizeof *n);
  n->next = nocrossref_list;
  n->list = l;
  n->onlyfirst = FALSE;
  nocrossref_list = n;

  /* Set notice_all so that we get informed about all symbols.  */
  link_info.notice_all = TRUE;
}

/* Record a section that cannot be referenced from a list of sections.  */

void
lang_add_nocrossref_to (lang_nocrossref_type *l)
{
  lang_add_nocrossref (l);
  nocrossref_list->onlyfirst = TRUE;
}

/* Overlay handling.  We handle overlays with some static variables.  */

/* The overlay virtual address.  */
static etree_type *overlay_vma;
/* And subsection alignment.  */
static etree_type *overlay_subalign;

/* An expression for the maximum section size seen so far.  */
static etree_type *overlay_max;

/* A list of all the sections in this overlay.  */

struct overlay_list {
  struct overlay_list *next;
  lang_output_section_statement_type *os;
};

static struct overlay_list *overlay_list;

/* Start handling an overlay.  */

void
lang_enter_overlay (etree_type *vma_expr, etree_type *subalign)
{
  /* The grammar should prevent nested overlays from occurring.  */
  ASSERT (overlay_vma == NULL
	  && overlay_subalign == NULL
	  && overlay_max == NULL);

  overlay_vma = vma_expr;
  overlay_subalign = subalign;
}

/* Start a section in an overlay.  We handle this by calling
   lang_enter_output_section_statement with the correct VMA.
   lang_leave_overlay sets up the LMA and memory regions.  */

void
lang_enter_overlay_section (const char *name)
{
  struct overlay_list *n;
  etree_type *size;

  lang_enter_output_section_statement (name, overlay_vma, overlay_section,
				       0, overlay_subalign, 0, 0, 0);

  /* If this is the first section, then base the VMA of future
     sections on this one.  This will work correctly even if `.' is
     used in the addresses.  */
  if (overlay_list == NULL)
    overlay_vma = exp_nameop (ADDR, name);

  /* Remember the section.  */
  n = (struct overlay_list *) xmalloc (sizeof *n);
  n->os = current_section;
  n->next = overlay_list;
  overlay_list = n;

  size = exp_nameop (SIZEOF, name);

  /* Arrange to work out the maximum section end address.  */
  if (overlay_max == NULL)
    overlay_max = size;
  else
    overlay_max = exp_binop (MAX_K, overlay_max, size);
}

/* Finish a section in an overlay.  There isn't any special to do
   here.  */

void
lang_leave_overlay_section (fill_type *fill,
			    lang_output_section_phdr_list *phdrs)
{
  const char *name;
  char *clean, *s2;
  const char *s1;
  char *buf;

  name = current_section->name;

  /* For now, assume that DEFAULT_MEMORY_REGION is the run-time memory
     region and that no load-time region has been specified.  It doesn't
     really matter what we say here, since lang_leave_overlay will
     override it.  */
  lang_leave_output_section_statement (fill, DEFAULT_MEMORY_REGION, phdrs, 0);

  /* Define the magic symbols.  */

  clean = (char *) xmalloc (strlen (name) + 1);
  s2 = clean;
  for (s1 = name; *s1 != '\0'; s1++)
    if (ISALNUM (*s1) || *s1 == '_')
      *s2++ = *s1;
  *s2 = '\0';

  buf = (char *) xmalloc (strlen (clean) + sizeof "__load_start_");
  sprintf (buf, "__load_start_%s", clean);
  lang_add_assignment (exp_provide (buf,
				    exp_nameop (LOADADDR, name),
				    FALSE));

  buf = (char *) xmalloc (strlen (clean) + sizeof "__load_stop_");
  sprintf (buf, "__load_stop_%s", clean);
  lang_add_assignment (exp_provide (buf,
				    exp_binop ('+',
					       exp_nameop (LOADADDR, name),
					       exp_nameop (SIZEOF, name)),
				    FALSE));

  free (clean);
}

/* Finish an overlay.  If there are any overlay wide settings, this
   looks through all the sections in the overlay and sets them.  */

void
lang_leave_overlay (etree_type *lma_expr,
		    int nocrossrefs,
		    fill_type *fill,
		    const char *memspec,
		    lang_output_section_phdr_list *phdrs,
		    const char *lma_memspec)
{
  lang_memory_region_type *region;
  lang_memory_region_type *lma_region;
  struct overlay_list *l;
  lang_nocrossref_type *nocrossref;

  lang_get_regions (&region, &lma_region,
		    memspec, lma_memspec,
		    lma_expr != NULL, FALSE);

  nocrossref = NULL;

  /* After setting the size of the last section, set '.' to end of the
     overlay region.  */
  if (overlay_list != NULL)
    {
      overlay_list->os->update_dot = 1;
      overlay_list->os->update_dot_tree
	= exp_assign (".", exp_binop ('+', overlay_vma, overlay_max), FALSE);
    }

  l = overlay_list;
  while (l != NULL)
    {
      struct overlay_list *next;

      if (fill != NULL && l->os->fill == NULL)
	l->os->fill = fill;

      l->os->region = region;
      l->os->lma_region = lma_region;

      /* The first section has the load address specified in the
	 OVERLAY statement.  The rest are worked out from that.
	 The base address is not needed (and should be null) if
	 an LMA region was specified.  */
      if (l->next == 0)
	{
	  l->os->load_base = lma_expr;
	  l->os->sectype = first_overlay_section;
	}
      if (phdrs != NULL && l->os->phdrs == NULL)
	l->os->phdrs = phdrs;

      if (nocrossrefs)
	{
	  lang_nocrossref_type *nc;

	  nc = (lang_nocrossref_type *) xmalloc (sizeof *nc);
	  nc->name = l->os->name;
	  nc->next = nocrossref;
	  nocrossref = nc;
	}

      next = l->next;
      free (l);
      l = next;
    }

  if (nocrossref != NULL)
    lang_add_nocrossref (nocrossref);

  overlay_vma = NULL;
  overlay_list = NULL;
  overlay_max = NULL;
  overlay_subalign = NULL;
}

/* Version handling.  This is only useful for ELF.  */

/* If PREV is NULL, return first version pattern matching particular symbol.
   If PREV is non-NULL, return first version pattern matching particular
   symbol after PREV (previously returned by lang_vers_match).  */

static struct bfd_elf_version_expr *
lang_vers_match (struct bfd_elf_version_expr_head *head,
		 struct bfd_elf_version_expr *prev,
		 const char *sym)
{
  const char *c_sym;
  const char *cxx_sym = sym;
  const char *java_sym = sym;
  struct bfd_elf_version_expr *expr = NULL;
  enum demangling_styles curr_style;

  curr_style = CURRENT_DEMANGLING_STYLE;
  cplus_demangle_set_style (no_demangling);
  c_sym = bfd_demangle (link_info.output_bfd, sym, DMGL_NO_OPTS);
  if (!c_sym)
    c_sym = sym;
  cplus_demangle_set_style (curr_style);

  if (head->mask & BFD_ELF_VERSION_CXX_TYPE)
    {
      cxx_sym = bfd_demangle (link_info.output_bfd, sym,
			      DMGL_PARAMS | DMGL_ANSI);
      if (!cxx_sym)
	cxx_sym = sym;
    }
  if (head->mask & BFD_ELF_VERSION_JAVA_TYPE)
    {
      java_sym = bfd_demangle (link_info.output_bfd, sym, DMGL_JAVA);
      if (!java_sym)
	java_sym = sym;
    }

  if (head->htab && (prev == NULL || prev->literal))
    {
      struct bfd_elf_version_expr e;

      switch (prev ? prev->mask : 0)
	{
	case 0:
	  if (head->mask & BFD_ELF_VERSION_C_TYPE)
	    {
	      e.pattern = c_sym;
	      expr = (struct bfd_elf_version_expr *)
		  htab_find ((htab_t) head->htab, &e);
	      while (expr && strcmp (expr->pattern, c_sym) == 0)
		if (expr->mask == BFD_ELF_VERSION_C_TYPE)
		  goto out_ret;
		else
		  expr = expr->next;
	    }
	  /* Fallthrough */
	case BFD_ELF_VERSION_C_TYPE:
	  if (head->mask & BFD_ELF_VERSION_CXX_TYPE)
	    {
	      e.pattern = cxx_sym;
	      expr = (struct bfd_elf_version_expr *)
		  htab_find ((htab_t) head->htab, &e);
	      while (expr && strcmp (expr->pattern, cxx_sym) == 0)
		if (expr->mask == BFD_ELF_VERSION_CXX_TYPE)
		  goto out_ret;
		else
		  expr = expr->next;
	    }
	  /* Fallthrough */
	case BFD_ELF_VERSION_CXX_TYPE:
	  if (head->mask & BFD_ELF_VERSION_JAVA_TYPE)
	    {
	      e.pattern = java_sym;
	      expr = (struct bfd_elf_version_expr *)
		  htab_find ((htab_t) head->htab, &e);
	      while (expr && strcmp (expr->pattern, java_sym) == 0)
		if (expr->mask == BFD_ELF_VERSION_JAVA_TYPE)
		  goto out_ret;
		else
		  expr = expr->next;
	    }
	  /* Fallthrough */
	default:
	  break;
	}
    }

  /* Finally, try the wildcards.  */
  if (prev == NULL || prev->literal)
    expr = head->remaining;
  else
    expr = prev->next;
  for (; expr; expr = expr->next)
    {
      const char *s;

      if (!expr->pattern)
	continue;

      if (expr->pattern[0] == '*' && expr->pattern[1] == '\0')
	break;

      if (expr->mask == BFD_ELF_VERSION_JAVA_TYPE)
	s = java_sym;
      else if (expr->mask == BFD_ELF_VERSION_CXX_TYPE)
	s = cxx_sym;
      else
	s = c_sym;
      if (fnmatch (expr->pattern, s, 0) == 0)
	break;
    }

 out_ret:
  if (c_sym != sym)
    free ((char *) c_sym);
  if (cxx_sym != sym)
    free ((char *) cxx_sym);
  if (java_sym != sym)
    free ((char *) java_sym);
  return expr;
}

/* Return NULL if the PATTERN argument is a glob pattern, otherwise,
   return a pointer to the symbol name with any backslash quotes removed.  */

static const char *
realsymbol (const char *pattern)
{
  const char *p;
  bfd_boolean changed = FALSE, backslash = FALSE;
  char *s, *symbol = (char *) xmalloc (strlen (pattern) + 1);

  for (p = pattern, s = symbol; *p != '\0'; ++p)
    {
      /* It is a glob pattern only if there is no preceding
	 backslash.  */
      if (backslash)
	{
	  /* Remove the preceding backslash.  */
	  *(s - 1) = *p;
	  backslash = FALSE;
	  changed = TRUE;
	}
      else
	{
	  if (*p == '?' || *p == '*' || *p == '[')
	    {
	      free (symbol);
	      return NULL;
	    }

	  *s++ = *p;
	  backslash = *p == '\\';
	}
    }

  if (changed)
    {
      *s = '\0';
      return symbol;
    }
  else
    {
      free (symbol);
      return pattern;
    }
}

/* This is called for each variable name or match expression.  NEW_NAME is
   the name of the symbol to match, or, if LITERAL_P is FALSE, a glob
   pattern to be matched against symbol names.  */

struct bfd_elf_version_expr *
lang_new_vers_pattern (struct bfd_elf_version_expr *orig,
		       const char *new_name,
		       const char *lang,
		       bfd_boolean literal_p)
{
  struct bfd_elf_version_expr *ret;

  ret = (struct bfd_elf_version_expr *) xmalloc (sizeof *ret);
  ret->next = orig;
  ret->symver = 0;
  ret->script = 0;
  ret->literal = TRUE;
  ret->pattern = literal_p ? new_name : realsymbol (new_name);
  if (ret->pattern == NULL)
    {
      ret->pattern = new_name;
      ret->literal = FALSE;
    }

  if (lang == NULL || strcasecmp (lang, "C") == 0)
    ret->mask = BFD_ELF_VERSION_C_TYPE;
  else if (strcasecmp (lang, "C++") == 0)
    ret->mask = BFD_ELF_VERSION_CXX_TYPE;
  else if (strcasecmp (lang, "Java") == 0)
    ret->mask = BFD_ELF_VERSION_JAVA_TYPE;
  else
    {
      einfo (_("%X%P: unknown language `%s' in version information\n"),
	     lang);
      ret->mask = BFD_ELF_VERSION_C_TYPE;
    }

  return ldemul_new_vers_pattern (ret);
}

/* This is called for each set of variable names and match
   expressions.  */

struct bfd_elf_version_tree *
lang_new_vers_node (struct bfd_elf_version_expr *globals,
		    struct bfd_elf_version_expr *locals)
{
  struct bfd_elf_version_tree *ret;

  ret = (struct bfd_elf_version_tree *) xcalloc (1, sizeof *ret);
  ret->globals.list = globals;
  ret->locals.list = locals;
  ret->match = lang_vers_match;
  ret->name_indx = (unsigned int) -1;
  return ret;
}

/* This static variable keeps track of version indices.  */

static int version_index;

static hashval_t
version_expr_head_hash (const void *p)
{
  const struct bfd_elf_version_expr *e =
      (const struct bfd_elf_version_expr *) p;

  return htab_hash_string (e->pattern);
}

static int
version_expr_head_eq (const void *p1, const void *p2)
{
  const struct bfd_elf_version_expr *e1 =
      (const struct bfd_elf_version_expr *) p1;
  const struct bfd_elf_version_expr *e2 =
      (const struct bfd_elf_version_expr *) p2;

  return strcmp (e1->pattern, e2->pattern) == 0;
}

static void
lang_finalize_version_expr_head (struct bfd_elf_version_expr_head *head)
{
  size_t count = 0;
  struct bfd_elf_version_expr *e, *next;
  struct bfd_elf_version_expr **list_loc, **remaining_loc;

  for (e = head->list; e; e = e->next)
    {
      if (e->literal)
	count++;
      head->mask |= e->mask;
    }

  if (count)
    {
      head->htab = htab_create (count * 2, version_expr_head_hash,
				version_expr_head_eq, NULL);
      list_loc = &head->list;
      remaining_loc = &head->remaining;
      for (e = head->list; e; e = next)
	{
	  next = e->next;
	  if (!e->literal)
	    {
	      *remaining_loc = e;
	      remaining_loc = &e->next;
	    }
	  else
	    {
	      void **loc = htab_find_slot ((htab_t) head->htab, e, INSERT);

	      if (*loc)
		{
		  struct bfd_elf_version_expr *e1, *last;

		  e1 = (struct bfd_elf_version_expr *) *loc;
		  last = NULL;
		  do
		    {
		      if (e1->mask == e->mask)
			{
			  last = NULL;
			  break;
			}
		      last = e1;
		      e1 = e1->next;
		    }
		  while (e1 && strcmp (e1->pattern, e->pattern) == 0);

		  if (last == NULL)
		    {
		      /* This is a duplicate.  */
		      /* FIXME: Memory leak.  Sometimes pattern is not
			 xmalloced alone, but in larger chunk of memory.  */
		      /* free (e->pattern); */
		      free (e);
		    }
		  else
		    {
		      e->next = last->next;
		      last->next = e;
		    }
		}
	      else
		{
		  *loc = e;
		  *list_loc = e;
		  list_loc = &e->next;
		}
	    }
	}
      *remaining_loc = NULL;
      *list_loc = head->remaining;
    }
  else
    head->remaining = head->list;
}

/* This is called when we know the name and dependencies of the
   version.  */

void
lang_register_vers_node (const char *name,
			 struct bfd_elf_version_tree *version,
			 struct bfd_elf_version_deps *deps)
{
  struct bfd_elf_version_tree *t, **pp;
  struct bfd_elf_version_expr *e1;

  if (name == NULL)
    name = "";

  if (link_info.version_info != NULL
      && (name[0] == '\0' || link_info.version_info->name[0] == '\0'))
    {
      einfo (_("%X%P: anonymous version tag cannot be combined"
	       " with other version tags\n"));
      free (version);
      return;
    }

  /* Make sure this node has a unique name.  */
  for (t = link_info.version_info; t != NULL; t = t->next)
    if (strcmp (t->name, name) == 0)
      einfo (_("%X%P: duplicate version tag `%s'\n"), name);

  lang_finalize_version_expr_head (&version->globals);
  lang_finalize_version_expr_head (&version->locals);

  /* Check the global and local match names, and make sure there
     aren't any duplicates.  */

  for (e1 = version->globals.list; e1 != NULL; e1 = e1->next)
    {
      for (t = link_info.version_info; t != NULL; t = t->next)
	{
	  struct bfd_elf_version_expr *e2;

	  if (t->locals.htab && e1->literal)
	    {
	      e2 = (struct bfd_elf_version_expr *)
		  htab_find ((htab_t) t->locals.htab, e1);
	      while (e2 && strcmp (e1->pattern, e2->pattern) == 0)
		{
		  if (e1->mask == e2->mask)
		    einfo (_("%X%P: duplicate expression `%s'"
			     " in version information\n"), e1->pattern);
		  e2 = e2->next;
		}
	    }
	  else if (!e1->literal)
	    for (e2 = t->locals.remaining; e2 != NULL; e2 = e2->next)
	      if (strcmp (e1->pattern, e2->pattern) == 0
		  && e1->mask == e2->mask)
		einfo (_("%X%P: duplicate expression `%s'"
			 " in version information\n"), e1->pattern);
	}
    }

  for (e1 = version->locals.list; e1 != NULL; e1 = e1->next)
    {
      for (t = link_info.version_info; t != NULL; t = t->next)
	{
	  struct bfd_elf_version_expr *e2;

	  if (t->globals.htab && e1->literal)
	    {
	      e2 = (struct bfd_elf_version_expr *)
		  htab_find ((htab_t) t->globals.htab, e1);
	      while (e2 && strcmp (e1->pattern, e2->pattern) == 0)
		{
		  if (e1->mask == e2->mask)
		    einfo (_("%X%P: duplicate expression `%s'"
			     " in version information\n"),
			   e1->pattern);
		  e2 = e2->next;
		}
	    }
	  else if (!e1->literal)
	    for (e2 = t->globals.remaining; e2 != NULL; e2 = e2->next)
	      if (strcmp (e1->pattern, e2->pattern) == 0
		  && e1->mask == e2->mask)
		einfo (_("%X%P: duplicate expression `%s'"
			 " in version information\n"), e1->pattern);
	}
    }

  version->deps = deps;
  version->name = name;
  if (name[0] != '\0')
    {
      ++version_index;
      version->vernum = version_index;
    }
  else
    version->vernum = 0;

  for (pp = &link_info.version_info; *pp != NULL; pp = &(*pp)->next)
    ;
  *pp = version;
}

/* This is called when we see a version dependency.  */

struct bfd_elf_version_deps *
lang_add_vers_depend (struct bfd_elf_version_deps *list, const char *name)
{
  struct bfd_elf_version_deps *ret;
  struct bfd_elf_version_tree *t;

  ret = (struct bfd_elf_version_deps *) xmalloc (sizeof *ret);
  ret->next = list;

  for (t = link_info.version_info; t != NULL; t = t->next)
    {
      if (strcmp (t->name, name) == 0)
	{
	  ret->version_needed = t;
	  return ret;
	}
    }

  einfo (_("%X%P: unable to find version dependency `%s'\n"), name);

  ret->version_needed = NULL;
  return ret;
}

static void
lang_do_version_exports_section (void)
{
  struct bfd_elf_version_expr *greg = NULL, *lreg;

  LANG_FOR_EACH_INPUT_STATEMENT (is)
    {
      asection *sec = bfd_get_section_by_name (is->the_bfd, ".exports");
      char *contents, *p;
      bfd_size_type len;

      if (sec == NULL)
	continue;

      len = sec->size;
      contents = (char *) xmalloc (len);
      if (!bfd_get_section_contents (is->the_bfd, sec, contents, 0, len))
	einfo (_("%X%P: unable to read .exports section contents\n"), sec);

      p = contents;
      while (p < contents + len)
	{
	  greg = lang_new_vers_pattern (greg, p, NULL, FALSE);
	  p = strchr (p, '\0') + 1;
	}

      /* Do not free the contents, as we used them creating the regex.  */

      /* Do not include this section in the link.  */
      sec->flags |= SEC_EXCLUDE | SEC_KEEP;
    }

  lreg = lang_new_vers_pattern (NULL, "*", NULL, FALSE);
  lang_register_vers_node (command_line.version_exports_section,
			   lang_new_vers_node (greg, lreg), NULL);
}

/* Evaluate LENGTH and ORIGIN parts of MEMORY spec */

static void
lang_do_memory_regions (void)
{
  lang_memory_region_type *r = lang_memory_region_list;

  for (; r != NULL; r = r->next)
    {
      if (r->origin_exp)
	{
	  exp_fold_tree_no_dot (r->origin_exp);
	  if (expld.result.valid_p)
	    {
	      r->origin = expld.result.value;
	      r->current = r->origin;
	    }
	  else
	    einfo (_("%F%P: invalid origin for memory region %s\n"),
		   r->name_list.name);
	}
      if (r->length_exp)
	{
	  exp_fold_tree_no_dot (r->length_exp);
	  if (expld.result.valid_p)
	    r->length = expld.result.value;
	  else
	    einfo (_("%F%P: invalid length for memory region %s\n"),
		   r->name_list.name);
	}
    }
}

void
lang_add_unique (const char *name)
{
  struct unique_sections *ent;

  for (ent = unique_section_list; ent; ent = ent->next)
    if (strcmp (ent->name, name) == 0)
      return;

  ent = (struct unique_sections *) xmalloc (sizeof *ent);
  ent->name = xstrdup (name);
  ent->next = unique_section_list;
  unique_section_list = ent;
}

/* Append the list of dynamic symbols to the existing one.  */

void
lang_append_dynamic_list (struct bfd_elf_version_expr *dynamic)
{
  if (link_info.dynamic_list)
    {
      struct bfd_elf_version_expr *tail;
      for (tail = dynamic; tail->next != NULL; tail = tail->next)
	;
      tail->next = link_info.dynamic_list->head.list;
      link_info.dynamic_list->head.list = dynamic;
    }
  else
    {
      struct bfd_elf_dynamic_list *d;

      d = (struct bfd_elf_dynamic_list *) xcalloc (1, sizeof *d);
      d->head.list = dynamic;
      d->match = lang_vers_match;
      link_info.dynamic_list = d;
    }
}

/* Append the list of C++ typeinfo dynamic symbols to the existing
   one.  */

void
lang_append_dynamic_list_cpp_typeinfo (void)
{
  const char *symbols[] =
    {
      "typeinfo name for*",
      "typeinfo for*"
    };
  struct bfd_elf_version_expr *dynamic = NULL;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (symbols); i++)
    dynamic = lang_new_vers_pattern (dynamic, symbols [i], "C++",
				     FALSE);

  lang_append_dynamic_list (dynamic);
}

/* Append the list of C++ operator new and delete dynamic symbols to the
   existing one.  */

void
lang_append_dynamic_list_cpp_new (void)
{
  const char *symbols[] =
    {
      "operator new*",
      "operator delete*"
    };
  struct bfd_elf_version_expr *dynamic = NULL;
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE (symbols); i++)
    dynamic = lang_new_vers_pattern (dynamic, symbols [i], "C++",
				     FALSE);

  lang_append_dynamic_list (dynamic);
}

/* Scan a space and/or comma separated string of features.  */

void
lang_ld_feature (char *str)
{
  char *p, *q;

  p = str;
  while (*p)
    {
      char sep;
      while (*p == ',' || ISSPACE (*p))
	++p;
      if (!*p)
	break;
      q = p + 1;
      while (*q && *q != ',' && !ISSPACE (*q))
	++q;
      sep = *q;
      *q = 0;
      if (strcasecmp (p, "SANE_EXPR") == 0)
	config.sane_expr = TRUE;
      else
	einfo (_("%X%P: unknown feature `%s'\n"), p);
      *q = sep;
      p = q;
    }
}

/* Pretty print memory amount.  */

static void
lang_print_memory_size (bfd_vma sz)
{
  if ((sz & 0x3fffffff) == 0)
    printf ("%10" BFD_VMA_FMT "u GB", sz >> 30);
  else if ((sz & 0xfffff) == 0)
    printf ("%10" BFD_VMA_FMT "u MB", sz >> 20);
  else if ((sz & 0x3ff) == 0)
    printf ("%10" BFD_VMA_FMT "u KB", sz >> 10);
  else
    printf (" %10" BFD_VMA_FMT "u B", sz);
}

/* Implement --print-memory-usage: disply per region memory usage.  */

void
lang_print_memory_usage (void)
{
  lang_memory_region_type *r;

  printf ("Memory region         Used Size  Region Size  %%age Used\n");
  for (r = lang_memory_region_list; r->next != NULL; r = r->next)
    {
      bfd_vma used_length = r->current - r->origin;

      printf ("%16s: ",r->name_list.name);
      lang_print_memory_size (used_length);
      lang_print_memory_size ((bfd_vma) r->length);

      if (r->length != 0)
	{
	  double percent = used_length * 100.0 / r->length;
	  printf ("    %6.2f%%", percent);
	}
      printf ("\n");
    }
}
