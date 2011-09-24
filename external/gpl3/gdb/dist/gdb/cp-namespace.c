/* Helper routines for C++ support in GDB.
   Copyright (C) 2003, 2004, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

   Contributed by David Carlton and by Kealia, Inc.

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
#include "cp-support.h"
#include "gdb_obstack.h"
#include "symtab.h"
#include "symfile.h"
#include "gdb_assert.h"
#include "block.h"
#include "objfiles.h"
#include "gdbtypes.h"
#include "dictionary.h"
#include "command.h"
#include "frame.h"
#include "buildsym.h"
#include "language.h"

static struct symbol *lookup_namespace_scope (const char *name,
					      const struct block *block,
					      const domain_enum domain,
					      const char *scope,
					      int scope_len);

static struct symbol *lookup_symbol_file (const char *name,
					  const struct block *block,
					  const domain_enum domain,
					  int anonymous_namespace);

static struct type *cp_lookup_transparent_type_loop (const char *name,
						     const char *scope,
						     int scope_len);

static void initialize_namespace_symtab (struct objfile *objfile);

static struct block *get_possible_namespace_block (struct objfile *objfile);

static void free_namespace_block (struct symtab *symtab);

static int check_possible_namespace_symbols_loop (const char *name,
						  int len,
						  struct objfile *objfile);

static int check_one_possible_namespace_symbol (const char *name,
						int len,
						struct objfile *objfile);

static struct symbol *lookup_possible_namespace_symbol (const char *name);

static void maintenance_cplus_namespace (char *args, int from_tty);

/* Check to see if SYMBOL refers to an object contained within an
   anonymous namespace; if so, add an appropriate using directive.  */

void
cp_scan_for_anonymous_namespaces (const struct symbol *symbol)
{
  if (SYMBOL_DEMANGLED_NAME (symbol) != NULL)
    {
      const char *name = SYMBOL_DEMANGLED_NAME (symbol);
      unsigned int previous_component;
      unsigned int next_component;

      /* Start with a quick-and-dirty check for mention of "(anonymous
	 namespace)".  */

      if (!cp_is_anonymous (name))
	return;

      previous_component = 0;
      next_component = cp_find_first_component (name + previous_component);

      while (name[next_component] == ':')
	{
	  if (((next_component - previous_component)
	       == CP_ANONYMOUS_NAMESPACE_LEN)
	      && strncmp (name + previous_component,
			  CP_ANONYMOUS_NAMESPACE_STR,
			  CP_ANONYMOUS_NAMESPACE_LEN) == 0)
	    {
	      int dest_len = (previous_component == 0
			      ? 0 : previous_component - 2);
	      int src_len = next_component;

	      char *dest = alloca (dest_len + 1);
	      char *src = alloca (src_len + 1);

	      memcpy (dest, name, dest_len);
	      memcpy (src, name, src_len);

	      dest[dest_len] = '\0';
	      src[src_len] = '\0';

	      /* We've found a component of the name that's an
		 anonymous namespace.  So add symbols in it to the
		 namespace given by the previous component if there is
		 one, or to the global namespace if there isn't.  */
	      cp_add_using_directive (dest, src, NULL, NULL,
	                              &SYMBOL_SYMTAB (symbol)->objfile->objfile_obstack);
	    }
	  /* The "+ 2" is for the "::".  */
	  previous_component = next_component + 2;
	  next_component = (previous_component
			    + cp_find_first_component (name
						       + previous_component));
	}
    }
}


/* Add a using directive to using_directives.  If the using directive
   in question has already been added, don't add it twice.

   Create a new struct using_direct which imports the namespace SRC
   into the scope DEST.  ALIAS is the name of the imported namespace
   in the current scope.  If ALIAS is NULL then the namespace is known
   by its original name.  DECLARATION is the name if the imported
   varable if this is a declaration import (Eg. using A::x), otherwise
   it is NULL.  The arguments are copied into newly allocated memory
   so they can be temporaries.  */

