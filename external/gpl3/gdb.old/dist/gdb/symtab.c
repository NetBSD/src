/* Symbol table lookup for the GNU debugger, GDB.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "gdbtypes.h"
#include "gdbcore.h"
#include "frame.h"
#include "target.h"
#include "value.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "gdb_regex.h"
#include "expression.h"
#include "language.h"
#include "demangle.h"
#include "inferior.h"
#include "source.h"
#include "filenames.h"		/* for FILENAME_CMP */
#include "objc-lang.h"
#include "d-lang.h"
#include "ada-lang.h"
#include "go-lang.h"
#include "p-lang.h"
#include "addrmap.h"
#include "cli/cli-utils.h"
#include "cli/cli-style.h"
#include "fnmatch.h"
#include "hashtab.h"
#include "typeprint.h"

#include "gdb_obstack.h"
#include "block.h"
#include "dictionary.h"

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>
#include "cp-abi.h"
#include "cp-support.h"
#include "observable.h"
#include "solist.h"
#include "macrotab.h"
#include "macroscope.h"

#include "parser-defs.h"
#include "completer.h"
#include "progspace-and-thread.h"
#include "common/gdb_optional.h"
#include "filename-seen-cache.h"
#include "arch-utils.h"
#include <algorithm>
#include "common/pathstuff.h"

/* Forward declarations for local functions.  */

static void rbreak_command (const char *, int);

static int find_line_common (struct linetable *, int, int *, int);

static struct block_symbol
  lookup_symbol_aux (const char *name,
		     symbol_name_match_type match_type,
		     const struct block *block,
		     const domain_enum domain,
		     enum language language,
		     struct field_of_this_result *);

static
struct block_symbol lookup_local_symbol (const char *name,
					 symbol_name_match_type match_type,
					 const struct block *block,
					 const domain_enum domain,
					 enum language language);

static struct block_symbol
  lookup_symbol_in_objfile (struct objfile *objfile, int block_index,
			    const char *name, const domain_enum domain);

/* See symtab.h.  */
const struct block_symbol null_block_symbol = { NULL, NULL };

/* Program space key for finding name and language of "main".  */

static const struct program_space_data *main_progspace_key;

/* Type of the data stored on the program space.  */

struct main_info
{
  /* Name of "main".  */

  char *name_of_main;

  /* Language of "main".  */

  enum language language_of_main;
};

/* Program space key for finding its symbol cache.  */

static const struct program_space_data *symbol_cache_key;

/* The default symbol cache size.
   There is no extra cpu cost for large N (except when flushing the cache,
   which is rare).  The value here is just a first attempt.  A better default
   value may be higher or lower.  A prime number can make up for a bad hash
   computation, so that's why the number is what it is.  */
#define DEFAULT_SYMBOL_CACHE_SIZE 1021

/* The maximum symbol cache size.
   There's no method to the decision of what value to use here, other than
   there's no point in allowing a user typo to make gdb consume all memory.  */
#define MAX_SYMBOL_CACHE_SIZE (1024*1024)

/* symbol_cache_lookup returns this if a previous lookup failed to find the
   symbol in any objfile.  */
#define SYMBOL_LOOKUP_FAILED \
 ((struct block_symbol) {(struct symbol *) 1, NULL})
#define SYMBOL_LOOKUP_FAILED_P(SIB) (SIB.symbol == (struct symbol *) 1)

/* Recording lookups that don't find the symbol is just as important, if not
   more so, than recording found symbols.  */

enum symbol_cache_slot_state
{
  SYMBOL_SLOT_UNUSED,
  SYMBOL_SLOT_NOT_FOUND,
  SYMBOL_SLOT_FOUND
};

struct symbol_cache_slot
{
  enum symbol_cache_slot_state state;

  /* The objfile that was current when the symbol was looked up.
     This is only needed for global blocks, but for simplicity's sake
     we allocate the space for both.  If data shows the extra space used
     for static blocks is a problem, we can split things up then.

     Global blocks need cache lookup to include the objfile context because
     we need to account for gdbarch_iterate_over_objfiles_in_search_order
     which can traverse objfiles in, effectively, any order, depending on
     the current objfile, thus affecting which symbol is found.  Normally,
     only the current objfile is searched first, and then the rest are
     searched in recorded order; but putting cache lookup inside
     gdbarch_iterate_over_objfiles_in_search_order would be awkward.
     Instead we just make the current objfile part of the context of
     cache lookup.  This means we can record the same symbol multiple times,
     each with a different "current objfile" that was in effect when the
     lookup was saved in the cache, but cache space is pretty cheap.  */
  const struct objfile *objfile_context;

  union
  {
    struct block_symbol found;
    struct
    {
      char *name;
      domain_enum domain;
    } not_found;
  } value;
};

/* Symbols don't specify global vs static block.
   So keep them in separate caches.  */

struct block_symbol_cache
{
  unsigned int hits;
  unsigned int misses;
  unsigned int collisions;

  /* SYMBOLS is a variable length array of this size.
     One can imagine that in general one cache (global/static) should be a
     fraction of the size of the other, but there's no data at the moment
     on which to decide.  */
  unsigned int size;

  struct symbol_cache_slot symbols[1];
};

/* The symbol cache.

   Searching for symbols in the static and global blocks over multiple objfiles
   again and again can be slow, as can searching very big objfiles.  This is a
   simple cache to improve symbol lookup performance, which is critical to
   overall gdb performance.

   Symbols are hashed on the name, its domain, and block.
   They are also hashed on their objfile for objfile-specific lookups.  */

struct symbol_cache
{
  struct block_symbol_cache *global_symbols;
  struct block_symbol_cache *static_symbols;
};

/* When non-zero, print debugging messages related to symtab creation.  */
unsigned int symtab_create_debug = 0;

/* When non-zero, print debugging messages related to symbol lookup.  */
unsigned int symbol_lookup_debug = 0;

/* The size of the cache is staged here.  */
static unsigned int new_symbol_cache_size = DEFAULT_SYMBOL_CACHE_SIZE;

/* The current value of the symbol cache size.
   This is saved so that if the user enters a value too big we can restore
   the original value from here.  */
static unsigned int symbol_cache_size = DEFAULT_SYMBOL_CACHE_SIZE;

/* Non-zero if a file may be known by two different basenames.
   This is the uncommon case, and significantly slows down gdb.
   Default set to "off" to not slow down the common case.  */
int basenames_may_differ = 0;

/* Allow the user to configure the debugger behavior with respect
   to multiple-choice menus when more than one symbol matches during
   a symbol lookup.  */

const char multiple_symbols_ask[] = "ask";
const char multiple_symbols_all[] = "all";
const char multiple_symbols_cancel[] = "cancel";
static const char *const multiple_symbols_modes[] =
{
  multiple_symbols_ask,
  multiple_symbols_all,
  multiple_symbols_cancel,
  NULL
};
static const char *multiple_symbols_mode = multiple_symbols_all;

/* Read-only accessor to AUTO_SELECT_MODE.  */

const char *
multiple_symbols_select_mode (void)
{
  return multiple_symbols_mode;
}

/* Return the name of a domain_enum.  */

const char *
domain_name (domain_enum e)
{
  switch (e)
    {
    case UNDEF_DOMAIN: return "UNDEF_DOMAIN";
    case VAR_DOMAIN: return "VAR_DOMAIN";
    case STRUCT_DOMAIN: return "STRUCT_DOMAIN";
    case MODULE_DOMAIN: return "MODULE_DOMAIN";
    case LABEL_DOMAIN: return "LABEL_DOMAIN";
    case COMMON_BLOCK_DOMAIN: return "COMMON_BLOCK_DOMAIN";
    default: gdb_assert_not_reached ("bad domain_enum");
    }
}

/* Return the name of a search_domain .  */

const char *
search_domain_name (enum search_domain e)
{
  switch (e)
    {
    case VARIABLES_DOMAIN: return "VARIABLES_DOMAIN";
    case FUNCTIONS_DOMAIN: return "FUNCTIONS_DOMAIN";
    case TYPES_DOMAIN: return "TYPES_DOMAIN";
    case ALL_DOMAIN: return "ALL_DOMAIN";
    default: gdb_assert_not_reached ("bad search_domain");
    }
}

/* See symtab.h.  */

struct symtab *
compunit_primary_filetab (const struct compunit_symtab *cust)
{
  gdb_assert (COMPUNIT_FILETABS (cust) != NULL);

  /* The primary file symtab is the first one in the list.  */
  return COMPUNIT_FILETABS (cust);
}

/* See symtab.h.  */

enum language
compunit_language (const struct compunit_symtab *cust)
{
  struct symtab *symtab = compunit_primary_filetab (cust);

/* The language of the compunit symtab is the language of its primary
   source file.  */
  return SYMTAB_LANGUAGE (symtab);
}

/* See symtab.h.  */

bool
minimal_symbol::data_p () const
{
  return type == mst_data
    || type == mst_bss
    || type == mst_abs
    || type == mst_file_data
    || type == mst_file_bss;
}

/* See symtab.h.  */

bool
minimal_symbol::text_p () const
{
  return type == mst_text
    || type == mst_text_gnu_ifunc
    || type == mst_data_gnu_ifunc
    || type == mst_slot_got_plt
    || type == mst_solib_trampoline
    || type == mst_file_text;
}

/* See whether FILENAME matches SEARCH_NAME using the rule that we
   advertise to the user.  (The manual's description of linespecs
   describes what we advertise).  Returns true if they match, false
   otherwise.  */

int
compare_filenames_for_search (const char *filename, const char *search_name)
{
  int len = strlen (filename);
  size_t search_len = strlen (search_name);

  if (len < search_len)
    return 0;

  /* The tail of FILENAME must match.  */
  if (FILENAME_CMP (filename + len - search_len, search_name) != 0)
    return 0;

  /* Either the names must completely match, or the character
     preceding the trailing SEARCH_NAME segment of FILENAME must be a
     directory separator.

     The check !IS_ABSOLUTE_PATH ensures SEARCH_NAME "/dir/file.c"
     cannot match FILENAME "/path//dir/file.c" - as user has requested
     absolute path.  The sama applies for "c:\file.c" possibly
     incorrectly hypothetically matching "d:\dir\c:\file.c".

     The HAS_DRIVE_SPEC purpose is to make FILENAME "c:file.c"
     compatible with SEARCH_NAME "file.c".  In such case a compiler had
     to put the "c:file.c" name into debug info.  Such compatibility
     works only on GDB built for DOS host.  */
  return (len == search_len
	  || (!IS_ABSOLUTE_PATH (search_name)
	      && IS_DIR_SEPARATOR (filename[len - search_len - 1]))
	  || (HAS_DRIVE_SPEC (filename)
	      && STRIP_DRIVE_SPEC (filename) == &filename[len - search_len]));
}

/* Same as compare_filenames_for_search, but for glob-style patterns.
   Heads up on the order of the arguments.  They match the order of
   compare_filenames_for_search, but it's the opposite of the order of
   arguments to gdb_filename_fnmatch.  */

int
compare_glob_filenames_for_search (const char *filename,
				   const char *search_name)
{
  /* We rely on the property of glob-style patterns with FNM_FILE_NAME that
     all /s have to be explicitly specified.  */
  int file_path_elements = count_path_elements (filename);
  int search_path_elements = count_path_elements (search_name);

  if (search_path_elements > file_path_elements)
    return 0;

  if (IS_ABSOLUTE_PATH (search_name))
    {
      return (search_path_elements == file_path_elements
	      && gdb_filename_fnmatch (search_name, filename,
				       FNM_FILE_NAME | FNM_NOESCAPE) == 0);
    }

  {
    const char *file_to_compare
      = strip_leading_path_elements (filename,
				     file_path_elements - search_path_elements);

    return gdb_filename_fnmatch (search_name, file_to_compare,
				 FNM_FILE_NAME | FNM_NOESCAPE) == 0;
  }
}

/* Check for a symtab of a specific name by searching some symtabs.
   This is a helper function for callbacks of iterate_over_symtabs.

   If NAME is not absolute, then REAL_PATH is NULL
   If NAME is absolute, then REAL_PATH is the gdb_realpath form of NAME.

   The return value, NAME, REAL_PATH and CALLBACK are identical to the
   `map_symtabs_matching_filename' method of quick_symbol_functions.

   FIRST and AFTER_LAST indicate the range of compunit symtabs to search.
   Each symtab within the specified compunit symtab is also searched.
   AFTER_LAST is one past the last compunit symtab to search; NULL means to
   search until the end of the list.  */

bool
iterate_over_some_symtabs (const char *name,
			   const char *real_path,
			   struct compunit_symtab *first,
			   struct compunit_symtab *after_last,
			   gdb::function_view<bool (symtab *)> callback)
{
  struct compunit_symtab *cust;
  const char* base_name = lbasename (name);

  for (cust = first; cust != NULL && cust != after_last; cust = cust->next)
    {
      for (symtab *s : compunit_filetabs (cust))
	{
	  if (compare_filenames_for_search (s->filename, name))
	    {
	      if (callback (s))
		return true;
	      continue;
	    }

	  /* Before we invoke realpath, which can get expensive when many
	     files are involved, do a quick comparison of the basenames.  */
	  if (! basenames_may_differ
	      && FILENAME_CMP (base_name, lbasename (s->filename)) != 0)
	    continue;

	  if (compare_filenames_for_search (symtab_to_fullname (s), name))
	    {
	      if (callback (s))
		return true;
	      continue;
	    }

	  /* If the user gave us an absolute path, try to find the file in
	     this symtab and use its absolute path.  */
	  if (real_path != NULL)
	    {
	      const char *fullname = symtab_to_fullname (s);

	      gdb_assert (IS_ABSOLUTE_PATH (real_path));
	      gdb_assert (IS_ABSOLUTE_PATH (name));
	      if (FILENAME_CMP (real_path, fullname) == 0)
		{
		  if (callback (s))
		    return true;
		  continue;
		}
	    }
	}
    }

  return false;
}

/* Check for a symtab of a specific name; first in symtabs, then in
   psymtabs.  *If* there is no '/' in the name, a match after a '/'
   in the symtab filename will also work.

   Calls CALLBACK with each symtab that is found.  If CALLBACK returns
   true, the search stops.  */

void
iterate_over_symtabs (const char *name,
		      gdb::function_view<bool (symtab *)> callback)
{
  gdb::unique_xmalloc_ptr<char> real_path;

  /* Here we are interested in canonicalizing an absolute path, not
     absolutizing a relative path.  */
  if (IS_ABSOLUTE_PATH (name))
    {
      real_path = gdb_realpath (name);
      gdb_assert (IS_ABSOLUTE_PATH (real_path.get ()));
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (iterate_over_some_symtabs (name, real_path.get (),
				     objfile->compunit_symtabs, NULL,
				     callback))
	return;
    }

  /* Same search rules as above apply here, but now we look thru the
     psymtabs.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->sf
	  && objfile->sf->qf->map_symtabs_matching_filename (objfile,
							     name,
							     real_path.get (),
							     callback))
	return;
    }
}

/* A wrapper for iterate_over_symtabs that returns the first matching
   symtab, or NULL.  */

struct symtab *
lookup_symtab (const char *name)
{
  struct symtab *result = NULL;

  iterate_over_symtabs (name, [&] (symtab *symtab)
    {
      result = symtab;
      return true;
    });

  return result;
}


/* Mangle a GDB method stub type.  This actually reassembles the pieces of the
   full method name, which consist of the class name (from T), the unadorned
   method name from METHOD_ID, and the signature for the specific overload,
   specified by SIGNATURE_ID.  Note that this function is g++ specific.  */

char *
gdb_mangle_name (struct type *type, int method_id, int signature_id)
{
  int mangled_name_len;
  char *mangled_name;
  struct fn_field *f = TYPE_FN_FIELDLIST1 (type, method_id);
  struct fn_field *method = &f[signature_id];
  const char *field_name = TYPE_FN_FIELDLIST_NAME (type, method_id);
  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, signature_id);
  const char *newname = TYPE_NAME (type);

  /* Does the form of physname indicate that it is the full mangled name
     of a constructor (not just the args)?  */
  int is_full_physname_constructor;

  int is_constructor;
  int is_destructor = is_destructor_name (physname);
  /* Need a new type prefix.  */
  const char *const_prefix = method->is_const ? "C" : "";
  const char *volatile_prefix = method->is_volatile ? "V" : "";
  char buf[20];
  int len = (newname == NULL ? 0 : strlen (newname));

  /* Nothing to do if physname already contains a fully mangled v3 abi name
     or an operator name.  */
  if ((physname[0] == '_' && physname[1] == 'Z')
      || is_operator_name (field_name))
    return xstrdup (physname);

  is_full_physname_constructor = is_constructor_name (physname);

  is_constructor = is_full_physname_constructor 
    || (newname && strcmp (field_name, newname) == 0);

  if (!is_destructor)
    is_destructor = (startswith (physname, "__dt"));

  if (is_destructor || is_full_physname_constructor)
    {
      mangled_name = (char *) xmalloc (strlen (physname) + 1);
      strcpy (mangled_name, physname);
      return mangled_name;
    }

  if (len == 0)
    {
      xsnprintf (buf, sizeof (buf), "__%s%s", const_prefix, volatile_prefix);
    }
  else if (physname[0] == 't' || physname[0] == 'Q')
    {
      /* The physname for template and qualified methods already includes
         the class name.  */
      xsnprintf (buf, sizeof (buf), "__%s%s", const_prefix, volatile_prefix);
      newname = NULL;
      len = 0;
    }
  else
    {
      xsnprintf (buf, sizeof (buf), "__%s%s%d", const_prefix,
		 volatile_prefix, len);
    }
  mangled_name_len = ((is_constructor ? 0 : strlen (field_name))
		      + strlen (buf) + len + strlen (physname) + 1);

  mangled_name = (char *) xmalloc (mangled_name_len);
  if (is_constructor)
    mangled_name[0] = '\0';
  else
    strcpy (mangled_name, field_name);

  strcat (mangled_name, buf);
  /* If the class doesn't have a name, i.e. newname NULL, then we just
     mangle it using 0 for the length of the class.  Thus it gets mangled
     as something starting with `::' rather than `classname::'.  */
  if (newname != NULL)
    strcat (mangled_name, newname);

  strcat (mangled_name, physname);
  return (mangled_name);
}

/* Set the demangled name of GSYMBOL to NAME.  NAME must be already
   correctly allocated.  */

void
symbol_set_demangled_name (struct general_symbol_info *gsymbol,
                           const char *name,
                           struct obstack *obstack)
{
  if (gsymbol->language == language_ada)
    {
      if (name == NULL)
	{
	  gsymbol->ada_mangled = 0;
	  gsymbol->language_specific.obstack = obstack;
	}
      else
	{
	  gsymbol->ada_mangled = 1;
	  gsymbol->language_specific.demangled_name = name;
	}
    }
  else
    gsymbol->language_specific.demangled_name = name;
}

/* Return the demangled name of GSYMBOL.  */

const char *
symbol_get_demangled_name (const struct general_symbol_info *gsymbol)
{
  if (gsymbol->language == language_ada)
    {
      if (!gsymbol->ada_mangled)
	return NULL;
      /* Fall through.  */
    }

  return gsymbol->language_specific.demangled_name;
}


/* Initialize the language dependent portion of a symbol
   depending upon the language for the symbol.  */

void
symbol_set_language (struct general_symbol_info *gsymbol,
                     enum language language,
		     struct obstack *obstack)
{
  gsymbol->language = language;
  if (gsymbol->language == language_cplus
      || gsymbol->language == language_d
      || gsymbol->language == language_go
      || gsymbol->language == language_objc
      || gsymbol->language == language_fortran)
    {
      symbol_set_demangled_name (gsymbol, NULL, obstack);
    }
  else if (gsymbol->language == language_ada)
    {
      gdb_assert (gsymbol->ada_mangled == 0);
      gsymbol->language_specific.obstack = obstack;
    }
  else
    {
      memset (&gsymbol->language_specific, 0,
	      sizeof (gsymbol->language_specific));
    }
}

/* Functions to initialize a symbol's mangled name.  */

/* Objects of this type are stored in the demangled name hash table.  */
struct demangled_name_entry
{
  const char *mangled;
  char demangled[1];
};

/* Hash function for the demangled name hash.  */

static hashval_t
hash_demangled_name_entry (const void *data)
{
  const struct demangled_name_entry *e
    = (const struct demangled_name_entry *) data;

  return htab_hash_string (e->mangled);
}

/* Equality function for the demangled name hash.  */

static int
eq_demangled_name_entry (const void *a, const void *b)
{
  const struct demangled_name_entry *da
    = (const struct demangled_name_entry *) a;
  const struct demangled_name_entry *db
    = (const struct demangled_name_entry *) b;

  return strcmp (da->mangled, db->mangled) == 0;
}

/* Create the hash table used for demangled names.  Each hash entry is
   a pair of strings; one for the mangled name and one for the demangled
   name.  The entry is hashed via just the mangled name.  */

static void
create_demangled_names_hash (struct objfile_per_bfd_storage *per_bfd)
{
  /* Choose 256 as the starting size of the hash table, somewhat arbitrarily.
     The hash table code will round this up to the next prime number.
     Choosing a much larger table size wastes memory, and saves only about
     1% in symbol reading.  */

  per_bfd->demangled_names_hash = htab_create_alloc
    (256, hash_demangled_name_entry, eq_demangled_name_entry,
     NULL, xcalloc, xfree);
}

/* Try to determine the demangled name for a symbol, based on the
   language of that symbol.  If the language is set to language_auto,
   it will attempt to find any demangling algorithm that works and
   then set the language appropriately.  The returned name is allocated
   by the demangler and should be xfree'd.  */

static char *
symbol_find_demangled_name (struct general_symbol_info *gsymbol,
			    const char *mangled)
{
  char *demangled = NULL;
  int i;

  if (gsymbol->language == language_unknown)
    gsymbol->language = language_auto;

  if (gsymbol->language != language_auto)
    {
      const struct language_defn *lang = language_def (gsymbol->language);

      language_sniff_from_mangled_name (lang, mangled, &demangled);
      return demangled;
    }

  for (i = language_unknown; i < nr_languages; ++i)
    {
      enum language l = (enum language) i;
      const struct language_defn *lang = language_def (l);

      if (language_sniff_from_mangled_name (lang, mangled, &demangled))
	{
	  gsymbol->language = l;
	  return demangled;
	}
    }

  return NULL;
}

/* Set both the mangled and demangled (if any) names for GSYMBOL based
   on LINKAGE_NAME and LEN.  Ordinarily, NAME is copied onto the
   objfile's obstack; but if COPY_NAME is 0 and if NAME is
   NUL-terminated, then this function assumes that NAME is already
   correctly saved (either permanently or with a lifetime tied to the
   objfile), and it will not be copied.

   The hash table corresponding to OBJFILE is used, and the memory
   comes from the per-BFD storage_obstack.  LINKAGE_NAME is copied,
   so the pointer can be discarded after calling this function.  */

