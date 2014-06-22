/* Symbol table lookup for the GNU debugger, GDB.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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

#include "hashtab.h"

#include "gdb_obstack.h"
#include "block.h"
#include "dictionary.h"

#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <ctype.h>
#include "cp-abi.h"
#include "cp-support.h"
#include "observer.h"
#include "gdb_assert.h"
#include "solist.h"
#include "macrotab.h"
#include "macroscope.h"

#include "psymtab.h"
#include "parser-defs.h"

/* Prototypes for local functions */

static void rbreak_command (char *, int);

static void types_info (char *, int);

static void functions_info (char *, int);

static void variables_info (char *, int);

static void sources_info (char *, int);

static int find_line_common (struct linetable *, int, int *, int);

static struct symbol *lookup_symbol_aux (const char *name,
					 const struct block *block,
					 const domain_enum domain,
					 enum language language,
					 struct field_of_this_result *is_a_field_of_this);

static
struct symbol *lookup_symbol_aux_local (const char *name,
					const struct block *block,
					const domain_enum domain,
					enum language language);

static
struct symbol *lookup_symbol_aux_symtabs (int block_index,
					  const char *name,
					  const domain_enum domain);

static
struct symbol *lookup_symbol_aux_quick (struct objfile *objfile,
					int block_index,
					const char *name,
					const domain_enum domain);

void _initialize_symtab (void);

/* */

/* When non-zero, print debugging messages related to symtab creation.  */
unsigned int symtab_create_debug = 0;

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

/* Block in which the most recently searched-for symbol was found.
   Might be better to make this a parameter to lookup_symbol and
   value_of_this.  */

const struct block *block_found;

/* Return the name of a domain_enum.  */