void
cp_add_using_directive (const char *dest,
			const char *src,
			const char *alias,
			const char *declaration,
                        struct obstack *obstack)
{
  struct using_direct *current;
  struct using_direct *new;
  
  /* Has it already been added?  */

  for (current = using_directives; current != NULL; current = current->next)
    {
      if (strcmp (current->import_src, src) == 0
          && strcmp (current->import_dest, dest) == 0
          && ((alias == NULL && current->alias == NULL)
              || (alias != NULL && current->alias != NULL
        	  && strcmp (alias, current->alias) == 0))
	  && ((declaration == NULL && current->declaration == NULL)
	      || (declaration != NULL && current->declaration != NULL
		  && strcmp (declaration, current->declaration) == 0)))
	return;
    }

  new = OBSTACK_ZALLOC (obstack, struct using_direct);

  new->import_src = obsavestring (src, strlen (src), obstack);
  new->import_dest = obsavestring (dest, strlen (dest), obstack);

  if (alias != NULL)
    new->alias = obsavestring (alias, strlen (alias), obstack);

  if (declaration != NULL)
    new->declaration = obsavestring (declaration, strlen (declaration),
                                     obstack);

  new->next = using_directives;
  using_directives = new;
}

/* Record the namespace that the function defined by SYMBOL was
   defined in, if necessary.  BLOCK is the associated block; use
   OBSTACK for allocation.  */

void
cp_set_block_scope (const struct symbol *symbol,
		    struct block *block,
		    struct obstack *obstack,
		    const char *processing_current_prefix,
		    int processing_has_namespace_info)
{
  if (processing_has_namespace_info)
    {
      block_set_scope
	(block, obsavestring (processing_current_prefix,
			      strlen (processing_current_prefix),
			      obstack),
	 obstack);
    }
  else if (SYMBOL_DEMANGLED_NAME (symbol) != NULL)
    {
      /* Try to figure out the appropriate namespace from the
	 demangled name.  */

      /* FIXME: carlton/2003-04-15: If the function in question is
	 a method of a class, the name will actually include the
	 name of the class as well.  This should be harmless, but
	 is a little unfortunate.  */

      const char *name = SYMBOL_DEMANGLED_NAME (symbol);
      unsigned int prefix_len = cp_entire_prefix_len (name);

      block_set_scope (block,
		       obsavestring (name, prefix_len, obstack),
		       obstack);
    }
}

/* Test whether or not NAMESPACE looks like it mentions an anonymous
   namespace; return nonzero if so.  */

int
cp_is_anonymous (const char *namespace)
{
  return (strstr (namespace, CP_ANONYMOUS_NAMESPACE_STR)
	  != NULL);
}

/* The C++-specific version of name lookup for static and global
   names.  This makes sure that names get looked for in all namespaces
   that are in scope.  NAME is the natural name of the symbol that
   we're looking for, BLOCK is the block that we're searching within,
   DOMAIN says what kind of symbols we're looking for, and if SYMTAB
   is non-NULL, we should store the symtab where we found the symbol
   in it.  */

struct symbol *
cp_lookup_symbol_nonlocal (const char *name,
			   const struct block *block,
			   const domain_enum domain)
{
  struct symbol *sym;
  const char *scope = block_scope (block);

  sym = lookup_namespace_scope (name, block,
				domain, scope, 0);
  if (sym != NULL)
    return sym;

  return cp_lookup_symbol_namespace (scope, name,
				     block, domain);
}

/* Look up NAME in the C++ namespace NAMESPACE.  Other arguments are
   as in cp_lookup_symbol_nonlocal.  */

static struct symbol *
cp_lookup_symbol_in_namespace (const char *namespace,
                               const char *name,
                               const struct block *block,
                               const domain_enum domain)
{
  if (namespace[0] == '\0')
    {
      return lookup_symbol_file (name, block, domain, 0);
    }
  else
    {
      char *concatenated_name = alloca (strlen (namespace) + 2
					+ strlen (name) + 1);

      strcpy (concatenated_name, namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, name);
      return lookup_symbol_file (concatenated_name, block, domain,
				 cp_is_anonymous (namespace));
    }
}