void
symbol_set_names (struct general_symbol_info *gsymbol,
		  const char *linkage_name, int len, int copy_name,
		  struct objfile_per_bfd_storage *per_bfd)
{
  struct demangled_name_entry **slot;
  /* A 0-terminated copy of the linkage name.  */
  const char *linkage_name_copy;
  struct demangled_name_entry entry;

  if (gsymbol->language == language_ada)
    {
      /* In Ada, we do the symbol lookups using the mangled name, so
         we can save some space by not storing the demangled name.  */
      if (!copy_name)
	gsymbol->name = linkage_name;
      else
	{
	  char *name = (char *) obstack_alloc (&per_bfd->storage_obstack,
					       len + 1);

	  memcpy (name, linkage_name, len);
	  name[len] = '\0';
	  gsymbol->name = name;
	}
      symbol_set_demangled_name (gsymbol, NULL, &per_bfd->storage_obstack);

      return;
    }

  if (per_bfd->demangled_names_hash == NULL)
    create_demangled_names_hash (per_bfd);

  if (linkage_name[len] != '\0')
    {
      char *alloc_name;

      alloc_name = (char *) alloca (len + 1);
      memcpy (alloc_name, linkage_name, len);
      alloc_name[len] = '\0';

      linkage_name_copy = alloc_name;
    }
  else
    linkage_name_copy = linkage_name;

  /* Set the symbol language.  */
  char *demangled_name_ptr
    = symbol_find_demangled_name (gsymbol, linkage_name_copy);
  gdb::unique_xmalloc_ptr<char> demangled_name (demangled_name_ptr);

  entry.mangled = linkage_name_copy;
  slot = ((struct demangled_name_entry **)
	  htab_find_slot (per_bfd->demangled_names_hash,
			  &entry, INSERT));

  /* If this name is not in the hash table, add it.  */
  if (*slot == NULL
      /* A C version of the symbol may have already snuck into the table.
	 This happens to, e.g., main.init (__go_init_main).  Cope.  */
      || (gsymbol->language == language_go
	  && (*slot)->demangled[0] == '\0'))
    {
      int demangled_len = demangled_name ? strlen (demangled_name.get ()) : 0;

      /* Suppose we have demangled_name==NULL, copy_name==0, and
	 linkage_name_copy==linkage_name.  In this case, we already have the
	 mangled name saved, and we don't have a demangled name.  So,
	 you might think we could save a little space by not recording
	 this in the hash table at all.
	 
	 It turns out that it is actually important to still save such
	 an entry in the hash table, because storing this name gives
	 us better bcache hit rates for partial symbols.  */
      if (!copy_name && linkage_name_copy == linkage_name)
	{
	  *slot
	    = ((struct demangled_name_entry *)
	       obstack_alloc (&per_bfd->storage_obstack,
			      offsetof (struct demangled_name_entry, demangled)
			      + demangled_len + 1));
	  (*slot)->mangled = linkage_name;
	}
      else
	{
	  char *mangled_ptr;

	  /* If we must copy the mangled name, put it directly after
	     the demangled name so we can have a single
	     allocation.  */
	  *slot
	    = ((struct demangled_name_entry *)
	       obstack_alloc (&per_bfd->storage_obstack,
			      offsetof (struct demangled_name_entry, demangled)
			      + len + demangled_len + 2));
	  mangled_ptr = &((*slot)->demangled[demangled_len + 1]);
	  strcpy (mangled_ptr, linkage_name_copy);
	  (*slot)->mangled = mangled_ptr;
	}

      if (demangled_name != NULL)
	strcpy ((*slot)->demangled, demangled_name.get());
      else
	(*slot)->demangled[0] = '\0';
    }

  gsymbol->name = (*slot)->mangled;
  if ((*slot)->demangled[0] != '\0')
    symbol_set_demangled_name (gsymbol, (*slot)->demangled,
			       &per_bfd->storage_obstack);
  else
    symbol_set_demangled_name (gsymbol, NULL, &per_bfd->storage_obstack);
}

/* Return the source code name of a symbol.  In languages where
   demangling is necessary, this is the demangled name.  */

const char *
symbol_natural_name (const struct general_symbol_info *gsymbol)
{
  switch (gsymbol->language)
    {
    case language_cplus:
    case language_d:
    case language_go:
    case language_objc:
    case language_fortran:
      if (symbol_get_demangled_name (gsymbol) != NULL)
	return symbol_get_demangled_name (gsymbol);
      break;
    case language_ada:
      return ada_decode_symbol (gsymbol);
    default:
      break;
    }
  return gsymbol->name;
}

/* Return the demangled name for a symbol based on the language for
   that symbol.  If no demangled name exists, return NULL.  */

const char *
symbol_demangled_name (const struct general_symbol_info *gsymbol)
{
  const char *dem_name = NULL;

  switch (gsymbol->language)
    {
    case language_cplus:
    case language_d:
    case language_go:
    case language_objc:
    case language_fortran:
      dem_name = symbol_get_demangled_name (gsymbol);
      break;
    case language_ada:
      dem_name = ada_decode_symbol (gsymbol);
      break;
    default:
      break;
    }
  return dem_name;
}

/* Return the search name of a symbol---generally the demangled or
   linkage name of the symbol, depending on how it will be searched for.
   If there is no distinct demangled name, then returns the same value
   (same pointer) as SYMBOL_LINKAGE_NAME.  */

const char *
symbol_search_name (const struct general_symbol_info *gsymbol)
{
  if (gsymbol->language == language_ada)
    return gsymbol->name;
  else
    return symbol_natural_name (gsymbol);
}

/* See symtab.h.  */

bool
symbol_matches_search_name (const struct general_symbol_info *gsymbol,
			    const lookup_name_info &name)
{
  symbol_name_matcher_ftype *name_match
    = get_symbol_name_matcher (language_def (gsymbol->language), name);
  return name_match (symbol_search_name (gsymbol), name, NULL);
}



/* Return 1 if the two sections are the same, or if they could
   plausibly be copies of each other, one in an original object
   file and another in a separated debug file.  */

int
matching_obj_sections (struct obj_section *obj_first,
		       struct obj_section *obj_second)
{
  asection *first = obj_first? obj_first->the_bfd_section : NULL;
  asection *second = obj_second? obj_second->the_bfd_section : NULL;

  /* If they're the same section, then they match.  */
  if (first == second)
    return 1;

  /* If either is NULL, give up.  */
  if (first == NULL || second == NULL)
    return 0;

  /* This doesn't apply to absolute symbols.  */
  if (first->owner == NULL || second->owner == NULL)
    return 0;

  /* If they're in the same object file, they must be different sections.  */
  if (first->owner == second->owner)
    return 0;

  /* Check whether the two sections are potentially corresponding.  They must
     have the same size, address, and name.  We can't compare section indexes,
     which would be more reliable, because some sections may have been
     stripped.  */
  if (bfd_get_section_size (first) != bfd_get_section_size (second))
    return 0;

  /* In-memory addresses may start at a different offset, relativize them.  */
  if (bfd_get_section_vma (first->owner, first)
      - bfd_get_start_address (first->owner)
      != bfd_get_section_vma (second->owner, second)
	 - bfd_get_start_address (second->owner))
    return 0;

  if (bfd_get_section_name (first->owner, first) == NULL
      || bfd_get_section_name (second->owner, second) == NULL
      || strcmp (bfd_get_section_name (first->owner, first),
		 bfd_get_section_name (second->owner, second)) != 0)
    return 0;

  /* Otherwise check that they are in corresponding objfiles.  */

  struct objfile *obj = NULL;
  for (objfile *objfile : current_program_space->objfiles ())
    if (objfile->obfd == first->owner)
      {
	obj = objfile;
	break;
      }
  gdb_assert (obj != NULL);

  if (obj->separate_debug_objfile != NULL
      && obj->separate_debug_objfile->obfd == second->owner)
    return 1;
  if (obj->separate_debug_objfile_backlink != NULL
      && obj->separate_debug_objfile_backlink->obfd == second->owner)
    return 1;

  return 0;
}

/* See symtab.h.  */

void
expand_symtab_containing_pc (CORE_ADDR pc, struct obj_section *section)
{
  struct bound_minimal_symbol msymbol;

  /* If we know that this is not a text address, return failure.  This is
     necessary because we loop based on texthigh and textlow, which do
     not include the data ranges.  */
  msymbol = lookup_minimal_symbol_by_pc_section (pc, section);
  if (msymbol.minsym && msymbol.minsym->data_p ())
    return;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      struct compunit_symtab *cust = NULL;

      if (objfile->sf)
	cust = objfile->sf->qf->find_pc_sect_compunit_symtab (objfile, msymbol,
							      pc, section, 0);
      if (cust)
	return;
    }
}

/* Hash function for the symbol cache.  */

static unsigned int
hash_symbol_entry (const struct objfile *objfile_context,
		   const char *name, domain_enum domain)
{
  unsigned int hash = (uintptr_t) objfile_context;

  if (name != NULL)
    hash += htab_hash_string (name);

  /* Because of symbol_matches_domain we need VAR_DOMAIN and STRUCT_DOMAIN
     to map to the same slot.  */
  if (domain == STRUCT_DOMAIN)
    hash += VAR_DOMAIN * 7;
  else
    hash += domain * 7;

  return hash;
}

/* Equality function for the symbol cache.  */

static int
eq_symbol_entry (const struct symbol_cache_slot *slot,
		 const struct objfile *objfile_context,
		 const char *name, domain_enum domain)
{
  const char *slot_name;
  domain_enum slot_domain;

  if (slot->state == SYMBOL_SLOT_UNUSED)
    return 0;

  if (slot->objfile_context != objfile_context)
    return 0;

  if (slot->state == SYMBOL_SLOT_NOT_FOUND)
    {
      slot_name = slot->value.not_found.name;
      slot_domain = slot->value.not_found.domain;
    }
  else
    {
      slot_name = SYMBOL_SEARCH_NAME (slot->value.found.symbol);
      slot_domain = SYMBOL_DOMAIN (slot->value.found.symbol);
    }

  /* NULL names match.  */
  if (slot_name == NULL && name == NULL)
    {
      /* But there's no point in calling symbol_matches_domain in the
	 SYMBOL_SLOT_FOUND case.  */
      if (slot_domain != domain)
	return 0;
    }
  else if (slot_name != NULL && name != NULL)
    {
      /* It's important that we use the same comparison that was done
	 the first time through.  If the slot records a found symbol,
	 then this means using the symbol name comparison function of
	 the symbol's language with SYMBOL_SEARCH_NAME.  See
	 dictionary.c.  It also means using symbol_matches_domain for
	 found symbols.  See block.c.

	 If the slot records a not-found symbol, then require a precise match.
	 We could still be lax with whitespace like strcmp_iw though.  */

      if (slot->state == SYMBOL_SLOT_NOT_FOUND)
	{
	  if (strcmp (slot_name, name) != 0)
	    return 0;
	  if (slot_domain != domain)
	    return 0;
	}
      else
	{
	  struct symbol *sym = slot->value.found.symbol;
	  lookup_name_info lookup_name (name, symbol_name_match_type::FULL);

	  if (!SYMBOL_MATCHES_SEARCH_NAME (sym, lookup_name))
	    return 0;

	  if (!symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				      slot_domain, domain))
	    return 0;
	}
    }
  else
    {
      /* Only one name is NULL.  */
      return 0;
    }

  return 1;
}

/* Given a cache of size SIZE, return the size of the struct (with variable
   length array) in bytes.  */

static size_t
symbol_cache_byte_size (unsigned int size)
{
  return (sizeof (struct block_symbol_cache)
	  + ((size - 1) * sizeof (struct symbol_cache_slot)));
}

/* Resize CACHE.  */

static void
resize_symbol_cache (struct symbol_cache *cache, unsigned int new_size)
{
  /* If there's no change in size, don't do anything.
     All caches have the same size, so we can just compare with the size
     of the global symbols cache.  */
  if ((cache->global_symbols != NULL
       && cache->global_symbols->size == new_size)
      || (cache->global_symbols == NULL
	  && new_size == 0))
    return;

  xfree (cache->global_symbols);
  xfree (cache->static_symbols);

  if (new_size == 0)
    {
      cache->global_symbols = NULL;
      cache->static_symbols = NULL;
    }
  else
    {
      size_t total_size = symbol_cache_byte_size (new_size);

      cache->global_symbols
	= (struct block_symbol_cache *) xcalloc (1, total_size);
      cache->static_symbols
	= (struct block_symbol_cache *) xcalloc (1, total_size);
      cache->global_symbols->size = new_size;
      cache->static_symbols->size = new_size;
    }
}

/* Make a symbol cache of size SIZE.  */

static struct symbol_cache *
make_symbol_cache (unsigned int size)
{
  struct symbol_cache *cache;

  cache = XCNEW (struct symbol_cache);
  resize_symbol_cache (cache, symbol_cache_size);
  return cache;
}

/* Free the space used by CACHE.  */

static void
free_symbol_cache (struct symbol_cache *cache)
{
  xfree (cache->global_symbols);
  xfree (cache->static_symbols);
  xfree (cache);
}

/* Return the symbol cache of PSPACE.
   Create one if it doesn't exist yet.  */

static struct symbol_cache *
get_symbol_cache (struct program_space *pspace)
{
  struct symbol_cache *cache
    = (struct symbol_cache *) program_space_data (pspace, symbol_cache_key);

  if (cache == NULL)
    {
      cache = make_symbol_cache (symbol_cache_size);
      set_program_space_data (pspace, symbol_cache_key, cache);
    }

  return cache;
}

/* Delete the symbol cache of PSPACE.
   Called when PSPACE is destroyed.  */

static void
symbol_cache_cleanup (struct program_space *pspace, void *data)
{
  struct symbol_cache *cache = (struct symbol_cache *) data;

  free_symbol_cache (cache);
}

/* Set the size of the symbol cache in all program spaces.  */

static void
set_symbol_cache_size (unsigned int new_size)
{
  struct program_space *pspace;

  ALL_PSPACES (pspace)
    {
      struct symbol_cache *cache
	= (struct symbol_cache *) program_space_data (pspace, symbol_cache_key);

      /* The pspace could have been created but not have a cache yet.  */
      if (cache != NULL)
	resize_symbol_cache (cache, new_size);
    }
}

/* Called when symbol-cache-size is set.  */

static void
set_symbol_cache_size_handler (const char *args, int from_tty,
			       struct cmd_list_element *c)
{
  if (new_symbol_cache_size > MAX_SYMBOL_CACHE_SIZE)
    {
      /* Restore the previous value.
	 This is the value the "show" command prints.  */
      new_symbol_cache_size = symbol_cache_size;

      error (_("Symbol cache size is too large, max is %u."),
	     MAX_SYMBOL_CACHE_SIZE);
    }
  symbol_cache_size = new_symbol_cache_size;

  set_symbol_cache_size (symbol_cache_size);
}

/* Lookup symbol NAME,DOMAIN in BLOCK in the symbol cache of PSPACE.
   OBJFILE_CONTEXT is the current objfile, which may be NULL.
   The result is the symbol if found, SYMBOL_LOOKUP_FAILED if a previous lookup
   failed (and thus this one will too), or NULL if the symbol is not present
   in the cache.
   If the symbol is not present in the cache, then *BSC_PTR and *SLOT_PTR are
   set to the cache and slot of the symbol to save the result of a full lookup
   attempt.  */

static struct block_symbol
symbol_cache_lookup (struct symbol_cache *cache,
		     struct objfile *objfile_context, int block,
		     const char *name, domain_enum domain,
		     struct block_symbol_cache **bsc_ptr,
		     struct symbol_cache_slot **slot_ptr)
{
  struct block_symbol_cache *bsc;
  unsigned int hash;
  struct symbol_cache_slot *slot;

  if (block == GLOBAL_BLOCK)
    bsc = cache->global_symbols;
  else
    bsc = cache->static_symbols;
  if (bsc == NULL)
    {
      *bsc_ptr = NULL;
      *slot_ptr = NULL;
      return (struct block_symbol) {NULL, NULL};
    }

  hash = hash_symbol_entry (objfile_context, name, domain);
  slot = bsc->symbols + hash % bsc->size;

  if (eq_symbol_entry (slot, objfile_context, name, domain))
    {
      if (symbol_lookup_debug)
	fprintf_unfiltered (gdb_stdlog,
			    "%s block symbol cache hit%s for %s, %s\n",
			    block == GLOBAL_BLOCK ? "Global" : "Static",
			    slot->state == SYMBOL_SLOT_NOT_FOUND
			    ? " (not found)" : "",
			    name, domain_name (domain));
      ++bsc->hits;
      if (slot->state == SYMBOL_SLOT_NOT_FOUND)
	return SYMBOL_LOOKUP_FAILED;
      return slot->value.found;
    }

  /* Symbol is not present in the cache.  */

  *bsc_ptr = bsc;
  *slot_ptr = slot;

  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "%s block symbol cache miss for %s, %s\n",
			  block == GLOBAL_BLOCK ? "Global" : "Static",
			  name, domain_name (domain));
    }
  ++bsc->misses;
  return (struct block_symbol) {NULL, NULL};
}

/* Clear out SLOT.  */

static void
symbol_cache_clear_slot (struct symbol_cache_slot *slot)
{
  if (slot->state == SYMBOL_SLOT_NOT_FOUND)
    xfree (slot->value.not_found.name);
  slot->state = SYMBOL_SLOT_UNUSED;
}

/* Mark SYMBOL as found in SLOT.
   OBJFILE_CONTEXT is the current objfile when the lookup was done, or NULL
   if it's not needed to distinguish lookups (STATIC_BLOCK).  It is *not*
   necessarily the objfile the symbol was found in.  */

static void
symbol_cache_mark_found (struct block_symbol_cache *bsc,
			 struct symbol_cache_slot *slot,
			 struct objfile *objfile_context,
			 struct symbol *symbol,
			 const struct block *block)
{
  if (bsc == NULL)
    return;
  if (slot->state != SYMBOL_SLOT_UNUSED)
    {
      ++bsc->collisions;
      symbol_cache_clear_slot (slot);
    }
  slot->state = SYMBOL_SLOT_FOUND;
  slot->objfile_context = objfile_context;
  slot->value.found.symbol = symbol;
  slot->value.found.block = block;
}

/* Mark symbol NAME, DOMAIN as not found in SLOT.
   OBJFILE_CONTEXT is the current objfile when the lookup was done, or NULL
   if it's not needed to distinguish lookups (STATIC_BLOCK).  */

static void
symbol_cache_mark_not_found (struct block_symbol_cache *bsc,
			     struct symbol_cache_slot *slot,
			     struct objfile *objfile_context,
			     const char *name, domain_enum domain)
{
  if (bsc == NULL)
    return;
  if (slot->state != SYMBOL_SLOT_UNUSED)
    {
      ++bsc->collisions;
      symbol_cache_clear_slot (slot);
    }
  slot->state = SYMBOL_SLOT_NOT_FOUND;
  slot->objfile_context = objfile_context;
  slot->value.not_found.name = xstrdup (name);
  slot->value.not_found.domain = domain;
}

/* Flush the symbol cache of PSPACE.  */

static void
symbol_cache_flush (struct program_space *pspace)
{
  struct symbol_cache *cache
    = (struct symbol_cache *) program_space_data (pspace, symbol_cache_key);
  int pass;

  if (cache == NULL)
    return;
  if (cache->global_symbols == NULL)
    {
      gdb_assert (symbol_cache_size == 0);
      gdb_assert (cache->static_symbols == NULL);
      return;
    }

  /* If the cache is untouched since the last flush, early exit.
     This is important for performance during the startup of a program linked
     with 100s (or 1000s) of shared libraries.  */
  if (cache->global_symbols->misses == 0
      && cache->static_symbols->misses == 0)
    return;

  gdb_assert (cache->global_symbols->size == symbol_cache_size);
  gdb_assert (cache->static_symbols->size == symbol_cache_size);

  for (pass = 0; pass < 2; ++pass)
    {
      struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;
      unsigned int i;

      for (i = 0; i < bsc->size; ++i)
	symbol_cache_clear_slot (&bsc->symbols[i]);
    }

  cache->global_symbols->hits = 0;
  cache->global_symbols->misses = 0;
  cache->global_symbols->collisions = 0;
  cache->static_symbols->hits = 0;
  cache->static_symbols->misses = 0;
  cache->static_symbols->collisions = 0;
}

/* Dump CACHE.  */

static void
symbol_cache_dump (const struct symbol_cache *cache)
{
  int pass;

  if (cache->global_symbols == NULL)
    {
      printf_filtered ("  <disabled>\n");
      return;
    }

  for (pass = 0; pass < 2; ++pass)
    {
      const struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;
      unsigned int i;

      if (pass == 0)
	printf_filtered ("Global symbols:\n");
      else
	printf_filtered ("Static symbols:\n");

      for (i = 0; i < bsc->size; ++i)
	{
	  const struct symbol_cache_slot *slot = &bsc->symbols[i];

	  QUIT;

	  switch (slot->state)
	    {
	    case SYMBOL_SLOT_UNUSED:
	      break;
	    case SYMBOL_SLOT_NOT_FOUND:
	      printf_filtered ("  [%4u] = %s, %s %s (not found)\n", i,
			       host_address_to_string (slot->objfile_context),
			       slot->value.not_found.name,
			       domain_name (slot->value.not_found.domain));
	      break;
	    case SYMBOL_SLOT_FOUND:
	      {
		struct symbol *found = slot->value.found.symbol;
		const struct objfile *context = slot->objfile_context;

		printf_filtered ("  [%4u] = %s, %s %s\n", i,
				 host_address_to_string (context),
				 SYMBOL_PRINT_NAME (found),
				 domain_name (SYMBOL_DOMAIN (found)));
		break;
	      }
	    }
	}
    }
}

/* The "mt print symbol-cache" command.  */

static void
maintenance_print_symbol_cache (const char *args, int from_tty)
{
  struct program_space *pspace;

  ALL_PSPACES (pspace)
    {
      struct symbol_cache *cache;

      printf_filtered (_("Symbol cache for pspace %d\n%s:\n"),
		       pspace->num,
		       pspace->symfile_object_file != NULL
		       ? objfile_name (pspace->symfile_object_file)
		       : "(no object file)");

      /* If the cache hasn't been created yet, avoid creating one.  */
      cache
	= (struct symbol_cache *) program_space_data (pspace, symbol_cache_key);
      if (cache == NULL)
	printf_filtered ("  <empty>\n");
      else
	symbol_cache_dump (cache);
    }
}

/* The "mt flush-symbol-cache" command.  */

static void
maintenance_flush_symbol_cache (const char *args, int from_tty)
{
  struct program_space *pspace;

  ALL_PSPACES (pspace)
    {
      symbol_cache_flush (pspace);
    }
}