const char *
domain_name (domain_enum e)
{
  switch (e)
    {
    case UNDEF_DOMAIN: return "UNDEF_DOMAIN";
    case VAR_DOMAIN: return "VAR_DOMAIN";
    case STRUCT_DOMAIN: return "STRUCT_DOMAIN";
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

/* Set the primary field in SYMTAB.  */

void
set_symtab_primary (struct symtab *symtab, int primary)
{
  symtab->primary = primary;

  if (symtab_create_debug && primary)
    {
      fprintf_unfiltered (gdb_stdlog,
			  "Created primary symtab %s for %s.\n",
			  host_address_to_string (symtab),
			  symtab_to_filename_for_display (symtab));
    }
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

/* Check for a symtab of a specific name by searching some symtabs.
   This is a helper function for callbacks of iterate_over_symtabs.

   If NAME is not absolute, then REAL_PATH is NULL
   If NAME is absolute, then REAL_PATH is the gdb_realpath form of NAME.

   The return value, NAME, REAL_PATH, CALLBACK, and DATA
   are identical to the `map_symtabs_matching_filename' method of
   quick_symbol_functions.

   FIRST and AFTER_LAST indicate the range of symtabs to search.
   AFTER_LAST is one past the last symtab to search; NULL means to
   search until the end of the list.  */

int
iterate_over_some_symtabs (const char *name,
			   const char *real_path,
			   int (*callback) (struct symtab *symtab,
					    void *data),
			   void *data,
			   struct symtab *first,
			   struct symtab *after_last)
{
  struct symtab *s = NULL;
  const char* base_name = lbasename (name);

  for (s = first; s != NULL && s != after_last; s = s->next)
    {
      if (compare_filenames_for_search (s->filename, name))
	{
	  if (callback (s, data))
	    return 1;
	  continue;
	}

      /* Before we invoke realpath, which can get expensive when many
	 files are involved, do a quick comparison of the basenames.  */
      if (! basenames_may_differ
	  && FILENAME_CMP (base_name, lbasename (s->filename)) != 0)
	continue;

      if (compare_filenames_for_search (symtab_to_fullname (s), name))
	{
	  if (callback (s, data))
	    return 1;
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
	      if (callback (s, data))
		return 1;
	      continue;
	    }
	}
    }

  return 0;
}

/* Check for a symtab of a specific name; first in symtabs, then in
   psymtabs.  *If* there is no '/' in the name, a match after a '/'
   in the symtab filename will also work.

   Calls CALLBACK with each symtab that is found and with the supplied
   DATA.  If CALLBACK returns true, the search stops.  */

void
iterate_over_symtabs (const char *name,
		      int (*callback) (struct symtab *symtab,
				       void *data),
		      void *data)
{
  struct objfile *objfile;
  char *real_path = NULL;
  struct cleanup *cleanups = make_cleanup (null_cleanup, NULL);

  /* Here we are interested in canonicalizing an absolute path, not
     absolutizing a relative path.  */
  if (IS_ABSOLUTE_PATH (name))
    {
      real_path = gdb_realpath (name);
      make_cleanup (xfree, real_path);
      gdb_assert (IS_ABSOLUTE_PATH (real_path));
    }

  ALL_OBJFILES (objfile)
  {
    if (iterate_over_some_symtabs (name, real_path, callback, data,
				   objfile->symtabs, NULL))
      {
	do_cleanups (cleanups);
	return;
      }
  }

  /* Same search rules as above apply here, but now we look thru the
     psymtabs.  */

  ALL_OBJFILES (objfile)
  {
    if (objfile->sf
	&& objfile->sf->qf->map_symtabs_matching_filename (objfile,
							   name,
							   real_path,
							   callback,
							   data))
      {
	do_cleanups (cleanups);
	return;
      }
  }

  do_cleanups (cleanups);
}

/* The callback function used by lookup_symtab.  */

static int
lookup_symtab_callback (struct symtab *symtab, void *data)
{
  struct symtab **result_ptr = data;

  *result_ptr = symtab;
  return 1;
}

/* A wrapper for iterate_over_symtabs that returns the first matching
   symtab, or NULL.  */

struct symtab *
lookup_symtab (const char *name)
{
  struct symtab *result = NULL;

  iterate_over_symtabs (name, lookup_symtab_callback, &result);
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
  const char *newname = type_name_no_tag (type);

  /* Does the form of physname indicate that it is the full mangled name
     of a constructor (not just the args)?  */
  int is_full_physname_constructor;

  int is_constructor;
  int is_destructor = is_destructor_name (physname);
  /* Need a new type prefix.  */
  char *const_prefix = method->is_const ? "C" : "";
  char *volatile_prefix = method->is_volatile ? "V" : "";
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
    is_destructor = (strncmp (physname, "__dt", 4) == 0);

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

/* Initialize the cplus_specific structure.  'cplus_specific' should
   only be allocated for use with cplus symbols.  */

static void
symbol_init_cplus_specific (struct general_symbol_info *gsymbol,
			    struct obstack *obstack)
{
  /* A language_specific structure should not have been previously
     initialized.  */
  gdb_assert (gsymbol->language_specific.cplus_specific == NULL);
  gdb_assert (obstack != NULL);

  gsymbol->language_specific.cplus_specific =
    OBSTACK_ZALLOC (obstack, struct cplus_specific);
}

/* Set the demangled name of GSYMBOL to NAME.  NAME must be already
   correctly allocated.  For C++ symbols a cplus_specific struct is
   allocated so OBJFILE must not be NULL.  If this is a non C++ symbol
   OBJFILE can be NULL.  */

void
symbol_set_demangled_name (struct general_symbol_info *gsymbol,
                           const char *name,
                           struct obstack *obstack)
{
  if (gsymbol->language == language_cplus)
    {
      if (gsymbol->language_specific.cplus_specific == NULL)
	symbol_init_cplus_specific (gsymbol, obstack);

      gsymbol->language_specific.cplus_specific->demangled_name = name;
    }
  else if (gsymbol->language == language_ada)
    {
      if (name == NULL)
	{
	  gsymbol->ada_mangled = 0;
	  gsymbol->language_specific.obstack = obstack;
	}
      else
	{
	  gsymbol->ada_mangled = 1;
	  gsymbol->language_specific.mangled_lang.demangled_name = name;
	}
    }
  else
    gsymbol->language_specific.mangled_lang.demangled_name = name;
}

/* Return the demangled name of GSYMBOL.  */

const char *
symbol_get_demangled_name (const struct general_symbol_info *gsymbol)
{
  if (gsymbol->language == language_cplus)
    {
      if (gsymbol->language_specific.cplus_specific != NULL)
	return gsymbol->language_specific.cplus_specific->demangled_name;
      else
	return NULL;
    }
  else if (gsymbol->language == language_ada)
    {
      if (!gsymbol->ada_mangled)
	return NULL;
      /* Fall through.  */
    }

  return gsymbol->language_specific.mangled_lang.demangled_name;
}


/* Initialize the language dependent portion of a symbol
   depending upon the language for the symbol.  */

void
symbol_set_language (struct general_symbol_info *gsymbol,
                     enum language language,
		     struct obstack *obstack)
{
  gsymbol->language = language;
  if (gsymbol->language == language_d
      || gsymbol->language == language_go
      || gsymbol->language == language_java
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
  else if (gsymbol->language == language_cplus)
    gsymbol->language_specific.cplus_specific = NULL;
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
  const struct demangled_name_entry *e = data;

  return htab_hash_string (e->mangled);
}

/* Equality function for the demangled name hash.  */

static int
eq_demangled_name_entry (const void *a, const void *b)
{
  const struct demangled_name_entry *da = a;
  const struct demangled_name_entry *db = b;

  return strcmp (da->mangled, db->mangled) == 0;
}

/* Create the hash table used for demangled names.  Each hash entry is
   a pair of strings; one for the mangled name and one for the demangled
   name.  The entry is hashed via just the mangled name.  */

static void
create_demangled_names_hash (struct objfile *objfile)
{
  /* Choose 256 as the starting size of the hash table, somewhat arbitrarily.
     The hash table code will round this up to the next prime number.
     Choosing a much larger table size wastes memory, and saves only about
     1% in symbol reading.  */

  objfile->per_bfd->demangled_names_hash = htab_create_alloc
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

  if (gsymbol->language == language_unknown)
    gsymbol->language = language_auto;

  if (gsymbol->language == language_objc
      || gsymbol->language == language_auto)
    {
      demangled =
	objc_demangle (mangled, 0);
      if (demangled != NULL)
	{
	  gsymbol->language = language_objc;
	  return demangled;
	}
    }
  if (gsymbol->language == language_cplus
      || gsymbol->language == language_auto)
    {
      demangled =
        gdb_demangle (mangled, DMGL_PARAMS | DMGL_ANSI);
      if (demangled != NULL)
	{
	  gsymbol->language = language_cplus;
	  return demangled;
	}
    }
  if (gsymbol->language == language_java)
    {
      demangled =
        gdb_demangle (mangled,
		      DMGL_PARAMS | DMGL_ANSI | DMGL_JAVA);
      if (demangled != NULL)
	{
	  gsymbol->language = language_java;
	  return demangled;
	}
    }
  if (gsymbol->language == language_d
      || gsymbol->language == language_auto)
    {
      demangled = d_demangle(mangled, 0);
      if (demangled != NULL)
	{
	  gsymbol->language = language_d;
	  return demangled;
	}
    }
  /* FIXME(dje): Continually adding languages here is clumsy.
     Better to just call la_demangle if !auto, and if auto then call
     a utility routine that tries successive languages in turn and reports
     which one it finds.  I realize the la_demangle options may be different
     for different languages but there's already a FIXME for that.  */
  if (gsymbol->language == language_go
      || gsymbol->language == language_auto)
    {
      demangled = go_demangle (mangled, 0);
      if (demangled != NULL)
	{
	  gsymbol->language = language_go;
	  return demangled;
	}
    }

  /* We could support `gsymbol->language == language_fortran' here to provide
     module namespaces also for inferiors with only minimal symbol table (ELF
     symbols).  Just the mangling standard is not standardized across compilers
     and there is no DW_AT_producer available for inferiors with only the ELF
     symbols to check the mangling kind.  */

  /* Check for Ada symbols last.  See comment below explaining why.  */

  if (gsymbol->language == language_auto)
   {
     const char *demangled = ada_decode (mangled);

     if (demangled != mangled && demangled != NULL && demangled[0] != '<')
       {
	 /* Set the gsymbol language to Ada, but still return NULL.
	    Two reasons for that:

	      1. For Ada, we prefer computing the symbol's decoded name
		 on the fly rather than pre-compute it, in order to save
		 memory (Ada projects are typically very large).

	      2. There are some areas in the definition of the GNAT
		 encoding where, with a bit of bad luck, we might be able
		 to decode a non-Ada symbol, generating an incorrect
		 demangled name (Eg: names ending with "TB" for instance
		 are identified as task bodies and so stripped from
		 the decoded name returned).

		 Returning NULL, here, helps us get a little bit of
		 the best of both worlds.  Because we're last, we should
		 not affect any of the other languages that were able to
		 demangle the symbol before us; we get to correctly tag
		 Ada symbols as such; and even if we incorrectly tagged
		 a non-Ada symbol, which should be rare, any routing
		 through the Ada language should be transparent (Ada
		 tries to behave much like C/C++ with non-Ada symbols).  */
	 gsymbol->language = language_ada;
	 return NULL;
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

/* We have to be careful when dealing with Java names: when we run
   into a Java minimal symbol, we don't know it's a Java symbol, so it
   gets demangled as a C++ name.  This is unfortunate, but there's not
   much we can do about it: but when demangling partial symbols and
   regular symbols, we'd better not reuse the wrong demangled name.
   (See PR gdb/1039.)  We solve this by putting a distinctive prefix
   on Java names when storing them in the hash table.  */

/* FIXME: carlton/2003-03-13: This is an unfortunate situation.  I
   don't mind the Java prefix so much: different languages have
   different demangling requirements, so it's only natural that we
   need to keep language data around in our demangling cache.  But
   it's not good that the minimal symbol has the wrong demangled name.
   Unfortunately, I can't think of any easy solution to that
   problem.  */

#define JAVA_PREFIX "##JAVA$$"
#define JAVA_PREFIX_LEN 8

void
symbol_set_names (struct general_symbol_info *gsymbol,
		  const char *linkage_name, int len, int copy_name,
		  struct objfile *objfile)
{
  struct demangled_name_entry **slot;
  /* A 0-terminated copy of the linkage name.  */
  const char *linkage_name_copy;
  /* A copy of the linkage name that might have a special Java prefix
     added to it, for use when looking names up in the hash table.  */
  const char *lookup_name;
  /* The length of lookup_name.  */
  int lookup_len;
  struct demangled_name_entry entry;
  struct objfile_per_bfd_storage *per_bfd = objfile->per_bfd;

  if (gsymbol->language == language_ada)
    {
      /* In Ada, we do the symbol lookups using the mangled name, so
         we can save some space by not storing the demangled name.

         As a side note, we have also observed some overlap between
         the C++ mangling and Ada mangling, similarly to what has
         been observed with Java.  Because we don't store the demangled
         name with the symbol, we don't need to use the same trick
         as Java.  */
      if (!copy_name)
	gsymbol->name = linkage_name;
      else
	{
	  char *name = obstack_alloc (&per_bfd->storage_obstack, len + 1);

	  memcpy (name, linkage_name, len);
	  name[len] = '\0';
	  gsymbol->name = name;
	}
      symbol_set_demangled_name (gsymbol, NULL, &per_bfd->storage_obstack);

      return;
    }

  if (per_bfd->demangled_names_hash == NULL)
    create_demangled_names_hash (objfile);

  /* The stabs reader generally provides names that are not
     NUL-terminated; most of the other readers don't do this, so we
     can just use the given copy, unless we're in the Java case.  */
  if (gsymbol->language == language_java)
    {
      char *alloc_name;

      lookup_len = len + JAVA_PREFIX_LEN;
      alloc_name = alloca (lookup_len + 1);
      memcpy (alloc_name, JAVA_PREFIX, JAVA_PREFIX_LEN);
      memcpy (alloc_name + JAVA_PREFIX_LEN, linkage_name, len);
      alloc_name[lookup_len] = '\0';

      lookup_name = alloc_name;
      linkage_name_copy = alloc_name + JAVA_PREFIX_LEN;
    }
  else if (linkage_name[len] != '\0')
    {
      char *alloc_name;

      lookup_len = len;
      alloc_name = alloca (lookup_len + 1);
      memcpy (alloc_name, linkage_name, len);
      alloc_name[lookup_len] = '\0';

      lookup_name = alloc_name;
      linkage_name_copy = alloc_name;
    }
  else
    {
      lookup_len = len;
      lookup_name = linkage_name;
      linkage_name_copy = linkage_name;
    }

  entry.mangled = lookup_name;
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
      char *demangled_name = symbol_find_demangled_name (gsymbol,
							 linkage_name_copy);
      int demangled_len = demangled_name ? strlen (demangled_name) : 0;

      /* Suppose we have demangled_name==NULL, copy_name==0, and
	 lookup_name==linkage_name.  In this case, we already have the
	 mangled name saved, and we don't have a demangled name.  So,
	 you might think we could save a little space by not recording
	 this in the hash table at all.
	 
	 It turns out that it is actually important to still save such
	 an entry in the hash table, because storing this name gives
	 us better bcache hit rates for partial symbols.  */
      if (!copy_name && lookup_name == linkage_name)
	{
	  *slot = obstack_alloc (&per_bfd->storage_obstack,
				 offsetof (struct demangled_name_entry,
					   demangled)
				 + demangled_len + 1);
	  (*slot)->mangled = lookup_name;
	}
      else
	{
	  char *mangled_ptr;

	  /* If we must copy the mangled name, put it directly after
	     the demangled name so we can have a single
	     allocation.  */
	  *slot = obstack_alloc (&per_bfd->storage_obstack,
				 offsetof (struct demangled_name_entry,
					   demangled)
				 + lookup_len + demangled_len + 2);
	  mangled_ptr = &((*slot)->demangled[demangled_len + 1]);
	  strcpy (mangled_ptr, lookup_name);
	  (*slot)->mangled = mangled_ptr;
	}

      if (demangled_name != NULL)
	{
	  strcpy ((*slot)->demangled, demangled_name);
	  xfree (demangled_name);
	}
      else
	(*slot)->demangled[0] = '\0';
    }

  gsymbol->name = (*slot)->mangled + lookup_len - len;
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
    case language_java:
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
    case language_java:
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

/* Initialize the structure fields to zero values.  */

void
init_sal (struct symtab_and_line *sal)
{
  sal->pspace = NULL;
  sal->symtab = 0;
  sal->section = 0;
  sal->line = 0;
  sal->pc = 0;
  sal->end = 0;
  sal->explicit_pc = 0;
  sal->explicit_line = 0;
  sal->probe = NULL;
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
  struct objfile *obj;

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

  ALL_OBJFILES (obj)
    if (obj->obfd == first->owner)
      break;
  gdb_assert (obj != NULL);

  if (obj->separate_debug_objfile != NULL
      && obj->separate_debug_objfile->obfd == second->owner)
    return 1;
  if (obj->separate_debug_objfile_backlink != NULL
      && obj->separate_debug_objfile_backlink->obfd == second->owner)
    return 1;

  return 0;
}

struct symtab *
find_pc_sect_symtab_via_partial (CORE_ADDR pc, struct obj_section *section)
{
  struct objfile *objfile;
  struct minimal_symbol *msymbol;

  /* If we know that this is not a text address, return failure.  This is
     necessary because we loop based on texthigh and textlow, which do
     not include the data ranges.  */
  msymbol = lookup_minimal_symbol_by_pc_section (pc, section).minsym;
  if (msymbol
      && (MSYMBOL_TYPE (msymbol) == mst_data
	  || MSYMBOL_TYPE (msymbol) == mst_bss
	  || MSYMBOL_TYPE (msymbol) == mst_abs
	  || MSYMBOL_TYPE (msymbol) == mst_file_data
	  || MSYMBOL_TYPE (msymbol) == mst_file_bss))
    return NULL;

  ALL_OBJFILES (objfile)
  {
    struct symtab *result = NULL;

    if (objfile->sf)
      result = objfile->sf->qf->find_pc_sect_symtab (objfile, msymbol,
						     pc, section, 0);
    if (result)
      return result;
  }

  return NULL;
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
    ginfo->section = SYMBOL_SECTION (msym);
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

  /* We either have an OBJFILE, or we can get at it from the sym's
     symtab.  Anything else is a bug.  */
  gdb_assert (objfile || SYMBOL_SYMTAB (sym));

  if (objfile == NULL)
    objfile = SYMBOL_SYMTAB (sym)->objfile;

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
      addr = BLOCK_START (SYMBOL_BLOCK_VALUE (sym));
      break;

    default:
      /* Nothing else will be listed in the minsyms -- no use looking
	 it up.  */
      return sym;
    }

  fixup_section (&sym->ginfo, addr, objfile);

  return sym;
}

/* Compute the demangled form of NAME as used by the various symbol
   lookup functions.  The result is stored in *RESULT_NAME.  Returns a
   cleanup which can be used to clean up the result.

   For Ada, this function just sets *RESULT_NAME to NAME, unmodified.
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

struct cleanup *
demangle_for_lookup (const char *name, enum language lang,
		     const char **result_name)
{
  char *demangled_name = NULL;
  const char *modified_name = NULL;
  struct cleanup *cleanup = make_cleanup (null_cleanup, 0);

  modified_name = name;

  /* If we are using C++, D, Go, or Java, demangle the name before doing a
     lookup, so we can always binary search.  */
  if (lang == language_cplus)
    {
      demangled_name = gdb_demangle (name, DMGL_ANSI | DMGL_PARAMS);
      if (demangled_name)
	{
	  modified_name = demangled_name;
	  make_cleanup (xfree, demangled_name);
	}
      else
	{
	  /* If we were given a non-mangled name, canonicalize it
	     according to the language (so far only for C++).  */
	  demangled_name = cp_canonicalize_string (name);
	  if (demangled_name)
	    {
	      modified_name = demangled_name;
	      make_cleanup (xfree, demangled_name);
	    }
	}
    }
  else if (lang == language_java)
    {
      demangled_name = gdb_demangle (name,
				     DMGL_ANSI | DMGL_PARAMS | DMGL_JAVA);
      if (demangled_name)
	{
	  modified_name = demangled_name;
	  make_cleanup (xfree, demangled_name);
	}
    }
  else if (lang == language_d)
    {
      demangled_name = d_demangle (name, 0);
      if (demangled_name)
	{
	  modified_name = demangled_name;
	  make_cleanup (xfree, demangled_name);
	}
    }
  else if (lang == language_go)
    {
      demangled_name = go_demangle (name, 0);
      if (demangled_name)
	{
	  modified_name = demangled_name;
	  make_cleanup (xfree, demangled_name);
	}
    }

  *result_name = modified_name;
  return cleanup;
}

/* Find the definition for a specified symbol name NAME
   in domain DOMAIN, visible from lexical block BLOCK.
   Returns the struct symbol pointer, or zero if no symbol is found.
   C++: if IS_A_FIELD_OF_THIS is nonzero on entry, check to see if
   NAME is a field of the current implied argument `this'.  If so set
   *IS_A_FIELD_OF_THIS to 1, otherwise set it to zero.
   BLOCK_FOUND is set to the block in which NAME is found (in the case of
   a field of `this', value_of_this sets BLOCK_FOUND to the proper value.)  */

/* This function (or rather its subordinates) have a bunch of loops and
   it would seem to be attractive to put in some QUIT's (though I'm not really
   sure whether it can run long enough to be really important).  But there
   are a few calls for which it would appear to be bad news to quit
   out of here: e.g., find_proc_desc in alpha-mdebug-tdep.c.  (Note
   that there is C++ code below which can error(), but that probably
   doesn't affect these calls since they are looking for a known
   variable and thus can probably assume it will never hit the C++
   code).  */

struct symbol *
lookup_symbol_in_language (const char *name, const struct block *block,
			   const domain_enum domain, enum language lang,
			   struct field_of_this_result *is_a_field_of_this)
{
  const char *modified_name;
  struct symbol *returnval;
  struct cleanup *cleanup = demangle_for_lookup (name, lang, &modified_name);

  returnval = lookup_symbol_aux (modified_name, block, domain, lang,
				 is_a_field_of_this);
  do_cleanups (cleanup);

  return returnval;
}

/* Behave like lookup_symbol_in_language, but performed with the
   current language.  */

struct symbol *
lookup_symbol (const char *name, const struct block *block,
	       domain_enum domain,
	       struct field_of_this_result *is_a_field_of_this)
{
  return lookup_symbol_in_language (name, block, domain,
				    current_language->la_language,
				    is_a_field_of_this);
}

/* Look up the `this' symbol for LANG in BLOCK.  Return the symbol if
   found, or NULL if not found.  */

struct symbol *
lookup_language_this (const struct language_defn *lang,
		      const struct block *block)
{
  if (lang->la_name_of_this == NULL || block == NULL)
    return NULL;

  while (block)
    {
      struct symbol *sym;

      sym = lookup_block_symbol (block, lang->la_name_of_this, VAR_DOMAIN);
      if (sym != NULL)
	{
	  block_found = block;
	  return sym;
	}
      if (BLOCK_FUNCTION (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  return NULL;
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
  CHECK_TYPEDEF (type);

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

static struct symbol *
lookup_symbol_aux (const char *name, const struct block *block,
		   const domain_enum domain, enum language language,
		   struct field_of_this_result *is_a_field_of_this)
{
  struct symbol *sym;
  const struct language_defn *langdef;

  /* Make sure we do something sensible with is_a_field_of_this, since
     the callers that set this parameter to some non-null value will
     certainly use it later.  If we don't set it, the contents of
     is_a_field_of_this are undefined.  */
  if (is_a_field_of_this != NULL)
    memset (is_a_field_of_this, 0, sizeof (*is_a_field_of_this));

  /* Search specified block and its superiors.  Don't search
     STATIC_BLOCK or GLOBAL_BLOCK.  */

  sym = lookup_symbol_aux_local (name, block, domain, language);
  if (sym != NULL)
    return sym;

  /* If requested to do so by the caller and if appropriate for LANGUAGE,
     check to see if NAME is a field of `this'.  */

  langdef = language_def (language);

  /* Don't do this check if we are searching for a struct.  It will
     not be found by check_field, but will be found by other
     means.  */
  if (is_a_field_of_this != NULL && domain != STRUCT_DOMAIN)
    {
      struct symbol *sym = lookup_language_this (langdef, block);

      if (sym)
	{
	  struct type *t = sym->type;

	  /* I'm not really sure that type of this can ever
	     be typedefed; just be safe.  */
	  CHECK_TYPEDEF (t);
	  if (TYPE_CODE (t) == TYPE_CODE_PTR
	      || TYPE_CODE (t) == TYPE_CODE_REF)
	    t = TYPE_TARGET_TYPE (t);

	  if (TYPE_CODE (t) != TYPE_CODE_STRUCT
	      && TYPE_CODE (t) != TYPE_CODE_UNION)
	    error (_("Internal error: `%s' is not an aggregate"),
		   langdef->la_name_of_this);

	  if (check_field (t, name, is_a_field_of_this))
	    return NULL;
	}
    }

  /* Now do whatever is appropriate for LANGUAGE to look
     up static and global variables.  */

  sym = langdef->la_lookup_symbol_nonlocal (name, block, domain);
  if (sym != NULL)
    return sym;

  /* Now search all static file-level symbols.  Not strictly correct,
     but more useful than an error.  */

  return lookup_static_symbol_aux (name, domain);
}

/* Search all static file-level symbols for NAME from DOMAIN.  Do the symtabs
   first, then check the psymtabs.  If a psymtab indicates the existence of the
   desired name as a file-level static, then do psymtab-to-symtab conversion on
   the fly and return the found symbol.  */

struct symbol *
lookup_static_symbol_aux (const char *name, const domain_enum domain)
{
  struct objfile *objfile;
  struct symbol *sym;

  sym = lookup_symbol_aux_symtabs (STATIC_BLOCK, name, domain);
  if (sym != NULL)
    return sym;

  ALL_OBJFILES (objfile)
  {
    sym = lookup_symbol_aux_quick (objfile, STATIC_BLOCK, name, domain);
    if (sym != NULL)
      return sym;
  }

  return NULL;
}

/* Check to see if the symbol is defined in BLOCK or its superiors.
   Don't search STATIC_BLOCK or GLOBAL_BLOCK.  */

static struct symbol *
lookup_symbol_aux_local (const char *name, const struct block *block,
                         const domain_enum domain,
                         enum language language)
{
  struct symbol *sym;
  const struct block *static_block = block_static_block (block);
  const char *scope = block_scope (block);
  
  /* Check if either no block is specified or it's a global block.  */

  if (static_block == NULL)
    return NULL;

  while (block != static_block)
    {
      sym = lookup_symbol_aux_block (name, block, domain);
      if (sym != NULL)
	return sym;

      if (language == language_cplus || language == language_fortran)
        {
          sym = cp_lookup_symbol_imports_or_template (scope, name, block,
						      domain);
          if (sym != NULL)
            return sym;
        }

      if (BLOCK_FUNCTION (block) != NULL && block_inlined_p (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  /* We've reached the edge of the function without finding a result.  */

  return NULL;
}

/* Look up OBJFILE to BLOCK.  */

struct objfile *
lookup_objfile_from_block (const struct block *block)
{
  struct objfile *obj;
  struct symtab *s;

  if (block == NULL)
    return NULL;

  block = block_global_block (block);
  /* Go through SYMTABS.  */
  ALL_SYMTABS (obj, s)
    if (block == BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), GLOBAL_BLOCK))
      {
	if (obj->separate_debug_objfile_backlink)
	  obj = obj->separate_debug_objfile_backlink;

	return obj;
      }

  return NULL;
}

/* Look up a symbol in a block; if found, fixup the symbol, and set
   block_found appropriately.  */

struct symbol *
lookup_symbol_aux_block (const char *name, const struct block *block,
			 const domain_enum domain)
{
  struct symbol *sym;

  sym = lookup_block_symbol (block, name, domain);
  if (sym)
    {
      block_found = block;
      return fixup_symbol_section (sym, NULL);
    }

  return NULL;
}

/* Check all global symbols in OBJFILE in symtabs and
   psymtabs.  */

struct symbol *
lookup_global_symbol_from_objfile (const struct objfile *main_objfile,
				   const char *name,
				   const domain_enum domain)
{
  const struct objfile *objfile;
  struct symbol *sym;
  struct blockvector *bv;
  const struct block *block;
  struct symtab *s;

  for (objfile = main_objfile;
       objfile;
       objfile = objfile_separate_debug_iterate (main_objfile, objfile))
    {
      /* Go through symtabs.  */
      ALL_OBJFILE_PRIMARY_SYMTABS (objfile, s)
	{
	  bv = BLOCKVECTOR (s);
	  block = BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK);
	  sym = lookup_block_symbol (block, name, domain);
	  if (sym)
	    {
	      block_found = block;
	      return fixup_symbol_section (sym, (struct objfile *)objfile);
	    }
	}

      sym = lookup_symbol_aux_quick ((struct objfile *) objfile, GLOBAL_BLOCK,
				     name, domain);
      if (sym)
	return sym;
    }

  return NULL;
}

/* Check to see if the symbol is defined in one of the OBJFILE's
   symtabs.  BLOCK_INDEX should be either GLOBAL_BLOCK or STATIC_BLOCK,
   depending on whether or not we want to search global symbols or
   static symbols.  */

static struct symbol *
lookup_symbol_aux_objfile (struct objfile *objfile, int block_index,
			   const char *name, const domain_enum domain)
{
  struct symbol *sym = NULL;
  struct blockvector *bv;
  const struct block *block;
  struct symtab *s;

  ALL_OBJFILE_PRIMARY_SYMTABS (objfile, s)
    {
      bv = BLOCKVECTOR (s);
      block = BLOCKVECTOR_BLOCK (bv, block_index);
      sym = lookup_block_symbol (block, name, domain);
      if (sym)
	{
	  block_found = block;
	  return fixup_symbol_section (sym, objfile);
	}
    }

  return NULL;
}

/* Same as lookup_symbol_aux_objfile, except that it searches all
   objfiles.  Return the first match found.  */

static struct symbol *
lookup_symbol_aux_symtabs (int block_index, const char *name,
			   const domain_enum domain)
{
  struct symbol *sym;
  struct objfile *objfile;

  ALL_OBJFILES (objfile)
  {
    sym = lookup_symbol_aux_objfile (objfile, block_index, name, domain);
    if (sym)
      return sym;
  }

  return NULL;
}

/* Wrapper around lookup_symbol_aux_objfile for search_symbols.
   Look up LINKAGE_NAME in DOMAIN in the global and static blocks of OBJFILE
   and all related objfiles.  */

static struct symbol *
lookup_symbol_in_objfile_from_linkage_name (struct objfile *objfile,
					    const char *linkage_name,
					    domain_enum domain)
{
  enum language lang = current_language->la_language;
  const char *modified_name;
  struct cleanup *cleanup = demangle_for_lookup (linkage_name, lang,
						 &modified_name);
  struct objfile *main_objfile, *cur_objfile;

  if (objfile->separate_debug_objfile_backlink)
    main_objfile = objfile->separate_debug_objfile_backlink;
  else
    main_objfile = objfile;

  for (cur_objfile = main_objfile;
       cur_objfile;
       cur_objfile = objfile_separate_debug_iterate (main_objfile, cur_objfile))
    {
      struct symbol *sym;

      sym = lookup_symbol_aux_objfile (cur_objfile, GLOBAL_BLOCK,
				       modified_name, domain);
      if (sym == NULL)
	sym = lookup_symbol_aux_objfile (cur_objfile, STATIC_BLOCK,
					 modified_name, domain);
      if (sym != NULL)
	{
	  do_cleanups (cleanup);
	  return sym;
	}
    }

  do_cleanups (cleanup);
  return NULL;
}

/* A helper function that throws an exception when a symbol was found
   in a psymtab but not in a symtab.  */

static void ATTRIBUTE_NORETURN
error_in_psymtab_expansion (int kind, const char *name, struct symtab *symtab)
{
  error (_("\
Internal: %s symbol `%s' found in %s psymtab but not in symtab.\n\
%s may be an inlined function, or may be a template function\n	 \
(if a template, try specifying an instantiation: %s<type>)."),
	 kind == GLOBAL_BLOCK ? "global" : "static",
	 name, symtab_to_filename_for_display (symtab), name, name);
}

/* A helper function for lookup_symbol_aux that interfaces with the
   "quick" symbol table functions.  */

static struct symbol *
lookup_symbol_aux_quick (struct objfile *objfile, int kind,
			 const char *name, const domain_enum domain)
{
  struct symtab *symtab;
  struct blockvector *bv;
  const struct block *block;
  struct symbol *sym;

  if (!objfile->sf)
    return NULL;
  symtab = objfile->sf->qf->lookup_symbol (objfile, kind, name, domain);
  if (!symtab)
    return NULL;

  bv = BLOCKVECTOR (symtab);
  block = BLOCKVECTOR_BLOCK (bv, kind);
  sym = lookup_block_symbol (block, name, domain);
  if (!sym)
    error_in_psymtab_expansion (kind, name, symtab);
  return fixup_symbol_section (sym, objfile);
}

/* A default version of lookup_symbol_nonlocal for use by languages
   that can't think of anything better to do.  This implements the C
   lookup rules.  */

struct symbol *
basic_lookup_symbol_nonlocal (const char *name,
			      const struct block *block,
			      const domain_enum domain)
{
  struct symbol *sym;

  /* NOTE: carlton/2003-05-19: The comments below were written when
     this (or what turned into this) was part of lookup_symbol_aux;
     I'm much less worried about these questions now, since these
     decisions have turned out well, but I leave these comments here
     for posterity.  */

  /* NOTE: carlton/2002-12-05: There is a question as to whether or
     not it would be appropriate to search the current global block
     here as well.  (That's what this code used to do before the
     is_a_field_of_this check was moved up.)  On the one hand, it's
     redundant with the lookup_symbol_aux_symtabs search that happens
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

  sym = lookup_symbol_static (name, block, domain);
  if (sym != NULL)
    return sym;

  return lookup_symbol_global (name, block, domain);
}

/* Lookup a symbol in the static block associated to BLOCK, if there
   is one; do nothing if BLOCK is NULL or a global block.  */

struct symbol *
lookup_symbol_static (const char *name,
		      const struct block *block,
		      const domain_enum domain)
{
  const struct block *static_block = block_static_block (block);

  if (static_block != NULL)
    return lookup_symbol_aux_block (name, static_block, domain);
  else
    return NULL;
}

/* Private data to be used with lookup_symbol_global_iterator_cb.  */

struct global_sym_lookup_data
{
  /* The name of the symbol we are searching for.  */
  const char *name;

  /* The domain to use for our search.  */
  domain_enum domain;

  /* The field where the callback should store the symbol if found.
     It should be initialized to NULL before the search is started.  */
  struct symbol *result;
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

  gdb_assert (data->result == NULL);

  data->result = lookup_symbol_aux_objfile (objfile, GLOBAL_BLOCK,
					    data->name, data->domain);
  if (data->result == NULL)
    data->result = lookup_symbol_aux_quick (objfile, GLOBAL_BLOCK,
					    data->name, data->domain);

  /* If we found a match, tell the iterator to stop.  Otherwise,
     keep going.  */
  return (data->result != NULL);
}

/* Lookup a symbol in all files' global blocks (searching psymtabs if
   necessary).  */

struct symbol *
lookup_symbol_global (const char *name,
		      const struct block *block,
		      const domain_enum domain)
{
  struct symbol *sym = NULL;
  struct objfile *objfile = NULL;
  struct global_sym_lookup_data lookup_data;

  /* Call library-specific lookup procedure.  */
  objfile = lookup_objfile_from_block (block);
  if (objfile != NULL)
    sym = solib_global_lookup (objfile, name, domain);
  if (sym != NULL)
    return sym;

  memset (&lookup_data, 0, sizeof (lookup_data));
  lookup_data.name = name;
  lookup_data.domain = domain;
  gdbarch_iterate_over_objfiles_in_search_order
    (objfile != NULL ? get_objfile_arch (objfile) : target_gdbarch (),
     lookup_symbol_global_iterator_cb, &lookup_data, objfile);

  return lookup_data.result;
}

int
symbol_matches_domain (enum language symbol_language,
		       domain_enum symbol_domain,
		       domain_enum domain)
{
  /* For C++ "struct foo { ... }" also defines a typedef for "foo".
     A Java class declaration also defines a typedef for the class.
     Similarly, any Ada type declaration implicitly defines a typedef.  */
  if (symbol_language == language_cplus
      || symbol_language == language_d
      || symbol_language == language_java
      || symbol_language == language_ada)
    {
      if ((domain == VAR_DOMAIN || domain == STRUCT_DOMAIN)
	  && symbol_domain == STRUCT_DOMAIN)
	return 1;
    }
  /* For all other languages, strict match is required.  */
  return (symbol_domain == domain);
}

/* Look up a type named NAME in the struct_domain.  The type returned
   must not be opaque -- i.e., must have at least one field
   defined.  */

struct type *
lookup_transparent_type (const char *name)
{
  return current_language->la_lookup_transparent_type (name);
}

/* A helper for basic_lookup_transparent_type that interfaces with the
   "quick" symbol table functions.  */

static struct type *
basic_lookup_transparent_type_quick (struct objfile *objfile, int kind,
				     const char *name)
{
  struct symtab *symtab;
  struct blockvector *bv;
  struct block *block;
  struct symbol *sym;

  if (!objfile->sf)
    return NULL;
  symtab = objfile->sf->qf->lookup_symbol (objfile, kind, name, STRUCT_DOMAIN);
  if (!symtab)
    return NULL;

  bv = BLOCKVECTOR (symtab);
  block = BLOCKVECTOR_BLOCK (bv, kind);
  sym = lookup_block_symbol (block, name, STRUCT_DOMAIN);
  if (!sym)
    error_in_psymtab_expansion (kind, name, symtab);

  if (!TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)))
    return SYMBOL_TYPE (sym);

  return NULL;
}

/* The standard implementation of lookup_transparent_type.  This code
   was modeled on lookup_symbol -- the parts not relevant to looking
   up types were just left out.  In particular it's assumed here that
   types are available in struct_domain and only at file-static or
   global blocks.  */

struct type *
basic_lookup_transparent_type (const char *name)
{
  struct symbol *sym;
  struct symtab *s = NULL;
  struct blockvector *bv;
  struct objfile *objfile;
  struct block *block;
  struct type *t;

  /* Now search all the global symbols.  Do the symtab's first, then
     check the psymtab's.  If a psymtab indicates the existence
     of the desired name as a global, then do psymtab-to-symtab
     conversion on the fly and return the found symbol.  */

  ALL_OBJFILES (objfile)
  {
    ALL_OBJFILE_PRIMARY_SYMTABS (objfile, s)
      {
	bv = BLOCKVECTOR (s);
	block = BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK);
	sym = lookup_block_symbol (block, name, STRUCT_DOMAIN);
	if (sym && !TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)))
	  {
	    return SYMBOL_TYPE (sym);
	  }
      }
  }

  ALL_OBJFILES (objfile)
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

  ALL_OBJFILES (objfile)
  {
    ALL_OBJFILE_PRIMARY_SYMTABS (objfile, s)
      {
	bv = BLOCKVECTOR (s);
	block = BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK);
	sym = lookup_block_symbol (block, name, STRUCT_DOMAIN);
	if (sym && !TYPE_IS_OPAQUE (SYMBOL_TYPE (sym)))
	  {
	    return SYMBOL_TYPE (sym);
	  }
      }
  }

  ALL_OBJFILES (objfile)
  {
    t = basic_lookup_transparent_type_quick (objfile, STATIC_BLOCK, name);
    if (t)
      return t;
  }

  return (struct type *) 0;
}

/* Search BLOCK for symbol NAME in DOMAIN.

   Note that if NAME is the demangled form of a C++ symbol, we will fail
   to find a match during the binary search of the non-encoded names, but
   for now we don't worry about the slight inefficiency of looking for
   a match we'll never find, since it will go pretty quick.  Once the
   binary search terminates, we drop through and do a straight linear
   search on the symbols.  Each symbol which is marked as being a ObjC/C++
   symbol (language_cplus or language_objc set) has both the encoded and
   non-encoded names tested for a match.  */

struct symbol *
lookup_block_symbol (const struct block *block, const char *name,
		     const domain_enum domain)
{
  struct block_iterator iter;
  struct symbol *sym;

  if (!BLOCK_FUNCTION (block))
    {
      for (sym = block_iter_name_first (block, name, &iter);
	   sym != NULL;
	   sym = block_iter_name_next (name, &iter))
	{
	  if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				     SYMBOL_DOMAIN (sym), domain))
	    return sym;
	}
      return NULL;
    }
  else
    {
      /* Note that parameter symbols do not always show up last in the
	 list; this loop makes sure to take anything else other than
	 parameter symbols first; it only uses parameter symbols as a
	 last resort.  Note that this only takes up extra computation
	 time on a match.  */

      struct symbol *sym_found = NULL;

      for (sym = block_iter_name_first (block, name, &iter);
	   sym != NULL;
	   sym = block_iter_name_next (name, &iter))
	{
	  if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				     SYMBOL_DOMAIN (sym), domain))
	    {
	      sym_found = sym;
	      if (!SYMBOL_IS_ARGUMENT (sym))
		{
		  break;
		}
	    }
	}
      return (sym_found);	/* Will be NULL if not found.  */
    }
}

