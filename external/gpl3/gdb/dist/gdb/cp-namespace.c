/* Helper routines for C++ support in GDB.
   Copyright (C) 2003-2014 Free Software Foundation, Inc.

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
					  int anonymous_namespace,
					  int search);

static struct type *cp_lookup_transparent_type_loop (const char *name,
						     const char *scope,
						     int scope_len);

/* Check to see if SYMBOL refers to an object contained within an
   anonymous namespace; if so, add an appropriate using directive.  */

void
cp_scan_for_anonymous_namespaces (const struct symbol *const symbol,
				  struct objfile *const objfile)
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
	      cp_add_using_directive (dest, src, NULL, NULL, NULL, 1,
	                              &objfile->objfile_obstack);
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
   it is NULL.  EXCLUDES is a list of names not to import from an
   imported module or NULL.  If COPY_NAMES is non-zero, then the
   arguments are copied into newly allocated memory so they can be
   temporaries.  For EXCLUDES the VEC pointers are copied but the
   pointed to characters are not copied.  */

void
cp_add_using_directive (const char *dest,
			const char *src,
			const char *alias,
			const char *declaration,
			VEC (const_char_ptr) *excludes,
			int copy_names,
                        struct obstack *obstack)
{
  struct using_direct *current;
  struct using_direct *new;
  
  /* Has it already been added?  */

  for (current = using_directives; current != NULL; current = current->next)
    {
      int ix;
      const char *param;

      if (strcmp (current->import_src, src) != 0)
	continue;
      if (strcmp (current->import_dest, dest) != 0)
	continue;
      if ((alias == NULL && current->alias != NULL)
	  || (alias != NULL && current->alias == NULL)
	  || (alias != NULL && current->alias != NULL
	      && strcmp (alias, current->alias) != 0))
	continue;
      if ((declaration == NULL && current->declaration != NULL)
	  || (declaration != NULL && current->declaration == NULL)
	  || (declaration != NULL && current->declaration != NULL
	      && strcmp (declaration, current->declaration) != 0))
	continue;

      /* Compare the contents of EXCLUDES.  */
      for (ix = 0; VEC_iterate (const_char_ptr, excludes, ix, param); ix++)
	if (current->excludes[ix] == NULL
	    || strcmp (param, current->excludes[ix]) != 0)
	  break;
      if (ix < VEC_length (const_char_ptr, excludes)
	  || current->excludes[ix] != NULL)
	continue;

      /* Parameters exactly match CURRENT.  */
      return;
    }

  new = obstack_alloc (obstack, (sizeof (*new)
				 + (VEC_length (const_char_ptr, excludes)
				    * sizeof (*new->excludes))));
  memset (new, 0, sizeof (*new));

  if (copy_names)
    {
      new->import_src = obstack_copy0 (obstack, src, strlen (src));
      new->import_dest = obstack_copy0 (obstack, dest, strlen (dest));
    }
  else
    {
      new->import_src = src;
      new->import_dest = dest;
    }

  if (alias != NULL && copy_names)
    new->alias = obstack_copy0 (obstack, alias, strlen (alias));
  else
    new->alias = alias;

  if (declaration != NULL && copy_names)
    new->declaration = obstack_copy0 (obstack,
				      declaration, strlen (declaration));
  else
    new->declaration = declaration;

  memcpy (new->excludes, VEC_address (const_char_ptr, excludes),
	  VEC_length (const_char_ptr, excludes) * sizeof (*new->excludes));
  new->excludes[VEC_length (const_char_ptr, excludes)] = NULL;

  new->next = using_directives;
  using_directives = new;
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
   as in cp_lookup_symbol_nonlocal.  If SEARCH is non-zero, search
   through base classes for a matching symbol.  */

static struct symbol *
cp_lookup_symbol_in_namespace (const char *namespace,
                               const char *name,
                               const struct block *block,
                               const domain_enum domain, int search)
{
  if (namespace[0] == '\0')
    {
      return lookup_symbol_file (name, block, domain, 0, search);
    }
  else
    {
      char *concatenated_name = alloca (strlen (namespace) + 2
					+ strlen (name) + 1);

      strcpy (concatenated_name, namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, name);
      return lookup_symbol_file (concatenated_name, block, domain,
				 cp_is_anonymous (namespace), search);
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
					 block, domain, 1);
  
  if (sym != NULL)
    return sym;

  /* Go through the using directives.  If any of them add new names to
     the namespace we're searching in, see if we can find a match by
     applying them.  */

  for (current = block_using (block);
       current != NULL;
       current = current->next)
    {
      const char **excludep;

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
						 block, domain, 1);

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

	  /* Do not follow CURRENT if NAME matches its EXCLUDES.  */
	  for (excludep = current->excludes; *excludep; excludep++)
	    if (strcmp (name, *excludep) == 0)
	      break;
	  if (*excludep)
	    {
	      discard_cleanups (searched_cleanup);
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
						   block, domain, 1);
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
	  struct gdbarch *arch
	    = get_objfile_arch (SYMBOL_SYMTAB (function)->objfile);
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
		{
		  do_cleanups (cleanups);
		  return result;
		}
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
				       block, domain, 1);
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
					block, domain, 1);
}