/* Print usage statistics of CACHE.  */

static void
symbol_cache_stats (struct symbol_cache *cache)
{
  int pass;

  if (cache->global_symbols == NULL)
    {
      printf_filtered ("  <disabled>\n");
      return;
    }

  for (pass = 0; pass < 2; ++pass)
    {
      const struct block_symbol_cache *bsc
	= pass == 0 ? cache->global_symbols : cache->static_symbols;

      QUIT;

      if (pass == 0)
	printf_filtered ("Global block cache stats:\n");
      else
	printf_filtered ("Static block cache stats:\n");

      printf_filtered ("  size:       %u\n", bsc->size);
      printf_filtered ("  hits:       %u\n", bsc->hits);
      printf_filtered ("  misses:     %u\n", bsc->misses);
      printf_filtered ("  collisions: %u\n", bsc->collisions);
    }
}

/* The "mt print symbol-cache-statistics" command.  */

static void
maintenance_print_symbol_cache_statistics (const char *args, int from_tty)
{
  struct program_space *pspace;

  ALL_PSPACES (pspace)
    {
      struct symbol_cache *cache;

      printf_filtered (_("Symbol cache statistics for pspace %d\n%s:\n"),
		       pspace->num,
		       pspace->symfile_object_file != NULL
		       ? objfile_name (pspace->symfile_object_file)
		       : "(no object file)");

      /* If the cache hasn't been created yet, avoid creating one.  */
      cache
	= (struct symbol_cache *) program_space_data (pspace, symbol_cache_key);
      if (cache == NULL)
 	printf_filtered ("  empty, no stats available\n");
      else
	symbol_cache_stats (cache);
    }
}

/* This module's 'new_objfile' observer.  */

static void
symtab_new_objfile_observer (struct objfile *objfile)
{
  /* Ideally we'd use OBJFILE->pspace, but OBJFILE may be NULL.  */
  symbol_cache_flush (current_program_space);
}

/* This module's 'free_objfile' observer.  */

static void
symtab_free_objfile_observer (struct objfile *objfile)
{
  symbol_cache_flush (objfile->pspace);
}

/* Debug symbols usually don't have section information.  We need to dig that
   out of the minimal symbols and stash that in the debug symbol.  */

void
fixup_section (struct general_symbol_info *ginfo,
	       CORE_ADDR addr, struct objfile *objfile)
{
  struct minimal_symbol *msym;

  /* First, check whether a minimal symbol with the same name exists
     and points to the same address.  The address check is required
     e.g. on PowerPC64, where the minimal symbol for a function will
     point to the function descriptor, while the debug symbol will
     point to the actual function code.  */
  msym = lookup_minimal_symbol_by_pc_name (addr, ginfo->name, objfile);
  if (msym)
    ginfo->section = MSYMBOL_SECTION (msym);
  else
    {
      /* Static, function-local variables do appear in the linker
	 (minimal) symbols, but are frequently given names that won't
	 be found via lookup_minimal_symbol().  E.g., it has been
	 observed in frv-uclinux (ELF) executables that a static,
	 function-local variable named "foo" might appear in the
	 linker symbols as "foo.6" or "foo.3".  Thus, there is no
	 point in attempting to extend the lookup-by-name mechanism to
	 handle this case due to the fact that there can be multiple
	 names.

	 So, instead, search the section table when lookup by name has
	 failed.  The ``addr'' and ``endaddr'' fields may have already
	 been relocated.  If so, the relocation offset (i.e. the
	 ANOFFSET value) needs to be subtracted from these values when
	 performing the comparison.  We unconditionally subtract it,
	 because, when no relocation has been performed, the ANOFFSET
	 value will simply be zero.

	 The address of the symbol whose section we're fixing up HAS
	 NOT BEEN adjusted (relocated) yet.  It can't have been since
	 the section isn't yet known and knowing the section is
	 necessary in order to add the correct relocation value.  In
	 other words, we wouldn't even be in this function (attempting
	 to compute the section) if it were already known.

	 Note that it is possible to search the minimal symbols
	 (subtracting the relocation value if necessary) to find the
	 matching minimal symbol, but this is overkill and much less
	 efficient.  It is not necessary to find the matching minimal
	 symbol, only its section.

	 Note that this technique (of doing a section table search)
	 can fail when unrelocated section addresses overlap.  For
	 this reason, we still attempt a lookup by name prior to doing
	 a search of the section table.  */

      struct obj_section *s;
      int fallback = -1;

      ALL_OBJFILE_OSECTIONS (objfile, s)
	{
	  int idx = s - objfile->sections;
	  CORE_ADDR offset = ANOFFSET (objfile->section_offsets, idx);

	  if (fallback == -1)
	    fallback = idx;

	  if (obj_section_addr (s) - offset <= addr
	      && addr < obj_section_endaddr (s) - offset)
	    {
	      ginfo->section = idx;
	      return;
	    }
	}

      /* If we didn't find the section, assume it is in the first
	 section.  If there is no allocated section, then it hardly
	 matters what we pick, so just pick zero.  */
      if (fallback == -1)
	ginfo->section = 0;
      else
	ginfo->section = fallback;
    }
}

struct symbol *
fixup_symbol_section (struct symbol *sym, struct objfile *objfile)
{
  CORE_ADDR addr;

  if (!sym)
    return NULL;

  if (!SYMBOL_OBJFILE_OWNED (sym))
    return sym;

  /* We either have an OBJFILE, or we can get at it from the sym's
     symtab.  Anything else is a bug.  */
  gdb_assert (objfile || symbol_symtab (sym));

  if (objfile == NULL)
    objfile = symbol_objfile (sym);

  if (SYMBOL_OBJ_SECTION (objfile, sym))
    return sym;

  /* We should have an objfile by now.  */
  gdb_assert (objfile);

  switch (SYMBOL_CLASS (sym))
    {
    case LOC_STATIC:
    case LOC_LABEL:
      addr = SYMBOL_VALUE_ADDRESS (sym);
      break;
    case LOC_BLOCK:
      addr = BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym));
      break;

    default:
      /* Nothing else will be listed in the minsyms -- no use looking
	 it up.  */
      return sym;
    }

  fixup_section (&sym->ginfo, addr, objfile);

  return sym;
}

/* See symtab.h.  */

demangle_for_lookup_info::demangle_for_lookup_info
  (const lookup_name_info &lookup_name, language lang)
{
  demangle_result_storage storage;

  if (lookup_name.ignore_parameters () && lang == language_cplus)
    {
      gdb::unique_xmalloc_ptr<char> without_params
	= cp_remove_params_if_any (lookup_name.name ().c_str (),
				   lookup_name.completion_mode ());

      if (without_params != NULL)
	{
	  if (lookup_name.match_type () != symbol_name_match_type::SEARCH_NAME)
	    m_demangled_name = demangle_for_lookup (without_params.get (),
						    lang, storage);
	  return;
	}
    }

  if (lookup_name.match_type () == symbol_name_match_type::SEARCH_NAME)
    m_demangled_name = lookup_name.name ();
  else
    m_demangled_name = demangle_for_lookup (lookup_name.name ().c_str (),
					    lang, storage);
}

/* See symtab.h.  */

const lookup_name_info &
lookup_name_info::match_any ()
{
  /* Lookup any symbol that "" would complete.  I.e., this matches all
     symbol names.  */
  static const lookup_name_info lookup_name ({}, symbol_name_match_type::FULL,
					     true);

  return lookup_name;
}

/* Compute the demangled form of NAME as used by the various symbol
   lookup functions.  The result can either be the input NAME
   directly, or a pointer to a buffer owned by the STORAGE object.

   For Ada, this function just returns NAME, unmodified.
   Normally, Ada symbol lookups are performed using the encoded name
   rather than the demangled name, and so it might seem to make sense
   for this function to return an encoded version of NAME.
   Unfortunately, we cannot do this, because this function is used in
   circumstances where it is not appropriate to try to encode NAME.
   For instance, when displaying the frame info, we demangle the name
   of each parameter, and then perform a symbol lookup inside our
   function using that demangled name.  In Ada, certain functions
   have internally-generated parameters whose name contain uppercase
   characters.  Encoding those name would result in those uppercase
   characters to become lowercase, and thus cause the symbol lookup
   to fail.  */

const char *
demangle_for_lookup (const char *name, enum language lang,
		     demangle_result_storage &storage)
{
  /* If we are using C++, D, or Go, demangle the name before doing a
     lookup, so we can always binary search.  */
  if (lang == language_cplus)
    {
      char *demangled_name = gdb_demangle (name, DMGL_ANSI | DMGL_PARAMS);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (demangled_name);

      /* If we were given a non-mangled name, canonicalize it
	 according to the language (so far only for C++).  */
      std::string canon = cp_canonicalize_string (name);
      if (!canon.empty ())
	return storage.swap_string (canon);
    }
  else if (lang == language_d)
    {
      char *demangled_name = d_demangle (name, 0);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (demangled_name);
    }
  else if (lang == language_go)
    {
      char *demangled_name = go_demangle (name, 0);
      if (demangled_name != NULL)
	return storage.set_malloc_ptr (demangled_name);
    }

  return name;
}

/* See symtab.h.  */

unsigned int
search_name_hash (enum language language, const char *search_name)
{
  return language_def (language)->la_search_name_hash (search_name);
}

/* See symtab.h.

   This function (or rather its subordinates) have a bunch of loops and
   it would seem to be attractive to put in some QUIT's (though I'm not really
   sure whether it can run long enough to be really important).  But there
   are a few calls for which it would appear to be bad news to quit
   out of here: e.g., find_proc_desc in alpha-mdebug-tdep.c.  (Note
   that there is C++ code below which can error(), but that probably
   doesn't affect these calls since they are looking for a known
   variable and thus can probably assume it will never hit the C++
   code).  */

struct block_symbol
lookup_symbol_in_language (const char *name, const struct block *block,
			   const domain_enum domain, enum language lang,
			   struct field_of_this_result *is_a_field_of_this)
{
  demangle_result_storage storage;
  const char *modified_name = demangle_for_lookup (name, lang, storage);

  return lookup_symbol_aux (modified_name,
			    symbol_name_match_type::FULL,
			    block, domain, lang,
			    is_a_field_of_this);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol (const char *name, const struct block *block,
	       domain_enum domain,
	       struct field_of_this_result *is_a_field_of_this)
{
  return lookup_symbol_in_language (name, block, domain,
				    current_language->la_language,
				    is_a_field_of_this);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol_search_name (const char *search_name, const struct block *block,
			   domain_enum domain)
{
  return lookup_symbol_aux (search_name, symbol_name_match_type::SEARCH_NAME,
			    block, domain, language_asm, NULL);
}

/* See symtab.h.  */

struct block_symbol
lookup_language_this (const struct language_defn *lang,
		      const struct block *block)
{
  if (lang->la_name_of_this == NULL || block == NULL)
    return (struct block_symbol) {NULL, NULL};

  if (symbol_lookup_debug > 1)
    {
      struct objfile *objfile = lookup_objfile_from_block (block);

      fprintf_unfiltered (gdb_stdlog,
			  "lookup_language_this (%s, %s (objfile %s))",
			  lang->la_name, host_address_to_string (block),
			  objfile_debug_name (objfile));
    }

  while (block)
    {
      struct symbol *sym;

      sym = block_lookup_symbol (block, lang->la_name_of_this,
				 symbol_name_match_type::SEARCH_NAME,
				 VAR_DOMAIN);
      if (sym != NULL)
	{
	  if (symbol_lookup_debug > 1)
	    {
	      fprintf_unfiltered (gdb_stdlog, " = %s (%s, block %s)\n",
				  SYMBOL_PRINT_NAME (sym),
				  host_address_to_string (sym),
				  host_address_to_string (block));
	    }
	  return (struct block_symbol) {sym, block};
	}
      if (BLOCK_FUNCTION (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  if (symbol_lookup_debug > 1)
    fprintf_unfiltered (gdb_stdlog, " = NULL\n");
  return (struct block_symbol) {NULL, NULL};
}

/* Given TYPE, a structure/union,
   return 1 if the component named NAME from the ultimate target
   structure/union is defined, otherwise, return 0.  */

static int
check_field (struct type *type, const char *name,
	     struct field_of_this_result *is_a_field_of_this)
{
  int i;

  /* The type may be a stub.  */
  type = check_typedef (type);

  for (i = TYPE_NFIELDS (type) - 1; i >= TYPE_N_BASECLASSES (type); i--)
    {
      const char *t_field_name = TYPE_FIELD_NAME (type, i);

      if (t_field_name && (strcmp_iw (t_field_name, name) == 0))
	{
	  is_a_field_of_this->type = type;
	  is_a_field_of_this->field = &TYPE_FIELD (type, i);
	  return 1;
	}
    }

  /* C++: If it was not found as a data field, then try to return it
     as a pointer to a method.  */

  for (i = TYPE_NFN_FIELDS (type) - 1; i >= 0; --i)
    {
      if (strcmp_iw (TYPE_FN_FIELDLIST_NAME (type, i), name) == 0)
	{
	  is_a_field_of_this->type = type;
	  is_a_field_of_this->fn_field = &TYPE_FN_FIELDLIST (type, i);
	  return 1;
	}
    }

  for (i = TYPE_N_BASECLASSES (type) - 1; i >= 0; i--)
    if (check_field (TYPE_BASECLASS (type, i), name, is_a_field_of_this))
      return 1;

  return 0;
}

/* Behave like lookup_symbol except that NAME is the natural name
   (e.g., demangled name) of the symbol that we're looking for.  */

static struct block_symbol
lookup_symbol_aux (const char *name, symbol_name_match_type match_type,
		   const struct block *block,
		   const domain_enum domain, enum language language,
		   struct field_of_this_result *is_a_field_of_this)
{
  struct block_symbol result;
  const struct language_defn *langdef;

  if (symbol_lookup_debug)
    {
      struct objfile *objfile = lookup_objfile_from_block (block);

      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_aux (%s, %s (objfile %s), %s, %s)\n",
			  name, host_address_to_string (block),
			  objfile != NULL
			  ? objfile_debug_name (objfile) : "NULL",
			  domain_name (domain), language_str (language));
    }

  /* Make sure we do something sensible with is_a_field_of_this, since
     the callers that set this parameter to some non-null value will
     certainly use it later.  If we don't set it, the contents of
     is_a_field_of_this are undefined.  */
  if (is_a_field_of_this != NULL)
    memset (is_a_field_of_this, 0, sizeof (*is_a_field_of_this));

  /* Search specified block and its superiors.  Don't search
     STATIC_BLOCK or GLOBAL_BLOCK.  */

  result = lookup_local_symbol (name, match_type, block, domain, language);
  if (result.symbol != NULL)
    {
      if (symbol_lookup_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "lookup_symbol_aux (...) = %s\n",
			      host_address_to_string (result.symbol));
	}
      return result;
    }

  /* If requested to do so by the caller and if appropriate for LANGUAGE,
     check to see if NAME is a field of `this'.  */

  langdef = language_def (language);

  /* Don't do this check if we are searching for a struct.  It will
     not be found by check_field, but will be found by other
     means.  */
  if (is_a_field_of_this != NULL && domain != STRUCT_DOMAIN)
    {
      result = lookup_language_this (langdef, block);

      if (result.symbol)
	{
	  struct type *t = result.symbol->type;

	  /* I'm not really sure that type of this can ever
	     be typedefed; just be safe.  */
	  t = check_typedef (t);
	  if (TYPE_CODE (t) == TYPE_CODE_PTR || TYPE_IS_REFERENCE (t))
	    t = TYPE_TARGET_TYPE (t);

	  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
	      && TYPE_CODE (t) != TYPE_CODE_UNION)
	    error (_("Internal error: `%s' is not an aggregate"),
		   langdef->la_name_of_this);

	  if (check_field (t, name, is_a_field_of_this))
	    {
	      if (symbol_lookup_debug)
		{
		  fprintf_unfiltered (gdb_stdlog,
				      "lookup_symbol_aux (...) = NULL\n");
		}
	      return (struct block_symbol) {NULL, NULL};
	    }
	}
    }

  /* Now do whatever is appropriate for LANGUAGE to look
     up static and global variables.  */

  result = langdef->la_lookup_symbol_nonlocal (langdef, name, block, domain);
  if (result.symbol != NULL)
    {
      if (symbol_lookup_debug)
	{
	  fprintf_unfiltered (gdb_stdlog, "lookup_symbol_aux (...) = %s\n",
			      host_address_to_string (result.symbol));
	}
      return result;
    }

  /* Now search all static file-level symbols.  Not strictly correct,
     but more useful than an error.  */

  result = lookup_static_symbol (name, domain);
  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog, "lookup_symbol_aux (...) = %s\n",
			  result.symbol != NULL
			    ? host_address_to_string (result.symbol)
			    : "NULL");
    }
  return result;
}

/* Check to see if the symbol is defined in BLOCK or its superiors.
   Don't search STATIC_BLOCK or GLOBAL_BLOCK.  */

static struct block_symbol
lookup_local_symbol (const char *name,
		     symbol_name_match_type match_type,
		     const struct block *block,
		     const domain_enum domain,
		     enum language language)
{
  struct symbol *sym;
  const struct block *static_block = block_static_block (block);
  const char *scope = block_scope (block);
  
  /* Check if either no block is specified or it's a global block.  */

  if (static_block == NULL)
    return (struct block_symbol) {NULL, NULL};

  while (block != static_block)
    {
      sym = lookup_symbol_in_block (name, match_type, block, domain);
      if (sym != NULL)
	return (struct block_symbol) {sym, block};

      if (language == language_cplus || language == language_fortran)
        {
          struct block_symbol blocksym
	    = cp_lookup_symbol_imports_or_template (scope, name, block,
						    domain);

          if (blocksym.symbol != NULL)
            return blocksym;
        }

      if (BLOCK_FUNCTION (block) != NULL && block_inlined_p (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  /* We've reached the end of the function without finding a result.  */

  return (struct block_symbol) {NULL, NULL};
}

/* See symtab.h.  */

struct objfile *
lookup_objfile_from_block (const struct block *block)
{
  if (block == NULL)
    return NULL;

  block = block_global_block (block);
  /* Look through all blockvectors.  */
  for (objfile *obj : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : obj->compunits ())
	if (block == BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust),
					GLOBAL_BLOCK))
	  {
	    if (obj->separate_debug_objfile_backlink)
	      obj = obj->separate_debug_objfile_backlink;

	    return obj;
	  }
    }

  return NULL;
}

/* See symtab.h.  */

struct symbol *
lookup_symbol_in_block (const char *name, symbol_name_match_type match_type,
			const struct block *block,
			const domain_enum domain)
{
  struct symbol *sym;

  if (symbol_lookup_debug > 1)
    {
      struct objfile *objfile = lookup_objfile_from_block (block);

      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_block (%s, %s (objfile %s), %s)",
			  name, host_address_to_string (block),
			  objfile_debug_name (objfile),
			  domain_name (domain));
    }

  sym = block_lookup_symbol (block, name, match_type, domain);
  if (sym)
    {
      if (symbol_lookup_debug > 1)
	{
	  fprintf_unfiltered (gdb_stdlog, " = %s\n",
			      host_address_to_string (sym));
	}
      return fixup_symbol_section (sym, NULL);
    }

  if (symbol_lookup_debug > 1)
    fprintf_unfiltered (gdb_stdlog, " = NULL\n");
  return NULL;
}

/* See symtab.h.  */

struct block_symbol
lookup_global_symbol_from_objfile (struct objfile *main_objfile,
				   const char *name,
				   const domain_enum domain)
{
  struct objfile *objfile;

  for (objfile = main_objfile;
       objfile;
       objfile = objfile_separate_debug_iterate (main_objfile, objfile))
    {
      struct block_symbol result
        = lookup_symbol_in_objfile (objfile, GLOBAL_BLOCK, name, domain);

      if (result.symbol != NULL)
	return result;
    }

  return (struct block_symbol) {NULL, NULL};
}

/* Check to see if the symbol is defined in one of the OBJFILE's
   symtabs.  BLOCK_INDEX should be either GLOBAL_BLOCK or STATIC_BLOCK,
   depending on whether or not we want to search global symbols or
   static symbols.  */

static struct block_symbol
lookup_symbol_in_objfile_symtabs (struct objfile *objfile, int block_index,
				  const char *name, const domain_enum domain)
{
  gdb_assert (block_index == GLOBAL_BLOCK || block_index == STATIC_BLOCK);

  if (symbol_lookup_debug > 1)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_objfile_symtabs (%s, %s, %s, %s)",
			  objfile_debug_name (objfile),
			  block_index == GLOBAL_BLOCK
			  ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
			  name, domain_name (domain));
    }

  for (compunit_symtab *cust : objfile->compunits ())
    {
      const struct blockvector *bv;
      const struct block *block;
      struct block_symbol result;

      bv = COMPUNIT_BLOCKVECTOR (cust);
      block = BLOCKVECTOR_BLOCK (bv, block_index);
      result.symbol = block_lookup_symbol_primary (block, name, domain);
      result.block = block;
      if (result.symbol != NULL)
	{
	  if (symbol_lookup_debug > 1)
	    {
	      fprintf_unfiltered (gdb_stdlog, " = %s (block %s)\n",
				  host_address_to_string (result.symbol),
				  host_address_to_string (block));
	    }
	  result.symbol = fixup_symbol_section (result.symbol, objfile);
	  return result;

	}
    }

  if (symbol_lookup_debug > 1)
    fprintf_unfiltered (gdb_stdlog, " = NULL\n");
  return (struct block_symbol) {NULL, NULL};
}

/* Wrapper around lookup_symbol_in_objfile_symtabs for search_symbols.
   Look up LINKAGE_NAME in DOMAIN in the global and static blocks of OBJFILE
   and all associated separate debug objfiles.

   Normally we only look in OBJFILE, and not any separate debug objfiles
   because the outer loop will cause them to be searched too.  This case is
   different.  Here we're called from search_symbols where it will only
   call us for the objfile that contains a matching minsym.  */

static struct block_symbol
lookup_symbol_in_objfile_from_linkage_name (struct objfile *objfile,
					    const char *linkage_name,
					    domain_enum domain)
{
  enum language lang = current_language->la_language;
  struct objfile *main_objfile, *cur_objfile;

  demangle_result_storage storage;
  const char *modified_name = demangle_for_lookup (linkage_name, lang, storage);

  if (objfile->separate_debug_objfile_backlink)
    main_objfile = objfile->separate_debug_objfile_backlink;
  else
    main_objfile = objfile;

  for (cur_objfile = main_objfile;
       cur_objfile;
       cur_objfile = objfile_separate_debug_iterate (main_objfile, cur_objfile))
    {
      struct block_symbol result;

      result = lookup_symbol_in_objfile_symtabs (cur_objfile, GLOBAL_BLOCK,
						 modified_name, domain);
      if (result.symbol == NULL)
	result = lookup_symbol_in_objfile_symtabs (cur_objfile, STATIC_BLOCK,
						   modified_name, domain);
      if (result.symbol != NULL)
	return result;
    }

  return (struct block_symbol) {NULL, NULL};
}