/* Iterate over the symbols named NAME, matching DOMAIN, in BLOCK.
   
   For each symbol that matches, CALLBACK is called.  The symbol and
   DATA are passed to the callback.
   
   If CALLBACK returns zero, the iteration ends.  Otherwise, the
   search continues.  */

void
iterate_over_symbols (const struct block *block, const char *name,
		      const domain_enum domain,
		      symbol_found_callback_ftype *callback,
		      void *data)
{
  struct block_iterator iter;
  struct symbol *sym;

  for (sym = block_iter_name_first (block, name, &iter);
       sym != NULL;
       sym = block_iter_name_next (name, &iter))
    {
      if (symbol_matches_domain (SYMBOL_LANGUAGE (sym),
				 SYMBOL_DOMAIN (sym), domain))
	{
	  if (!callback (sym, data))
	    return;
	}
    }
}

/* Find the symtab associated with PC and SECTION.  Look through the
   psymtabs and read in another symtab if necessary.  */

struct symtab *
find_pc_sect_symtab (CORE_ADDR pc, struct obj_section *section)
{
  struct block *b;
  struct blockvector *bv;
  struct symtab *s = NULL;
  struct symtab *best_s = NULL;
  struct objfile *objfile;
  CORE_ADDR distance = 0;
  struct minimal_symbol *msymbol;

  /* If we know that this is not a text address, return failure.  This is
     necessary because we loop based on the block's high and low code
     addresses, which do not include the data ranges, and because
     we call find_pc_sect_psymtab which has a similar restriction based
     on the partial_symtab's texthigh and textlow.  */
  msymbol = lookup_minimal_symbol_by_pc_section (pc, section).minsym;
  if (msymbol
      && (MSYMBOL_TYPE (msymbol) == mst_data
	  || MSYMBOL_TYPE (msymbol) == mst_bss
	  || MSYMBOL_TYPE (msymbol) == mst_abs
	  || MSYMBOL_TYPE (msymbol) == mst_file_data
	  || MSYMBOL_TYPE (msymbol) == mst_file_bss))
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

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    bv = BLOCKVECTOR (s);
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
	if ((objfile->flags & OBJF_REORDERED) && objfile->sf)
	  {
	    struct symtab *result;

	    result
	      = objfile->sf->qf->find_pc_sect_symtab (objfile,
						      msymbol,
						      pc, section,
						      0);
	    if (result)
	      return result;
	  }
	if (section != 0)
	  {
	    struct block_iterator iter;
	    struct symbol *sym = NULL;

	    ALL_BLOCK_SYMBOLS (b, iter, sym)
	      {
		fixup_symbol_section (sym, objfile);
		if (matching_obj_sections (SYMBOL_OBJ_SECTION (objfile, sym),
					   section))
		  break;
	      }
	    if (sym == NULL)
	      continue;		/* No symbol in this symtab matches
				   section.  */
	  }
	distance = BLOCK_END (b) - BLOCK_START (b);
	best_s = s;
      }
  }

  if (best_s != NULL)
    return (best_s);

  /* Not found in symtabs, search the "quick" symtabs (e.g. psymtabs).  */

  ALL_OBJFILES (objfile)
  {
    struct symtab *result;

    if (!objfile->sf)
      continue;
    result = objfile->sf->qf->find_pc_sect_symtab (objfile,
						   msymbol,
						   pc, section,
						   1);
    if (result)
      return result;
  }

  return NULL;
}