/* Used for cleanups to reset the "searched" flag incase
   of an error.  */

static void
reset_directive_searched (void *data)
{
  struct using_direct *direct = data;
  direct->searched = 0;
}

/* Search for NAME by applying all import statements belonging to
   BLOCK which are applicable in SCOPE.  If DECLARATION_ONLY the
   search is restricted to using declarations.
   Example:

     namespace A {
       int x;
     }
     using A::x;

   If SEARCH_PARENTS the search will include imports which are
   applicable in parents of SCOPE.
   Example:

     namespace A {
       using namespace X;
       namespace B {
         using namespace Y;
       }
     }

   If SCOPE is "A::B" and SEARCH_PARENTS is true the imports of
   namespaces X and Y will be considered.  If SEARCH_PARENTS is false
   only the import of Y is considered.  */

struct symbol *
cp_lookup_symbol_imports (const char *scope,
                          const char *name,
                          const struct block *block,
                          const domain_enum domain,
                          const int declaration_only,
                          const int search_parents)
{
  struct using_direct *current;
  struct symbol *sym = NULL;
  int len;
  int directive_match;
  struct cleanup *searched_cleanup;

  /* First, try to find the symbol in the given namespace.  */
  if (!declaration_only)
    sym = cp_lookup_symbol_in_namespace (scope, name,
					 block, domain);
  
  if (sym != NULL)
    return sym;

  /* Go through the using directives.  If any of them add new names to
     the namespace we're searching in, see if we can find a match by
     applying them.  */

  for (current = block_using (block);
       current != NULL;
       current = current->next)
    {
      len = strlen (current->import_dest);
      directive_match = (search_parents
                         ? (strncmp (scope, current->import_dest,
                                     strlen (current->import_dest)) == 0
                            && (len == 0
                                || scope[len] == ':'
				|| scope[len] == '\0'))
                         : strcmp (scope, current->import_dest) == 0);

      /* If the import destination is the current scope or one of its
         ancestors then it is applicable.  */
      if (directive_match && !current->searched)
	{
	/* Mark this import as searched so that the recursive call
           does not search it again.  */
	current->searched = 1;
	searched_cleanup = make_cleanup (reset_directive_searched,
					 current);

	/* If there is an import of a single declaration, compare the
	   imported declaration (after optional renaming by its alias)
	   with the sought out name.  If there is a match pass
	   current->import_src as NAMESPACE to direct the search
	   towards the imported namespace.  */
	if (current->declaration
	    && strcmp (name, current->alias
		       ? current->alias : current->declaration) == 0)
	  sym = cp_lookup_symbol_in_namespace (current->import_src,
					       current->declaration,
					       block, domain);

	/* If this is a DECLARATION_ONLY search or a symbol was found
	   or this import statement was an import declaration, the
	   search of this import is complete.  */
        if (declaration_only || sym != NULL || current->declaration)
          {
            current->searched = 0;
            discard_cleanups (searched_cleanup);

            if (sym != NULL)
              return sym;

            continue;
          }

	if (current->alias != NULL
	    && strcmp (name, current->alias) == 0)
	  /* If the import is creating an alias and the alias matches
	     the sought name.  Pass current->import_src as the NAME to
	     direct the search towards the aliased namespace.  */
	  {
	    sym = cp_lookup_symbol_in_namespace (scope,
						 current->import_src,
						 block, domain);
	  }
	else if (current->alias == NULL)
	  {
	    /* If this import statement creates no alias, pass
               current->inner as NAMESPACE to direct the search
               towards the imported namespace.  */
	    sym = cp_lookup_symbol_imports (current->import_src,
					    name, block,
					    domain, 0, 0);
	  }
	current->searched = 0;
	discard_cleanups (searched_cleanup);

	if (sym != NULL)
	  return sym;
	}
    }

  return NULL;
}