/* A helper function that throws an exception when a symbol was found
   in a psymtab but not in a symtab.  */

static void ATTRIBUTE_NORETURN
error_in_psymtab_expansion (int block_index, const char *name,
			    struct compunit_symtab *cust)
{
  error (_("\
Internal: %s symbol `%s' found in %s psymtab but not in symtab.\n\
%s may be an inlined function, or may be a template function\n	 \
(if a template, try specifying an instantiation: %s<type>)."),
	 block_index == GLOBAL_BLOCK ? "global" : "static",
	 name,
	 symtab_to_filename_for_display (compunit_primary_filetab (cust)),
	 name, name);
}

/* A helper function for various lookup routines that interfaces with
   the "quick" symbol table functions.  */

static struct block_symbol
lookup_symbol_via_quick_fns (struct objfile *objfile, int block_index,
			     const char *name, const domain_enum domain)
{
  struct compunit_symtab *cust;
  const struct blockvector *bv;
  const struct block *block;
  struct block_symbol result;

  if (!objfile->sf)
    return (struct block_symbol) {NULL, NULL};

  if (symbol_lookup_debug > 1)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_via_quick_fns (%s, %s, %s, %s)\n",
			  objfile_debug_name (objfile),
			  block_index == GLOBAL_BLOCK
			  ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
			  name, domain_name (domain));
    }

  cust = objfile->sf->qf->lookup_symbol (objfile, block_index, name, domain);
  if (cust == NULL)
    {
      if (symbol_lookup_debug > 1)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "lookup_symbol_via_quick_fns (...) = NULL\n");
	}
      return (struct block_symbol) {NULL, NULL};
    }

  bv = COMPUNIT_BLOCKVECTOR (cust);
  block = BLOCKVECTOR_BLOCK (bv, block_index);
  result.symbol = block_lookup_symbol (block, name,
				       symbol_name_match_type::FULL, domain);
  if (result.symbol == NULL)
    error_in_psymtab_expansion (block_index, name, cust);

  if (symbol_lookup_debug > 1)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_via_quick_fns (...) = %s (block %s)\n",
			  host_address_to_string (result.symbol),
			  host_address_to_string (block));
    }

  result.symbol = fixup_symbol_section (result.symbol, objfile);
  result.block = block;
  return result;
}

/* See symtab.h.  */

struct block_symbol
basic_lookup_symbol_nonlocal (const struct language_defn *langdef,
			      const char *name,
			      const struct block *block,
			      const domain_enum domain)
{
  struct block_symbol result;

  /* NOTE: carlton/2003-05-19: The comments below were written when
     this (or what turned into this) was part of lookup_symbol_aux;
     I'm much less worried about these questions now, since these
     decisions have turned out well, but I leave these comments here
     for posterity.  */

  /* NOTE: carlton/2002-12-05: There is a question as to whether or
     not it would be appropriate to search the current global block
     here as well.  (That's what this code used to do before the
     is_a_field_of_this check was moved up.)  On the one hand, it's
     redundant with the lookup in all objfiles search that happens
     next.  On the other hand, if decode_line_1 is passed an argument
     like filename:var, then the user presumably wants 'var' to be
     searched for in filename.  On the third hand, there shouldn't be
     multiple global variables all of which are named 'var', and it's
     not like decode_line_1 has ever restricted its search to only
     global variables in a single filename.  All in all, only
     searching the static block here seems best: it's correct and it's
     cleanest.  */

  /* NOTE: carlton/2002-12-05: There's also a possible performance
     issue here: if you usually search for global symbols in the
     current file, then it would be slightly better to search the
     current global block before searching all the symtabs.  But there
     are other factors that have a much greater effect on performance
     than that one, so I don't think we should worry about that for
     now.  */

  /* NOTE: dje/2014-10-26: The lookup in all objfiles search could skip
     the current objfile.  Searching the current objfile first is useful
     for both matching user expectations as well as performance.  */

  result = lookup_symbol_in_static_block (name, block, domain);
  if (result.symbol != NULL)
    return result;

  /* If we didn't find a definition for a builtin type in the static block,
     search for it now.  This is actually the right thing to do and can be
     a massive performance win.  E.g., when debugging a program with lots of
     shared libraries we could search all of them only to find out the
     builtin type isn't defined in any of them.  This is common for types
     like "void".  */
  if (domain == VAR_DOMAIN)
    {
      struct gdbarch *gdbarch;

      if (block == NULL)
	gdbarch = target_gdbarch ();
      else
	gdbarch = block_gdbarch (block);
      result.symbol = language_lookup_primitive_type_as_symbol (langdef,
								gdbarch, name);
      result.block = NULL;
      if (result.symbol != NULL)
	return result;
    }

  return lookup_global_symbol (name, block, domain);
}

/* See symtab.h.  */

struct block_symbol
lookup_symbol_in_static_block (const char *name,
			       const struct block *block,
			       const domain_enum domain)
{
  const struct block *static_block = block_static_block (block);
  struct symbol *sym;

  if (static_block == NULL)
    return (struct block_symbol) {NULL, NULL};

  if (symbol_lookup_debug)
    {
      struct objfile *objfile = lookup_objfile_from_block (static_block);

      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_static_block (%s, %s (objfile %s),"
			  " %s)\n",
			  name,
			  host_address_to_string (block),
			  objfile_debug_name (objfile),
			  domain_name (domain));
    }

  sym = lookup_symbol_in_block (name,
				symbol_name_match_type::FULL,
				static_block, domain);
  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_static_block (...) = %s\n",
			  sym != NULL ? host_address_to_string (sym) : "NULL");
    }
  return (struct block_symbol) {sym, static_block};
}

/* Perform the standard symbol lookup of NAME in OBJFILE:
   1) First search expanded symtabs, and if not found
   2) Search the "quick" symtabs (partial or .gdb_index).
   BLOCK_INDEX is one of GLOBAL_BLOCK or STATIC_BLOCK.  */

static struct block_symbol
lookup_symbol_in_objfile (struct objfile *objfile, int block_index,
			  const char *name, const domain_enum domain)
{
  struct block_symbol result;

  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_objfile (%s, %s, %s, %s)\n",
			  objfile_debug_name (objfile),
			  block_index == GLOBAL_BLOCK
			  ? "GLOBAL_BLOCK" : "STATIC_BLOCK",
			  name, domain_name (domain));
    }

  result = lookup_symbol_in_objfile_symtabs (objfile, block_index,
					     name, domain);
  if (result.symbol != NULL)
    {
      if (symbol_lookup_debug)
	{
	  fprintf_unfiltered (gdb_stdlog,
			      "lookup_symbol_in_objfile (...) = %s"
			      " (in symtabs)\n",
			      host_address_to_string (result.symbol));
	}
      return result;
    }

  result = lookup_symbol_via_quick_fns (objfile, block_index,
					name, domain);
  if (symbol_lookup_debug)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "lookup_symbol_in_objfile (...) = %s%s\n",
			  result.symbol != NULL
			  ? host_address_to_string (result.symbol)
			  : "NULL",
			  result.symbol != NULL ? " (via quick fns)" : "");
    }
  return result;
}

/* See symtab.h.  */

struct block_symbol
lookup_static_symbol (const char *name, const domain_enum domain)
{
  struct symbol_cache *cache = get_symbol_cache (current_program_space);
  struct block_symbol result;
  struct block_symbol_cache *bsc;
  struct symbol_cache_slot *slot;

  /* Lookup in STATIC_BLOCK is not current-objfile-dependent, so just pass
     NULL for OBJFILE_CONTEXT.  */
  result = symbol_cache_lookup (cache, NULL, STATIC_BLOCK, name, domain,
				&bsc, &slot);
  if (result.symbol != NULL)
    {
      if (SYMBOL_LOOKUP_FAILED_P (result))
	return (struct block_symbol) {NULL, NULL};
      return result;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      result = lookup_symbol_in_objfile (objfile, STATIC_BLOCK, name, domain);
      if (result.symbol != NULL)
	{
	  /* Still pass NULL for OBJFILE_CONTEXT here.  */
	  symbol_cache_mark_found (bsc, slot, NULL, result.symbol,
				   result.block);
	  return result;
	}
    }

  /* Still pass NULL for OBJFILE_CONTEXT here.  */
  symbol_cache_mark_not_found (bsc, slot, NULL, name, domain);
  return (struct block_symbol) {NULL, NULL};
}

/* Private data to be used with lookup_symbol_global_iterator_cb.  */

struct global_sym_lookup_data
{
  /* The name of the symbol we are searching for.  */
  const char *name;

  /* The domain to use for our search.  */
  domain_enum domain;

  /* The field where the callback should store the symbol if found.
     It should be initialized to {NULL, NULL} before the search is started.  */
  struct block_symbol result;
};

/* A callback function for gdbarch_iterate_over_objfiles_in_search_order.
   It searches by name for a symbol in the GLOBAL_BLOCK of the given
   OBJFILE.  The arguments for the search are passed via CB_DATA,
   which in reality is a pointer to struct global_sym_lookup_data.  */

static int
lookup_symbol_global_iterator_cb (struct objfile *objfile,
				  void *cb_data)
{
  struct global_sym_lookup_data *data =
    (struct global_sym_lookup_data *) cb_data;

  gdb_assert (data->result.symbol == NULL
	      && data->result.block == NULL);

  data->result = lookup_symbol_in_objfile (objfile, GLOBAL_BLOCK,
					   data->name, data->domain);

  /* If we found a match, tell the iterator to stop.  Otherwise,
     keep going.  */
  return (data->result.symbol != NULL);
}

/* See symtab.h.  */

struct block_symbol
lookup_global_symbol (const char *name,
		      const struct block *block,
		      const domain_enum domain)
{
  struct symbol_cache *cache = get_symbol_cache (current_program_space);
  struct block_symbol result;
  struct objfile *objfile;
  struct global_sym_lookup_data lookup_data;
  struct block_symbol_cache *bsc;
  struct symbol_cache_slot *slot;

  objfile = lookup_objfile_from_block (block);

  /* First see if we can find the symbol in the cache.
     This works because we use the current objfile to qualify the lookup.  */
  result = symbol_cache_lookup (cache, objfile, GLOBAL_BLOCK, name, domain,
				&bsc, &slot);
  if (result.symbol != NULL)
    {
      if (SYMBOL_LOOKUP_FAILED_P (result))
	return (struct block_symbol) {NULL, NULL};
      return result;
    }

  /* Call library-specific lookup procedure.  */
  if (objfile != NULL)
    result = solib_global_lookup (objfile, name, domain);

  /* If that didn't work go a global search (of global blocks, heh).  */
  if (result.symbol == NULL)
    {
      memset (&lookup_data, 0, sizeof (lookup_data));
      lookup_data.name = name;
      lookup_data.domain = domain;
      gdbarch_iterate_over_objfiles_in_search_order
	(objfile != NULL ? get_objfile_arch (objfile) : target_gdbarch (),
	 lookup_symbol_global_iterator_cb, &lookup_data, objfile);
      result = lookup_data.result;
    }

  if (result.symbol != NULL)
    symbol_cache_mark_found (bsc, slot, objfile, result.symbol, result.block);
  else
    symbol_cache_mark_not_found (bsc, slot, objfile, name, domain);

  return result;
}

int
symbol_matches_domain (enum language symbol_language,
		       domain_enum symbol_domain,
		       domain_enum domain)
{
  /* For C++ "struct foo { ... }" also defines a typedef for "foo".
     Similarly, any Ada type declaration implicitly defines a typedef.  */
  if (symbol_language == language_cplus
      || symbol_language == language_d
      || symbol_language == language_ada
      || symbol_language == language_rust)
    {
      if ((domain == VAR_DOMAIN || domain == STRUCT_DOMAIN)
	  && symbol_domain == STRUCT_DOMAIN)
	return 1;
    }
  /* For all other languages, strict match is required.  */
  return (symbol_domain == domain);
}

/* See symtab.h.  */

struct type *
lookup_transparent_type (const char *name)
{
  return current_language->la_lookup_transparent_type (name);
}

/* A helper for basic_lookup_transparent_type that interfaces with the
   "quick" symbol table functions.  */

static struct type *
basic_lookup_transparent_type_quick (struct objfile *objfile, int block_index,
				     const char *name)
{
  struct compunit_symtab *cust;
  const struct blockvector *bv;
  struct block *block;
  struct symbol *sym;

  if (!objfile->sf)
    return NULL;
  cust = objfile->sf->qf->lookup_symbol (objfile, block_index, name,
					 STRUCT_DOMAIN);
  if (cust == NULL)
    return NULL;

  bv = COMPUNIT_BLOCKVECTOR (cust);
  block = BLOCKVECTOR_BLOCK (bv, block_index);
  sym = block_find_symbol (block, name, STRUCT_DOMAIN,
			   block_find_non_opaque_type, NULL);
  if (sym == NULL)
    error_in_psymtab_expansion (block_index, name, cust);
  gdb_assert (!TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)));
  return SYMBOL_TYPE (sym);
}

/* Subroutine of basic_lookup_transparent_type to simplify it.
   Look up the non-opaque definition of NAME in BLOCK_INDEX of OBJFILE.
   BLOCK_INDEX is either GLOBAL_BLOCK or STATIC_BLOCK.  */

static struct type *
basic_lookup_transparent_type_1 (struct objfile *objfile, int block_index,
				 const char *name)
{
  const struct blockvector *bv;
  const struct block *block;
  const struct symbol *sym;

  for (compunit_symtab *cust : objfile->compunits ())
    {
      bv = COMPUNIT_BLOCKVECTOR (cust);
      block = BLOCKVECTOR_BLOCK (bv, block_index);
      sym = block_find_symbol (block, name, STRUCT_DOMAIN,
			       block_find_non_opaque_type, NULL);
      if (sym != NULL)
	{
	  gdb_assert (!TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)));
	  return SYMBOL_TYPE (sym);
	}
    }

  return NULL;
}

/* The standard implementation of lookup_transparent_type.  This code
   was modeled on lookup_symbol -- the parts not relevant to looking
   up types were just left out.  In particular it's assumed here that
   types are available in STRUCT_DOMAIN and only in file-static or
   global blocks.  */

struct type *
basic_lookup_transparent_type (const char *name)
{
  struct type *t;

  /* Now search all the global symbols.  Do the symtab's first, then
     check the psymtab's.  If a psymtab indicates the existence
     of the desired name as a global, then do psymtab-to-symtab
     conversion on the fly and return the found symbol.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_1 (objfile, GLOBAL_BLOCK, name);
      if (t)
	return t;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_quick (objfile, GLOBAL_BLOCK, name);
      if (t)
	return t;
    }

  /* Now search the static file-level symbols.
     Not strictly correct, but more useful than an error.
     Do the symtab's first, then
     check the psymtab's.  If a psymtab indicates the existence
     of the desired name as a file-level static, then do psymtab-to-symtab
     conversion on the fly and return the found symbol.  */

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_1 (objfile, STATIC_BLOCK, name);
      if (t)
	return t;
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      t = basic_lookup_transparent_type_quick (objfile, STATIC_BLOCK, name);
      if (t)
	return t;
    }

  return (struct type *) 0;
}

/* Iterate over the symbols named NAME, matching DOMAIN, in BLOCK.

   For each symbol that matches, CALLBACK is called.  The symbol is
   passed to the callback.

   If CALLBACK returns false, the iteration ends.  Otherwise, the
   search continues.  */

void
iterate_over_symbols (const struct block *block,
		      const lookup_name_info &name,
		      const domain_enum domain,
		      gdb::function_view<symbol_found_callback_ftype> callback)
{
  struct block_iterator iter;
  struct symbol *sym;

  ALL_BLOCK_SYMBOLS_WITH_NAME (block, name, iter, sym)
    {
      if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				 SYMBOL_DOMAIN (sym), domain))
	{
	  struct block_symbol block_sym = {sym, block};

	  if (!callback (&block_sym))
	    return;
	}
    }
}

/* Find the compunit symtab associated with PC and SECTION.
   This will read in debug info as necessary.  */

struct compunit_symtab *
find_pc_sect_compunit_symtab (CORE_ADDR pc, struct obj_section *section)
{
  struct compunit_symtab *best_cust = NULL;
  CORE_ADDR distance = 0;
  struct bound_minimal_symbol msymbol;

  /* If we know that this is not a text address, return failure.  This is
     necessary because we loop based on the block's high and low code
     addresses, which do not include the data ranges, and because
     we call find_pc_sect_psymtab which has a similar restriction based
     on the partial_symtab's texthigh and textlow.  */
  msymbol = lookup_minimal_symbol_by_pc_section (pc, section);
  if (msymbol.minsym && msymbol.minsym->data_p ())
    return NULL;

  /* Search all symtabs for the one whose file contains our address, and which
     is the smallest of all the ones containing the address.  This is designed
     to deal with a case like symtab a is at 0x1000-0x2000 and 0x3000-0x4000
     and symtab b is at 0x2000-0x3000.  So the GLOBAL_BLOCK for a is from
     0x1000-0x4000, but for address 0x2345 we want to return symtab b.

     This happens for native ecoff format, where code from included files
     gets its own symtab.  The symtab for the included file should have
     been read in already via the dependency mechanism.
     It might be swifter to create several symtabs with the same name
     like xcoff does (I'm not sure).

     It also happens for objfiles that have their functions reordered.
     For these, the symtab we are looking for is not necessarily read in.  */

  for (objfile *obj_file : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : obj_file->compunits ())
	{
	  struct block *b;
	  const struct blockvector *bv;

	  bv = COMPUNIT_BLOCKVECTOR (cust);
	  b = BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK);

	  if (BLOCK_START (b) <= pc
	      && BLOCK_END (b) > pc
	      && (distance == 0
		  || BLOCK_END (b) - BLOCK_START (b) < distance))
	    {
	      /* For an objfile that has its functions reordered,
		 find_pc_psymtab will find the proper partial symbol table
		 and we simply return its corresponding symtab.  */
	      /* In order to better support objfiles that contain both
		 stabs and coff debugging info, we continue on if a psymtab
		 can't be found.  */
	      if ((obj_file->flags & OBJF_REORDERED) && obj_file->sf)
		{
		  struct compunit_symtab *result;

		  result
		    = obj_file->sf->qf->find_pc_sect_compunit_symtab (obj_file,
								      msymbol,
								      pc,
								      section,
								      0);
		  if (result != NULL)
		    return result;
		}
	      if (section != 0)
		{
		  struct block_iterator iter;
		  struct symbol *sym = NULL;

		  ALL_BLOCK_SYMBOLS (b, iter, sym)
		    {
		      fixup_symbol_section (sym, obj_file);
		      if (matching_obj_sections (SYMBOL_OBJ_SECTION (obj_file,
								     sym),
						 section))
			break;
		    }
		  if (sym == NULL)
		    continue;		/* No symbol in this symtab matches
					   section.  */
		}
	      distance = BLOCK_END (b) - BLOCK_START (b);
	      best_cust = cust;
	    }
	}
    }

  if (best_cust != NULL)
    return best_cust;

  /* Not found in symtabs, search the "quick" symtabs (e.g. psymtabs).  */

  for (objfile *objf : current_program_space->objfiles ())
    {
      struct compunit_symtab *result;

      if (!objf->sf)
	continue;
      result = objf->sf->qf->find_pc_sect_compunit_symtab (objf,
							   msymbol,
							   pc, section,
							   1);
      if (result != NULL)
	return result;
    }

  return NULL;
}

/* Find the compunit symtab associated with PC.
   This will read in debug info as necessary.
   Backward compatibility, no section.  */

struct compunit_symtab *
find_pc_compunit_symtab (CORE_ADDR pc)
{
  return find_pc_sect_compunit_symtab (pc, find_pc_mapped_section (pc));
}

/* See symtab.h.  */

struct symbol *
find_symbol_at_address (CORE_ADDR address)
{
  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->sf == NULL
	  || objfile->sf->qf->find_compunit_symtab_by_address == NULL)
	continue;

      struct compunit_symtab *symtab
	= objfile->sf->qf->find_compunit_symtab_by_address (objfile, address);
      if (symtab != NULL)
	{
	  const struct blockvector *bv = COMPUNIT_BLOCKVECTOR (symtab);

	  for (int i = GLOBAL_BLOCK; i <= STATIC_BLOCK; ++i)
	    {
	      struct block *b = BLOCKVECTOR_BLOCK (bv, i);
	      struct block_iterator iter;
	      struct symbol *sym;

	      ALL_BLOCK_SYMBOLS (b, iter, sym)
		{
		  if (SYMBOL_CLASS (sym) == LOC_STATIC
		      && SYMBOL_VALUE_ADDRESS (sym) == address)
		    return sym;
		}
	    }
	}
    }

  return NULL;
}



/* Find the source file and line number for a given PC value and SECTION.
   Return a structure containing a symtab pointer, a line number,
   and a pc range for the entire source line.
   The value's .pc field is NOT the specified pc.
   NOTCURRENT nonzero means, if specified pc is on a line boundary,
   use the line that ends there.  Otherwise, in that case, the line
   that begins there is used.  */

/* The big complication here is that a line may start in one file, and end just
   before the start of another file.  This usually occurs when you #include
   code in the middle of a subroutine.  To properly find the end of a line's PC
   range, we must search all symtabs associated with this compilation unit, and
   find the one whose first PC is closer than that of the next line in this
   symtab.  */