/* Find the symtab associated with PC.  Look through the psymtabs and read
   in another symtab if necessary.  Backward compatibility, no section.  */

struct symtab *
find_pc_symtab (CORE_ADDR pc)
{
  return find_pc_sect_symtab (pc, find_pc_mapped_section (pc));
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

/* If it's worth the effort, we could be using a binary search.  */

struct symtab_and_line
find_pc_sect_line (CORE_ADDR pc, struct obj_section *section, int notcurrent)
{
  struct symtab *s;
  struct linetable *l;
  int len;
  int i;
  struct linetable_entry *item;
  struct symtab_and_line val;
  struct blockvector *bv;
  struct bound_minimal_symbol msymbol;
  struct minimal_symbol *mfunsym;
  struct objfile *objfile;

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

  init_sal (&val);		/* initialize to zeroes */

  val.pspace = current_program_space;

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
	mfunsym
	  = lookup_minimal_symbol_text (SYMBOL_LINKAGE_NAME (msymbol.minsym),
					NULL);
	if (mfunsym == NULL)
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
	else if (SYMBOL_VALUE_ADDRESS (mfunsym)
		 == SYMBOL_VALUE_ADDRESS (msymbol.minsym))
	  /* Avoid infinite recursion */
	  /* See above comment about why warning is commented out.  */
	  /* warning ("In stub for %s; unable to find real function/line info",
	     SYMBOL_LINKAGE_NAME (msymbol)); */
	  ;
	/* fall through */
	else
	  return find_pc_line (SYMBOL_VALUE_ADDRESS (mfunsym), 0);
      }


  s = find_pc_sect_symtab (pc, section);
  if (!s)
    {
      /* If no symbol information, return previous pc.  */
      if (notcurrent)
	pc++;
      val.pc = pc;
      return val;
    }

  bv = BLOCKVECTOR (s);
  objfile = s->objfile;

  /* Look at all the symtabs that share this blockvector.
     They all have the same apriori range, that we found was right;
     but they have different line tables.  */

  ALL_OBJFILE_SYMTABS (objfile, s)
    {
      if (BLOCKVECTOR (s) != bv)
	continue;

      /* Find the best line in this symtab.  */
      l = LINETABLE (s);
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

      for (i = 0; i < len; i++, item++)
	{
	  /* Leave prev pointing to the linetable entry for the last line
	     that started at or before PC.  */
	  if (item->pc > pc)
	    break;

	  prev = item;
	}

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
	  best_symtab = s;

	  /* Discard BEST_END if it's before the PC of the current BEST.  */
	  if (best_end <= best->pc)
	    best_end = 0;
	}

      /* If another line (denoted by ITEM) is in the linetable and its
         PC is after BEST's PC, but before the current BEST_END, then
	 use ITEM's PC as the new best_end.  */
      if (best && i < len && item->pc > best->pc
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

/* Find line number LINE in any symtab whose name is the same as
   SYMTAB.

   If found, return the symtab that contains the linetable in which it was
   found, set *INDEX to the index in the linetable of the best entry
   found, and set *EXACT_MATCH nonzero if the value returned is an
   exact match.

   If not found, return NULL.  */

struct symtab *
find_line_symtab (struct symtab *symtab, int line,
		  int *index, int *exact_match)
{
  int exact = 0;  /* Initialized here to avoid a compiler warning.  */

  /* BEST_INDEX and BEST_LINETABLE identify the smallest linenumber > LINE
     so far seen.  */

  int best_index;
  struct linetable *best_linetable;
  struct symtab *best_symtab;

  /* First try looking it up in the given symtab.  */
  best_linetable = LINETABLE (symtab);
  best_symtab = symtab;
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

      struct objfile *objfile;
      struct symtab *s;

      if (best_index >= 0)
	best = best_linetable->item[best_index].line;
      else
	best = 0;

      ALL_OBJFILES (objfile)
      {
	if (objfile->sf)
	  objfile->sf->qf->expand_symtabs_with_fullname (objfile,
						   symtab_to_fullname (symtab));
      }

      ALL_SYMTABS (objfile, s)
      {
	struct linetable *l;
	int ind;

	if (FILENAME_CMP (symtab->filename, s->filename) != 0)
	  continue;
	if (FILENAME_CMP (symtab_to_fullname (symtab),
			  symtab_to_fullname (s)) != 0)
	  continue;	
	l = LINETABLE (s);
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
   exactly match LINE.  Returns NULL if there are no exact matches,
   but updates BEST_ITEM in this case.  */

VEC (CORE_ADDR) *
find_pcs_for_symtab_line (struct symtab *symtab, int line,
			  struct linetable_entry **best_item)
{
  int start = 0;
  VEC (CORE_ADDR) *result = NULL;

  /* First, collect all the PCs that are at this line.  */
  while (1)
    {
      int was_exact;
      int idx;

      idx = find_line_common (LINETABLE (symtab), line, &was_exact, start);
      if (idx < 0)
	break;

      if (!was_exact)
	{
	  struct linetable_entry *item = &LINETABLE (symtab)->item[idx];

	  if (*best_item == NULL || item->line < (*best_item)->line)
	    *best_item = item;

	  break;
	}

      VEC_safe_push (CORE_ADDR, result, LINETABLE (symtab)->item[idx].pc);
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
      l = LINETABLE (symtab);
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
  l = LINETABLE (symtab);
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

/* Given a function symbol SYM, find the symtab and line for the start
   of the function.
   If the argument FUNFIRSTLINE is nonzero, we want the first line
   of real code inside the function.  */

struct symtab_and_line
find_function_start_sal (struct symbol *sym, int funfirstline)
{
  struct symtab_and_line sal;

  fixup_symbol_section (sym, NULL);
  sal = find_pc_sect_line (BLOCK_START (SYMBOL_BLOCK_VALUE (sym)),
			   SYMBOL_OBJ_SECTION (SYMBOL_OBJFILE (sym), sym), 0);

  /* We always should have a line for the function start address.
     If we don't, something is odd.  Create a plain SAL refering
     just the PC and hope that skip_prologue_sal (if requested)
     can find a line number for after the prologue.  */
  if (sal.pc < BLOCK_START (SYMBOL_BLOCK_VALUE (sym)))
    {
      init_sal (&sal);
      sal.pspace = current_program_space;
      sal.pc = BLOCK_START (SYMBOL_BLOCK_VALUE (sym));
      sal.section = SYMBOL_OBJ_SECTION (SYMBOL_OBJFILE (sym), sym);
    }

  if (funfirstline)
    skip_prologue_sal (&sal);

  return sal;
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
  struct cleanup *old_chain;
  CORE_ADDR pc, saved_pc;
  struct obj_section *section;
  const char *name;
  struct objfile *objfile;
  struct gdbarch *gdbarch;
  struct block *b, *function_block;
  int force_skip, skip;

  /* Do not change the SAL if PC was specified explicitly.  */
  if (sal->explicit_pc)
    return;

  old_chain = save_current_space_and_thread ();
  switch_to_program_space_and_thread (sal->pspace);

  sym = find_pc_sect_function (sal->pc, sal->section);
  if (sym != NULL)
    {
      fixup_symbol_section (sym, NULL);

      pc = BLOCK_START (SYMBOL_BLOCK_VALUE (sym));
      section = SYMBOL_OBJ_SECTION (SYMBOL_OBJFILE (sym), sym);
      name = SYMBOL_LINKAGE_NAME (sym);
      objfile = SYMBOL_SYMTAB (sym)->objfile;
    }
  else
    {
      struct bound_minimal_symbol msymbol
        = lookup_minimal_symbol_by_pc_section (sal->pc, sal->section);

      if (msymbol.minsym == NULL)
	{
	  do_cleanups (old_chain);
	  return;
	}

      objfile = msymbol.objfile;
      pc = SYMBOL_VALUE_ADDRESS (msymbol.minsym);
      section = SYMBOL_OBJ_SECTION (objfile, msymbol.minsym);
      name = SYMBOL_LINKAGE_NAME (msymbol.minsym);
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
  if (sym && SYMBOL_SYMTAB (sym)->locations_valid)
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
      if (skip)
	pc = gdbarch_skip_prologue (gdbarch, pc);

      /* For overlays, map pc back into its mapped VMA range.  */
      pc = overlay_mapped_address (pc, section);

      /* Calculate line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);

      /* Check if gdbarch_skip_prologue left us in mid-line, and the next
	 line is still part of the same function.  */
      if (skip && start_sal.pc != pc
	  && (sym ? (BLOCK_START (SYMBOL_BLOCK_VALUE (sym)) <= start_sal.end
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
      pc = skip_prologue_using_lineinfo (pc, SYMBOL_SYMTAB (sym));
      /* Recalculate the line number.  */
      start_sal = find_pc_sect_line (pc, section, 0);
    }

  do_cleanups (old_chain);

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
      sal->symtab = SYMBOL_SYMTAB (BLOCK_FUNCTION (function_block));
    }
}

/* If P is of the form "operator[ \t]+..." where `...' is
   some legitimate operator text, return a pointer to the
   beginning of the substring of the operator text.
   Otherwise, return "".  */

static char *
operator_chars (char *p, char **end)
{
  *end = "";
  if (strncmp (p, "operator", 8))
    return *end;
  p += 8;

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
      char *q = p + 1;

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


/* Cache to watch for file names already seen by filename_seen.  */

struct filename_seen_cache
{
  /* Table of files seen so far.  */
  htab_t tab;
  /* Initial size of the table.  It automagically grows from here.  */
#define INITIAL_FILENAME_SEEN_CACHE_SIZE 100
};

/* filename_seen_cache constructor.  */

static struct filename_seen_cache *
create_filename_seen_cache (void)
{
  struct filename_seen_cache *cache;

  cache = XNEW (struct filename_seen_cache);
  cache->tab = htab_create_alloc (INITIAL_FILENAME_SEEN_CACHE_SIZE,
				  filename_hash, filename_eq,
				  NULL, xcalloc, xfree);

  return cache;
}

/* Empty the cache, but do not delete it.  */

static void
clear_filename_seen_cache (struct filename_seen_cache *cache)
{
  htab_empty (cache->tab);
}

/* filename_seen_cache destructor.
   This takes a void * argument as it is generally used as a cleanup.  */

static void
delete_filename_seen_cache (void *ptr)
{
  struct filename_seen_cache *cache = ptr;

  htab_delete (cache->tab);
  xfree (cache);
}

/* If FILE is not already in the table of files in CACHE, return zero;
   otherwise return non-zero.  Optionally add FILE to the table if ADD
   is non-zero.

   NOTE: We don't manage space for FILE, we assume FILE lives as long
   as the caller needs.  */

static int
filename_seen (struct filename_seen_cache *cache, const char *file, int add)
{
  void **slot;

  /* Is FILE in tab?  */
  slot = htab_find_slot (cache->tab, file, add ? INSERT : NO_INSERT);
  if (*slot != NULL)
    return 1;

  /* No; maybe add it to tab.  */
  if (add)
    *slot = (char *) file;

  return 0;
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
  if (filename_seen (data->filename_seen_cache, name, 1))
    {
      /* Yes; don't print it again.  */
      return;
    }

  /* No; print it and reset *FIRST.  */
  if (! data->first)
    printf_filtered (", ");
  data->first = 0;

  wrap_here ("");
  fputs_filtered (name, gdb_stdout);
}

/* A callback for map_partial_symbol_filenames.  */

static void
output_partial_symbol_filename (const char *filename, const char *fullname,
				void *data)
{
  output_source_filename (fullname ? fullname : filename, data);
}

static void
sources_info (char *ignore, int from_tty)
{
  struct symtab *s;
  struct objfile *objfile;
  struct output_source_filename_data data;
  struct cleanup *cleanups;

  if (!have_full_symbols () && !have_partial_symbols ())
    {
      error (_("No symbol table is loaded.  Use the \"file\" command."));
    }

  data.filename_seen_cache = create_filename_seen_cache ();
  cleanups = make_cleanup (delete_filename_seen_cache,
			   data.filename_seen_cache);

  printf_filtered ("Source files for which symbols have been read in:\n\n");

  data.first = 1;
  ALL_SYMTABS (objfile, s)
  {
    const char *fullname = symtab_to_fullname (s);

    output_source_filename (fullname, &data);
  }
  printf_filtered ("\n\n");

  printf_filtered ("Source files for which symbols "
		   "will be read in on demand:\n\n");

  clear_filename_seen_cache (data.filename_seen_cache);
  data.first = 1;
  map_partial_symbol_filenames (output_partial_symbol_filename, &data,
				1 /*need_fullname*/);
  printf_filtered ("\n");

  do_cleanups (cleanups);
}

/* Compare FILE against all the NFILES entries of FILES.  If BASENAMES is
   non-zero compare only lbasename of FILES.  */

static int
file_matches (const char *file, char *files[], int nfiles, int basenames)
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

/* Free any memory associated with a search.  */

void
free_search_symbols (struct symbol_search *symbols)
{
  struct symbol_search *p;
  struct symbol_search *next;

  for (p = symbols; p != NULL; p = next)
    {
      next = p->next;
      xfree (p);
    }
}

static void
do_free_search_symbols_cleanup (void *symbolsp)
{
  struct symbol_search *symbols = *(struct symbol_search **) symbolsp;

  free_search_symbols (symbols);
}

struct cleanup *
make_cleanup_free_search_symbols (struct symbol_search **symbolsp)
{
  return make_cleanup (do_free_search_symbols_cleanup, symbolsp);
}

/* Helper function for sort_search_symbols_remove_dups and qsort.  Can only
   sort symbols, not minimal symbols.  */

static int
compare_search_syms (const void *sa, const void *sb)
{
  struct symbol_search *sym_a = *(struct symbol_search **) sa;
  struct symbol_search *sym_b = *(struct symbol_search **) sb;
  int c;

  c = FILENAME_CMP (sym_a->symtab->filename, sym_b->symtab->filename);
  if (c != 0)
    return c;

  if (sym_a->block != sym_b->block)
    return sym_a->block - sym_b->block;

  return strcmp (SYMBOL_PRINT_NAME (sym_a->symbol),
		 SYMBOL_PRINT_NAME (sym_b->symbol));
}

/* Sort the NFOUND symbols in list FOUND and remove duplicates.
   The duplicates are freed, and the new list is returned in
   *NEW_HEAD, *NEW_TAIL.  */

static void
sort_search_symbols_remove_dups (struct symbol_search *found, int nfound,
				 struct symbol_search **new_head,
				 struct symbol_search **new_tail)
{
  struct symbol_search **symbols, *symp, *old_next;
  int i, j, nunique;

  gdb_assert (found != NULL && nfound > 0);

  /* Build an array out of the list so we can easily sort them.  */
  symbols = (struct symbol_search **) xmalloc (sizeof (struct symbol_search *)
					       * nfound);
  symp = found;
  for (i = 0; i < nfound; i++)
    {
      gdb_assert (symp != NULL);
      gdb_assert (symp->block >= 0 && symp->block <= 1);
      symbols[i] = symp;
      symp = symp->next;
    }
  gdb_assert (symp == NULL);

  qsort (symbols, nfound, sizeof (struct symbol_search *),
	 compare_search_syms);

  /* Collapse out the dups.  */
  for (i = 1, j = 1; i < nfound; ++i)
    {
      if (compare_search_syms (&symbols[j - 1], &symbols[i]) != 0)
	symbols[j++] = symbols[i];
      else
	xfree (symbols[i]);
    }
  nunique = j;
  symbols[j - 1]->next = NULL;

  /* Rebuild the linked list.  */
  for (i = 0; i < nunique - 1; i++)
    symbols[i]->next = symbols[i + 1];
  symbols[nunique - 1]->next = NULL;

  *new_head = symbols[0];
  *new_tail = symbols[nunique - 1];
  xfree (symbols);
}

/* An object of this type is passed as the user_data to the
   expand_symtabs_matching method.  */
struct search_symbols_data
{
  int nfiles;
  char **files;

  /* It is true if PREG contains valid data, false otherwise.  */
  unsigned preg_p : 1;
  regex_t preg;
};

/* A callback for expand_symtabs_matching.  */

static int
search_symbols_file_matches (const char *filename, void *user_data,
			     int basenames)
{
  struct search_symbols_data *data = user_data;

  return file_matches (filename, data->files, data->nfiles, basenames);
}

/* A callback for expand_symtabs_matching.  */

static int
search_symbols_name_matches (const char *symname, void *user_data)
{
  struct search_symbols_data *data = user_data;

  return !data->preg_p || regexec (&data->preg, symname, 0, NULL, 0) == 0;
}

/* Search the symbol table for matches to the regular expression REGEXP,
   returning the results in *MATCHES.

   Only symbols of KIND are searched:
   VARIABLES_DOMAIN - search all symbols, excluding functions, type names,
                      and constants (enums)
   FUNCTIONS_DOMAIN - search all functions
   TYPES_DOMAIN     - search all type names
   ALL_DOMAIN       - an internal error for this function

   free_search_symbols should be called when *MATCHES is no longer needed.

   Within each file the results are sorted locally; each symtab's global and
   static blocks are separately alphabetized.
   Duplicate entries are removed.  */

void
search_symbols (char *regexp, enum search_domain kind,
		int nfiles, char *files[],
		struct symbol_search **matches)
{
  struct symtab *s;
  struct blockvector *bv;
  struct block *b;
  int i = 0;
  struct block_iterator iter;
  struct symbol *sym;
  struct objfile *objfile;
  struct minimal_symbol *msymbol;
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
  struct symbol_search *found;
  struct symbol_search *tail;
  struct search_symbols_data datum;
  int nfound;

  /* OLD_CHAIN .. RETVAL_CHAIN is always freed, RETVAL_CHAIN .. current
     CLEANUP_CHAIN is freed only in the case of an error.  */
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  struct cleanup *retval_chain;

  gdb_assert (kind <= TYPES_DOMAIN);

  ourtype = types[kind];
  ourtype2 = types2[kind];
  ourtype3 = types3[kind];
  ourtype4 = types4[kind];

  *matches = NULL;
  datum.preg_p = 0;

  if (regexp != NULL)
    {
      /* Make sure spacing is right for C++ operators.
         This is just a courtesy to make the matching less sensitive
         to how many spaces the user leaves between 'operator'
         and <TYPENAME> or <OPERATOR>.  */
      char *opend;
      char *opname = operator_chars (regexp, &opend);
      int errcode;

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

      errcode = regcomp (&datum.preg, regexp,
			 REG_NOSUB | (case_sensitivity == case_sensitive_off
				      ? REG_ICASE : 0));
      if (errcode != 0)
	{
	  char *err = get_regcomp_error (errcode, &datum.preg);

	  make_cleanup (xfree, err);
	  error (_("Invalid regexp (%s): %s"), err, regexp);
	}
      datum.preg_p = 1;
      make_regfree_cleanup (&datum.preg);
    }

  /* Search through the partial symtabs *first* for all symbols
     matching the regexp.  That way we don't have to reproduce all of
     the machinery below.  */

  datum.nfiles = nfiles;
  datum.files = files;
  ALL_OBJFILES (objfile)
  {
    if (objfile->sf)
      objfile->sf->qf->expand_symtabs_matching (objfile,
						(nfiles == 0
						 ? NULL
						 : search_symbols_file_matches),
						search_symbols_name_matches,
						kind,
						&datum);
  }

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
      ALL_MSYMBOLS (objfile, msymbol)
      {
        QUIT;

	if (msymbol->created_by_gdb)
	  continue;

	if (MSYMBOL_TYPE (msymbol) == ourtype
	    || MSYMBOL_TYPE (msymbol) == ourtype2
	    || MSYMBOL_TYPE (msymbol) == ourtype3
	    || MSYMBOL_TYPE (msymbol) == ourtype4)
	  {
	    if (!datum.preg_p
		|| regexec (&datum.preg, SYMBOL_NATURAL_NAME (msymbol), 0,
			    NULL, 0) == 0)
	      {
		/* Note: An important side-effect of these lookup functions
		   is to expand the symbol table if msymbol is found, for the
		   benefit of the next loop on ALL_PRIMARY_SYMTABS.  */
		if (kind == FUNCTIONS_DOMAIN
		    ? find_pc_symtab (SYMBOL_VALUE_ADDRESS (msymbol)) == NULL
		    : (lookup_symbol_in_objfile_from_linkage_name
		       (objfile, SYMBOL_LINKAGE_NAME (msymbol), VAR_DOMAIN)
		       == NULL))
		  found_misc = 1;
	      }
	  }
      }
    }

  found = NULL;
  tail = NULL;
  nfound = 0;
  retval_chain = make_cleanup_free_search_symbols (&found);

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    bv = BLOCKVECTOR (s);
    for (i = GLOBAL_BLOCK; i <= STATIC_BLOCK; i++)
      {
	b = BLOCKVECTOR_BLOCK (bv, i);
	ALL_BLOCK_SYMBOLS (b, iter, sym)
	  {
	    struct symtab *real_symtab = SYMBOL_SYMTAB (sym);

	    QUIT;

	    /* Check first sole REAL_SYMTAB->FILENAME.  It does not need to be
	       a substring of symtab_to_fullname as it may contain "./" etc.  */
	    if ((file_matches (real_symtab->filename, files, nfiles, 0)
		 || ((basenames_may_differ
		      || file_matches (lbasename (real_symtab->filename),
				       files, nfiles, 1))
		     && file_matches (symtab_to_fullname (real_symtab),
				      files, nfiles, 0)))
		&& ((!datum.preg_p
		     || regexec (&datum.preg, SYMBOL_NATURAL_NAME (sym), 0,
				 NULL, 0) == 0)
		    && ((kind == VARIABLES_DOMAIN
			 && SYMBOL_CLASS (sym) != LOC_TYPEDEF
			 && SYMBOL_CLASS (sym) != LOC_UNRESOLVED
			 && SYMBOL_CLASS (sym) != LOC_BLOCK
			 /* LOC_CONST can be used for more than just enums,
			    e.g., c++ static const members.
			    We only want to skip enums here.  */
			 && !(SYMBOL_CLASS (sym) == LOC_CONST
			      && TYPE_CODE (SYMBOL_TYPE (sym))
			      == TYPE_CODE_ENUM))
			|| (kind == FUNCTIONS_DOMAIN 
			    && SYMBOL_CLASS (sym) == LOC_BLOCK)
			|| (kind == TYPES_DOMAIN
			    && SYMBOL_CLASS (sym) == LOC_TYPEDEF))))
	      {
		/* match */
		struct symbol_search *psr = (struct symbol_search *)
		  xmalloc (sizeof (struct symbol_search));
		psr->block = i;
		psr->symtab = real_symtab;
		psr->symbol = sym;
		memset (&psr->msymbol, 0, sizeof (psr->msymbol));
		psr->next = NULL;
		if (tail == NULL)
		  found = psr;
		else
		  tail->next = psr;
		tail = psr;
		nfound ++;
	      }
	  }
      }
  }

  if (found != NULL)
    {
      sort_search_symbols_remove_dups (found, nfound, &found, &tail);
      /* Note: nfound is no longer useful beyond this point.  */
    }

  /* If there are no eyes, avoid all contact.  I mean, if there are
     no debug symbols, then print directly from the msymbol_vector.  */

  if (found_misc || (nfiles == 0 && kind != FUNCTIONS_DOMAIN))
    {
      ALL_MSYMBOLS (objfile, msymbol)
      {
        QUIT;

	if (msymbol->created_by_gdb)
	  continue;

	if (MSYMBOL_TYPE (msymbol) == ourtype
	    || MSYMBOL_TYPE (msymbol) == ourtype2
	    || MSYMBOL_TYPE (msymbol) == ourtype3
	    || MSYMBOL_TYPE (msymbol) == ourtype4)
	  {
	    if (!datum.preg_p
		|| regexec (&datum.preg, SYMBOL_NATURAL_NAME (msymbol), 0,
			    NULL, 0) == 0)
	      {
		/* For functions we can do a quick check of whether the
		   symbol might be found via find_pc_symtab.  */
		if (kind != FUNCTIONS_DOMAIN
		    || find_pc_symtab (SYMBOL_VALUE_ADDRESS (msymbol)) == NULL)
		  {
		    if (lookup_symbol_in_objfile_from_linkage_name
			(objfile, SYMBOL_LINKAGE_NAME (msymbol), VAR_DOMAIN)
			== NULL)
		      {
			/* match */
			struct symbol_search *psr = (struct symbol_search *)
			  xmalloc (sizeof (struct symbol_search));
			psr->block = i;
			psr->msymbol.minsym = msymbol;
			psr->msymbol.objfile = objfile;
			psr->symtab = NULL;
			psr->symbol = NULL;
			psr->next = NULL;
			if (tail == NULL)
			  found = psr;
			else
			  tail->next = psr;
			tail = psr;
		      }
		  }
	      }
	  }
      }
    }

  discard_cleanups (retval_chain);
  do_cleanups (old_chain);
  *matches = found;
}

/* Helper function for symtab_symbol_info, this function uses
   the data returned from search_symbols() to print information
   regarding the match to gdb_stdout.  */

static void
print_symbol_info (enum search_domain kind,
		   struct symtab *s, struct symbol *sym,
		   int block, const char *last)
{
  const char *s_filename = symtab_to_filename_for_display (s);

  if (last == NULL || filename_cmp (last, s_filename) != 0)
    {
      fputs_filtered ("\nFile ", gdb_stdout);
      fputs_filtered (s_filename, gdb_stdout);
      fputs_filtered (":\n", gdb_stdout);
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
    tmp = hex_string_custom (SYMBOL_VALUE_ADDRESS (msymbol.minsym)
			     & (CORE_ADDR) 0xffffffff,
			     8);
  else
    tmp = hex_string_custom (SYMBOL_VALUE_ADDRESS (msymbol.minsym),
			     16);
  printf_filtered ("%s  %s\n",
		   tmp, SYMBOL_PRINT_NAME (msymbol.minsym));
}

/* This is the guts of the commands "info functions", "info types", and
   "info variables".  It calls search_symbols to find all matches and then
   print_[m]symbol_info to print out some useful information about the
   matches.  */

static void
symtab_symbol_info (char *regexp, enum search_domain kind, int from_tty)
{
  static const char * const classnames[] =
    {"variable", "function", "type"};
  struct symbol_search *symbols;
  struct symbol_search *p;
  struct cleanup *old_chain;
  const char *last_filename = NULL;
  int first = 1;

  gdb_assert (kind <= TYPES_DOMAIN);

  /* Must make sure that if we're interrupted, symbols gets freed.  */
  search_symbols (regexp, kind, 0, (char **) NULL, &symbols);
  old_chain = make_cleanup_free_search_symbols (&symbols);

  if (regexp != NULL)
    printf_filtered (_("All %ss matching regular expression \"%s\":\n"),
		     classnames[kind], regexp);
  else
    printf_filtered (_("All defined %ss:\n"), classnames[kind]);

  for (p = symbols; p != NULL; p = p->next)
    {
      QUIT;

      if (p->msymbol.minsym != NULL)
	{
	  if (first)
	    {
	      printf_filtered (_("\nNon-debugging symbols:\n"));
	      first = 0;
	    }
	  print_msymbol_info (p->msymbol);
	}
      else
	{
	  print_symbol_info (kind,
			     p->symtab,
			     p->symbol,
			     p->block,
			     last_filename);
	  last_filename = symtab_to_filename_for_display (p->symtab);
	}
    }

  do_cleanups (old_chain);
}

static void
variables_info (char *regexp, int from_tty)
{
  symtab_symbol_info (regexp, VARIABLES_DOMAIN, from_tty);
}

static void
functions_info (char *regexp, int from_tty)
{
  symtab_symbol_info (regexp, FUNCTIONS_DOMAIN, from_tty);
}


static void
types_info (char *regexp, int from_tty)
{
  symtab_symbol_info (regexp, TYPES_DOMAIN, from_tty);
}

/* Breakpoint all functions matching regular expression.  */

void
rbreak_command_wrapper (char *regexp, int from_tty)
{
  rbreak_command (regexp, from_tty);
}

/* A cleanup function that calls end_rbreak_breakpoints.  */

static void
do_end_rbreak_breakpoints (void *ignore)
{
  end_rbreak_breakpoints ();
}

static void
rbreak_command (char *regexp, int from_tty)
{
  struct symbol_search *ss;
  struct symbol_search *p;
  struct cleanup *old_chain;
  char *string = NULL;
  int len = 0;
  char **files = NULL, *file_name;
  int nfiles = 0;

  if (regexp)
    {
      char *colon = strchr (regexp, ':');

      if (colon && *(colon + 1) != ':')
	{
	  int colon_index;

	  colon_index = colon - regexp;
	  file_name = alloca (colon_index + 1);
	  memcpy (file_name, regexp, colon_index);
	  file_name[colon_index--] = 0;
	  while (isspace (file_name[colon_index]))
	    file_name[colon_index--] = 0; 
	  files = &file_name;
	  nfiles = 1;
	  regexp = skip_spaces (colon + 1);
	}
    }

  search_symbols (regexp, FUNCTIONS_DOMAIN, nfiles, files, &ss);
  old_chain = make_cleanup_free_search_symbols (&ss);
  make_cleanup (free_current_contents, &string);

  start_rbreak_breakpoints ();
  make_cleanup (do_end_rbreak_breakpoints, NULL);
  for (p = ss; p != NULL; p = p->next)
    {
      if (p->msymbol.minsym == NULL)
	{
	  const char *fullname = symtab_to_fullname (p->symtab);

	  int newlen = (strlen (fullname)
			+ strlen (SYMBOL_LINKAGE_NAME (p->symbol))
			+ 4);

	  if (newlen > len)
	    {
	      string = xrealloc (string, newlen);
	      len = newlen;
	    }
	  strcpy (string, fullname);
	  strcat (string, ":'");
	  strcat (string, SYMBOL_LINKAGE_NAME (p->symbol));
	  strcat (string, "'");
	  break_command (string, from_tty);
	  print_symbol_info (FUNCTIONS_DOMAIN,
			     p->symtab,
			     p->symbol,
			     p->block,
			     symtab_to_filename_for_display (p->symtab));
	}
      else
	{
	  int newlen = (strlen (SYMBOL_LINKAGE_NAME (p->msymbol.minsym)) + 3);

	  if (newlen > len)
	    {
	      string = xrealloc (string, newlen);
	      len = newlen;
	    }
	  strcpy (string, "'");
	  strcat (string, SYMBOL_LINKAGE_NAME (p->msymbol.minsym));
	  strcat (string, "'");

	  break_command (string, from_tty);
	  printf_filtered ("<function, no debug info> %s;\n",
			   SYMBOL_PRINT_NAME (p->msymbol.minsym));
	}
    }

  do_cleanups (old_chain);
}


/* Evaluate if NAME matches SYM_TEXT and SYM_TEXT_LEN.

   Either sym_text[sym_text_len] != '(' and then we search for any
   symbol starting with SYM_TEXT text.

   Otherwise sym_text[sym_text_len] == '(' and then we require symbol name to
   be terminated at that point.  Partial symbol tables do not have parameters
   information.  */

static int
compare_symbol_name (const char *name, const char *sym_text, int sym_text_len)
{
  int (*ncmp) (const char *, const char *, size_t);

  ncmp = (case_sensitivity == case_sensitive_on ? strncmp : strncasecmp);

  if (ncmp (name, sym_text, sym_text_len) != 0)
    return 0;

  if (sym_text[sym_text_len] == '(')
    {
      /* User searches for `name(someth...'.  Require NAME to be terminated.
	 Normally psymtabs and gdbindex have no parameter types so '\0' will be
	 present but accept even parameters presence.  In this case this
	 function is in fact strcmp_iw but whitespace skipping is not supported
	 for tab completion.  */

      if (name[sym_text_len] != '\0' && name[sym_text_len] != '(')
	return 0;
    }

  return 1;
}

/* Free any memory associated with a completion list.  */

static void
free_completion_list (VEC (char_ptr) **list_ptr)
{
  int i;
  char *p;

  for (i = 0; VEC_iterate (char_ptr, *list_ptr, i, p); ++i)
    xfree (p);
  VEC_free (char_ptr, *list_ptr);
}

/* Callback for make_cleanup.  */

static void
do_free_completion_list (void *list)
{
  free_completion_list (list);
}

/* Helper routine for make_symbol_completion_list.  */

static VEC (char_ptr) *return_val;

#define COMPLETION_LIST_ADD_SYMBOL(symbol, sym_text, len, text, word) \
      completion_list_add_name \
	(SYMBOL_NATURAL_NAME (symbol), (sym_text), (len), (text), (word))

/*  Test to see if the symbol specified by SYMNAME (which is already
   demangled for C++ symbols) matches SYM_TEXT in the first SYM_TEXT_LEN
   characters.  If so, add it to the current completion list.  */

static void
completion_list_add_name (const char *symname,
			  const char *sym_text, int sym_text_len,
			  const char *text, const char *word)
{
  /* Clip symbols that cannot match.  */
  if (!compare_symbol_name (symname, sym_text, sym_text_len))
    return;

  /* We have a match for a completion, so add SYMNAME to the current list
     of matches.  Note that the name is moved to freshly malloc'd space.  */

  {
    char *new;

    if (word == sym_text)
      {
	new = xmalloc (strlen (symname) + 5);
	strcpy (new, symname);
      }
    else if (word > sym_text)
      {
	/* Return some portion of symname.  */
	new = xmalloc (strlen (symname) + 5);
	strcpy (new, symname + (word - sym_text));
      }
    else
      {
	/* Return some of SYM_TEXT plus symname.  */
	new = xmalloc (strlen (symname) + (sym_text - word) + 5);
	strncpy (new, word, sym_text - word);
	new[sym_text - word] = '\0';
	strcat (new, symname);
      }

    VEC_safe_push (char_ptr, return_val, new);
  }
}

/* ObjC: In case we are completing on a selector, look as the msymbol
   again and feed all the selectors into the mill.  */

static void
completion_list_objc_symbol (struct minimal_symbol *msymbol,
			     const char *sym_text, int sym_text_len,
			     const char *text, const char *word)
{
  static char *tmp = NULL;
  static unsigned int tmplen = 0;

  const char *method, *category, *selector;
  char *tmp2 = NULL;

  method = SYMBOL_NATURAL_NAME (msymbol);

  /* Is it a method?  */
  if ((method[0] != '-') && (method[0] != '+'))
    return;

  if (sym_text[0] == '[')
    /* Complete on shortened method method.  */
    completion_list_add_name (method + 1, sym_text, sym_text_len, text, word);

  while ((strlen (method) + 1) >= tmplen)
    {
      if (tmplen == 0)
	tmplen = 1024;
      else
	tmplen *= 2;
      tmp = xrealloc (tmp, tmplen);
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
      completion_list_add_name (tmp, sym_text, sym_text_len, text, word);
      if (sym_text[0] == '[')
	completion_list_add_name (tmp + 1, sym_text, sym_text_len, text, word);
    }

  if (selector != NULL)
    {
      /* Complete on selector only.  */
      strcpy (tmp, selector);
      tmp2 = strchr (tmp, ']');
      if (tmp2 != NULL)
	*tmp2 = '\0';

      completion_list_add_name (tmp, sym_text, sym_text_len, text, word);
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
completion_list_add_fields (struct symbol *sym, const char *sym_text,
			    int sym_text_len, const char *text,
			    const char *word)
{
  if (SYMBOL_CLASS (sym) == LOC_TYPEDEF)
    {
      struct type *t = SYMBOL_TYPE (sym);
      enum type_code c = TYPE_CODE (t);
      int j;

      if (c == TYPE_CODE_UNION || c == TYPE_CODE_STRUCT)
	for (j = TYPE_N_BASECLASSES (t); j < TYPE_NFIELDS (t); j++)
	  if (TYPE_FIELD_NAME (t, j))
	    completion_list_add_name (TYPE_FIELD_NAME (t, j),
				      sym_text, sym_text_len, text, word);
    }
}

/* Type of the user_data argument passed to add_macro_name or
   expand_partial_symbol_name.  The contents are simply whatever is
   needed by completion_list_add_name.  */
struct add_name_data
{
  const char *sym_text;
  int sym_text_len;
  const char *text;
  const char *word;
};

/* A callback used with macro_for_each and macro_for_each_in_scope.
   This adds a macro's name to the current completion list.  */

static void
add_macro_name (const char *name, const struct macro_definition *ignore,
		struct macro_source_file *ignore2, int ignore3,
		void *user_data)
{
  struct add_name_data *datum = (struct add_name_data *) user_data;

  completion_list_add_name ((char *) name,
			    datum->sym_text, datum->sym_text_len,
			    datum->text, datum->word);
}

/* A callback for expand_partial_symbol_names.  */

static int
expand_partial_symbol_name (const char *name, void *user_data)
{
  struct add_name_data *datum = (struct add_name_data *) user_data;

  return compare_symbol_name (name, datum->sym_text, datum->sym_text_len);
}

VEC (char_ptr) *
default_make_symbol_completion_list_break_on (const char *text,
					      const char *word,
					      const char *break_on,
					      enum type_code code)
{
  /* Problem: All of the symbols have to be copied because readline
     frees them.  I'm not going to worry about this; hopefully there
     won't be that many.  */

  struct symbol *sym;
  struct symtab *s;
  struct minimal_symbol *msymbol;
  struct objfile *objfile;
  struct block *b;
  const struct block *surrounding_static_block, *surrounding_global_block;
  struct block_iterator iter;
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;
  /* Length of sym_text.  */
  int sym_text_len;
  struct add_name_data datum;
  struct cleanup *back_to;

  /* Now look for the symbol we are supposed to complete on.  */
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
	return NULL;
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

  sym_text_len = strlen (sym_text);

  /* Prepare SYM_TEXT_LEN for compare_symbol_name.  */

  if (current_language->la_language == language_cplus
      || current_language->la_language == language_java
      || current_language->la_language == language_fortran)
    {
      /* These languages may have parameters entered by user but they are never
	 present in the partial symbol tables.  */

      const char *cs = memchr (sym_text, '(', sym_text_len);

      if (cs)
	sym_text_len = cs - sym_text;
    }
  gdb_assert (sym_text[sym_text_len] == '\0' || sym_text[sym_text_len] == '(');

  return_val = NULL;
  back_to = make_cleanup (do_free_completion_list, &return_val);

  datum.sym_text = sym_text;
  datum.sym_text_len = sym_text_len;
  datum.text = text;
  datum.word = word;

  /* Look through the partial symtabs for all symbols which begin
     by matching SYM_TEXT.  Expand all CUs that you find to the list.
     The real names will get added by COMPLETION_LIST_ADD_SYMBOL below.  */
  expand_partial_symbol_names (expand_partial_symbol_name, &datum);

  /* At this point scan through the misc symbol vectors and add each
     symbol you find to the list.  Eventually we want to ignore
     anything that isn't a text symbol (everything else will be
     handled by the psymtab code above).  */

  if (code == TYPE_CODE_UNDEF)
    {
      ALL_MSYMBOLS (objfile, msymbol)
	{
	  QUIT;
	  COMPLETION_LIST_ADD_SYMBOL (msymbol, sym_text, sym_text_len, text,
				      word);

	  completion_list_objc_symbol (msymbol, sym_text, sym_text_len, text,
				       word);
	}
    }

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
		COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text,
					    word);
		completion_list_add_fields (sym, sym_text, sym_text_len, text,
					    word);
	      }
	    else if (SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN
		     && TYPE_CODE (SYMBOL_TYPE (sym)) == code)
	      COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text,
					  word);
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
	  completion_list_add_fields (sym, sym_text, sym_text_len, text, word);

      if (surrounding_global_block != NULL)
	ALL_BLOCK_SYMBOLS (surrounding_global_block, iter, sym)
	  completion_list_add_fields (sym, sym_text, sym_text_len, text, word);
    }

  /* Go through the symtabs and check the externs and statics for
     symbols which match.  */

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), GLOBAL_BLOCK);
    ALL_BLOCK_SYMBOLS (b, iter, sym)
      {
	if (code == TYPE_CODE_UNDEF
	    || (SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN
		&& TYPE_CODE (SYMBOL_TYPE (sym)) == code))
	  COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text, word);
      }
  }

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), STATIC_BLOCK);
    ALL_BLOCK_SYMBOLS (b, iter, sym)
      {
	if (code == TYPE_CODE_UNDEF
	    || (SYMBOL_DOMAIN (sym) == STRUCT_DOMAIN
		&& TYPE_CODE (SYMBOL_TYPE (sym)) == code))
	  COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text, word);
      }
  }

  /* Skip macros if we are completing a struct tag -- arguable but
     usually what is expected.  */
  if (current_language->la_macro_expansion == macro_expansion_c
      && code == TYPE_CODE_UNDEF)
    {
      struct macro_scope *scope;

      /* Add any macros visible in the default scope.  Note that this
	 may yield the occasional wrong result, because an expression
	 might be evaluated in a scope other than the default.  For
	 example, if the user types "break file:line if <TAB>", the
	 resulting expression will be evaluated at "file:line" -- but
	 at there does not seem to be a way to detect this at
	 completion time.  */
      scope = default_macro_scope ();
      if (scope)
	{
	  macro_for_each_in_scope (scope->file, scope->line,
				   add_macro_name, &datum);
	  xfree (scope);
	}

      /* User-defined macros are always visible.  */
      macro_for_each (macro_user_macros, add_macro_name, &datum);
    }

  discard_cleanups (back_to);
  return (return_val);
}