/* Helper function that searches an array of symbols for one named
   NAME.  */

static struct symbol *
search_symbol_list (const char *name, int num,
		    struct symbol **syms)
{
  int i;

  /* Maybe we should store a dictionary in here instead.  */
  for (i = 0; i < num; ++i)
    {
      if (strcmp (name, SYMBOL_NATURAL_NAME (syms[i])) == 0)
	return syms[i];
    }
  return NULL;
}

/* Like cp_lookup_symbol_imports, but if BLOCK is a function, it
   searches through the template parameters of the function and the
   function's type.  */

struct symbol *
cp_lookup_symbol_imports_or_template (const char *scope,
				      const char *name,
				      const struct block *block,
				      const domain_enum domain)
{
  struct symbol *function = BLOCK_FUNCTION (block);

  if (function != NULL && SYMBOL_LANGUAGE (function) == language_cplus)
    {
      int i;
      struct cplus_specific *cps
	= function->ginfo.language_specific.cplus_specific;

      /* Search the function's template parameters.  */
      if (SYMBOL_IS_CPLUS_TEMPLATE_FUNCTION (function))
	{
	  struct template_symbol *templ 
	    = (struct template_symbol *) function;
	  struct symbol *result;

	  result = search_symbol_list (name,
				       templ->n_template_arguments,
				       templ->template_arguments);
	  if (result != NULL)
	    return result;
	}

      /* Search the template parameters of the function's defining
	 context.  */
      if (SYMBOL_NATURAL_NAME (function))
	{
	  struct type *context;
	  char *name_copy = xstrdup (SYMBOL_NATURAL_NAME (function));
	  struct cleanup *cleanups = make_cleanup (xfree, name_copy);
	  const struct language_defn *lang = language_def (language_cplus);
	  struct gdbarch *arch = SYMBOL_SYMTAB (function)->objfile->gdbarch;
	  const struct block *parent = BLOCK_SUPERBLOCK (block);

	  while (1)
	    {
	      struct symbol *result;
	      unsigned int prefix_len = cp_entire_prefix_len (name_copy);

	      if (prefix_len == 0)
		context = NULL;
	      else
		{
		  name_copy[prefix_len] = '\0';
		  context = lookup_typename (lang, arch,
					     name_copy,
					     parent, 1);
		}

	      if (context == NULL)
		break;

	      result
		= search_symbol_list (name,
				      TYPE_N_TEMPLATE_ARGUMENTS (context),
				      TYPE_TEMPLATE_ARGUMENTS (context));
	      if (result != NULL)
		return result;
	    }

	  do_cleanups (cleanups);
	}
    }

  return cp_lookup_symbol_imports (scope, name, block, domain, 1, 1);
}

 /* Searches for NAME in the current namespace, and by applying
    relevant import statements belonging to BLOCK and its parents.
    SCOPE is the namespace scope of the context in which the search is
    being evaluated.  */

struct symbol*
cp_lookup_symbol_namespace (const char *scope,
                            const char *name,
                            const struct block *block,
                            const domain_enum domain)
{
  struct symbol *sym;
  
  /* First, try to find the symbol in the given namespace.  */
  sym = cp_lookup_symbol_in_namespace (scope, name,
				       block, domain);
  if (sym != NULL)
    return sym;

  /* Search for name in namespaces imported to this and parent
     blocks.  */
  while (block != NULL)
    {
      sym = cp_lookup_symbol_imports (scope, name, block,
				      domain, 0, 1);

      if (sym)
	return sym;

      block = BLOCK_SUPERBLOCK (block);
    }

  return NULL;
}