struct symtab_and_line
find_pc_sect_line (CORE_ADDR pc, struct obj_section *section, int notcurrent)
{
  struct compunit_symtab *cust;
  struct linetable *l;
  int len;
  struct linetable_entry *item;
  const struct blockvector *bv;
  struct bound_minimal_symbol msymbol;

  /* Info on best line seen so far, and where it starts, and its file.  */

  struct linetable_entry *best = NULL;
  CORE_ADDR best_end = 0;
  struct symtab *best_symtab = 0;

  /* Store here the first line number
     of a file which contains the line at the smallest pc after PC.
     If we don't find a line whose range contains PC,
     we will use a line one less than this,
     with a range from the start of that file to the first line's pc.  */
  struct linetable_entry *alt = NULL;

  /* Info on best line seen in this file.  */

  struct linetable_entry *prev;

  /* If this pc is not from the current frame,
     it is the address of the end of a call instruction.
     Quite likely that is the start of the following statement.
     But what we want is the statement containing the instruction.
     Fudge the pc to make sure we get that.  */

  /* It's tempting to assume that, if we can't find debugging info for
     any function enclosing PC, that we shouldn't search for line
     number info, either.  However, GAS can emit line number info for
     assembly files --- very helpful when debugging hand-written
     assembly code.  In such a case, we'd have no debug info for the
     function, but we would have line info.  */

  if (notcurrent)
    pc -= 1;

  /* elz: added this because this function returned the wrong
     information if the pc belongs to a stub (import/export)
     to call a shlib function.  This stub would be anywhere between
     two functions in the target, and the line info was erroneously
     taken to be the one of the line before the pc.  */

  /* RT: Further explanation:

   * We have stubs (trampolines) inserted between procedures.
   *
   * Example: "shr1" exists in a shared library, and a "shr1" stub also
   * exists in the main image.
   *
   * In the minimal symbol table, we have a bunch of symbols
   * sorted by start address.  The stubs are marked as "trampoline",
   * the others appear as text. E.g.:
   *
   *  Minimal symbol table for main image
   *     main:  code for main (text symbol)
   *     shr1: stub  (trampoline symbol)
   *     foo:   code for foo (text symbol)
   *     ...
   *  Minimal symbol table for "shr1" image:
   *     ...
   *     shr1: code for shr1 (text symbol)
   *     ...
   *
   * So the code below is trying to detect if we are in the stub
   * ("shr1" stub), and if so, find the real code ("shr1" trampoline),
   * and if found,  do the symbolization from the real-code address
   * rather than the stub address.
   *
   * Assumptions being made about the minimal symbol table:
   *   1. lookup_minimal_symbol_by_pc() will return a trampoline only
   *      if we're really in the trampoline.s If we're beyond it (say
   *      we're in "foo" in the above example), it'll have a closer
   *      symbol (the "foo" text symbol for example) and will not
   *      return the trampoline.
   *   2. lookup_minimal_symbol_text() will find a real text symbol
   *      corresponding to the trampoline, and whose address will
   *      be different than the trampoline address.  I put in a sanity
   *      check for the address being the same, to avoid an
   *      infinite recursion.
   */
  msymbol = lookup_minimal_symbol_by_pc (pc);
  if (msymbol.minsym != NULL)
    if (MSYMBOL_TYPE (msymbol.minsym) == mst_solib_trampoline)
      {
	struct bound_minimal_symbol mfunsym
	  = lookup_minimal_symbol_text (MSYMBOL_LINKAGE_NAME (msymbol.minsym),
					NULL);

	if (mfunsym.minsym == NULL)
	  /* I eliminated this warning since it is coming out
	   * in the following situation:
	   * gdb shmain // test program with shared libraries
	   * (gdb) break shr1  // function in shared lib
	   * Warning: In stub for ...
	   * In the above situation, the shared lib is not loaded yet,
	   * so of course we can't find the real func/line info,
	   * but the "break" still works, and the warning is annoying.
	   * So I commented out the warning.  RT */
	  /* warning ("In stub for %s; unable to find real function/line info",
	     SYMBOL_LINKAGE_NAME (msymbol)); */
	  ;
	/* fall through */
	else if (BMSYMBOL_VALUE_ADDRESS (mfunsym)
		 == BMSYMBOL_VALUE_ADDRESS (msymbol))
	  /* Avoid infinite recursion */
	  /* See above comment about why warning is commented out.  */
	  /* warning ("In stub for %s; unable to find real function/line info",
	     SYMBOL_LINKAGE_NAME (msymbol)); */
	  ;
	/* fall through */
	else
	  return find_pc_line (BMSYMBOL_VALUE_ADDRESS (mfunsym), 0);
      }

  symtab_and_line val;
  val.pspace = current_program_space;

  cust = find_pc_sect_compunit_symtab (pc, section);
  if (cust == NULL)
    {
      /* If no symbol information, return previous pc.  */
      if (notcurrent)
	pc++;
      val.pc = pc;
      return val;
    }

  bv = COMPUNIT_BLOCKVECTOR (cust);

  /* Look at all the symtabs that share this blockvector.
     They all have the same apriori range, that we found was right;
     but they have different line tables.  */

  for (symtab *iter_s : compunit_filetabs (cust))
    {
      /* Find the best line in this symtab.  */
      l = SYMTAB_LINETABLE (iter_s);
      if (!l)
	continue;
      len = l->nitems;
      if (len <= 0)
	{
	  /* I think len can be zero if the symtab lacks line numbers
	     (e.g. gcc -g1).  (Either that or the LINETABLE is NULL;
	     I'm not sure which, and maybe it depends on the symbol
	     reader).  */
	  continue;
	}

      prev = NULL;
      item = l->item;		/* Get first line info.  */

      /* Is this file's first line closer than the first lines of other files?
         If so, record this file, and its first line, as best alternate.  */
      if (item->pc > pc && (!alt || item->pc < alt->pc))
	alt = item;

      auto pc_compare = [](const CORE_ADDR & comp_pc,
			   const struct linetable_entry & lhs)->bool
      {
	return comp_pc < lhs.pc;
      };

      struct linetable_entry *first = item;
      struct linetable_entry *last = item + len;
      item = std::upper_bound (first, last, pc, pc_compare);
      if (item != first)
	prev = item - 1;		/* Found a matching item.  */

      /* At this point, prev points at the line whose start addr is <= pc, and
         item points at the next line.  If we ran off the end of the linetable
         (pc >= start of the last line), then prev == item.  If pc < start of
         the first line, prev will not be set.  */

      /* Is this file's best line closer than the best in the other files?
         If so, record this file, and its best line, as best so far.  Don't
         save prev if it represents the end of a function (i.e. line number
         0) instead of a real line.  */

      if (prev && prev->line && (!best || prev->pc > best->pc))
	{
	  best = prev;
	  best_symtab = iter_s;

	  /* Discard BEST_END if it's before the PC of the current BEST.  */
	  if (best_end <= best->pc)
	    best_end = 0;
	}

      /* If another line (denoted by ITEM) is in the linetable and its
	 PC is after BEST's PC, but before the current BEST_END, then
	 use ITEM's PC as the new best_end.  */
      if (best && item < last && item->pc > best->pc
	  && (best_end == 0 || best_end > item->pc))
	best_end = item->pc;
    }

  if (!best_symtab)
    {
      /* If we didn't find any line number info, just return zeros.
	 We used to return alt->line - 1 here, but that could be
	 anywhere; if we don't have line number info for this PC,
	 don't make some up.  */
      val.pc = pc;
    }
  else if (best->line == 0)
    {
      /* If our best fit is in a range of PC's for which no line
	 number info is available (line number is zero) then we didn't
	 find any valid line information.  */
      val.pc = pc;
    }
  else
    {
      val.symtab = best_symtab;
      val.line = best->line;
      val.pc = best->pc;
      if (best_end && (!alt || best_end < alt->pc))
	val.end = best_end;
      else if (alt)
	val.end = alt->pc;
      else
	val.end = BLOCK_END (BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK));
    }
  val.section = section;
  return val;
}

/* Backward compatibility (no section).  */

struct symtab_and_line
find_pc_line (CORE_ADDR pc, int notcurrent)
{
  struct obj_section *section;

  section = find_pc_overlay (pc);
  if (pc_in_unmapped_range (pc, section))
    pc = overlay_mapped_address (pc, section);
  return find_pc_sect_line (pc, section, notcurrent);
}

/* See symtab.h.  */

struct symtab *
find_pc_line_symtab (CORE_ADDR pc)
{
  struct symtab_and_line sal;

  /* This always passes zero for NOTCURRENT to find_pc_line.
     There are currently no callers that ever pass non-zero.  */
  sal = find_pc_line (pc, 0);
  return sal.symtab;
}

/* Find line number LINE in any symtab whose name is the same as
   SYMTAB.

   If found, return the symtab that contains the linetable in which it was
   found, set *INDEX to the index in the linetable of the best entry
   found, and set *EXACT_MATCH nonzero if the value returned is an
   exact match.

   If not found, return NULL.  */

struct symtab *
find_line_symtab (struct symtab *sym_tab, int line,
		  int *index, int *exact_match)
{
  int exact = 0;  /* Initialized here to avoid a compiler warning.  */

  /* BEST_INDEX and BEST_LINETABLE identify the smallest linenumber > LINE
     so far seen.  */

  int best_index;
  struct linetable *best_linetable;
  struct symtab *best_symtab;

  /* First try looking it up in the given symtab.  */
  best_linetable = SYMTAB_LINETABLE (sym_tab);
  best_symtab = sym_tab;
  best_index = find_line_common (best_linetable, line, &exact, 0);
  if (best_index < 0 || !exact)
    {
      /* Didn't find an exact match.  So we better keep looking for
         another symtab with the same name.  In the case of xcoff,
         multiple csects for one source file (produced by IBM's FORTRAN
         compiler) produce multiple symtabs (this is unavoidable
         assuming csects can be at arbitrary places in memory and that
         the GLOBAL_BLOCK of a symtab has a begin and end address).  */

      /* BEST is the smallest linenumber > LINE so far seen,
         or 0 if none has been seen so far.
         BEST_INDEX and BEST_LINETABLE identify the item for it.  */
      int best;

      if (best_index >= 0)
	best = best_linetable->item[best_index].line;
      else
	best = 0;

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  if (objfile->sf)
	    objfile->sf->qf->expand_symtabs_with_fullname
	      (objfile, symtab_to_fullname (sym_tab));
	}

      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (compunit_symtab *cu : objfile->compunits ())
	    {
	      for (symtab *s : compunit_filetabs (cu))
		{
		  struct linetable *l;
		  int ind;

		  if (FILENAME_CMP (sym_tab->filename, s->filename) != 0)
		    continue;
		  if (FILENAME_CMP (symtab_to_fullname (sym_tab),
				    symtab_to_fullname (s)) != 0)
		    continue;	
		  l = SYMTAB_LINETABLE (s);
		  ind = find_line_common (l, line, &exact, 0);
		  if (ind >= 0)
		    {
		      if (exact)
			{
			  best_index = ind;
			  best_linetable = l;
			  best_symtab = s;
			  goto done;
			}
		      if (best == 0 || l->item[ind].line < best)
			{
			  best = l->item[ind].line;
			  best_index = ind;
			  best_linetable = l;
			  best_symtab = s;
			}
		    }
		}
	    }
	}
    }
done:
  if (best_index < 0)
    return NULL;

  if (index)
    *index = best_index;
  if (exact_match)
    *exact_match = exact;

  return best_symtab;
}

/* Given SYMTAB, returns all the PCs function in the symtab that
   exactly match LINE.  Returns an empty vector if there are no exact
   matches, but updates BEST_ITEM in this case.  */

std::vector<CORE_ADDR>
find_pcs_for_symtab_line (struct symtab *symtab, int line,
			  struct linetable_entry **best_item)
{
  int start = 0;
  std::vector<CORE_ADDR> result;

  /* First, collect all the PCs that are at this line.  */
  while (1)
    {
      int was_exact;
      int idx;

      idx = find_line_common (SYMTAB_LINETABLE (symtab), line, &was_exact,
			      start);
      if (idx < 0)
	break;

      if (!was_exact)
	{
	  struct linetable_entry *item = &SYMTAB_LINETABLE (symtab)->item[idx];

	  if (*best_item == NULL || item->line < (*best_item)->line)
	    *best_item = item;

	  break;
	}

      result.push_back (SYMTAB_LINETABLE (symtab)->item[idx].pc);
      start = idx + 1;
    }

  return result;
}


/* Set the PC value for a given source file and line number and return true.
   Returns zero for invalid line number (and sets the PC to 0).
   The source file is specified with a struct symtab.  */

int
find_line_pc (struct symtab *symtab, int line, CORE_ADDR *pc)
{
  struct linetable *l;
  int ind;

  *pc = 0;
  if (symtab == 0)
    return 0;

  symtab = find_line_symtab (symtab, line, &ind, NULL);
  if (symtab != NULL)
    {
      l = SYMTAB_LINETABLE (symtab);
      *pc = l->item[ind].pc;
      return 1;
    }
  else
    return 0;
}

/* Find the range of pc values in a line.
   Store the starting pc of the line into *STARTPTR
   and the ending pc (start of next line) into *ENDPTR.
   Returns 1 to indicate success.
   Returns 0 if could not find the specified line.  */

int
find_line_pc_range (struct symtab_and_line sal, CORE_ADDR *startptr,
		    CORE_ADDR *endptr)
{
  CORE_ADDR startaddr;
  struct symtab_and_line found_sal;

  startaddr = sal.pc;
  if (startaddr == 0 && !find_line_pc (sal.symtab, sal.line, &startaddr))
    return 0;

  /* This whole function is based on address.  For example, if line 10 has
     two parts, one from 0x100 to 0x200 and one from 0x300 to 0x400, then
     "info line *0x123" should say the line goes from 0x100 to 0x200
     and "info line *0x355" should say the line goes from 0x300 to 0x400.
     This also insures that we never give a range like "starts at 0x134
     and ends at 0x12c".  */

  found_sal = find_pc_sect_line (startaddr, sal.section, 0);
  if (found_sal.line != sal.line)
    {
      /* The specified line (sal) has zero bytes.  */
      *startptr = found_sal.pc;
      *endptr = found_sal.pc;
    }
  else
    {
      *startptr = found_sal.pc;
      *endptr = found_sal.end;
    }
  return 1;
}

/* Given a line table and a line number, return the index into the line
   table for the pc of the nearest line whose number is >= the specified one.
   Return -1 if none is found.  The value is >= 0 if it is an index.
   START is the index at which to start searching the line table.

   Set *EXACT_MATCH nonzero if the value returned is an exact match.  */

static int
find_line_common (struct linetable *l, int lineno,
		  int *exact_match, int start)
{
  int i;
  int len;

  /* BEST is the smallest linenumber > LINENO so far seen,
     or 0 if none has been seen so far.
     BEST_INDEX identifies the item for it.  */

  int best_index = -1;
  int best = 0;

  *exact_match = 0;

  if (lineno <= 0)
    return -1;
  if (l == 0)
    return -1;

  len = l->nitems;
  for (i = start; i < len; i++)
    {
      struct linetable_entry *item = &(l->item[i]);

      if (item->line == lineno)
	{
	  /* Return the first (lowest address) entry which matches.  */
	  *exact_match = 1;
	  return i;
	}

      if (item->line > lineno && (best == 0 || item->line < best))
	{
	  best = item->line;
	  best_index = i;
	}
    }

  /* If we got here, we didn't get an exact match.  */
  return best_index;
}

int
find_pc_line_pc_range (CORE_ADDR pc, CORE_ADDR *startptr, CORE_ADDR *endptr)
{
  struct symtab_and_line sal;

  sal = find_pc_line (pc, 0);
  *startptr = sal.pc;
  *endptr = sal.end;
  return sal.symtab != 0;
}

/* Helper for find_function_start_sal.  Does most of the work, except
   setting the sal's symbol.  */

static symtab_and_line
find_function_start_sal_1 (CORE_ADDR func_addr, obj_section *section,
			   bool funfirstline)
{
  symtab_and_line sal = find_pc_sect_line (func_addr, section, 0);

  if (funfirstline && sal.symtab != NULL
      && (COMPUNIT_LOCATIONS_VALID (SYMTAB_COMPUNIT (sal.symtab))
	  || SYMTAB_LANGUAGE (sal.symtab) == language_asm))
    {
      struct gdbarch *gdbarch = get_objfile_arch (SYMTAB_OBJFILE (sal.symtab));

      sal.pc = func_addr;
      if (gdbarch_skip_entrypoint_p (gdbarch))
	sal.pc = gdbarch_skip_entrypoint (gdbarch, sal.pc);
      return sal;
    }

  /* We always should have a line for the function start address.
     If we don't, something is odd.  Create a plain SAL referring
     just the PC and hope that skip_prologue_sal (if requested)
     can find a line number for after the prologue.  */
  if (sal.pc < func_addr)
    {
      sal = {};
      sal.pspace = current_program_space;
      sal.pc = func_addr;
      sal.section = section;
    }

  if (funfirstline)
    skip_prologue_sal (&sal);

  return sal;
}

/* See symtab.h.  */

symtab_and_line
find_function_start_sal (CORE_ADDR func_addr, obj_section *section,
			 bool funfirstline)
{
  symtab_and_line sal
    = find_function_start_sal_1 (func_addr, section, funfirstline);

  /* find_function_start_sal_1 does a linetable search, so it finds
     the symtab and linenumber, but not a symbol.  Fill in the
     function symbol too.  */
  sal.symbol = find_pc_sect_containing_function (sal.pc, sal.section);

  return sal;
}

/* See symtab.h.  */

symtab_and_line
find_function_start_sal (symbol *sym, bool funfirstline)
{
  fixup_symbol_section (sym, NULL);
  symtab_and_line sal
    = find_function_start_sal_1 (BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym)),
				 SYMBOL_OBJ_SECTION (symbol_objfile (sym), sym),
				 funfirstline);
  sal.symbol = sym;
  return sal;
}


/* Given a function start address FUNC_ADDR and SYMTAB, find the first
   address for that function that has an entry in SYMTAB's line info
   table.  If such an entry cannot be found, return FUNC_ADDR
   unaltered.  */

static CORE_ADDR
skip_prologue_using_lineinfo (CORE_ADDR func_addr, struct symtab *symtab)
{
  CORE_ADDR func_start, func_end;
  struct linetable *l;
  int i;

  /* Give up if this symbol has no lineinfo table.  */
  l = SYMTAB_LINETABLE (symtab);
  if (l == NULL)
    return func_addr;

  /* Get the range for the function's PC values, or give up if we
     cannot, for some reason.  */
  if (!find_pc_partial_function (func_addr, NULL, &func_start, &func_end))
    return func_addr;

  /* Linetable entries are ordered by PC values, see the commentary in
     symtab.h where `struct linetable' is defined.  Thus, the first
     entry whose PC is in the range [FUNC_START..FUNC_END[ is the
     address we are looking for.  */
  for (i = 0; i < l->nitems; i++)
    {
      struct linetable_entry *item = &(l->item[i]);

      /* Don't use line numbers of zero, they mark special entries in
	 the table.  See the commentary on symtab.h before the
	 definition of struct linetable.  */
      if (item->line > 0 && func_start <= item->pc && item->pc < func_end)
	return item->pc;
    }

  return func_addr;
}

/* Adjust SAL to the first instruction past the function prologue.
   If the PC was explicitly specified, the SAL is not changed.
   If the line number was explicitly specified, at most the SAL's PC
   is updated.  If SAL is already past the prologue, then do nothing.  */

void
skip_prologue_sal (struct symtab_and_line *sal)
{
  struct symbol *sym;
  struct symtab_and_line start_sal;
  CORE_ADDR pc, saved_pc;
  struct obj_section *section;
  const char *name;
  struct objfile *objfile;
  struct gdbarch *gdbarch;
  const struct block *b, *function_block;
  int force_skip, skip;

  /* Do not change the SAL if PC was specified explicitly.  */
  if (sal->explicit_pc)
    return;

  scoped_restore_current_pspace_and_thread restore_pspace_thread;

  switch_to_program_space_and_thread (sal->pspace);

  sym = find_pc_sect_function (sal->pc, sal->section);
  if (sym != NULL)
    {
      fixup_symbol_section (sym, NULL);

      objfile = symbol_objfile (sym);
      pc = BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym));
      section = SYMBOL_OBJ_SECTION (objfile, sym);
      name = SYMBOL_LINKAGE_NAME (sym);
    }
  else
    {
      struct bound_minimal_symbol msymbol
        = lookup_minimal_symbol_by_pc_section (sal->pc, sal->section);

      if (msymbol.minsym == NULL)
	return;

      objfile = msymbol.objfile;
      pc = BMSYMBOL_VALUE_ADDRESS (msymbol);
      section = MSYMBOL_OBJ_SECTION (objfile, msymbol.minsym);
      name = MSYMBOL_LINKAGE_NAME (msymbol.minsym);
    }

  gdbarch = get_objfile_arch (objfile);

  /* Process the prologue in two passes.  In the first pass try to skip the
     prologue (SKIP is true) and verify there is a real need for it (indicated
     by FORCE_SKIP).  If no such reason was found run a second pass where the
     prologue is not skipped (SKIP is false).  */

  skip = 1;
  force_skip = 1;

  /* Be conservative - allow direct PC (without skipping prologue) only if we
     have proven the CU (Compilation Unit) supports it.  sal->SYMTAB does not
     have to be set by the caller so we use SYM instead.  */
  if (sym != NULL
      && COMPUNIT_LOCATIONS_VALID (SYMTAB_COMPUNIT (symbol_symtab (sym))))
    force_skip = 0;

  saved_pc = pc;
  do
    {
      pc = saved_pc;

      /* If the function is in an unmapped overlay, use its unmapped LMA address,
	 so that gdbarch_skip_prologue has something unique to work on.  */
      if (section_is_overlay (section) && !section_is_mapped (section))
	pc = overlay_unmapped_address (pc, section);

      /* Skip "first line" of function (which is actually its prologue).  */
      pc += gdbarch_deprecated_function_start_offset (gdbarch);
      if (gdbarch_skip_entrypoint_p (gdbarch))
        pc = gdbarch_skip_entrypoint (gdbarch, pc);
      if (skip)
	pc = gdbarch_skip_prologue_noexcept (gdbarch, pc);

      /* For overlays, map pc back into its mapped VMA range.  */
      pc = overlay_mapped_address (pc, section);

      /* Calculate line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);

      /* Check if gdbarch_skip_prologue left us in mid-line, and the next
	 line is still part of the same function.  */
      if (skip && start_sal.pc != pc
	  && (sym ? (BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym)) <= start_sal.end
		     && start_sal.end < BLOCK_END (SYMBOL_BLOCK_VALUE (sym)))
	      : (lookup_minimal_symbol_by_pc_section (start_sal.end, section).minsym
		 == lookup_minimal_symbol_by_pc_section (pc, section).minsym)))
	{
	  /* First pc of next line */
	  pc = start_sal.end;
	  /* Recalculate the line number (might not be N+1).  */
	  start_sal = find_pc_sect_line (pc, section, 0);
	}

      /* On targets with executable formats that don't have a concept of
	 constructors (ELF with .init has, PE doesn't), gcc emits a call
	 to `__main' in `main' between the prologue and before user
	 code.  */
      if (gdbarch_skip_main_prologue_p (gdbarch)
	  && name && strcmp_iw (name, "main") == 0)
	{
	  pc = gdbarch_skip_main_prologue (gdbarch, pc);
	  /* Recalculate the line number (might not be N+1).  */
	  start_sal = find_pc_sect_line (pc, section, 0);
	  force_skip = 1;
	}
    }
  while (!force_skip && skip--);

  /* If we still don't have a valid source line, try to find the first
     PC in the lineinfo table that belongs to the same function.  This
     happens with COFF debug info, which does not seem to have an
     entry in lineinfo table for the code after the prologue which has
     no direct relation to source.  For example, this was found to be
     the case with the DJGPP target using "gcc -gcoff" when the
     compiler inserted code after the prologue to make sure the stack
     is aligned.  */
  if (!force_skip && sym && start_sal.symtab == NULL)
    {
      pc = skip_prologue_using_lineinfo (pc, symbol_symtab (sym));
      /* Recalculate the line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);
    }

  /* If we're already past the prologue, leave SAL unchanged.  Otherwise
     forward SAL to the end of the prologue.  */
  if (sal->pc >= pc)
    return;

  sal->pc = pc;
  sal->section = section;

  /* Unless the explicit_line flag was set, update the SAL line
     and symtab to correspond to the modified PC location.  */
  if (sal->explicit_line)
    return;

  sal->symtab = start_sal.symtab;
  sal->line = start_sal.line;
  sal->end = start_sal.end;

  /* Check if we are now inside an inlined function.  If we can,
     use the call site of the function instead.  */
  b = block_for_pc_sect (sal->pc, sal->section);
  function_block = NULL;
  while (b != NULL)
    {
      if (BLOCK_FUNCTION (b) != NULL && block_inlined_p (b))
	function_block = b;
      else if (BLOCK_FUNCTION (b) != NULL)
	break;
      b = BLOCK_SUPERBLOCK (b);
    }
  if (function_block != NULL
      && SYMBOL_LINE (BLOCK_FUNCTION (function_block)) != 0)
    {
      sal->line = SYMBOL_LINE (BLOCK_FUNCTION (function_block));
      sal->symtab = symbol_symtab (BLOCK_FUNCTION (function_block));
    }
}