VEC (char_ptr) *
default_make_symbol_completion_list (const char *text, const char *word,
				     enum type_code code)
{
  return default_make_symbol_completion_list_break_on (text, word, "", code);
}

/* Return a vector of all symbols (regardless of class) which begin by
   matching TEXT.  If the answer is no symbols, then the return value
   is NULL.  */

VEC (char_ptr) *
make_symbol_completion_list (const char *text, const char *word)
{
  return current_language->la_make_symbol_completion_list (text, word,
							   TYPE_CODE_UNDEF);
}

/* Like make_symbol_completion_list, but only return STRUCT_DOMAIN
   symbols whose type code is CODE.  */

VEC (char_ptr) *
make_symbol_completion_type (const char *text, const char *word,
			     enum type_code code)
{
  gdb_assert (code == TYPE_CODE_UNION
	      || code == TYPE_CODE_STRUCT
	      || code == TYPE_CODE_CLASS
	      || code == TYPE_CODE_ENUM);
  return current_language->la_make_symbol_completion_list (text, word, code);
}

/* Like make_symbol_completion_list, but suitable for use as a
   completion function.  */

VEC (char_ptr) *
make_symbol_completion_list_fn (struct cmd_list_element *ignore,
				const char *text, const char *word)
{
  return make_symbol_completion_list (text, word);
}