/* Lookup NAME at namespace scope (or, in C terms, in static and
   global variables).  SCOPE is the namespace that the current
   function is defined within; only consider namespaces whose length
   is at least SCOPE_LEN.  Other arguments are as in
   cp_lookup_symbol_nonlocal.

   For example, if we're within a function A::B::f and looking for a
   symbol x, this will get called with NAME = "x", SCOPE = "A::B", and
   SCOPE_LEN = 0.  It then calls itself with NAME and SCOPE the same,
   but with SCOPE_LEN = 1.  And then it calls itself with NAME and
   SCOPE the same, but with SCOPE_LEN = 4.  This third call looks for
   "A::B::x"; if it doesn't find it, then the second call looks for
   "A::x", and if that call fails, then the first call looks for
   "x".  */

static struct symbol *
lookup_namespace_scope (const char *name,
			const struct block *block,
			const domain_enum domain,
			const char *scope,
			int scope_len)
{
  char *namespace;

  if (scope[scope_len] != '\0')
    {
      /* Recursively search for names in child namespaces first.  */

      struct symbol *sym;
      int new_scope_len = scope_len;

      /* If the current scope is followed by "::", skip past that.  */
      if (new_scope_len != 0)
	{
	  gdb_assert (scope[new_scope_len] == ':');
	  new_scope_len += 2;
	}
      new_scope_len += cp_find_first_component (scope + new_scope_len);
      sym = lookup_namespace_scope (name, block, domain,
				    scope, new_scope_len);
      if (sym != NULL)
	return sym;
    }

  /* Okay, we didn't find a match in our children, so look for the
     name in the current namespace.  */

  namespace = alloca (scope_len + 1);
  strncpy (namespace, scope, scope_len);
  namespace[scope_len] = '\0';
  return cp_lookup_symbol_in_namespace (namespace, name,
					block, domain);
}

/* Look up NAME in BLOCK's static block and in global blocks.  If
   ANONYMOUS_NAMESPACE is nonzero, the symbol in question is located
   within an anonymous namespace.  Other arguments are as in
   cp_lookup_symbol_nonlocal.  */

static struct symbol *
lookup_symbol_file (const char *name,
		    const struct block *block,
		    const domain_enum domain,
		    int anonymous_namespace)
{
  struct symbol *sym = NULL;

  sym = lookup_symbol_static (name, block, domain);
  if (sym != NULL)
    return sym;

  if (anonymous_namespace)
    {
      /* Symbols defined in anonymous namespaces have external linkage
	 but should be treated as local to a single file nonetheless.
	 So we only search the current file's global block.  */

      const struct block *global_block = block_global_block (block);
      
      if (global_block != NULL)
	sym = lookup_symbol_aux_block (name, global_block, domain);
    }
  else
    {
      sym = lookup_symbol_global (name, block, domain);
    }

  if (sym != NULL)
    return sym;

  /* Now call "lookup_possible_namespace_symbol".  Symbols in here
     claim to be associated to namespaces, but this claim might be
     incorrect: the names in question might actually correspond to
     classes instead of namespaces.  But if they correspond to
     classes, then we should have found a match for them above.  So if
     we find them now, they should be genuine.  */

  /* FIXME: carlton/2003-06-12: This is a hack and should eventually
     be deleted: see comments below.  */

  if (domain == VAR_DOMAIN)
    {
      sym = lookup_possible_namespace_symbol (name);
      if (sym != NULL)
	return sym;
    }

  return NULL;
}

/* Look up a type named NESTED_NAME that is nested inside the C++
   class or namespace given by PARENT_TYPE, from within the context
   given by BLOCK.  Return NULL if there is no such nested type.  */