/* Given PC at the function's start address, attempt to find the
   prologue end using SAL information.  Return zero if the skip fails.

   A non-optimized prologue traditionally has one SAL for the function
   and a second for the function body.  A single line function has
   them both pointing at the same line.

   An optimized prologue is similar but the prologue may contain
   instructions (SALs) from the instruction body.  Need to skip those
   while not getting into the function body.

   The functions end point and an increasing SAL line are used as
   indicators of the prologue's endpoint.

   This code is based on the function refine_prologue_limit
   (found in ia64).  */

CORE_ADDR
skip_prologue_using_sal (struct gdbarch *gdbarch, CORE_ADDR func_addr)
{
  struct symtab_and_line prologue_sal;
  CORE_ADDR start_pc;
  CORE_ADDR end_pc;
  const struct block *bl;

  /* Get an initial range for the function.  */
  find_pc_partial_function (func_addr, NULL, &start_pc, &end_pc);
  start_pc += gdbarch_deprecated_function_start_offset (gdbarch);

  prologue_sal = find_pc_line (start_pc, 0);
  if (prologue_sal.line != 0)
    {
      /* For languages other than assembly, treat two consecutive line
	 entries at the same address as a zero-instruction prologue.
	 The GNU assembler emits separate line notes for each instruction
	 in a multi-instruction macro, but compilers generally will not
	 do this.  */
      if (prologue_sal.symtab->language != language_asm)
	{
	  struct linetable *linetable = SYMTAB_LINETABLE (prologue_sal.symtab);
	  int idx = 0;

	  /* Skip any earlier lines, and any end-of-sequence marker
	     from a previous function.  */
	  while (linetable->item[idx].pc != prologue_sal.pc
		 || linetable->item[idx].line == 0)
	    idx++;

	  if (idx+1 < linetable->nitems
	      && linetable->item[idx+1].line != 0
	      && linetable->item[idx+1].pc == start_pc)
	    return start_pc;
	}

      /* If there is only one sal that covers the entire function,
	 then it is probably a single line function, like
	 "foo(){}".  */
      if (prologue_sal.end >= end_pc)
	return 0;

      while (prologue_sal.end < end_pc)
	{
	  struct symtab_and_line sal;

	  sal = find_pc_line (prologue_sal.end, 0);
	  if (sal.line == 0)
	    break;
	  /* Assume that a consecutive SAL for the same (or larger)
	     line mark the prologue -> body transition.  */
	  if (sal.line >= prologue_sal.line)
	    break;
	  /* Likewise if we are in a different symtab altogether
	     (e.g. within a file included via #include).  */
	  if (sal.symtab != prologue_sal.symtab)
	    break;

	  /* The line number is smaller.  Check that it's from the
	     same function, not something inlined.  If it's inlined,
	     then there is no point comparing the line numbers.  */
	  bl = block_for_pc (prologue_sal.end);
	  while (bl)
	    {
	      if (block_inlined_p (bl))
		break;
	      if (BLOCK_FUNCTION (bl))
		{
		  bl = NULL;
		  break;
		}
	      bl = BLOCK_SUPERBLOCK (bl);
	    }
	  if (bl != NULL)
	    break;

	  /* The case in which compiler's optimizer/scheduler has
	     moved instructions into the prologue.  We look ahead in
	     the function looking for address ranges whose
	     corresponding line number is less the first one that we
	     found for the function.  This is more conservative then
	     refine_prologue_limit which scans a large number of SALs
	     looking for any in the prologue.  */
	  prologue_sal = sal;
	}
    }

  if (prologue_sal.end < end_pc)
    /* Return the end of this line, or zero if we could not find a
       line.  */
    return prologue_sal.end;
  else
    /* Don't return END_PC, which is past the end of the function.  */
    return prologue_sal.pc;
}

/* See symtab.h.  */

symbol *
find_function_alias_target (bound_minimal_symbol msymbol)
{
  CORE_ADDR func_addr;
  if (!msymbol_is_function (msymbol.objfile, msymbol.minsym, &func_addr))
    return NULL;

  symbol *sym = find_pc_function (func_addr);
  if (sym != NULL
      && SYMBOL_CLASS (sym) == LOC_BLOCK
      && BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym)) == func_addr)
    return sym;

  return NULL;
}


/* If P is of the form "operator[ \t]+..." where `...' is
   some legitimate operator text, return a pointer to the
   beginning of the substring of the operator text.
   Otherwise, return "".  */

static const char *
operator_chars (const char *p, const char **end)
{
  *end = "";
  if (!startswith (p, CP_OPERATOR_STR))
    return *end;
  p += CP_OPERATOR_LEN;

  /* Don't get faked out by `operator' being part of a longer
     identifier.  */
  if (isalpha (*p) || *p == '_' || *p == '$' || *p == '\0')
    return *end;

  /* Allow some whitespace between `operator' and the operator symbol.  */
  while (*p == ' ' || *p == '\t')
    p++;

  /* Recognize 'operator TYPENAME'.  */

  if (isalpha (*p) || *p == '_' || *p == '$')
    {
      const char *q = p + 1;

      while (isalnum (*q) || *q == '_' || *q == '$')
	q++;
      *end = q;
      return p;
    }

  while (*p)
    switch (*p)
      {
      case '\\':			/* regexp quoting */
	if (p[1] == '*')
	  {
	    if (p[2] == '=')		/* 'operator\*=' */
	      *end = p + 3;
	    else			/* 'operator\*'  */
	      *end = p + 2;
	    return p;
	  }
	else if (p[1] == '[')
	  {
	    if (p[2] == ']')
	      error (_("mismatched quoting on brackets, "
		       "try 'operator\\[\\]'"));
	    else if (p[2] == '\\' && p[3] == ']')
	      {
		*end = p + 4;	/* 'operator\[\]' */
		return p;
	      }
	    else
	      error (_("nothing is allowed between '[' and ']'"));
	  }
	else
	  {
	    /* Gratuitous qoute: skip it and move on.  */
	    p++;
	    continue;
	  }
	break;
      case '!':
      case '=':
      case '*':
      case '/':
      case '%':
      case '^':
	if (p[1] == '=')
	  *end = p + 2;
	else
	  *end = p + 1;
	return p;
      case '<':
      case '>':
      case '+':
      case '-':
      case '&':
      case '|':
	if (p[0] == '-' && p[1] == '>')
	  {
	    /* Struct pointer member operator 'operator->'.  */
	    if (p[2] == '*')
	      {
		*end = p + 3;	/* 'operator->*' */
		return p;
	      }
	    else if (p[2] == '\\')
	      {
		*end = p + 4;	/* Hopefully 'operator->\*' */
		return p;
	      }
	    else
	      {
		*end = p + 2;	/* 'operator->' */
		return p;
	      }
	  }
	if (p[1] == '=' || p[1] == p[0])
	  *end = p + 2;
	else
	  *end = p + 1;
	return p;
      case '~':
      case ',':
	*end = p + 1;
	return p;
      case '(':
	if (p[1] != ')')
	  error (_("`operator ()' must be specified "
		   "without whitespace in `()'"));
	*end = p + 2;
	return p;
      case '?':
	if (p[1] != ':')
	  error (_("`operator ?:' must be specified "
		   "without whitespace in `?:'"));
	*end = p + 2;
	return p;
      case '[':
	if (p[1] != ']')
	  error (_("`operator []' must be specified "
		   "without whitespace in `[]'"));
	*end = p + 2;
	return p;
      default:
	error (_("`operator %s' not supported"), p);
	break;
      }

  *end = "";
  return *end;
}


/* Data structure to maintain printing state for output_source_filename.  */

struct output_source_filename_data
{
  /* Cache of what we've seen so far.  */
  struct filename_seen_cache *filename_seen_cache;

  /* Flag of whether we're printing the first one.  */
  int first;
};

/* Slave routine for sources_info.  Force line breaks at ,'s.
   NAME is the name to print.
   DATA contains the state for printing and watching for duplicates.  */

static void
output_source_filename (const char *name,
			struct output_source_filename_data *data)
{
  /* Since a single source file can result in several partial symbol
     tables, we need to avoid printing it more than once.  Note: if
     some of the psymtabs are read in and some are not, it gets
     printed both under "Source files for which symbols have been
     read" and "Source files for which symbols will be read in on
     demand".  I consider this a reasonable way to deal with the
     situation.  I'm not sure whether this can also happen for
     symtabs; it doesn't hurt to check.  */

  /* Was NAME already seen?  */
  if (data->filename_seen_cache->seen (name))
    {
      /* Yes; don't print it again.  */
      return;
    }

  /* No; print it and reset *FIRST.  */
  if (! data->first)
    printf_filtered (", ");
  data->first = 0;

  wrap_here ("");
  fputs_styled (name, file_name_style.style (), gdb_stdout);
}

/* A callback for map_partial_symbol_filenames.  */

static void
output_partial_symbol_filename (const char *filename, const char *fullname,
				void *data)
{
  output_source_filename (fullname ? fullname : filename,
			  (struct output_source_filename_data *) data);
}

static void
info_sources_command (const char *ignore, int from_tty)
{
  struct output_source_filename_data data;

  if (!have_full_symbols () && !have_partial_symbols ())
    {
      error (_("No symbol table is loaded.  Use the \"file\" command."));
    }

  filename_seen_cache filenames_seen;

  data.filename_seen_cache = &filenames_seen;

  printf_filtered ("Source files for which symbols have been read in:\n\n");

  data.first = 1;
  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *s : compunit_filetabs (cu))
	    {
	      const char *fullname = symtab_to_fullname (s);

	      output_source_filename (fullname, &data);
	    }
	}
    }
  printf_filtered ("\n\n");

  printf_filtered ("Source files for which symbols "
		   "will be read in on demand:\n\n");

  filenames_seen.clear ();
  data.first = 1;
  map_symbol_filenames (output_partial_symbol_filename, &data,
			1 /*need_fullname*/);
  printf_filtered ("\n");
}

/* Compare FILE against all the NFILES entries of FILES.  If BASENAMES is
   non-zero compare only lbasename of FILES.  */

static int
file_matches (const char *file, const char *files[], int nfiles, int basenames)
{
  int i;

  if (file != NULL && nfiles != 0)
    {
      for (i = 0; i < nfiles; i++)
	{
	  if (compare_filenames_for_search (file, (basenames
						   ? lbasename (files[i])
						   : files[i])))
	    return 1;
	}
    }
  else if (nfiles == 0)
    return 1;
  return 0;
}

/* Helper function for sort_search_symbols_remove_dups and qsort.  Can only
   sort symbols, not minimal symbols.  */

int
symbol_search::compare_search_syms (const symbol_search &sym_a,
				    const symbol_search &sym_b)
{
  int c;

  c = FILENAME_CMP (symbol_symtab (sym_a.symbol)->filename,
		    symbol_symtab (sym_b.symbol)->filename);
  if (c != 0)
    return c;

  if (sym_a.block != sym_b.block)
    return sym_a.block - sym_b.block;

  return strcmp (SYMBOL_PRINT_NAME (sym_a.symbol),
		 SYMBOL_PRINT_NAME (sym_b.symbol));
}

/* Returns true if the type_name of symbol_type of SYM matches TREG.
   If SYM has no symbol_type or symbol_name, returns false.  */

bool
treg_matches_sym_type_name (const compiled_regex &treg,
			    const struct symbol *sym)
{
  struct type *sym_type;
  std::string printed_sym_type_name;

  if (symbol_lookup_debug > 1)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "treg_matches_sym_type_name\n     sym %s\n",
			  SYMBOL_NATURAL_NAME (sym));
    }

  sym_type = SYMBOL_TYPE (sym);
  if (sym_type == NULL)
    return false;

  {
    scoped_switch_to_sym_language_if_auto l (sym);

    printed_sym_type_name = type_to_string (sym_type);
  }


  if (symbol_lookup_debug > 1)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "     sym_type_name %s\n",
			  printed_sym_type_name.c_str ());
    }


  if (printed_sym_type_name.empty ())
    return false;

  return treg.exec (printed_sym_type_name.c_str (), 0, NULL, 0) == 0;
}


/* Sort the symbols in RESULT and remove duplicates.  */

static void
sort_search_symbols_remove_dups (std::vector<symbol_search> *result)
{
  std::sort (result->begin (), result->end ());
  result->erase (std::unique (result->begin (), result->end ()),
		 result->end ());
}

/* Search the symbol table for matches to the regular expression REGEXP,
   returning the results.

   Only symbols of KIND are searched:
   VARIABLES_DOMAIN - search all symbols, excluding functions, type names,
                      and constants (enums).
		      if T_REGEXP is not NULL, only returns var that have
		      a type matching regular expression T_REGEXP.
   FUNCTIONS_DOMAIN - search all functions
   TYPES_DOMAIN     - search all type names
   ALL_DOMAIN       - an internal error for this function

   Within each file the results are sorted locally; each symtab's global and
   static blocks are separately alphabetized.
   Duplicate entries are removed.  */

std::vector<symbol_search>
search_symbols (const char *regexp, enum search_domain kind,
		const char *t_regexp,
		int nfiles, const char *files[])
{
  const struct blockvector *bv;
  struct block *b;
  int i = 0;
  struct block_iterator iter;
  struct symbol *sym;
  int found_misc = 0;
  static const enum minimal_symbol_type types[]
    = {mst_data, mst_text, mst_abs};
  static const enum minimal_symbol_type types2[]
    = {mst_bss, mst_file_text, mst_abs};
  static const enum minimal_symbol_type types3[]
    = {mst_file_data, mst_solib_trampoline, mst_abs};
  static const enum minimal_symbol_type types4[]
    = {mst_file_bss, mst_text_gnu_ifunc, mst_abs};
  enum minimal_symbol_type ourtype;
  enum minimal_symbol_type ourtype2;
  enum minimal_symbol_type ourtype3;
  enum minimal_symbol_type ourtype4;
  std::vector<symbol_search> result;
  gdb::optional<compiled_regex> preg;
  gdb::optional<compiled_regex> treg;

  gdb_assert (kind <= TYPES_DOMAIN);

  ourtype = types[kind];
  ourtype2 = types2[kind];
  ourtype3 = types3[kind];
  ourtype4 = types4[kind];

  if (regexp != NULL)
    {
      /* Make sure spacing is right for C++ operators.
         This is just a courtesy to make the matching less sensitive
         to how many spaces the user leaves between 'operator'
         and <TYPENAME> or <OPERATOR>.  */
      const char *opend;
      const char *opname = operator_chars (regexp, &opend);

      if (*opname)
	{
	  int fix = -1;		/* -1 means ok; otherwise number of
                                    spaces needed.  */

	  if (isalpha (*opname) || *opname == '_' || *opname == '$')
	    {
	      /* There should 1 space between 'operator' and 'TYPENAME'.  */
	      if (opname[-1] != ' ' || opname[-2] == ' ')
		fix = 1;
	    }
	  else
	    {
	      /* There should 0 spaces between 'operator' and 'OPERATOR'.  */
	      if (opname[-1] == ' ')
		fix = 0;
	    }
	  /* If wrong number of spaces, fix it.  */
	  if (fix >= 0)
	    {
	      char *tmp = (char *) alloca (8 + fix + strlen (opname) + 1);

	      sprintf (tmp, "operator%.*s%s", fix, " ", opname);
	      regexp = tmp;
	    }
	}

      int cflags = REG_NOSUB | (case_sensitivity == case_sensitive_off
				? REG_ICASE : 0);
      preg.emplace (regexp, cflags, _("Invalid regexp"));
    }

  if (t_regexp != NULL)
    {
      int cflags = REG_NOSUB | (case_sensitivity == case_sensitive_off
				? REG_ICASE : 0);
      treg.emplace (t_regexp, cflags, _("Invalid regexp"));
    }

  /* Search through the partial symtabs *first* for all symbols
     matching the regexp.  That way we don't have to reproduce all of
     the machinery below.  */
  expand_symtabs_matching ([&] (const char *filename, bool basenames)
			   {
			     return file_matches (filename, files, nfiles,
						  basenames);
			   },
			   lookup_name_info::match_any (),
			   [&] (const char *symname)
			   {
			     return (!preg.has_value ()
				     || preg->exec (symname,
						    0, NULL, 0) == 0);
			   },
			   NULL,
			   kind);

  /* Here, we search through the minimal symbol tables for functions
     and variables that match, and force their symbols to be read.
     This is in particular necessary for demangled variable names,
     which are no longer put into the partial symbol tables.
     The symbol will then be found during the scan of symtabs below.

     For functions, find_pc_symtab should succeed if we have debug info
     for the function, for variables we have to call
     lookup_symbol_in_objfile_from_linkage_name to determine if the variable
     has debug info.
     If the lookup fails, set found_misc so that we will rescan to print
     any matching symbols without debug info.
     We only search the objfile the msymbol came from, we no longer search
     all objfiles.  In large programs (1000s of shared libs) searching all
     objfiles is not worth the pain.  */

  if (nfiles == 0 && (kind == VARIABLES_DOMAIN || kind == FUNCTIONS_DOMAIN))
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (minimal_symbol *msymbol : objfile->msymbols ())
	    {
	      QUIT;

	      if (msymbol->created_by_gdb)
		continue;

	      if (MSYMBOL_TYPE (msymbol) == ourtype
		  || MSYMBOL_TYPE (msymbol) == ourtype2
		  || MSYMBOL_TYPE (msymbol) == ourtype3
		  || MSYMBOL_TYPE (msymbol) == ourtype4)
		{
		  if (!preg.has_value ()
		      || preg->exec (MSYMBOL_NATURAL_NAME (msymbol), 0,
				     NULL, 0) == 0)
		    {
		      /* Note: An important side-effect of these
			 lookup functions is to expand the symbol
			 table if msymbol is found, for the benefit of
			 the next loop on compunits.  */
		      if (kind == FUNCTIONS_DOMAIN
			  ? (find_pc_compunit_symtab
			     (MSYMBOL_VALUE_ADDRESS (objfile, msymbol))
			     == NULL)
			  : (lookup_symbol_in_objfile_from_linkage_name
			     (objfile, MSYMBOL_LINKAGE_NAME (msymbol),
			      VAR_DOMAIN)
			     .symbol == NULL))
			found_misc = 1;
		    }
		}
	    }
	}
    }

  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : objfile->compunits ())
	{
	  bv = COMPUNIT_BLOCKVECTOR (cust);
	  for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
	    {
	      b = BLOCKVECTOR_BLOCK (bv, i);
	      ALL_BLOCK_SYMBOLS (b, iter, sym)
		{
		  struct symtab *real_symtab = symbol_symtab (sym);

		  QUIT;

		  /* Check first sole REAL_SYMTAB->FILENAME.  It does
		     not need to be a substring of symtab_to_fullname as
		     it may contain "./" etc.  */
		  if ((file_matches (real_symtab->filename, files, nfiles, 0)
		       || ((basenames_may_differ
			    || file_matches (lbasename (real_symtab->filename),
					     files, nfiles, 1))
			   && file_matches (symtab_to_fullname (real_symtab),
					    files, nfiles, 0)))
		      && ((!preg.has_value ()
			   || preg->exec (SYMBOL_NATURAL_NAME (sym), 0,
					  NULL, 0) == 0)
			  && ((kind == VARIABLES_DOMAIN
			       && SYMBOL_CLASS (sym) != LOC_TYPEDEF
			       && SYMBOL_CLASS (sym) != LOC_UNRESOLVED
			       && SYMBOL_CLASS (sym) != LOC_BLOCK
			       /* LOC_CONST can be used for more than
				  just enums, e.g., c++ static const
				  members.  We only want to skip enums
				  here.  */
			       && !(SYMBOL_CLASS (sym) == LOC_CONST
				    && (TYPE_CODE (SYMBOL_TYPE (sym))
					== TYPE_CODE_ENUM))
			       && (!treg.has_value ()
				   || treg_matches_sym_type_name (*treg, sym)))
			      || (kind == FUNCTIONS_DOMAIN
				  && SYMBOL_CLASS (sym) == LOC_BLOCK
				  && (!treg.has_value ()
				      || treg_matches_sym_type_name (*treg,
								     sym)))
			      || (kind == TYPES_DOMAIN
				  && SYMBOL_CLASS (sym) == LOC_TYPEDEF))))
		    {
		      /* match */
		      result.emplace_back (i, sym);
		    }
		}
	    }
	}
    }

  if (!result.empty ())
    sort_search_symbols_remove_dups (&result);

  /* If there are no eyes, avoid all contact.  I mean, if there are
     no debug symbols, then add matching minsyms.  But if the user wants
     to see symbols matching a type regexp, then never give a minimal symbol,
     as we assume that a minimal symbol does not have a type.  */

  if ((found_misc || (nfiles == 0 && kind != FUNCTIONS_DOMAIN))
      && !treg.has_value ())
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (minimal_symbol *msymbol : objfile->msymbols ())
	    {
	      QUIT;

	      if (msymbol->created_by_gdb)
		continue;

	      if (MSYMBOL_TYPE (msymbol) == ourtype
		  || MSYMBOL_TYPE (msymbol) == ourtype2
		  || MSYMBOL_TYPE (msymbol) == ourtype3
		  || MSYMBOL_TYPE (msymbol) == ourtype4)
		{
		  if (!preg.has_value ()
		      || preg->exec (MSYMBOL_NATURAL_NAME (msymbol), 0,
				     NULL, 0) == 0)
		    {
		      /* For functions we can do a quick check of whether the
			 symbol might be found via find_pc_symtab.  */
		      if (kind != FUNCTIONS_DOMAIN
			  || (find_pc_compunit_symtab
			      (MSYMBOL_VALUE_ADDRESS (objfile, msymbol))
			      == NULL))
			{
			  if (lookup_symbol_in_objfile_from_linkage_name
			      (objfile, MSYMBOL_LINKAGE_NAME (msymbol),
			       VAR_DOMAIN)
			      .symbol == NULL)
			    {
			      /* match */
			      result.emplace_back (i, msymbol, objfile);
			    }
			}
		    }
		}
	    }
	}
    }

  return result;
}