/* Like make_symbol_completion_list, but returns a list of symbols
   defined in a source file FILE.  */

VEC (char_ptr) *
make_file_symbol_completion_list (const char *text, const char *word,
				  const char *srcfile)
{
  struct symbol *sym;
  struct symtab *s;
  struct block *b;
  struct block_iterator iter;
  /* The symbol we are completing on.  Points in same buffer as text.  */
  const char *sym_text;
  /* Length of sym_text.  */
  int sym_text_len;

  /* Now look for the symbol we are supposed to complete on.
     FIXME: This should be language-specific.  */
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
	return NULL;
      }
    else
      {
	/* Not a quoted string.  */
	sym_text = language_search_unquoted_string (text, p);
      }
  }

  sym_text_len = strlen (sym_text);

  return_val = NULL;

  /* Find the symtab for SRCFILE (this loads it if it was not yet read
     in).  */
  s = lookup_symtab (srcfile);
  if (s == NULL)
    {
      /* Maybe they typed the file with leading directories, while the
	 symbol tables record only its basename.  */
      const char *tail = lbasename (srcfile);

      if (tail > srcfile)
	s = lookup_symtab (tail);
    }

  /* If we have no symtab for that file, return an empty list.  */
  if (s == NULL)
    return (return_val);

  /* Go through this symtab and check the externs and statics for
     symbols which match.  */

  b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), GLOBAL_BLOCK);
  ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text, word);
    }

  b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), STATIC_BLOCK);
  ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      COMPLETION_LIST_ADD_SYMBOL (sym, sym_text, sym_text_len, text, word);
    }

  return (return_val);
}