struct type *
cp_lookup_nested_type (struct type *parent_type,
		       const char *nested_name,
		       const struct block *block)
{
  switch (TYPE_CODE (parent_type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_NAMESPACE:
    case TYPE_CODE_UNION:
      {
	/* NOTE: carlton/2003-11-10: We don't treat C++ class members
	   of classes like, say, data or function members.  Instead,
	   they're just represented by symbols whose names are
	   qualified by the name of the surrounding class.  This is
	   just like members of namespaces; in particular,
	   lookup_symbol_namespace works when looking them up.  */

	const char *parent_name = TYPE_TAG_NAME (parent_type);
	struct symbol *sym
	  = cp_lookup_symbol_in_namespace (parent_name, nested_name,
					   block, VAR_DOMAIN);
	char *concatenated_name;

	if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
	  return SYMBOL_TYPE (sym);

	/* Now search all static file-level symbols.  Not strictly
	   correct, but more useful than an error.  We do not try to
	   guess any imported namespace as even the fully specified
	   namespace seach is is already not C++ compliant and more
	   assumptions could make it too magic.  */

	concatenated_name = alloca (strlen (parent_name) + 2
				    + strlen (nested_name) + 1);
	sprintf (concatenated_name, "%s::%s",
		 parent_name, nested_name);
	sym = lookup_static_symbol_aux (concatenated_name,
					VAR_DOMAIN);
	if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
	  return SYMBOL_TYPE (sym);

	return NULL;
      }
    default:
      internal_error (__FILE__, __LINE__,
		      _("cp_lookup_nested_type called "
			"on a non-aggregate type."));
    }
}

/* The C++-version of lookup_transparent_type.  */

/* FIXME: carlton/2004-01-16: The problem that this is trying to
   address is that, unfortunately, sometimes NAME is wrong: it may not
   include the name of namespaces enclosing the type in question.
   lookup_transparent_type gets called when the type in question
   is a declaration, and we're trying to find its definition; but, for
   declarations, our type name deduction mechanism doesn't work.
   There's nothing we can do to fix this in general, I think, in the
   absence of debug information about namespaces (I've filed PR
   gdb/1511 about this); until such debug information becomes more
   prevalent, one heuristic which sometimes looks is to search for the
   definition in namespaces containing the current namespace.

   We should delete this functions once the appropriate debug
   information becomes more widespread.  (GCC 3.4 will be the first
   released version of GCC with such information.)  */

struct type *
cp_lookup_transparent_type (const char *name)
{
  /* First, try the honest way of looking up the definition.  */
  struct type *t = basic_lookup_transparent_type (name);
  const char *scope;

  if (t != NULL)
    return t;

  /* If that doesn't work and we're within a namespace, look there
     instead.  */
  scope = block_scope (get_selected_block (0));

  if (scope[0] == '\0')
    return NULL;

  return cp_lookup_transparent_type_loop (name, scope, 0);
}

/* Lookup the type definition associated to NAME in namespaces/classes
   containing SCOPE whose name is strictly longer than LENGTH.  LENGTH
   must be the index of the start of a component of SCOPE.  */

static struct type *
cp_lookup_transparent_type_loop (const char *name,
				 const char *scope,
				 int length)
{
  int scope_length = length + cp_find_first_component (scope + length);
  char *full_name;

  /* If the current scope is followed by "::", look in the next
     component.  */
  if (scope[scope_length] == ':')
    {
      struct type *retval
	= cp_lookup_transparent_type_loop (name, scope,
					   scope_length + 2);

      if (retval != NULL)
	return retval;
    }

  full_name = alloca (scope_length + 2 + strlen (name) + 1);
  strncpy (full_name, scope, scope_length);
  strncpy (full_name + scope_length, "::", 2);
  strcpy (full_name + scope_length + 2, name);

  return basic_lookup_transparent_type (full_name);
}

/* Now come functions for dealing with symbols associated to
   namespaces.  (They're used to store the namespaces themselves, not
   objects that live in the namespaces.)  These symbols come in two
   varieties: if we run into a DW_TAG_namespace DIE, then we know that
   we have a namespace, so dwarf2read.c creates a symbol for it just
   like normal.  But, unfortunately, versions of GCC through at least
   3.3 don't generate those DIE's.  Our solution is to try to guess
   their existence by looking at demangled names.  This might cause us
   to misidentify classes as namespaces, however.  So we put those
   symbols in a special block (one per objfile), and we only search
   that block as a last resort.  */

/* FIXME: carlton/2003-06-12: Once versions of GCC that generate
   DW_TAG_namespace have been out for a year or two, we should get rid
   of all of this "possible namespace" nonsense.  */

/* Allocate everything necessary for the possible namespace block
   associated to OBJFILE.  */

static void
initialize_namespace_symtab (struct objfile *objfile)
{
  struct symtab *namespace_symtab;
  struct blockvector *bv;
  struct block *bl;

  namespace_symtab = allocate_symtab ("<<C++-namespaces>>", objfile);
  namespace_symtab->language = language_cplus;
  namespace_symtab->free_code = free_nothing;
  namespace_symtab->dirname = NULL;

  bv = obstack_alloc (&objfile->objfile_obstack,
		      sizeof (struct blockvector)
		      + FIRST_LOCAL_BLOCK * sizeof (struct block *));
  BLOCKVECTOR_NBLOCKS (bv) = FIRST_LOCAL_BLOCK + 1;
  BLOCKVECTOR (namespace_symtab) = bv;
  
  /* Allocate empty GLOBAL_BLOCK and STATIC_BLOCK.  */

  bl = allocate_block (&objfile->objfile_obstack);
  BLOCK_DICT (bl) = dict_create_linear (&objfile->objfile_obstack,
					NULL);
  BLOCKVECTOR_BLOCK (bv, GLOBAL_BLOCK) = bl;
  bl = allocate_block (&objfile->objfile_obstack);
  BLOCK_DICT (bl) = dict_create_linear (&objfile->objfile_obstack,
					NULL);
  BLOCKVECTOR_BLOCK (bv, STATIC_BLOCK) = bl;

  /* Allocate the possible namespace block; we put it where the first
     local block will live, though I don't think there's any need to
     pretend that it's actually a local block (e.g. by setting
     BLOCK_SUPERBLOCK appropriately).  We don't use the global or
     static block because we don't want it searched during the normal
     search of all global/static blocks in lookup_symbol: we only want
     it used as a last resort.  */

  /* NOTE: carlton/2003-09-11: I considered not associating the fake
     symbols to a block/symtab at all.  But that would cause problems
     with lookup_symbol's SYMTAB argument and with block_found, so
     having a symtab/block for this purpose seems like the best
     solution for now.  */

  bl = allocate_block (&objfile->objfile_obstack);
  BLOCK_DICT (bl) = dict_create_hashed_expandable ();
  BLOCKVECTOR_BLOCK (bv, FIRST_LOCAL_BLOCK) = bl;

  namespace_symtab->free_func = free_namespace_block;

  objfile->cp_namespace_symtab = namespace_symtab;
}

/* Locate the possible namespace block associated to OBJFILE,
   allocating it if necessary.  */

static struct block *
get_possible_namespace_block (struct objfile *objfile)
{
  if (objfile->cp_namespace_symtab == NULL)
    initialize_namespace_symtab (objfile);

  return BLOCKVECTOR_BLOCK (BLOCKVECTOR (objfile->cp_namespace_symtab),
			    FIRST_LOCAL_BLOCK);
}

/* Free the dictionary associated to the possible namespace block.  */

static void
free_namespace_block (struct symtab *symtab)
{
  struct block *possible_namespace_block;

  possible_namespace_block = BLOCKVECTOR_BLOCK (BLOCKVECTOR (symtab),
						FIRST_LOCAL_BLOCK);
  gdb_assert (possible_namespace_block != NULL);
  dict_free (BLOCK_DICT (possible_namespace_block));
}

/* Ensure that there are symbols in the possible namespace block
   associated to OBJFILE for all initial substrings of NAME that look
   like namespaces or classes.  NAME should end in a member variable:
   it shouldn't consist solely of namespaces.  */

void
cp_check_possible_namespace_symbols (const char *name,
				     struct objfile *objfile)
{
  check_possible_namespace_symbols_loop (name,
					 cp_find_first_component (name),
					 objfile);
}

/* This is a helper loop for cp_check_possible_namespace_symbols; it
   ensures that there are symbols in the possible namespace block
   associated to OBJFILE for all namespaces that are initial
   substrings of NAME of length at least LEN.  It returns 1 if a
   previous loop had already created the shortest such symbol and 0
   otherwise.

   This function assumes that if there is already a symbol associated
   to a substring of NAME of a given length, then there are already
   symbols associated to all substrings of NAME whose length is less
   than that length.  So if cp_check_possible_namespace_symbols has
   been called once with argument "A::B::C::member", then that will
   create symbols "A", "A::B", and "A::B::C".  If it is then later
   called with argument "A::B::D::member", then the new call will
   generate a new symbol for "A::B::D", but once it sees that "A::B"
   has already been created, it doesn't bother checking to see if "A"
   has also been created.  */

static int
check_possible_namespace_symbols_loop (const char *name, int len,
				       struct objfile *objfile)
{
  if (name[len] == ':')
    {
      int done;
      int next_len = len + 2;

      next_len += cp_find_first_component (name + next_len);
      done = check_possible_namespace_symbols_loop (name, next_len,
						    objfile);

      if (!done)
	done = check_one_possible_namespace_symbol (name, len,
						    objfile);

      return done;
    }
  else
    return 0;
}

/* Check to see if there's already a possible namespace symbol in
   OBJFILE whose name is the initial substring of NAME of length LEN.
   If not, create one and return 0; otherwise, return 1.  */

static int
check_one_possible_namespace_symbol (const char *name, int len,
				     struct objfile *objfile)
{
  struct block *block = get_possible_namespace_block (objfile);
  char *name_copy = alloca (len + 1);
  struct symbol *sym;

  memcpy (name_copy, name, len);
  name_copy[len] = '\0';
  sym = lookup_block_symbol (block, name_copy, VAR_DOMAIN);

  if (sym == NULL)
    {
      struct type *type;

      type = init_type (TYPE_CODE_NAMESPACE, 0, 0,
			name_copy, objfile);

      TYPE_TAG_NAME (type) = TYPE_NAME (type);

      sym = obstack_alloc (&objfile->objfile_obstack,
			   sizeof (struct symbol));
      memset (sym, 0, sizeof (struct symbol));
      SYMBOL_SET_LANGUAGE (sym, language_cplus);
      /* Note that init_type copied the name to the objfile's
	 obstack.  */
      SYMBOL_SET_NAMES (sym, TYPE_NAME (type), len, 0, objfile);
      SYMBOL_CLASS (sym) = LOC_TYPEDEF;
      SYMBOL_TYPE (sym) = type;
      SYMBOL_DOMAIN (sym) = VAR_DOMAIN;

      dict_add_symbol (BLOCK_DICT (block), sym);

      return 0;
    }
  else
    return 1;
}

/* Look for a symbol named NAME in all the possible namespace blocks.
   If one is found, return it.  */

static struct symbol *
lookup_possible_namespace_symbol (const char *name)
{
  struct objfile *objfile;

  ALL_OBJFILES (objfile)
    {
      struct symbol *sym;

      sym = lookup_block_symbol (get_possible_namespace_block (objfile),
				 name, VAR_DOMAIN);

      if (sym != NULL)
	return sym;
    }

  return NULL;
}

/* Print out all the possible namespace symbols.  */

static void
maintenance_cplus_namespace (char *args, int from_tty)
{
  struct objfile *objfile;

  printf_unfiltered (_("Possible namespaces:\n"));
  ALL_OBJFILES (objfile)
    {
      struct dict_iterator iter;
      struct symbol *sym;

      ALL_BLOCK_SYMBOLS (get_possible_namespace_block (objfile),
			 iter, sym)
	{
	  printf_unfiltered ("%s\n", SYMBOL_PRINT_NAME (sym));
	}
    }
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_cp_namespace;

void
_initialize_cp_namespace (void)
{
  add_cmd ("namespace", class_maintenance,
	   maintenance_cplus_namespace,
	   _("Print the list of possible C++ namespaces."),
	   &maint_cplus_cmd_list);
}