/* Look up NAME in BLOCK's static block and in global blocks.  If
   ANONYMOUS_NAMESPACE is nonzero, the symbol in question is located
   within an anonymous namespace.  If SEARCH is non-zero, search through
   base classes for a matching symbol.  Other arguments are as in
   cp_lookup_symbol_nonlocal.  */

static struct symbol *
lookup_symbol_file (const char *name,
		    const struct block *block,
		    const domain_enum domain,
		    int anonymous_namespace, int search)
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

  if (search)
    {
      char *klass, *nested;
      unsigned int prefix_len;
      struct cleanup *cleanup;
      struct symbol *klass_sym;

      /* A simple lookup failed.  Check if the symbol was defined in
	 a base class.  */

      cleanup = make_cleanup (null_cleanup, NULL);

      /* Find the name of the class and the name of the method,
	 variable, etc.  */
      prefix_len = cp_entire_prefix_len (name);

      /* If no prefix was found, search "this".  */
      if (prefix_len == 0)
	{
	  struct type *type;
	  struct symbol *this;

	  this = lookup_language_this (language_def (language_cplus), block);
	  if (this == NULL)
	    {
	      do_cleanups (cleanup);
	      return NULL;
	    }

	  type = check_typedef (TYPE_TARGET_TYPE (SYMBOL_TYPE (this)));
	  klass = xstrdup (TYPE_NAME (type));
	  nested = xstrdup (name);
	}
      else
	{
	  /* The class name is everything up to and including PREFIX_LEN.  */
	  klass = savestring (name, prefix_len);

	  /* The rest of the name is everything else past the initial scope
	     operator.  */
	  nested = xstrdup (name + prefix_len + 2);
	}

      /* Add cleanups to free memory for these strings.  */
      make_cleanup (xfree, klass);
      make_cleanup (xfree, nested);

      /* Lookup a class named KLASS.  If none is found, there is nothing
	 more that can be done.  */
      klass_sym = lookup_symbol_global (klass, block, domain);
      if (klass_sym == NULL)
	{
	  do_cleanups (cleanup);
	  return NULL;
	}

      /* Look for a symbol named NESTED in this class.  */
      sym = cp_lookup_nested_symbol (SYMBOL_TYPE (klass_sym), nested, block);
      do_cleanups (cleanup);
    }

  return sym;
}

/* Search through the base classes of PARENT_TYPE for a base class
   named NAME and return its type.  If not found, return NULL.  */

struct type *
find_type_baseclass_by_name (struct type *parent_type, const char *name)
{
  int i;

  CHECK_TYPEDEF (parent_type);
  for (i = 0; i < TYPE_N_BASECLASSES (parent_type); ++i)
    {
      struct type *type = check_typedef (TYPE_BASECLASS (parent_type, i));
      const char *base_name = TYPE_BASECLASS_NAME (parent_type, i);

      if (base_name == NULL)
	continue;

      if (streq (base_name, name))
	return type;

      type = find_type_baseclass_by_name (type, name);
      if (type != NULL)
	return type;
    }

  return NULL;
}

/* Search through the base classes of PARENT_TYPE for a symbol named
   NAME in block BLOCK.  */