/* A helper function for make_source_files_completion_list.  It adds
   another file name to a list of possible completions, growing the
   list as necessary.  */

static void
add_filename_to_list (const char *fname, const char *text, const char *word,
		      VEC (char_ptr) **list)
{
  char *new;
  size_t fnlen = strlen (fname);

  if (word == text)
    {
      /* Return exactly fname.  */
      new = xmalloc (fnlen + 5);
      strcpy (new, fname);
    }
  else if (word > text)
    {
      /* Return some portion of fname.  */
      new = xmalloc (fnlen + 5);
      strcpy (new, fname + (word - text));
    }
  else
    {
      /* Return some of TEXT plus fname.  */
      new = xmalloc (fnlen + (text - word) + 5);
      strncpy (new, word, text - word);
      new[text - word] = '\0';
      strcat (new, fname);
    }
  VEC_safe_push (char_ptr, *list, new);
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
  VEC (char_ptr) **list;
};

/* A callback for map_partial_symbol_filenames.  */

static void
maybe_add_partial_symtab_filename (const char *filename, const char *fullname,
				   void *user_data)
{
  struct add_partial_filename_data *data = user_data;

  if (not_interesting_fname (filename))
    return;
  if (!filename_seen (data->filename_seen_cache, filename, 1)
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
	  && !filename_seen (data->filename_seen_cache, base_name, 1)
	  && filename_ncmp (base_name, data->text, data->text_len) == 0)
	add_filename_to_list (base_name, data->text, data->word, data->list);
    }
}