/* Helper function for symtab_symbol_info, this function uses
   the data returned from search_symbols() to print information
   regarding the match to gdb_stdout.  If LAST is not NULL,
   print file and line number information for the symbol as
   well.  Skip printing the filename if it matches LAST.  */

static void
print_symbol_info (enum search_domain kind,
		   struct symbol *sym,
		   int block, const char *last)
{
  scoped_switch_to_sym_language_if_auto l (sym);
  struct symtab *s = symbol_symtab (sym);

  if (last != NULL)
    {
      const char *s_filename = symtab_to_filename_for_display (s);

      if (filename_cmp (last, s_filename) != 0)
	{
	  fputs_filtered ("\nFile ", gdb_stdout);
	  fputs_styled (s_filename, file_name_style.style (), gdb_stdout);
	  fputs_filtered (":\n", gdb_stdout);
	}

      if (SYMBOL_LINE (sym) != 0)
	printf_filtered ("%d:\t", SYMBOL_LINE (sym));
      else
	puts_filtered ("\t");
    }

  if (kind != TYPES_DOMAIN && block == STATIC_BLOCK)
    printf_filtered ("static ");

  /* Typedef that is not a C++ class.  */
  if (kind == TYPES_DOMAIN
      && SYMBOL_DOMAIN (sym) != STRUCT_DOMAIN)
    typedef_print (SYMBOL_TYPE (sym), sym, gdb_stdout);
  /* variable, func, or typedef-that-is-c++-class.  */
  else if (kind < TYPES_DOMAIN
	   || (kind == TYPES_DOMAIN
	       && SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN))
    {
      type_print (SYMBOL_TYPE (sym),
		  (SYMBOL_CLASS (sym) == LOC_TYPEDEF
		   ? "" : SYMBOL_PRINT_NAME (sym)),
		  gdb_stdout, 0);

      printf_filtered (";\n");
    }
}

/* This help function for symtab_symbol_info() prints information
   for non-debugging symbols to gdb_stdout.  */

static void
print_msymbol_info (struct bound_minimal_symbol msymbol)
{
  struct gdbarch *gdbarch = get_objfile_arch (msymbol.objfile);
  char *tmp;

  if (gdbarch_addr_bit (gdbarch) <= 32)
    tmp = hex_string_custom (BMSYMBOL_VALUE_ADDRESS (msymbol)
			     & (CORE_ADDR) 0xffffffff,
			     8);
  else
    tmp = hex_string_custom (BMSYMBOL_VALUE_ADDRESS (msymbol),
			     16);
  fputs_styled (tmp, address_style.style (), gdb_stdout);
  fputs_filtered ("  ", gdb_stdout);
  if (msymbol.minsym->text_p ())
    fputs_styled (MSYMBOL_PRINT_NAME (msymbol.minsym),
		  function_name_style.style (),
		  gdb_stdout);
  else
    fputs_filtered (MSYMBOL_PRINT_NAME (msymbol.minsym), gdb_stdout);
  fputs_filtered ("\n", gdb_stdout);
}

/* This is the guts of the commands "info functions", "info types", and
   "info variables".  It calls search_symbols to find all matches and then
   print_[m]symbol_info to print out some useful information about the
   matches.  */

static void
symtab_symbol_info (bool quiet,
		    const char *regexp, enum search_domain kind,
		    const char *t_regexp, int from_tty)
{
  static const char * const classnames[] =
    {"variable", "function", "type"};
  const char *last_filename = "";
  int first = 1;

  gdb_assert (kind <= TYPES_DOMAIN);

  /* Must make sure that if we're interrupted, symbols gets freed.  */
  std::vector<symbol_search> symbols = search_symbols (regexp, kind,
						       t_regexp, 0, NULL);

  if (!quiet)
    {
      if (regexp != NULL)
	{
	  if (t_regexp != NULL)
	    printf_filtered
	      (_("All %ss matching regular expression \"%s\""
		 " with type matching regular expression \"%s\":\n"),
	       classnames[kind], regexp, t_regexp);
	  else
	    printf_filtered (_("All %ss matching regular expression \"%s\":\n"),
			     classnames[kind], regexp);
	}
      else
	{
	  if (t_regexp != NULL)
	    printf_filtered
	      (_("All defined %ss"
		 " with type matching regular expression \"%s\" :\n"),
	       classnames[kind], t_regexp);
	  else
	    printf_filtered (_("All defined %ss:\n"), classnames[kind]);
	}
    }

  for (const symbol_search &p : symbols)
    {
      QUIT;

      if (p.msymbol.minsym != NULL)
	{
	  if (first)
	    {
	      if (!quiet)
		printf_filtered (_("\nNon-debugging symbols:\n"));
	      first = 0;
	    }
	  print_msymbol_info (p.msymbol);
	}
      else
	{
	  print_symbol_info (kind,
			     p.symbol,
			     p.block,
			     last_filename);
	  last_filename
	    = symtab_to_filename_for_display (symbol_symtab (p.symbol));
	}
    }
}

static void
info_variables_command (const char *args, int from_tty)
{
  std::string regexp;
  std::string t_regexp;
  bool quiet = false;

  while (args != NULL
	 && extract_info_print_args (&args, &quiet, &regexp, &t_regexp))
    ;

  if (args != NULL)
    report_unrecognized_option_error ("info variables", args);

  symtab_symbol_info (quiet,
		      regexp.empty () ? NULL : regexp.c_str (),
		      VARIABLES_DOMAIN,
		      t_regexp.empty () ? NULL : t_regexp.c_str (),
		      from_tty);
}


static void
info_functions_command (const char *args, int from_tty)
{
  std::string regexp;
  std::string t_regexp;
  bool quiet = false;

  while (args != NULL
	 && extract_info_print_args (&args, &quiet, &regexp, &t_regexp))
    ;

  if (args != NULL)
    report_unrecognized_option_error ("info functions", args);

  symtab_symbol_info (quiet,
		      regexp.empty () ? NULL : regexp.c_str (),
		      FUNCTIONS_DOMAIN,
		      t_regexp.empty () ? NULL : t_regexp.c_str (),
		      from_tty);
}


static void
info_types_command (const char *regexp, int from_tty)
{
  symtab_symbol_info (false, regexp, TYPES_DOMAIN, NULL, from_tty);
}

/* Breakpoint all functions matching regular expression.  */

void
rbreak_command_wrapper (char *regexp, int from_tty)
{
  rbreak_command (regexp, from_tty);
}

static void
rbreak_command (const char *regexp, int from_tty)
{
  std::string string;
  const char **files = NULL;
  const char *file_name;
  int nfiles = 0;

  if (regexp)
    {
      const char *colon = strchr (regexp, ':');

      if (colon && *(colon + 1) != ':')
	{
	  int colon_index;
	  char *local_name;

	  colon_index = colon - regexp;
	  local_name = (char *) alloca (colon_index + 1);
	  memcpy (local_name, regexp, colon_index);
	  local_name[colon_index--] = 0;
	  while (isspace (local_name[colon_index]))
	    local_name[colon_index--] = 0;
	  file_name = local_name;
	  files = &file_name;
	  nfiles = 1;
	  regexp = skip_spaces (colon + 1);
	}
    }

  std::vector<symbol_search> symbols = search_symbols (regexp,
						       FUNCTIONS_DOMAIN,
						       NULL,
						       nfiles, files);

  scoped_rbreak_breakpoints finalize;
  for (const symbol_search &p : symbols)
    {
      if (p.msymbol.minsym == NULL)
	{
	  struct symtab *symtab = symbol_symtab (p.symbol);
	  const char *fullname = symtab_to_fullname (symtab);

	  string = string_printf ("%s:'%s'", fullname,
				  SYMBOL_LINKAGE_NAME (p.symbol));
	  break_command (&string[0], from_tty);
	  print_symbol_info (FUNCTIONS_DOMAIN, p.symbol, p.block, NULL);
	}
      else
	{
	  string = string_printf ("'%s'",
				  MSYMBOL_LINKAGE_NAME (p.msymbol.minsym));

	  break_command (&string[0], from_tty);
	  printf_filtered ("<function, no debug info> %s;\n",
			   MSYMBOL_PRINT_NAME (p.msymbol.minsym));
	}
    }
}


/* Evaluate if SYMNAME matches LOOKUP_NAME.  */

static int
compare_symbol_name (const char *symbol_name, language symbol_language,
		     const lookup_name_info &lookup_name,
		     completion_match_result &match_res)
{
  const language_defn *lang = language_def (symbol_language);

  symbol_name_matcher_ftype *name_match
    = get_symbol_name_matcher (lang, lookup_name);

  return name_match (symbol_name, lookup_name, &match_res);
}

/*  See symtab.h.  */

void
completion_list_add_name (completion_tracker &tracker,
			  language symbol_language,
			  const char *symname,
			  const lookup_name_info &lookup_name,
			  const char *text, const char *word)
{
  completion_match_result &match_res
    = tracker.reset_completion_match_result ();

  /* Clip symbols that cannot match.  */
  if (!compare_symbol_name (symname, symbol_language, lookup_name, match_res))
    return;

  /* Refresh SYMNAME from the match string.  It's potentially
     different depending on language.  (E.g., on Ada, the match may be
     the encoded symbol name wrapped in "<>").  */
  symname = match_res.match.match ();
  gdb_assert (symname != NULL);

  /* We have a match for a completion, so add SYMNAME to the current list
     of matches.  Note that the name is moved to freshly malloc'd space.  */

  {
    gdb::unique_xmalloc_ptr<char> completion
      = make_completion_match_str (symname, text, word);

    /* Here we pass the match-for-lcd object to add_completion.  Some
       languages match the user text against substrings of symbol
       names in some cases.  E.g., in C++, "b push_ba" completes to
       "std::vector::push_back", "std::string::push_back", etc., and
       in this case we want the completion lowest common denominator
       to be "push_back" instead of "std::".  */
    tracker.add_completion (std::move (completion),
			    &match_res.match_for_lcd, text, word);
  }
}

/* completion_list_add_name wrapper for struct symbol.  */

static void
completion_list_add_symbol (completion_tracker &tracker,
			    symbol *sym,
			    const lookup_name_info &lookup_name,
			    const char *text, const char *word)
{
  completion_list_add_name (tracker, SYMBOL_LANGUAGE (sym),
			    SYMBOL_NATURAL_NAME (sym),
			    lookup_name, text, word);
}

/* completion_list_add_name wrapper for struct minimal_symbol.  */

static void
completion_list_add_msymbol (completion_tracker &tracker,
			     minimal_symbol *sym,
			     const lookup_name_info &lookup_name,
			     const char *text, const char *word)
{
  completion_list_add_name (tracker, MSYMBOL_LANGUAGE (sym),
			    MSYMBOL_NATURAL_NAME (sym),
			    lookup_name, text, word);
}


/* ObjC: In case we are completing on a selector, look as the msymbol
   again and feed all the selectors into the mill.  */

static void
completion_list_objc_symbol (completion_tracker &tracker,
			     struct minimal_symbol *msymbol,
			     const lookup_name_info &lookup_name,
			     const char *text, const char *word)
{
  static char *tmp = NULL;
  static unsigned int tmplen = 0;

  const char *method, *category, *selector;
  char *tmp2 = NULL;

  method = MSYMBOL_NATURAL_NAME (msymbol);

  /* Is it a method?  */
  if ((method[0] != '-') && (method[0] != '+'))
    return;

  if (text[0] == '[')
    /* Complete on shortened method method.  */
    completion_list_add_name (tracker, language_objc,
			      method + 1,
			      lookup_name,
			      text, word);

  while ((strlen (method) + 1) >= tmplen)
    {
      if (tmplen == 0)
	tmplen = 1024;
      else
	tmplen *= 2;
      tmp = (char *) xrealloc (tmp, tmplen);
    }
  selector = strchr (method, ' ');
  if (selector != NULL)
    selector++;

  category = strchr (method, '(');

  if ((category != NULL) && (selector != NULL))
    {
      memcpy (tmp, method, (category - method));
      tmp[category - method] = ' ';
      memcpy (tmp + (category - method) + 1, selector, strlen (selector) + 1);
      completion_list_add_name (tracker, language_objc, tmp,
				lookup_name, text, word);
      if (text[0] == '[')
	completion_list_add_name (tracker, language_objc, tmp + 1,
				  lookup_name, text, word);
    }

  if (selector != NULL)
    {
      /* Complete on selector only.  */
      strcpy (tmp, selector);
      tmp2 = strchr (tmp, ']');
      if (tmp2 != NULL)
	*tmp2 = '\0';

      completion_list_add_name (tracker, language_objc, tmp,
				lookup_name, text, word);
    }
}

/* Break the non-quoted text based on the characters which are in
   symbols.  FIXME: This should probably be language-specific.  */

static const char *
language_search_unquoted_string (const char *text, const char *p)
{
  for (; p > text; --p)
    {
      if (isalnum (p[-1]) || p[-1] == '_' || p[-1] == '\0')
	continue;
      else
	{
	  if ((current_language->la_language == language_objc))
	    {
	      if (p[-1] == ':')     /* Might be part of a method name.  */
		continue;
	      else if (p[-1] == '[' && (p[-2] == '-' || p[-2] == '+'))
		p -= 2;             /* Beginning of a method name.  */
	      else if (p[-1] == ' ' || p[-1] == '(' || p[-1] == ')')
		{                   /* Might be part of a method name.  */
		  const char *t = p;

		  /* Seeing a ' ' or a '(' is not conclusive evidence
		     that we are in the middle of a method name.  However,
		     finding "-[" or "+[" should be pretty un-ambiguous.
		     Unfortunately we have to find it now to decide.  */

		  while (t > text)
		    if (isalnum (t[-1]) || t[-1] == '_' ||
			t[-1] == ' '    || t[-1] == ':' ||
			t[-1] == '('    || t[-1] == ')')
		      --t;
		    else
		      break;

		  if (t[-1] == '[' && (t[-2] == '-' || t[-2] == '+'))
		    p = t - 2;      /* Method name detected.  */
		  /* Else we leave with p unchanged.  */
		}
	    }
	  break;
	}
    }
  return p;
}

static void
completion_list_add_fields (completion_tracker &tracker,
			    struct symbol *sym,
			    const lookup_name_info &lookup_name,
			    const char *text, const char *word)
{
  if (SYMBOL_CLASS (sym) == LOC_TYPEDEF)
    {
      struct type *t = SYMBOL_TYPE (sym);
      enum type_code c = TYPE_CODE (t);
      int j;

      if (c == TYPE_CODE_UNION || c == TYPE_CODE_STRUCT)
	for (j = TYPE_N_BASECLASSES (t); j < TYPE_NFIELDS (t); j++)
	  if (TYPE_FIELD_NAME (t, j))
	    completion_list_add_name (tracker, SYMBOL_LANGUAGE (sym),
				      TYPE_FIELD_NAME (t, j),
				      lookup_name, text, word);
    }
}

/* See symtab.h.  */

bool
symbol_is_function_or_method (symbol *sym)
{
  switch (TYPE_CODE (SYMBOL_TYPE (sym)))
    {
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return true;
    default:
      return false;
    }
}

/* See symtab.h.  */

bool
symbol_is_function_or_method (minimal_symbol *msymbol)
{
  switch (MSYMBOL_TYPE (msymbol))
    {
    case mst_text:
    case mst_text_gnu_ifunc:
    case mst_solib_trampoline:
    case mst_file_text:
      return true;
    default:
      return false;
    }
}

/* See symtab.h.  */

bound_minimal_symbol
find_gnu_ifunc (const symbol *sym)
{
  if (SYMBOL_CLASS (sym) != LOC_BLOCK)
    return {};

  lookup_name_info lookup_name (SYMBOL_SEARCH_NAME (sym),
				symbol_name_match_type::SEARCH_NAME);
  struct objfile *objfile = symbol_objfile (sym);

  CORE_ADDR address = BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (sym));
  minimal_symbol *ifunc = NULL;

  iterate_over_minimal_symbols (objfile, lookup_name,
				[&] (minimal_symbol *minsym)
    {
      if (MSYMBOL_TYPE (minsym) == mst_text_gnu_ifunc
	  || MSYMBOL_TYPE (minsym) == mst_data_gnu_ifunc)
	{
	  CORE_ADDR msym_addr = MSYMBOL_VALUE_ADDRESS (objfile, minsym);
	  if (MSYMBOL_TYPE (minsym) == mst_data_gnu_ifunc)
	    {
	      struct gdbarch *gdbarch = get_objfile_arch (objfile);
	      msym_addr
		= gdbarch_convert_from_func_ptr_addr (gdbarch,
						      msym_addr,
						      current_top_target ());
	    }
	  if (msym_addr == address)
	    {
	      ifunc = minsym;
	      return true;
	    }
	}
      return false;
    });

  if (ifunc != NULL)
    return {ifunc, objfile};
  return {};
}

/* Add matching symbols from SYMTAB to the current completion list.  */

static void
add_symtab_completions (struct compunit_symtab *cust,
			completion_tracker &tracker,
			complete_symbol_mode mode,
			const lookup_name_info &lookup_name,
			const char *text, const char *word,
			enum type_code code)
{
  struct symbol *sym;
  const struct block *b;
  struct block_iterator iter;
  int i;

  if (cust == NULL)
    return;

  for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
    {
      QUIT;
      b = BLOCKVECTOR_BLOCK (COMPUNIT_BLOCKVECTOR (cust), i);
      ALL_BLOCK_SYMBOLS (b, iter, sym)
	{
	  if (completion_skip_symbol (mode, sym))
	    continue;

	  if (code == TYPE_CODE_UNDEF
	      || (SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN
		  && TYPE_CODE (SYMBOL_TYPE (sym)) == code))
	    completion_list_add_symbol (tracker, sym,
					lookup_name,
					text, word);
	}
    }
}

void
default_collect_symbol_completion_matches_break_on
  (completion_tracker &tracker, complete_symbol_mode mode,
   symbol_name_match_type name_match_type,
   const char *text, const char *word,
   const char *break_on, enum type_code code)
{
  /* Problem: All of the symbols have to be copied because readline
     frees them.  I'm not going to worry about this; hopefully there
     won't be that many.  */

  struct symbol *sym;
  const struct block *b;
  const struct block *surrounding_static_block, *surrounding_global_block;
  struct block_iterator iter;
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;

  /* Now look for the symbol we are supposed to complete on.  */
  if (mode == complete_symbol_mode::LINESPEC)
    sym_text = text;
  else
  {
    const char *p;
    char quote_found;
    const char *quote_pos = NULL;

    /* First see if this is a quoted string.  */
    quote_found = '\0';
    for (p = text; *p != '\0'; ++p)
      {
	if (quote_found != '\0')
	  {
	    if (*p == quote_found)
	      /* Found close quote.  */
	      quote_found = '\0';
	    else if (*p == '\\' && p[1] == quote_found)
	      /* A backslash followed by the quote character
	         doesn't end the string.  */
	      ++p;
	  }
	else if (*p == '\'' || *p == '"')
	  {
	    quote_found = *p;
	    quote_pos = p;
	  }
      }
    if (quote_found == '\'')
      /* A string within single quotes can be a symbol, so complete on it.  */
      sym_text = quote_pos + 1;
    else if (quote_found == '"')
      /* A double-quoted string is never a symbol, nor does it make sense
         to complete it any other way.  */
      {
	return;
      }
    else
      {
	/* It is not a quoted string.  Break it based on the characters
	   which are in symbols.  */
	while (p > text)
	  {
	    if (isalnum (p[-1]) || p[-1] == '_' || p[-1] == '\0'
		|| p[-1] == ':' || strchr (break_on, p[-1]) != NULL)
	      --p;
	    else
	      break;
	  }
	sym_text = p;
      }
  }

  lookup_name_info lookup_name (sym_text, name_match_type, true);

  /* At this point scan through the misc symbol vectors and add each
     symbol you find to the list.  Eventually we want to ignore
     anything that isn't a text symbol (everything else will be
     handled by the psymtab code below).  */

  if (code == TYPE_CODE_UNDEF)
    {
      for (objfile *objfile : current_program_space->objfiles ())
	{
	  for (minimal_symbol *msymbol : objfile->msymbols ())
	    {
	      QUIT;

	      if (completion_skip_symbol (mode, msymbol))
		continue;

	      completion_list_add_msymbol (tracker, msymbol, lookup_name,
					   sym_text, word);

	      completion_list_objc_symbol (tracker, msymbol, lookup_name,
					   sym_text, word);
	    }
	}
    }

  /* Add completions for all currently loaded symbol tables.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cust : objfile->compunits ())
	add_symtab_completions (cust, tracker, mode, lookup_name,
				sym_text, word, code);
    }

  /* Look through the partial symtabs for all symbols which begin by
     matching SYM_TEXT.  Expand all CUs that you find to the list.  */
  expand_symtabs_matching (NULL,
			   lookup_name,
			   NULL,
			   [&] (compunit_symtab *symtab) /* expansion notify */
			     {
			       add_symtab_completions (symtab,
						       tracker, mode, lookup_name,
						       sym_text, word, code);
			     },
			   ALL_DOMAIN);

  /* Search upwards from currently selected frame (so that we can
     complete on local vars).  Also catch fields of types defined in
     this places which match our text string.  Only complete on types
     visible from current context.  */

  b = get_selected_block (0);
  surrounding_static_block = block_static_block (b);
  surrounding_global_block = block_global_block (b);
  if (surrounding_static_block != NULL)
    while (b != surrounding_static_block)
      {
	QUIT;

	ALL_BLOCK_SYMBOLS (b, iter, sym)
	  {
	    if (code == TYPE_CODE_UNDEF)
	      {
		completion_list_add_symbol (tracker, sym, lookup_name,
					    sym_text, word);
		completion_list_add_fields (tracker, sym, lookup_name,
					    sym_text, word);
	      }
	    else if (SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN
		     && TYPE_CODE (SYMBOL_TYPE (sym)) == code)
	      completion_list_add_symbol (tracker, sym, lookup_name,
					  sym_text, word);
	  }

	/* Stop when we encounter an enclosing function.  Do not stop for
	   non-inlined functions - the locals of the enclosing function
	   are in scope for a nested function.  */
	if (BLOCK_FUNCTION (b) != NULL && block_inlined_p (b))
	  break;
	b = BLOCK_SUPERBLOCK (b);
      }

  /* Add fields from the file's types; symbols will be added below.  */

  if (code == TYPE_CODE_UNDEF)
    {
      if (surrounding_static_block != NULL)
	ALL_BLOCK_SYMBOLS (surrounding_static_block, iter, sym)
	  completion_list_add_fields (tracker, sym, lookup_name,
				      sym_text, word);

      if (surrounding_global_block != NULL)
	ALL_BLOCK_SYMBOLS (surrounding_global_block, iter, sym)
	  completion_list_add_fields (tracker, sym, lookup_name,
				      sym_text, word);
    }

  /* Skip macros if we are completing a struct tag -- arguable but
     usually what is expected.  */
  if (current_language->la_macro_expansion == macro_expansion_c
      && code == TYPE_CODE_UNDEF)
    {
      gdb::unique_xmalloc_ptr<struct macro_scope> scope;

      /* This adds a macro's name to the current completion list.  */
      auto add_macro_name = [&] (const char *macro_name,
				 const macro_definition *,
				 macro_source_file *,
				 int)
	{
	  completion_list_add_name (tracker, language_c, macro_name,
				    lookup_name, sym_text, word);
	};

      /* Add any macros visible in the default scope.  Note that this
	 may yield the occasional wrong result, because an expression
	 might be evaluated in a scope other than the default.  For
	 example, if the user types "break file:line if <TAB>", the
	 resulting expression will be evaluated at "file:line" -- but
	 at there does not seem to be a way to detect this at
	 completion time.  */
      scope = default_macro_scope ();
      if (scope)
	macro_for_each_in_scope (scope->file, scope->line,
				 add_macro_name);

      /* User-defined macros are always visible.  */
      macro_for_each (macro_user_macros, add_macro_name);
    }
}