static struct symbol *
find_symbol_in_baseclass (struct type *parent_type, const char *name,
			   const struct block *block)
{
  int i;
  struct symbol *sym;
  struct cleanup *cleanup;
  char *concatenated_name;

  sym = NULL;
  concatenated_name = NULL;
  cleanup = make_cleanup (free_current_contents, &concatenated_name);
  for (i = 0; i < TYPE_N_BASECLASSES (parent_type); ++i)
    {
      size_t len;
      struct type *base_type = TYPE_BASECLASS (parent_type, i);
      const char *base_name = TYPE_BASECLASS_NAME (parent_type, i);

      if (base_name == NULL)
	continue;

      /* Search this particular base class.  */
      sym = cp_lookup_symbol_in_namespace (base_name, name, block,
					   VAR_DOMAIN, 0);
      if (sym != NULL)
	break;

      /* Now search all static file-level symbols.  We have to do this for
	 things like typedefs in the class.  First search in this symtab,
	 what we want is possibly there.  */
      len = strlen (base_name) + 2 + strlen (name) + 1;
      concatenated_name = xrealloc (concatenated_name, len);
      xsnprintf (concatenated_name, len, "%s::%s", base_name, name);
      sym = lookup_symbol_static (concatenated_name, block, VAR_DOMAIN);
      if (sym != NULL)
	break;

      /* Nope.  We now have to search all static blocks in all objfiles,
	 even if block != NULL, because there's no guarantees as to which
	 symtab the symbol we want is in.  */
      sym = lookup_static_symbol_aux (concatenated_name, VAR_DOMAIN);
      if (sym != NULL)
	break;

      /* If this class has base classes, search them next.  */
      CHECK_TYPEDEF (base_type);
      if (TYPE_N_BASECLASSES (base_type) > 0)
	{
	  sym = find_symbol_in_baseclass (base_type, name, block);
	  if (sym != NULL)
	    break;
	}
    }

  do_cleanups (cleanup);
  return sym;
}

/* Look up a symbol named NESTED_NAME that is nested inside the C++
   class or namespace given by PARENT_TYPE, from within the context
   given by BLOCK.  Return NULL if there is no such nested type.  */

struct symbol *
cp_lookup_nested_symbol (struct type *parent_type,
			 const char *nested_name,
			 const struct block *block)
{
  /* type_name_no_tag_required provides better error reporting using the
     original type.  */
  struct type *saved_parent_type = parent_type;

  CHECK_TYPEDEF (parent_type);

  switch (TYPE_CODE (parent_type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_NAMESPACE:
    case TYPE_CODE_UNION:
    /* NOTE: Handle modules here as well, because Fortran is re-using the C++
       specific code to lookup nested symbols in modules, by calling the
       function pointer la_lookup_symbol_nonlocal, which ends up here.  */
    case TYPE_CODE_MODULE:
      {
	/* NOTE: carlton/2003-11-10: We don't treat C++ class members
	   of classes like, say, data or function members.  Instead,
	   they're just represented by symbols whose names are
	   qualified by the name of the surrounding class.  This is
	   just like members of namespaces; in particular,
	   lookup_symbol_namespace works when looking them up.  */

	int size;
	const char *parent_name = type_name_no_tag_or_error (saved_parent_type);
	struct symbol *sym
	  = cp_lookup_symbol_in_namespace (parent_name, nested_name,
					   block, VAR_DOMAIN, 0);
	char *concatenated_name;

	if (sym != NULL)
	  return sym;

	/* Now search all static file-level symbols.  We have to do this
	   for things like typedefs in the class.  We do not try to
	   guess any imported namespace as even the fully specified
	   namespace search is already not C++ compliant and more
	   assumptions could make it too magic.  */

	size = strlen (parent_name) + 2 + strlen (nested_name) + 1;
	concatenated_name = alloca (size);
	xsnprintf (concatenated_name, size, "%s::%s",
		 parent_name, nested_name);
	sym = lookup_static_symbol_aux (concatenated_name, VAR_DOMAIN);
	if (sym != NULL)
	  return sym;

	/* If no matching symbols were found, try searching any
	   base classes.  */
	return find_symbol_in_baseclass (parent_type, nested_name, block);
      }

    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return NULL;

    default:
      internal_error (__FILE__, __LINE__,
		      _("cp_lookup_nested_symbol called "
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

/* This used to do something but was removed when it became
   obsolete.  */

static void
maintenance_cplus_namespace (char *args, int from_tty)
{
  printf_unfiltered (_("The `maint namespace' command was removed.\n"));
}

/* Provide a prototype to silence -Wmissing-prototypes.  */
extern initialize_file_ftype _initialize_cp_namespace;

void
_initialize_cp_namespace (void)
{
  struct cmd_list_element *cmd;

  cmd = add_cmd ("namespace", class_maintenance,
		 maintenance_cplus_namespace,
		 _("Deprecated placeholder for removed functionality."),
		 &maint_cplus_cmd_list);
  deprecate_cmd (cmd, NULL);
}