/* Return a vector of all source files whose names begin with matching
   TEXT.  The file names are looked up in the symbol tables of this
   program.  If the answer is no matchess, then the return value is
   NULL.  */

VEC (char_ptr) *
make_source_files_completion_list (const char *text, const char *word)
{
  struct symtab *s;
  struct objfile *objfile;
  size_t text_len = strlen (text);
  VEC (char_ptr) *list = NULL;
  const char *base_name;
  struct add_partial_filename_data datum;
  struct filename_seen_cache *filename_seen_cache;
  struct cleanup *back_to, *cache_cleanup;

  if (!have_full_symbols () && !have_partial_symbols ())
    return list;

  back_to = make_cleanup (do_free_completion_list, &list);

  filename_seen_cache = create_filename_seen_cache ();
  cache_cleanup = make_cleanup (delete_filename_seen_cache,
				filename_seen_cache);

  ALL_SYMTABS (objfile, s)
    {
      if (not_interesting_fname (s->filename))
	continue;
      if (!filename_seen (filename_seen_cache, s->filename, 1)
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
	      && !filename_seen (filename_seen_cache, base_name, 1)
	      && filename_ncmp (base_name, text, text_len) == 0)
	    add_filename_to_list (base_name, text, word, &list);
	}
    }

  datum.filename_seen_cache = filename_seen_cache;
  datum.text = text;
  datum.word = word;
  datum.text_len = text_len;
  datum.list = &list;
  map_partial_symbol_filenames (maybe_add_partial_symtab_filename, &datum,
				0 /*need_fullname*/);

  do_cleanups (cache_cleanup);
  discard_cleanups (back_to);

  return list;
}

/* Determine if PC is in the prologue of a function.  The prologue is the area
   between the first instruction of a function, and the first executable line.
   Returns 1 if PC *might* be in prologue, 0 if definately *not* in prologue.

   If non-zero, func_start is where we think the prologue starts, possibly
   by previous examination of symbol table information.  */

int
in_prologue (struct gdbarch *gdbarch, CORE_ADDR pc, CORE_ADDR func_start)
{
  struct symtab_and_line sal;
  CORE_ADDR func_addr, func_end;

  /* We have several sources of information we can consult to figure
     this out.
     - Compilers usually emit line number info that marks the prologue
       as its own "source line".  So the ending address of that "line"
       is the end of the prologue.  If available, this is the most
       reliable method.
     - The minimal symbols and partial symbols, which can usually tell
       us the starting and ending addresses of a function.
     - If we know the function's start address, we can call the
       architecture-defined gdbarch_skip_prologue function to analyze the
       instruction stream and guess where the prologue ends.
     - Our `func_start' argument; if non-zero, this is the caller's
       best guess as to the function's entry point.  At the time of
       this writing, handle_inferior_event doesn't get this right, so
       it should be our last resort.  */

  /* Consult the partial symbol table, to find which function
     the PC is in.  */
  if (! find_pc_partial_function (pc, NULL, &func_addr, &func_end))
    {
      CORE_ADDR prologue_end;

      /* We don't even have minsym information, so fall back to using
         func_start, if given.  */
      if (! func_start)
	return 1;		/* We *might* be in a prologue.  */

      prologue_end = gdbarch_skip_prologue (gdbarch, func_start);

      return func_start <= pc && pc < prologue_end;
    }

  /* If we have line number information for the function, that's
     usually pretty reliable.  */
  sal = find_pc_line (func_addr, 0);

  /* Now sal describes the source line at the function's entry point,
     which (by convention) is the prologue.  The end of that "line",
     sal.end, is the end of the prologue.

     Note that, for functions whose source code is all on a single
     line, the line number information doesn't always end up this way.
     So we must verify that our purported end-of-prologue address is
     *within* the function, not at its start or end.  */
  if (sal.line == 0
      || sal.end <= func_addr
      || func_end <= sal.end)
    {
      /* We don't have any good line number info, so use the minsym
	 information, together with the architecture-specific prologue
	 scanning code.  */
      CORE_ADDR prologue_end = gdbarch_skip_prologue (gdbarch, func_addr);

      return func_addr <= pc && pc < prologue_end;
    }

  /* We have line number info, and it looks good.  */
  return func_addr <= pc && pc < sal.end;
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
  struct block *bl;

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
	  struct linetable *linetable = LINETABLE (prologue_sal.symtab);
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

/* Track MAIN */
static char *name_of_main;
enum language language_of_main = language_unknown;

void
set_main_name (const char *name)
{
  if (name_of_main != NULL)
    {
      xfree (name_of_main);
      name_of_main = NULL;
      language_of_main = language_unknown;
    }
  if (name != NULL)
    {
      name_of_main = xstrdup (name);
      language_of_main = language_unknown;
    }
}

/* Deduce the name of the main procedure, and set NAME_OF_MAIN
   accordingly.  */

static void
find_main_name (void)
{
  const char *new_main_name;

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
      set_main_name (new_main_name);
      return;
    }

  new_main_name = go_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name);
      return;
    }

  new_main_name = pascal_main_name ();
  if (new_main_name != NULL)
    {
      set_main_name (new_main_name);
      return;
    }

  /* The languages above didn't identify the name of the main procedure.
     Fallback to "main".  */
  set_main_name ("main");
}

char *
main_name (void)
{
  if (name_of_main == NULL)
    find_main_name ();

  return name_of_main;
}

/* Handle ``executable_changed'' events for the symtab module.  */

static void
symtab_observer_executable_changed (void)
{
  /* NAME_OF_MAIN may no longer be the same, so reset it for now.  */
  set_main_name (NULL);
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
    if (strncmp (producer, arm_idents[i], strlen (arm_idents[i])) == 0)
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
  gdb_assert (ops->read_needs_frame != NULL);
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
    symbol_impl[i].aclass = i;
}



/* Initialize the symbol SYM.  */

void
initialize_symbol (struct symbol *sym)
{
  memset (sym, 0, sizeof (*sym));
  SYMBOL_SECTION (sym) = -1;
}

/* Allocate and initialize a new 'struct symbol' on OBJFILE's
   obstack.  */

struct symbol *
allocate_symbol (struct objfile *objfile)
{
  struct symbol *result;

  result = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct symbol);
  SYMBOL_SECTION (result) = -1;

  return result;
}

/* Allocate and initialize a new 'struct template_symbol' on OBJFILE's
   obstack.  */

struct template_symbol *
allocate_template_symbol (struct objfile *objfile)
{
  struct template_symbol *result;

  result = OBSTACK_ZALLOC (&objfile->objfile_obstack, struct template_symbol);
  SYMBOL_SECTION (&result->base) = -1;

  return result;
}



void
_initialize_symtab (void)
{
  initialize_ordinary_address_classes ();

  add_info ("variables", variables_info, _("\
All global and static variable names, or those matching REGEXP."));
  if (dbx_commands)
    add_com ("whereis", class_info, variables_info, _("\
All global and static variable names, or those matching REGEXP."));

  add_info ("functions", functions_info,
	    _("All function names, or those matching REGEXP."));

  /* FIXME:  This command has at least the following problems:
     1.  It prints builtin types (in a very strange and confusing fashion).
     2.  It doesn't print right, e.g. with
     typedef struct foo *FOO
     type_print prints "FOO" when we want to make it (in this situation)
     print "struct foo *".
     I also think "ptype" or "whatis" is more likely to be useful (but if
     there is much disagreement "info types" can be fixed).  */
  add_info ("types", types_info,
	    _("All type names, or those matching REGEXP."));

  add_info ("sources", sources_info,
	    _("Source files in the program."));

  add_com ("rbreak", class_breakpoint, rbreak_command,
	   _("Set a breakpoint for all functions matching REGEXP."));

  if (xdb_commands)
    {
      add_com ("lf", class_info, sources_info,
	       _("Source files in the program"));
      add_com ("lg", class_info, variables_info, _("\
All global and static variable names, or those matching REGEXP."));
    }

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

  observer_attach_executable_changed (symtab_observer_executable_changed);
}