void
default_collect_symbol_completion_matches (completion_tracker &tracker,
					   complete_symbol_mode mode,
					   symbol_name_match_type name_match_type,
					   const char *text, const char *word,
					   enum type_code code)
{
  return default_collect_symbol_completion_matches_break_on (tracker, mode,
							     name_match_type,
							     text, word, "",
							     code);
}

/* Collect all symbols (regardless of class) which begin by matching
   TEXT.  */

void
collect_symbol_completion_matches (completion_tracker &tracker,
				   complete_symbol_mode mode,
				   symbol_name_match_type name_match_type,
				   const char *text, const char *word)
{
  current_language->la_collect_symbol_completion_matches (tracker, mode,
							  name_match_type,
							  text, word,
							  TYPE_CODE_UNDEF);
}

/* Like collect_symbol_completion_matches, but only collect
   STRUCT_DOMAIN symbols whose type code is CODE.  */

void
collect_symbol_completion_matches_type (completion_tracker &tracker,
					const char *text, const char *word,
					enum type_code code)
{
  complete_symbol_mode mode = complete_symbol_mode::EXPRESSION;
  symbol_name_match_type name_match_type = symbol_name_match_type::EXPRESSION;

  gdb_assert (code == TYPE_CODE_UNION
	      || code == TYPE_CODE_STRUCT
	      || code == TYPE_CODE_ENUM);
  current_language->la_collect_symbol_completion_matches (tracker, mode,
							  name_match_type,
							  text, word, code);
}

/* Like collect_symbol_completion_matches, but collects a list of
   symbols defined in all source files named SRCFILE.  */

void
collect_file_symbol_completion_matches (completion_tracker &tracker,
					complete_symbol_mode mode,
					symbol_name_match_type name_match_type,
					const char *text, const char *word,
					const char *srcfile)
{
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;

  /* Now look for the symbol we are supposed to complete on.
     FIXME: This should be language-specific.  */
  if (mode == complete_symbol_mode::LINESPEC)
    sym_text = text;
  else
  {
    const char *p;
    char quote_found;
    const char *quote_pos = NULL;

    /* First see if this is a quoted string.  */
    quote_found = '\0';
    for (p = text; *p != '\0'; ++p)
      {
	if (quote_found != '\0')
	  {
	    if (*p == quote_found)
	      /* Found close quote.  */
	      quote_found = '\0';
	    else if (*p == '\\' && p[1] == quote_found)
	      /* A backslash followed by the quote character
	         doesn't end the string.  */
	      ++p;
	  }
	else if (*p == '\'' || *p == '"')
	  {
	    quote_found = *p;
	    quote_pos = p;
	  }
      }
    if (quote_found == '\'')
      /* A string within single quotes can be a symbol, so complete on it.  */
      sym_text = quote_pos + 1;
    else if (quote_found == '"')
      /* A double-quoted string is never a symbol, nor does it make sense
         to complete it any other way.  */
      {
	return;
      }
    else
      {
	/* Not a quoted string.  */
	sym_text = language_search_unquoted_string (text, p);
      }
  }

  lookup_name_info lookup_name (sym_text, name_match_type, true);

  /* Go through symtabs for SRCFILE and check the externs and statics
     for symbols which match.  */
  iterate_over_symtabs (srcfile, [&] (symtab *s)
    {
      add_symtab_completions (SYMTAB_COMPUNIT (s),
			      tracker, mode, lookup_name,
			      sym_text, word, TYPE_CODE_UNDEF);
      return false;
    });
}

/* A helper function for make_source_files_completion_list.  It adds
   another file name to a list of possible completions, growing the
   list as necessary.  */

static void
add_filename_to_list (const char *fname, const char *text, const char *word,
		      completion_list *list)
{
  list->emplace_back (make_completion_match_str (fname, text, word));
}

static int
not_interesting_fname (const char *fname)
{
  static const char *illegal_aliens[] = {
    "_globals_",	/* inserted by coff_symtab_read */
    NULL
  };
  int i;

  for (i = 0; illegal_aliens[i]; i++)
    {
      if (filename_cmp (fname, illegal_aliens[i]) == 0)
	return 1;
    }
  return 0;
}

/* An object of this type is passed as the user_data argument to
   map_partial_symbol_filenames.  */
struct add_partial_filename_data
{
  struct filename_seen_cache *filename_seen_cache;
  const char *text;
  const char *word;
  int text_len;
  completion_list *list;
};

/* A callback for map_partial_symbol_filenames.  */

static void
maybe_add_partial_symtab_filename (const char *filename, const char *fullname,
				   void *user_data)
{
  struct add_partial_filename_data *data
    = (struct add_partial_filename_data *) user_data;

  if (not_interesting_fname (filename))
    return;
  if (!data->filename_seen_cache->seen (filename)
      && filename_ncmp (filename, data->text, data->text_len) == 0)
    {
      /* This file matches for a completion; add it to the
	 current list of matches.  */
      add_filename_to_list (filename, data->text, data->word, data->list);
    }
  else
    {
      const char *base_name = lbasename (filename);

      if (base_name != filename
	  && !data->filename_seen_cache->seen (base_name)
	  && filename_ncmp (base_name, data->text, data->text_len) == 0)
	add_filename_to_list (base_name, data->text, data->word, data->list);
    }
}

/* Return a list of all source files whose names begin with matching
   TEXT.  The file names are looked up in the symbol tables of this
   program.  */

completion_list
make_source_files_completion_list (const char *text, const char *word)
{
  size_t text_len = strlen (text);
  completion_list list;
  const char *base_name;
  struct add_partial_filename_data datum;

  if (!have_full_symbols () && !have_partial_symbols ())
    return list;

  filename_seen_cache filenames_seen;

  for (objfile *objfile : current_program_space->objfiles ())
    {
      for (compunit_symtab *cu : objfile->compunits ())
	{
	  for (symtab *s : compunit_filetabs (cu))
	    {
	      if (not_interesting_fname (s->filename))
		continue;
	      if (!filenames_seen.seen (s->filename)
		  && filename_ncmp (s->filename, text, text_len) == 0)
		{
		  /* This file matches for a completion; add it to the current
		     list of matches.  */
		  add_filename_to_list (s->filename, text, word, &list);
		}
	      else
		{
		  /* NOTE: We allow the user to type a base name when the
		     debug info records leading directories, but not the other
		     way around.  This is what subroutines of breakpoint
		     command do when they parse file names.  */
		  base_name = lbasename (s->filename);
		  if (base_name != s->filename
		      && !filenames_seen.seen (base_name)
		      && filename_ncmp (base_name, text, text_len) == 0)
		    add_filename_to_list (base_name, text, word, &list);
		}
	    }
	}
    }

  datum.filename_seen_cache = &filenames_seen;
  datum.text = text;
  datum.word = word;
  datum.text_len = text_len;
  datum.list = &list;
  map_symbol_filenames (maybe_add_partial_symtab_filename, &datum,
			0 /*need_fullname*/);

  return list;
}

/* Track MAIN */

/* Return the "main_info" object for the current program space.  If
   the object has not yet been created, create it and fill in some
   default values.  */

static struct main_info *
get_main_info (void)
{
  struct main_info *info
    = (struct main_info *) program_space_data (current_program_space,
					       main_progspace_key);

  if (info == NULL)
    {
      /* It may seem strange to store the main name in the progspace
	 and also in whatever objfile happens to see a main name in
	 its debug info.  The reason for this is mainly historical:
	 gdb returned "main" as the name even if no function named
	 "main" was defined the program; and this approach lets us
	 keep compatibility.  */
      info = XCNEW (struct main_info);
      info->language_of_main = language_unknown;
      set_program_space_data (current_program_space, main_progspace_key,
			      info);
    }

  return info;
}

/* A cleanup to destroy a struct main_info when a progspace is
   destroyed.  */

static void
main_info_cleanup (struct program_space *pspace, void *data)
{
  struct main_info *info = (struct main_info *) data;

  if (info != NULL)
    xfree (info->name_of_main);
  xfree (info);
}

static void
set_main_name (const char *name, enum language lang)
{
  struct main_info *info = get_main_info ();

  if (info->name_of_main != NULL)
    {
      xfree (info->name_of_main);
      info->name_of_main = NULL;
      info->language_of_main = language_unknown;
    }
  if (name != NULL)
    {
      info->name_of_main = xstrdup (name);
      info->language_of_main = lang;
    }
}

/* Deduce the name of the main procedure, and set NAME_OF_MAIN
   accordingly.  */

static void
find_main_name (void)
{
  const char *new_main_name;

  /* First check the objfiles to see whether a debuginfo reader has
     picked up the appropriate main name.  Historically the main name
     was found in a more or less random way; this approach instead
     relies on the order of objfile creation -- which still isn't
     guaranteed to get the correct answer, but is just probably more
     accurate.  */
  for (objfile *objfile : current_program_space->objfiles ())
    {
      if (objfile->per_bfd->name_of_main != NULL)
	{
	  set_main_name (objfile->per_bfd->name_of_main,
			 objfile->per_bfd->language_of_main);
	  return;
	}
    }

  /* Try to see if the main procedure is in Ada.  */
  /* FIXME: brobecker/2005-03-07: Another way of doing this would
     be to add a new method in the language vector, and call this
     method for each language until one of them returns a non-empty
     name.  This would allow us to remove this hard-coded call to
     an Ada function.  It is not clear that this is a better approach
     at this point, because all methods need to be written in a way
     such that false positives never be returned.  For instance, it is
     important that a method does not return a wrong name for the main
     procedure if the main procedure is actually written in a different
     language.  It is easy to guaranty this with Ada, since we use a
     special symbol generated only when the main in Ada to find the name
     of the main procedure.  It is difficult however to see how this can
     be guarantied for languages such as C, for instance.  This suggests
     that order of call for these methods becomes important, which means
     a more complicated approach.  */
  new_main_name = ada_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name, language_ada);
      return;
    }

  new_main_name = d_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name, language_d);
      return;
    }

  new_main_name = go_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name, language_go);
      return;
    }

  new_main_name = pascal_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name, language_pascal);
      return;
    }

  /* The languages above didn't identify the name of the main procedure.
     Fallback to "main".  */
  set_main_name ("main", language_unknown);
}

char *
main_name (void)
{
  struct main_info *info = get_main_info ();

  if (info->name_of_main == NULL)
    find_main_name ();

  return info->name_of_main;
}

/* Return the language of the main function.  If it is not known,
   return language_unknown.  */

enum language
main_language (void)
{
  struct main_info *info = get_main_info ();

  if (info->name_of_main == NULL)
    find_main_name ();

  return info->language_of_main;
}

/* Handle ``executable_changed'' events for the symtab module.  */

static void
symtab_observer_executable_changed (void)
{
  /* NAME_OF_MAIN may no longer be the same, so reset it for now.  */
  set_main_name (NULL, language_unknown);
}

/* Return 1 if the supplied producer string matches the ARM RealView
   compiler (armcc).  */

int
producer_is_realview (const char *producer)
{
  static const char *const arm_idents[] = {
    "ARM C Compiler, ADS",
    "Thumb C Compiler, ADS",
    "ARM C++ Compiler, ADS",
    "Thumb C++ Compiler, ADS",
    "ARM/Thumb C/C++ Compiler, RVCT",
    "ARM C/C++ Compiler, RVCT"
  };
  int i;

  if (producer == NULL)
    return 0;

  for (i = 0; i < ARRAY_SIZE (arm_idents); i++)
    if (startswith (producer, arm_idents[i]))
      return 1;

  return 0;
}



/* The next index to hand out in response to a registration request.  */

static int next_aclass_value = LOC_FINAL_VALUE;

/* The maximum number of "aclass" registrations we support.  This is
   constant for convenience.  */
#define MAX_SYMBOL_IMPLS (LOC_FINAL_VALUE + 10)

/* The objects representing the various "aclass" values.  The elements
   from 0 up to LOC_FINAL_VALUE-1 represent themselves, and subsequent
   elements are those registered at gdb initialization time.  */

static struct symbol_impl symbol_impl[MAX_SYMBOL_IMPLS];

/* The globally visible pointer.  This is separate from 'symbol_impl'
   so that it can be const.  */

const struct symbol_impl *symbol_impls = &symbol_impl[0];

/* Make sure we saved enough room in struct symbol.  */

gdb_static_assert (MAX_SYMBOL_IMPLS <= (1 << SYMBOL_ACLASS_BITS));

/* Register a computed symbol type.  ACLASS must be LOC_COMPUTED.  OPS
   is the ops vector associated with this index.  This returns the new
   index, which should be used as the aclass_index field for symbols
   of this type.  */

int
register_symbol_computed_impl (enum address_class aclass,
			       const struct symbol_computed_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_COMPUTED);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_computed = ops;

  /* Sanity check OPS.  */
  gdb_assert (ops != NULL);
  gdb_assert (ops->tracepoint_var_ref != NULL);
  gdb_assert (ops->describe_location != NULL);
  gdb_assert (ops->get_symbol_read_needs != NULL);
  gdb_assert (ops->read_variable != NULL);

  return result;
}

/* Register a function with frame base type.  ACLASS must be LOC_BLOCK.
   OPS is the ops vector associated with this index.  This returns the
   new index, which should be used as the aclass_index field for symbols
   of this type.  */

int
register_symbol_block_impl (enum address_class aclass,
			    const struct symbol_block_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_BLOCK);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_block = ops;

  /* Sanity check OPS.  */
  gdb_assert (ops != NULL);
  gdb_assert (ops->find_frame_base_location != NULL);

  return result;
}

/* Register a register symbol type.  ACLASS must be LOC_REGISTER or
   LOC_REGPARM_ADDR.  OPS is the register ops vector associated with
   this index.  This returns the new index, which should be used as
   the aclass_index field for symbols of this type.  */

int
register_symbol_register_impl (enum address_class aclass,
			       const struct symbol_register_ops *ops)
{
  int result = next_aclass_value++;

  gdb_assert (aclass == LOC_REGISTER || aclass == LOC_REGPARM_ADDR);
  gdb_assert (result < MAX_SYMBOL_IMPLS);
  symbol_impl[result].aclass = aclass;
  symbol_impl[result].ops_register = ops;

  return result;
}

/* Initialize elements of 'symbol_impl' for the constants in enum
   address_class.  */

static void
initialize_ordinary_address_classes (void)
{
  int i;

  for (i = 0; i < LOC_FINAL_VALUE; ++i)
    symbol_impl[i].aclass = (enum address_class) i;
}



/* Helper function to initialize the fields of an objfile-owned symbol.
   It assumed that *SYM is already all zeroes.  */

static void
initialize_objfile_symbol_1 (struct symbol *sym)
{
  SYMBOL_OBJFILE_OWNED (sym) = 1;
  SYMBOL_SECTION (sym) = -1;
}

/* Initialize the symbol SYM, and mark it as being owned by an objfile.  */

void
initialize_objfile_symbol (struct symbol *sym)
{
  memset (sym, 0, sizeof (*sym));
  initialize_objfile_symbol_1 (sym);
}

/* Allocate and initialize a new 'struct symbol' on OBJFILE's
   obstack.  */

struct symbol *
allocate_symbol (struct objfile *objfile)
{
  struct symbol *result;

  result = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct symbol);
  initialize_objfile_symbol_1 (result);

  return result;
}

/* Allocate and initialize a new 'struct template_symbol' on OBJFILE's
   obstack.  */

struct template_symbol *
allocate_template_symbol (struct objfile *objfile)
{
  struct template_symbol *result;

  result = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct template_symbol);
  initialize_objfile_symbol_1 (result);

  return result;
}

/* See symtab.h.  */

struct objfile *
symbol_objfile (const struct symbol *symbol)
{
  gdb_assert (SYMBOL_OBJFILE_OWNED (symbol));
  return SYMTAB_OBJFILE (symbol->owner.symtab);
}

/* See symtab.h.  */

struct gdbarch *
symbol_arch (const struct symbol *symbol)
{
  if (!SYMBOL_OBJFILE_OWNED (symbol))
    return symbol->owner.arch;
  return get_objfile_arch (SYMTAB_OBJFILE (symbol->owner.symtab));
}

/* See symtab.h.  */

struct symtab *
symbol_symtab (const struct symbol *symbol)
{
  gdb_assert (SYMBOL_OBJFILE_OWNED (symbol));
  return symbol->owner.symtab;
}

/* See symtab.h.  */

void
symbol_set_symtab (struct symbol *symbol, struct symtab *symtab)
{
  gdb_assert (SYMBOL_OBJFILE_OWNED (symbol));
  symbol->owner.symtab = symtab;
}



void
_initialize_symtab (void)
{
  initialize_ordinary_address_classes ();

  main_progspace_key
    = register_program_space_data_with_cleanup (NULL, main_info_cleanup);

  symbol_cache_key
    = register_program_space_data_with_cleanup (NULL, symbol_cache_cleanup);

  add_info ("variables", info_variables_command,
	    info_print_args_help (_("\
All global and static variable names or those matching REGEXPs.\n\
Usage: info variables [-q] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the global and static variables.\n"),
				  _("global and static variables")));
  if (dbx_commands)
    add_com ("whereis", class_info, info_variables_command,
	     info_print_args_help (_("\
All global and static variable names, or those matching REGEXPs.\n\
Usage: whereis [-q] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the global and static variables.\n"),
				   _("global and static variables")));

  add_info ("functions", info_functions_command,
	    info_print_args_help (_("\
All function names or those matching REGEXPs.\n\
Usage: info functions [-q] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the functions.\n"),
				  _("functions")));

  /* FIXME:  This command has at least the following problems:
     1.  It prints builtin types (in a very strange and confusing fashion).
     2.  It doesn't print right, e.g. with
     typedef struct foo *FOO
     type_print prints "FOO" when we want to make it (in this situation)
     print "struct foo *".
     I also think "ptype" or "whatis" is more likely to be useful (but if
     there is much disagreement "info types" can be fixed).  */
  add_info ("types", info_types_command,
	    _("All type names, or those matching REGEXP."));

  add_info ("sources", info_sources_command,
	    _("Source files in the program."));

  add_com ("rbreak", class_breakpoint, rbreak_command,
	   _("Set a breakpoint for all functions matching REGEXP."));

  add_setshow_enum_cmd ("multiple-symbols", no_class,
                        multiple_symbols_modes, &multiple_symbols_mode,
                        _("\
Set the debugger behavior when more than one symbol are possible matches\n\
in an expression."), _("\
Show how the debugger handles ambiguities in expressions."), _("\
Valid values are \"ask\", \"all\", \"cancel\", and the default is \"all\"."),
                        NULL, NULL, &setlist, &showlist);

  add_setshow_boolean_cmd ("basenames-may-differ", class_obscure,
			   &basenames_may_differ, _("\
Set whether a source file may have multiple base names."), _("\
Show whether a source file may have multiple base names."), _("\
(A \"base name\" is the name of a file with the directory part removed.\n\
Example: The base name of \"/home/user/hello.c\" is \"hello.c\".)\n\
If set, GDB will canonicalize file names (e.g., expand symlinks)\n\
before comparing them.  Canonicalization is an expensive operation,\n\
but it allows the same file be known by more than one base name.\n\
If not set (the default), all source files are assumed to have just\n\
one base name, and gdb will do file name comparisons more efficiently."),
			   NULL, NULL,
			   &setlist, &showlist);

  add_setshow_zuinteger_cmd ("symtab-create", no_class, &symtab_create_debug,
			     _("Set debugging of symbol table creation."),
			     _("Show debugging of symbol table creation."), _("\
When enabled (non-zero), debugging messages are printed when building\n\
symbol tables.  A value of 1 (one) normally provides enough information.\n\
A value greater than 1 provides more verbose information."),
			     NULL,
			     NULL,
			     &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("symbol-lookup", no_class, &symbol_lookup_debug,
			   _("\
Set debugging of symbol lookup."), _("\
Show debugging of symbol lookup."), _("\
When enabled (non-zero), symbol lookups are logged."),
			   NULL, NULL,
			   &setdebuglist, &showdebuglist);

  add_setshow_zuinteger_cmd ("symbol-cache-size", no_class,
			     &new_symbol_cache_size,
			     _("Set the size of the symbol cache."),
			     _("Show the size of the symbol cache."), _("\
The size of the symbol cache.\n\
If zero then the symbol cache is disabled."),
			     set_symbol_cache_size_handler, NULL,
			     &maintenance_set_cmdlist,
			     &maintenance_show_cmdlist);

  add_cmd ("symbol-cache", class_maintenance, maintenance_print_symbol_cache,
	   _("Dump the symbol cache for each program space."),
	   &maintenanceprintlist);

  add_cmd ("symbol-cache-statistics", class_maintenance,
	   maintenance_print_symbol_cache_statistics,
	   _("Print symbol cache statistics for each program space."),
	   &maintenanceprintlist);

  add_cmd ("flush-symbol-cache", class_maintenance,
	   maintenance_flush_symbol_cache,
	   _("Flush the symbol cache for each program space."),
	   &maintenancelist);

  gdb::observers::executable_changed.attach (symtab_observer_executable_changed);
  gdb::observers::new_objfile.attach (symtab_new_objfile_observer);
  gdb::observers::free_objfile.attach (symtab_free_objfile_observer);
}
