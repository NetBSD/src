/* Helper routines for C++ support in GDB.
   Copyright (C) 2002-2013 Free Software Foundation, Inc.

   Contributed by MontaVista Software.

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
#include "gdb_string.h"
#include "demangle.h"
#include "gdb_assert.h"
#include "gdbcmd.h"
#include "dictionary.h"
#include "objfiles.h"
#include "frame.h"
#include "symtab.h"
#include "block.h"
#include "complaints.h"
#include "gdbtypes.h"
#include "exceptions.h"
#include "expression.h"
#include "value.h"
#include "cp-abi.h"

#include "safe-ctype.h"

#include "psymtab.h"

#define d_left(dc) (dc)->u.s_binary.left
#define d_right(dc) (dc)->u.s_binary.right

/* Functions related to demangled name parsing.  */

static unsigned int cp_find_first_component_aux (const char *name,
						 int permissive);

static void demangled_name_complaint (const char *name);

/* Functions/variables related to overload resolution.  */

static int sym_return_val_size = -1;
static int sym_return_val_index;
static struct symbol **sym_return_val;

static void overload_list_add_symbol (struct symbol *sym,
				      const char *oload_name);

static void make_symbol_overload_list_using (const char *func_name,
					     const char *namespace);

static void make_symbol_overload_list_qualified (const char *func_name);

/* The list of "maint cplus" commands.  */

struct cmd_list_element *maint_cplus_cmd_list = NULL;

/* The actual commands.  */

static void maint_cplus_command (char *arg, int from_tty);
static void first_component_command (char *arg, int from_tty);

/* A list of typedefs which should not be substituted by replace_typedefs.  */
static const char * const ignore_typedefs[] =
  {
    "std::istream", "std::iostream", "std::ostream", "std::string"
  };

static void
  replace_typedefs (struct demangle_parse_info *info,
		    struct demangle_component *ret_comp,
		    canonicalization_ftype *finder,
		    void *data);

/* A convenience function to copy STRING into OBSTACK, returning a pointer
   to the newly allocated string and saving the number of bytes saved in LEN.

   It does not copy the terminating '\0' byte!  */

static char *
copy_string_to_obstack (struct obstack *obstack, const char *string,
			long *len)
{
  *len = strlen (string);
  return obstack_copy (obstack, string, *len);
}

/* A cleanup wrapper for cp_demangled_name_parse_free.  */

static void
do_demangled_name_parse_free_cleanup (void *data)
{
  struct demangle_parse_info *info = (struct demangle_parse_info *) data;

  cp_demangled_name_parse_free (info);
}

/* Create a cleanup for C++ name parsing.  */

struct cleanup *
make_cleanup_cp_demangled_name_parse_free (struct demangle_parse_info *info)
{
  return make_cleanup (do_demangled_name_parse_free_cleanup, info);
}

/* Return 1 if STRING is clearly already in canonical form.  This
   function is conservative; things which it does not recognize are
   assumed to be non-canonical, and the parser will sort them out
   afterwards.  This speeds up the critical path for alphanumeric
   identifiers.  */

static int
cp_already_canonical (const char *string)
{
  /* Identifier start character [a-zA-Z_].  */
  if (!ISIDST (string[0]))
    return 0;

  /* These are the only two identifiers which canonicalize to other
     than themselves or an error: unsigned -> unsigned int and
     signed -> int.  */
  if (string[0] == 'u' && strcmp (&string[1], "nsigned") == 0)
    return 0;
  else if (string[0] == 's' && strcmp (&string[1], "igned") == 0)
    return 0;

  /* Identifier character [a-zA-Z0-9_].  */
  while (ISIDNUM (string[1]))
    string++;

  if (string[1] == '\0')
    return 1;
  else
    return 0;
}

/* Inspect the given RET_COMP for its type.  If it is a typedef,
   replace the node with the typedef's tree.

   Returns 1 if any typedef substitutions were made, 0 otherwise.  */

static int
inspect_type (struct demangle_parse_info *info,
	      struct demangle_component *ret_comp,
	      canonicalization_ftype *finder,
	      void *data)
{
  int i;
  char *name;
  struct symbol *sym;
  volatile struct gdb_exception except;

  /* Copy the symbol's name from RET_COMP and look it up
     in the symbol table.  */
  name = (char *) alloca (ret_comp->u.s_name.len + 1);
  memcpy (name, ret_comp->u.s_name.s, ret_comp->u.s_name.len);
  name[ret_comp->u.s_name.len] = '\0';

  /* Ignore any typedefs that should not be substituted.  */
  for (i = 0; i < ARRAY_SIZE (ignore_typedefs); ++i)
    {
      if (strcmp (name, ignore_typedefs[i]) == 0)
	return 0;
    }

  sym = NULL;
  TRY_CATCH (except, RETURN_MASK_ALL)
  {
    sym = lookup_symbol (name, 0, VAR_DOMAIN, 0);
  }

  if (except.reason >= 0 && sym != NULL)
    {
      struct type *otype = SYMBOL_TYPE (sym);

      if (finder != NULL)
	{
	  const char *new_name = (*finder) (otype, data);

	  if (new_name != NULL)
	    {
	      ret_comp->u.s_name.s = new_name;
	      ret_comp->u.s_name.len = strlen (new_name);
	      return 1;
	    }

	  return 0;
	}

      /* If the type is a typedef, replace it.  */
      if (TYPE_CODE (otype) == TYPE_CODE_TYPEDEF)
	{
	  long len;
	  int is_anon;
	  struct type *type;
	  struct demangle_parse_info *i;
	  struct ui_file *buf;

	  /* Get the real type of the typedef.  */
	  type = check_typedef (otype);

	  is_anon = (TYPE_TAG_NAME (type) == NULL
		     && (TYPE_CODE (type) == TYPE_CODE_ENUM
			 || TYPE_CODE (type) == TYPE_CODE_STRUCT
			 || TYPE_CODE (type) == TYPE_CODE_UNION));
	  if (is_anon)
	    {
	      struct type *last = otype;

	      /* Find the last typedef for the type.  */
	      while (TYPE_TARGET_TYPE (last) != NULL
		     && (TYPE_CODE (TYPE_TARGET_TYPE (last))
			 == TYPE_CODE_TYPEDEF))
		last = TYPE_TARGET_TYPE (last);

	      /* If there is only one typedef for this anonymous type,
		 do not substitute it.  */
	      if (type == otype)
		return 0;
	      else
		/* Use the last typedef seen as the type for this
		   anonymous type.  */
		type = last;
	    }

	  buf = mem_fileopen ();
	  TRY_CATCH (except, RETURN_MASK_ERROR)
	  {
	    type_print (type, "", buf, -1);
	  }

	  /* If type_print threw an exception, there is little point
	     in continuing, so just bow out gracefully.  */
	  if (except.reason < 0)
	    {
	      ui_file_delete (buf);
	      return 0;
	    }

	  name = ui_file_obsavestring (buf, &info->obstack, &len);
	  ui_file_delete (buf);

	  /* Turn the result into a new tree.  Note that this
	     tree will contain pointers into NAME, so NAME cannot
	     be free'd until all typedef conversion is done and
	     the final result is converted into a string.  */
	  i = cp_demangled_name_to_comp (name, NULL);
	  if (i != NULL)
	    {
	      /* Merge the two trees.  */
	      cp_merge_demangle_parse_infos (info, ret_comp, i);

	      /* Replace any newly introduced typedefs -- but not
		 if the type is anonymous (that would lead to infinite
		 looping).  */
	      if (!is_anon)
		replace_typedefs (info, ret_comp, finder, data);
	    }
	  else
	    {
	      /* This shouldn't happen unless the type printer has
		 output something that the name parser cannot grok.
		 Nonetheless, an ounce of prevention...

		 Canonicalize the name again, and store it in the
		 current node (RET_COMP).  */
	      char *canon = cp_canonicalize_string_no_typedefs (name);

	      if (canon != NULL)
		{
		  /* Copy the canonicalization into the obstack and
		     free CANON.  */
		  name = copy_string_to_obstack (&info->obstack, canon, &len);
		  xfree (canon);
		}

	      ret_comp->u.s_name.s = name;
	      ret_comp->u.s_name.len = len;
	    }

	  return 1;
	}
    }

  return 0;
}

/* Replace any typedefs appearing in the qualified name
   (DEMANGLE_COMPONENT_QUAL_NAME) represented in RET_COMP for the name parse
   given in INFO.  */

static void
replace_typedefs_qualified_name (struct demangle_parse_info *info,
				 struct demangle_component *ret_comp,
				 canonicalization_ftype *finder,
				 void *data)
{
  long len;
  char *name;
  struct ui_file *buf = mem_fileopen ();
  struct demangle_component *comp = ret_comp;

  /* Walk each node of the qualified name, reconstructing the name of
     this element.  With every node, check for any typedef substitutions.
     If a substitution has occurred, replace the qualified name node
     with a DEMANGLE_COMPONENT_NAME node representing the new, typedef-
     substituted name.  */
  while (comp->type == DEMANGLE_COMPONENT_QUAL_NAME)
    {
      if (d_left (comp)->type == DEMANGLE_COMPONENT_NAME)
	{
	  struct demangle_component new;

	  ui_file_write (buf, d_left (comp)->u.s_name.s,
			 d_left (comp)->u.s_name.len);
	  name = ui_file_obsavestring (buf, &info->obstack, &len);
	  new.type = DEMANGLE_COMPONENT_NAME;
	  new.u.s_name.s = name;
	  new.u.s_name.len = len;
	  if (inspect_type (info, &new, finder, data))
	    {
	      char *n, *s;
	      long slen;

	      /* A typedef was substituted in NEW.  Convert it to a
		 string and replace the top DEMANGLE_COMPONENT_QUAL_NAME
		 node.  */

	      ui_file_rewind (buf);
	      n = cp_comp_to_string (&new, 100);
	      if (n == NULL)
		{
		  /* If something went astray, abort typedef substitutions.  */
		  ui_file_delete (buf);
		  return;
		}

	      s = copy_string_to_obstack (&info->obstack, n, &slen);
	      xfree (n);

	      d_left (ret_comp)->type = DEMANGLE_COMPONENT_NAME;
	      d_left (ret_comp)->u.s_name.s = s;
	      d_left (ret_comp)->u.s_name.len = slen;
	      d_right (ret_comp) = d_right (comp);
	      comp = ret_comp;
	      continue;
	    }
	}
      else
	{
	  /* The current node is not a name, so simply replace any
	     typedefs in it.  Then print it to the stream to continue
	     checking for more typedefs in the tree.  */
	  replace_typedefs (info, d_left (comp), finder, data);
	  name = cp_comp_to_string (d_left (comp), 100);
	  if (name == NULL)
	    {
	      /* If something went astray, abort typedef substitutions.  */
	      ui_file_delete (buf);
	      return;
	    }
	  fputs_unfiltered (name, buf);
	  xfree (name);
	}

      ui_file_write (buf, "::", 2);
      comp = d_right (comp);
    }

  /* If the next component is DEMANGLE_COMPONENT_NAME, save the qualified
     name assembled above and append the name given by COMP.  Then use this
     reassembled name to check for a typedef.  */

  if (comp->type == DEMANGLE_COMPONENT_NAME)
    {
      ui_file_write (buf, comp->u.s_name.s, comp->u.s_name.len);
      name = ui_file_obsavestring (buf, &info->obstack, &len);

      /* Replace the top (DEMANGLE_COMPONENT_QUAL_NAME) node
	 with a DEMANGLE_COMPONENT_NAME node containing the whole
	 name.  */
      ret_comp->type = DEMANGLE_COMPONENT_NAME;
      ret_comp->u.s_name.s = name;
      ret_comp->u.s_name.len = len;
      inspect_type (info, ret_comp, finder, data);
    }
  else
    replace_typedefs (info, comp, finder, data);

  ui_file_delete (buf);
}


/* A function to check const and volatile qualifiers for argument types.

   "Parameter declarations that differ only in the presence
   or absence of `const' and/or `volatile' are equivalent."
   C++ Standard N3290, clause 13.1.3 #4.  */

static void
check_cv_qualifiers (struct demangle_component *ret_comp)
{
  while (d_left (ret_comp) != NULL
	 && (d_left (ret_comp)->type == DEMANGLE_COMPONENT_CONST
	     || d_left (ret_comp)->type == DEMANGLE_COMPONENT_VOLATILE))
    {
      d_left (ret_comp) = d_left (d_left (ret_comp));
    }
}

/* Walk the parse tree given by RET_COMP, replacing any typedefs with
   their basic types.  */

static void
replace_typedefs (struct demangle_parse_info *info,
		  struct demangle_component *ret_comp,
		  canonicalization_ftype *finder,
		  void *data)
{
  if (ret_comp)
    {
      if (finder != NULL
	  && (ret_comp->type == DEMANGLE_COMPONENT_NAME
	      || ret_comp->type == DEMANGLE_COMPONENT_QUAL_NAME
	      || ret_comp->type == DEMANGLE_COMPONENT_TEMPLATE
	      || ret_comp->type == DEMANGLE_COMPONENT_BUILTIN_TYPE))
	{
	  char *local_name = cp_comp_to_string (ret_comp, 10);

	  if (local_name != NULL)
	    {
	      struct symbol *sym;
	      volatile struct gdb_exception except;

	      sym = NULL;
	      TRY_CATCH (except, RETURN_MASK_ALL)
		{
		  sym = lookup_symbol (local_name, 0, VAR_DOMAIN, 0);
		}
	      xfree (local_name);

	      if (except.reason >= 0 && sym != NULL)
		{
		  struct type *otype = SYMBOL_TYPE (sym);
		  const char *new_name = (*finder) (otype, data);

		  if (new_name != NULL)
		    {
		      ret_comp->type = DEMANGLE_COMPONENT_NAME;
		      ret_comp->u.s_name.s = new_name;
		      ret_comp->u.s_name.len = strlen (new_name);
		      return;
		    }
		}
	    }
	}

      switch (ret_comp->type)
	{
	case DEMANGLE_COMPONENT_ARGLIST:
	  check_cv_qualifiers (ret_comp);
	  /* Fall through */

	case DEMANGLE_COMPONENT_FUNCTION_TYPE:
	case DEMANGLE_COMPONENT_TEMPLATE:
	case DEMANGLE_COMPONENT_TEMPLATE_ARGLIST:
	case DEMANGLE_COMPONENT_TYPED_NAME:
	  replace_typedefs (info, d_left (ret_comp), finder, data);
	  replace_typedefs (info, d_right (ret_comp), finder, data);
	  break;

	case DEMANGLE_COMPONENT_NAME:
	  inspect_type (info, ret_comp, finder, data);
	  break;

	case DEMANGLE_COMPONENT_QUAL_NAME:
	  replace_typedefs_qualified_name (info, ret_comp, finder, data);
	  break;

	case DEMANGLE_COMPONENT_LOCAL_NAME:
	case DEMANGLE_COMPONENT_CTOR:
	case DEMANGLE_COMPONENT_ARRAY_TYPE:
	case DEMANGLE_COMPONENT_PTRMEM_TYPE:
	  replace_typedefs (info, d_right (ret_comp), finder, data);
	  break;

	case DEMANGLE_COMPONENT_CONST:
	case DEMANGLE_COMPONENT_RESTRICT:
	case DEMANGLE_COMPONENT_VOLATILE:
	case DEMANGLE_COMPONENT_VOLATILE_THIS:
	case DEMANGLE_COMPONENT_CONST_THIS:
	case DEMANGLE_COMPONENT_RESTRICT_THIS:
	case DEMANGLE_COMPONENT_POINTER:
	case DEMANGLE_COMPONENT_REFERENCE:
	  replace_typedefs (info, d_left (ret_comp), finder, data);
	  break;

	default:
	  break;
	}
    }
}

/* Parse STRING and convert it to canonical form, resolving any typedefs.
   If parsing fails, or if STRING is already canonical, return NULL.
   Otherwise return the canonical form.  The return value is allocated via
   xmalloc.  If FINDER is not NULL, then type components are passed to
   FINDER to be looked up.  DATA is passed verbatim to FINDER.  */

char *
cp_canonicalize_string_full (const char *string,
			     canonicalization_ftype *finder,
			     void *data)
{
  char *ret;
  unsigned int estimated_len;
  struct demangle_parse_info *info;

  ret = NULL;
  estimated_len = strlen (string) * 2;
  info = cp_demangled_name_to_comp (string, NULL);
  if (info != NULL)
    {
      /* Replace all the typedefs in the tree.  */
      replace_typedefs (info, info->tree, finder, data);

      /* Convert the tree back into a string.  */
      ret = cp_comp_to_string (info->tree, estimated_len);
      gdb_assert (ret != NULL);

      /* Free the parse information.  */
      cp_demangled_name_parse_free (info);

      /* Finally, compare the original string with the computed
	 name, returning NULL if they are the same.  */
      if (strcmp (string, ret) == 0)
	{
	  xfree (ret);
	  return NULL;
	}
    }

  return ret;
}

/* Like cp_canonicalize_string_full, but always passes NULL for
   FINDER.  */

char *
cp_canonicalize_string_no_typedefs (const char *string)
{
  return cp_canonicalize_string_full (string, NULL, NULL);
}

/* Parse STRING and convert it to canonical form.  If parsing fails,
   or if STRING is already canonical, return NULL.  Otherwise return
   the canonical form.  The return value is allocated via xmalloc.  */

char *
cp_canonicalize_string (const char *string)
{
  struct demangle_parse_info *info;
  unsigned int estimated_len;
  char *ret;

  if (cp_already_canonical (string))
    return NULL;

  info = cp_demangled_name_to_comp (string, NULL);
  if (info == NULL)
    return NULL;

  estimated_len = strlen (string) * 2;
  ret = cp_comp_to_string (info->tree, estimated_len);
  cp_demangled_name_parse_free (info);

  if (ret == NULL)
    {
      warning (_("internal error: string \"%s\" failed to be canonicalized"),
	       string);
      return NULL;
    }

  if (strcmp (string, ret) == 0)
    {
      xfree (ret);
      return NULL;
    }

  return ret;
}

/* Convert a mangled name to a demangle_component tree.  *MEMORY is
   set to the block of used memory that should be freed when finished
   with the tree.  DEMANGLED_P is set to the char * that should be
   freed when finished with the tree, or NULL if none was needed.
   OPTIONS will be passed to the demangler.  */

static struct demangle_parse_info *
mangled_name_to_comp (const char *mangled_name, int options,
		      void **memory, char **demangled_p)
{
  char *demangled_name;
  struct demangle_parse_info *info;

  /* If it looks like a v3 mangled name, then try to go directly
     to trees.  */
  if (mangled_name[0] == '_' && mangled_name[1] == 'Z')
    {
      struct demangle_component *ret;

      ret = cplus_demangle_v3_components (mangled_name,
					  options, memory);
      if (ret)
	{
	  info = cp_new_demangle_parse_info ();
	  info->tree = ret;
	  *demangled_p = NULL;
	  return info;
	}
    }

  /* If it doesn't, or if that failed, then try to demangle the
     name.  */
  demangled_name = cplus_demangle (mangled_name, options);
  if (demangled_name == NULL)
   return NULL;
  
  /* If we could demangle the name, parse it to build the component
     tree.  */
  info = cp_demangled_name_to_comp (demangled_name, NULL);

  if (info == NULL)
    {
      xfree (demangled_name);
      return NULL;
    }

  *demangled_p = demangled_name;
  return info;
}

/* Return the name of the class containing method PHYSNAME.  */

char *
cp_class_name_from_physname (const char *physname)
{
  void *storage = NULL;
  char *demangled_name = NULL, *ret;
  struct demangle_component *ret_comp, *prev_comp, *cur_comp;
  struct demangle_parse_info *info;
  int done;

  info = mangled_name_to_comp (physname, DMGL_ANSI,
			       &storage, &demangled_name);
  if (info == NULL)
    return NULL;

  done = 0;
  ret_comp = info->tree;

  /* First strip off any qualifiers, if we have a function or
     method.  */
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
        ret_comp = d_left (ret_comp);
        break;
      default:
	done = 1;
	break;
      }

  /* If what we have now is a function, discard the argument list.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TYPED_NAME)
    ret_comp = d_left (ret_comp);

  /* If what we have now is a template, strip off the template
     arguments.  The left subtree may be a qualified name.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TEMPLATE)
    ret_comp = d_left (ret_comp);

  /* What we have now should be a name, possibly qualified.
     Additional qualifiers could live in the left subtree or the right
     subtree.  Find the last piece.  */
  done = 0;
  prev_comp = NULL;
  cur_comp = ret_comp;
  while (!done)
    switch (cur_comp->type)
      {
      case DEMANGLE_COMPONENT_QUAL_NAME:
      case DEMANGLE_COMPONENT_LOCAL_NAME:
	prev_comp = cur_comp;
        cur_comp = d_right (cur_comp);
        break;
      case DEMANGLE_COMPONENT_TEMPLATE:
      case DEMANGLE_COMPONENT_NAME:
      case DEMANGLE_COMPONENT_CTOR:
      case DEMANGLE_COMPONENT_DTOR:
      case DEMANGLE_COMPONENT_OPERATOR:
      case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
	done = 1;
	break;
      default:
	done = 1;
	cur_comp = NULL;
	break;
      }

  ret = NULL;
  if (cur_comp != NULL && prev_comp != NULL)
    {
      /* We want to discard the rightmost child of PREV_COMP.  */
      *prev_comp = *d_left (prev_comp);
      /* The ten is completely arbitrary; we don't have a good
	 estimate.  */
      ret = cp_comp_to_string (ret_comp, 10);
    }

  xfree (storage);
  xfree (demangled_name);
  cp_demangled_name_parse_free (info);
  return ret;
}

/* Return the child of COMP which is the basename of a method,
   variable, et cetera.  All scope qualifiers are discarded, but
   template arguments will be included.  The component tree may be
   modified.  */

static struct demangle_component *
unqualified_name_from_comp (struct demangle_component *comp)
{
  struct demangle_component *ret_comp = comp, *last_template;
  int done;

  done = 0;
  last_template = NULL;
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_QUAL_NAME:
      case DEMANGLE_COMPONENT_LOCAL_NAME:
        ret_comp = d_right (ret_comp);
        break;
      case DEMANGLE_COMPONENT_TYPED_NAME:
        ret_comp = d_left (ret_comp);
        break;
      case DEMANGLE_COMPONENT_TEMPLATE:
	gdb_assert (last_template == NULL);
	last_template = ret_comp;
	ret_comp = d_left (ret_comp);
	break;
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
        ret_comp = d_left (ret_comp);
        break;
      case DEMANGLE_COMPONENT_NAME:
      case DEMANGLE_COMPONENT_CTOR:
      case DEMANGLE_COMPONENT_DTOR:
      case DEMANGLE_COMPONENT_OPERATOR:
      case DEMANGLE_COMPONENT_EXTENDED_OPERATOR:
	done = 1;
	break;
      default:
	return NULL;
	break;
      }

  if (last_template)
    {
      d_left (last_template) = ret_comp;
      return last_template;
    }

  return ret_comp;
}

/* Return the name of the method whose linkage name is PHYSNAME.  */

char *
method_name_from_physname (const char *physname)
{
  void *storage = NULL;
  char *demangled_name = NULL, *ret;
  struct demangle_component *ret_comp;
  struct demangle_parse_info *info;

  info = mangled_name_to_comp (physname, DMGL_ANSI,
			       &storage, &demangled_name);
  if (info == NULL)
    return NULL;

  ret_comp = unqualified_name_from_comp (info->tree);

  ret = NULL;
  if (ret_comp != NULL)
    /* The ten is completely arbitrary; we don't have a good
       estimate.  */
    ret = cp_comp_to_string (ret_comp, 10);

  xfree (storage);
  xfree (demangled_name);
  cp_demangled_name_parse_free (info);
  return ret;
}

/* If FULL_NAME is the demangled name of a C++ function (including an
   arg list, possibly including namespace/class qualifications),
   return a new string containing only the function name (without the
   arg list/class qualifications).  Otherwise, return NULL.  The
   caller is responsible for freeing the memory in question.  */

char *
cp_func_name (const char *full_name)
{
  char *ret;
  struct demangle_component *ret_comp;
  struct demangle_parse_info *info;

  info = cp_demangled_name_to_comp (full_name, NULL);
  if (!info)
    return NULL;

  ret_comp = unqualified_name_from_comp (info->tree);

  ret = NULL;
  if (ret_comp != NULL)
    ret = cp_comp_to_string (ret_comp, 10);

  cp_demangled_name_parse_free (info);
  return ret;
}

/* DEMANGLED_NAME is the name of a function, including parameters and
   (optionally) a return type.  Return the name of the function without
   parameters or return type, or NULL if we can not parse the name.  */

char *
cp_remove_params (const char *demangled_name)
{
  int done = 0;
  struct demangle_component *ret_comp;
  struct demangle_parse_info *info;
  char *ret = NULL;

  if (demangled_name == NULL)
    return NULL;

  info = cp_demangled_name_to_comp (demangled_name, NULL);
  if (info == NULL)
    return NULL;

  /* First strip off any qualifiers, if we have a function or method.  */
  ret_comp = info->tree;
  while (!done)
    switch (ret_comp->type)
      {
      case DEMANGLE_COMPONENT_CONST:
      case DEMANGLE_COMPONENT_RESTRICT:
      case DEMANGLE_COMPONENT_VOLATILE:
      case DEMANGLE_COMPONENT_CONST_THIS:
      case DEMANGLE_COMPONENT_RESTRICT_THIS:
      case DEMANGLE_COMPONENT_VOLATILE_THIS:
      case DEMANGLE_COMPONENT_VENDOR_TYPE_QUAL:
        ret_comp = d_left (ret_comp);
        break;
      default:
	done = 1;
	break;
      }

  /* What we have now should be a function.  Return its name.  */
  if (ret_comp->type == DEMANGLE_COMPONENT_TYPED_NAME)
    ret = cp_comp_to_string (d_left (ret_comp), 10);

  cp_demangled_name_parse_free (info);
  return ret;
}

/* Here are some random pieces of trivia to keep in mind while trying
   to take apart demangled names:

   - Names can contain function arguments or templates, so the process
     has to be, to some extent recursive: maybe keep track of your
     depth based on encountering <> and ().

   - Parentheses don't just have to happen at the end of a name: they
     can occur even if the name in question isn't a function, because
     a template argument might be a type that's a function.

   - Conversely, even if you're trying to deal with a function, its
     demangled name might not end with ')': it could be a const or
     volatile class method, in which case it ends with "const" or
     "volatile".

   - Parentheses are also used in anonymous namespaces: a variable
     'foo' in an anonymous namespace gets demangled as "(anonymous
     namespace)::foo".

   - And operator names can contain parentheses or angle brackets.  */

/* FIXME: carlton/2003-03-13: We have several functions here with
   overlapping functionality; can we combine them?  Also, do they
   handle all the above considerations correctly?  */


/* This returns the length of first component of NAME, which should be
   the demangled name of a C++ variable/function/method/etc.
   Specifically, it returns the index of the first colon forming the
   boundary of the first component: so, given 'A::foo' or 'A::B::foo'
   it returns the 1, and given 'foo', it returns 0.  */

/* The character in NAME indexed by the return value is guaranteed to
   always be either ':' or '\0'.  */

/* NOTE: carlton/2003-03-13: This function is currently only intended
   for internal use: it's probably not entirely safe when called on
   user-generated input, because some of the 'index += 2' lines in
   cp_find_first_component_aux might go past the end of malformed
   input.  */

unsigned int
cp_find_first_component (const char *name)
{
  return cp_find_first_component_aux (name, 0);
}

/* Helper function for cp_find_first_component.  Like that function,
   it returns the length of the first component of NAME, but to make
   the recursion easier, it also stops if it reaches an unexpected ')'
   or '>' if the value of PERMISSIVE is nonzero.  */

/* Let's optimize away calls to strlen("operator").  */

#define LENGTH_OF_OPERATOR 8

static unsigned int
cp_find_first_component_aux (const char *name, int permissive)
{
  unsigned int index = 0;
  /* Operator names can show up in unexpected places.  Since these can
     contain parentheses or angle brackets, they can screw up the
     recursion.  But not every string 'operator' is part of an
     operater name: e.g. you could have a variable 'cooperator'.  So
     this variable tells us whether or not we should treat the string
     'operator' as starting an operator.  */
  int operator_possible = 1;

  for (;; ++index)
    {
      switch (name[index])
	{
	case '<':
	  /* Template; eat it up.  The calls to cp_first_component
	     should only return (I hope!) when they reach the '>'
	     terminating the component or a '::' between two
	     components.  (Hence the '+ 2'.)  */
	  index += 1;
	  for (index += cp_find_first_component_aux (name + index, 1);
	       name[index] != '>';
	       index += cp_find_first_component_aux (name + index, 1))
	    {
	      if (name[index] != ':')
		{
		  demangled_name_complaint (name);
		  return strlen (name);
		}
	      index += 2;
	    }
	  operator_possible = 1;
	  break;
	case '(':
	  /* Similar comment as to '<'.  */
	  index += 1;
	  for (index += cp_find_first_component_aux (name + index, 1);
	       name[index] != ')';
	       index += cp_find_first_component_aux (name + index, 1))
	    {
	      if (name[index] != ':')
		{
		  demangled_name_complaint (name);
		  return strlen (name);
		}
	      index += 2;
	    }
	  operator_possible = 1;
	  break;
	case '>':
	case ')':
	  if (permissive)
	    return index;
	  else
	    {
	      demangled_name_complaint (name);
	      return strlen (name);
	    }
	case '\0':
	case ':':
	  return index;
	case 'o':
	  /* Operator names can screw up the recursion.  */
	  if (operator_possible
	      && strncmp (name + index, "operator",
			  LENGTH_OF_OPERATOR) == 0)
	    {
	      index += LENGTH_OF_OPERATOR;
	      while (ISSPACE(name[index]))
		++index;
	      switch (name[index])
		{
		  /* Skip over one less than the appropriate number of
		     characters: the for loop will skip over the last
		     one.  */
		case '<':
		  if (name[index + 1] == '<')
		    index += 1;
		  else
		    index += 0;
		  break;
		case '>':
		case '-':
		  if (name[index + 1] == '>')
		    index += 1;
		  else
		    index += 0;
		  break;
		case '(':
		  index += 1;
		  break;
		default:
		  index += 0;
		  break;
		}
	    }
	  operator_possible = 0;
	  break;
	case ' ':
	case ',':
	case '.':
	case '&':
	case '*':
	  /* NOTE: carlton/2003-04-18: I'm not sure what the precise
	     set of relevant characters are here: it's necessary to
	     include any character that can show up before 'operator'
	     in a demangled name, and it's safe to include any
	     character that can't be part of an identifier's name.  */
	  operator_possible = 1;
	  break;
	default:
	  operator_possible = 0;
	  break;
	}
    }
}

/* Complain about a demangled name that we don't know how to parse.
   NAME is the demangled name in question.  */

static void
demangled_name_complaint (const char *name)
{
  complaint (&symfile_complaints,
	     "unexpected demangled name '%s'", name);
}

/* If NAME is the fully-qualified name of a C++
   function/variable/method/etc., this returns the length of its
   entire prefix: all of the namespaces and classes that make up its
   name.  Given 'A::foo', it returns 1, given 'A::B::foo', it returns
   4, given 'foo', it returns 0.  */

unsigned int
cp_entire_prefix_len (const char *name)
{
  unsigned int current_len = cp_find_first_component (name);
  unsigned int previous_len = 0;

  while (name[current_len] != '\0')
    {
      gdb_assert (name[current_len] == ':');
      previous_len = current_len;
      /* Skip the '::'.  */
      current_len += 2;
      current_len += cp_find_first_component (name + current_len);
    }

  return previous_len;
}

/* Overload resolution functions.  */

/* Test to see if SYM is a symbol that we haven't seen corresponding
   to a function named OLOAD_NAME.  If so, add it to the current
   completion list.  */

static void
overload_list_add_symbol (struct symbol *sym,
			  const char *oload_name)
{
  int newsize;
  int i;
  char *sym_name;

  /* If there is no type information, we can't do anything, so
     skip.  */
  if (SYMBOL_TYPE (sym) == NULL)
    return;

  /* skip any symbols that we've already considered.  */
  for (i = 0; i < sym_return_val_index; ++i)
    if (strcmp (SYMBOL_LINKAGE_NAME (sym),
		SYMBOL_LINKAGE_NAME (sym_return_val[i])) == 0)
      return;

  /* Get the demangled name without parameters */
  sym_name = cp_remove_params (SYMBOL_NATURAL_NAME (sym));
  if (!sym_name)
    return;

  /* skip symbols that cannot match */
  if (strcmp (sym_name, oload_name) != 0)
    {
      xfree (sym_name);
      return;
    }

  xfree (sym_name);

  /* We have a match for an overload instance, so add SYM to the
     current list of overload instances */
  if (sym_return_val_index + 3 > sym_return_val_size)
    {
      newsize = (sym_return_val_size *= 2) * sizeof (struct symbol *);
      sym_return_val = (struct symbol **)
	xrealloc ((char *) sym_return_val, newsize);
    }
  sym_return_val[sym_return_val_index++] = sym;
  sym_return_val[sym_return_val_index] = NULL;
}

/* Return a null-terminated list of pointers to function symbols that
   are named FUNC_NAME and are visible within NAMESPACE.  */

struct symbol **
make_symbol_overload_list (const char *func_name,
			   const char *namespace)
{
  struct cleanup *old_cleanups;
  const char *name;

  sym_return_val_size = 100;
  sym_return_val_index = 0;
  sym_return_val = xmalloc ((sym_return_val_size + 1) *
			    sizeof (struct symbol *));
  sym_return_val[0] = NULL;

  old_cleanups = make_cleanup (xfree, sym_return_val);

  make_symbol_overload_list_using (func_name, namespace);

  if (namespace[0] == '\0')
    name = func_name;
  else
    {
      char *concatenated_name
	= alloca (strlen (namespace) + 2 + strlen (func_name) + 1);
      strcpy (concatenated_name, namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, func_name);
      name = concatenated_name;
    }

  make_symbol_overload_list_qualified (name);

  discard_cleanups (old_cleanups);

  return sym_return_val;
}

/* Add all symbols with a name matching NAME in BLOCK to the overload
   list.  */

static void
make_symbol_overload_list_block (const char *name,
                                 const struct block *block)
{
  struct block_iterator iter;
  struct symbol *sym;

  for (sym = block_iter_name_first (block, name, &iter);
       sym != NULL;
       sym = block_iter_name_next (name, &iter))
    overload_list_add_symbol (sym, name);
}

/* Adds the function FUNC_NAME from NAMESPACE to the overload set.  */

static void
make_symbol_overload_list_namespace (const char *func_name,
                                     const char *namespace)
{
  const char *name;
  const struct block *block = NULL;

  if (namespace[0] == '\0')
    name = func_name;
  else
    {
      char *concatenated_name
	= alloca (strlen (namespace) + 2 + strlen (func_name) + 1);

      strcpy (concatenated_name, namespace);
      strcat (concatenated_name, "::");
      strcat (concatenated_name, func_name);
      name = concatenated_name;
    }

  /* Look in the static block.  */
  block = block_static_block (get_selected_block (0));
  if (block)
    make_symbol_overload_list_block (name, block);

  /* Look in the global block.  */
  block = block_global_block (block);
  if (block)
    make_symbol_overload_list_block (name, block);

}

/* Search the namespace of the given type and namespace of and public
   base types.  */

static void
make_symbol_overload_list_adl_namespace (struct type *type,
                                         const char *func_name)
{
  char *namespace;
  const char *type_name;
  int i, prefix_len;

  while (TYPE_CODE (type) == TYPE_CODE_PTR
	 || TYPE_CODE (type) == TYPE_CODE_REF
         || TYPE_CODE (type) == TYPE_CODE_ARRAY
         || TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    {
      if (TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
	type = check_typedef(type);
      else
	type = TYPE_TARGET_TYPE (type);
    }

  type_name = TYPE_NAME (type);

  if (type_name == NULL)
    return;

  prefix_len = cp_entire_prefix_len (type_name);

  if (prefix_len != 0)
    {
      namespace = alloca (prefix_len + 1);
      strncpy (namespace, type_name, prefix_len);
      namespace[prefix_len] = '\0';

      make_symbol_overload_list_namespace (func_name, namespace);
    }

  /* Check public base type */
  if (TYPE_CODE (type) == TYPE_CODE_CLASS)
    for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
      {
	if (BASETYPE_VIA_PUBLIC (type, i))
	  make_symbol_overload_list_adl_namespace (TYPE_BASECLASS (type,
								   i),
						   func_name);
      }
}

/* Adds the overload list overload candidates for FUNC_NAME found
   through argument dependent lookup.  */

struct symbol **
make_symbol_overload_list_adl (struct type **arg_types, int nargs,
                               const char *func_name)
{
  int i;

  gdb_assert (sym_return_val_size != -1);

  for (i = 1; i <= nargs; i++)
    make_symbol_overload_list_adl_namespace (arg_types[i - 1],
					     func_name);

  return sym_return_val;
}

/* Used for cleanups to reset the "searched" flag in case of an
   error.  */

static void
reset_directive_searched (void *data)
{
  struct using_direct *direct = data;
  direct->searched = 0;
}

/* This applies the using directives to add namespaces to search in,
   and then searches for overloads in all of those namespaces.  It
   adds the symbols found to sym_return_val.  Arguments are as in
   make_symbol_overload_list.  */

static void
make_symbol_overload_list_using (const char *func_name,
				 const char *namespace)
{
  struct using_direct *current;
  const struct block *block;

  /* First, go through the using directives.  If any of them apply,
     look in the appropriate namespaces for new functions to match
     on.  */

  for (block = get_selected_block (0);
       block != NULL;
       block = BLOCK_SUPERBLOCK (block))
    for (current = block_using (block);
	current != NULL;
	current = current->next)
      {
	/* Prevent recursive calls.  */
	if (current->searched)
	  continue;

        /* If this is a namespace alias or imported declaration ignore
	   it.  */
        if (current->alias != NULL || current->declaration != NULL)
          continue;

        if (strcmp (namespace, current->import_dest) == 0)
	  {
	    /* Mark this import as searched so that the recursive call
	       does not search it again.  */
	    struct cleanup *old_chain;
	    current->searched = 1;
	    old_chain = make_cleanup (reset_directive_searched,
				      current);

	    make_symbol_overload_list_using (func_name,
					     current->import_src);

	    current->searched = 0;
	    discard_cleanups (old_chain);
	  }
      }

  /* Now, add names for this namespace.  */
  make_symbol_overload_list_namespace (func_name, namespace);
}

/* This does the bulk of the work of finding overloaded symbols.
   FUNC_NAME is the name of the overloaded function we're looking for
   (possibly including namespace info).  */

static void
make_symbol_overload_list_qualified (const char *func_name)
{
  struct symtab *s;
  struct objfile *objfile;
  const struct block *b, *surrounding_static_block = 0;

  /* Look through the partial symtabs for all symbols which begin by
     matching FUNC_NAME.  Make sure we read that symbol table in.  */

  ALL_OBJFILES (objfile)
  {
    if (objfile->sf)
      objfile->sf->qf->expand_symtabs_for_function (objfile, func_name);
  }

  /* Search upwards from currently selected frame (so that we can
     complete on local vars.  */

  for (b = get_selected_block (0); b != NULL; b = BLOCK_SUPERBLOCK (b))
    make_symbol_overload_list_block (func_name, b);

  surrounding_static_block = block_static_block (get_selected_block (0));

  /* Go through the symtabs and check the externs and statics for
     symbols which match.  */

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), GLOBAL_BLOCK);
    make_symbol_overload_list_block (func_name, b);
  }

  ALL_PRIMARY_SYMTABS (objfile, s)
  {
    QUIT;
    b = BLOCKVECTOR_BLOCK (BLOCKVECTOR (s), STATIC_BLOCK);
    /* Don't do this block twice.  */
    if (b == surrounding_static_block)
      continue;
    make_symbol_overload_list_block (func_name, b);
  }
}

/* Lookup the rtti type for a class name.  */

struct type *
cp_lookup_rtti_type (const char *name, struct block *block)
{
  struct symbol * rtti_sym;
  struct type * rtti_type;

  rtti_sym = lookup_symbol (name, block, STRUCT_DOMAIN, NULL);

  if (rtti_sym == NULL)
    {
      warning (_("RTTI symbol not found for class '%s'"), name);
      return NULL;
    }

  if (SYMBOL_CLASS (rtti_sym) != LOC_TYPEDEF)
    {
      warning (_("RTTI symbol for class '%s' is not a type"), name);
      return NULL;
    }

  rtti_type = SYMBOL_TYPE (rtti_sym);

  switch (TYPE_CODE (rtti_type))
    {
    case TYPE_CODE_CLASS:
      break;
    case TYPE_CODE_NAMESPACE:
      /* chastain/2003-11-26: the symbol tables often contain fake
	 symbols for namespaces with the same name as the struct.
	 This warning is an indication of a bug in the lookup order
	 or a bug in the way that the symbol tables are populated.  */
      warning (_("RTTI symbol for class '%s' is a namespace"), name);
      return NULL;
    default:
      warning (_("RTTI symbol for class '%s' has bad type"), name);
      return NULL;
    }

  return rtti_type;
}

/* Don't allow just "maintenance cplus".  */

static  void
maint_cplus_command (char *arg, int from_tty)
{
  printf_unfiltered (_("\"maintenance cplus\" must be followed "
		       "by the name of a command.\n"));
  help_list (maint_cplus_cmd_list,
	     "maintenance cplus ",
	     -1, gdb_stdout);
}

/* This is a front end for cp_find_first_component, for unit testing.
   Be careful when using it: see the NOTE above
   cp_find_first_component.  */

static void
first_component_command (char *arg, int from_tty)
{
  int len;  
  char *prefix; 

  if (!arg)
    return;

  len = cp_find_first_component (arg);
  prefix = alloca (len + 1);

  memcpy (prefix, arg, len);
  prefix[len] = '\0';

  printf_unfiltered ("%s\n", prefix);
}

extern initialize_file_ftype _initialize_cp_support; /* -Wmissing-prototypes */


/* Implement "info vtbl".  */

static void
info_vtbl_command (char *arg, int from_tty)
{
  struct value *value;

  value = parse_and_eval (arg);
  cplus_print_vtable (value);
}

void
_initialize_cp_support (void)
{
  add_prefix_cmd ("cplus", class_maintenance,
		  maint_cplus_command,
		  _("C++ maintenance commands."),
		  &maint_cplus_cmd_list,
		  "maintenance cplus ",
		  0, &maintenancelist);
  add_alias_cmd ("cp", "cplus",
		 class_maintenance, 1,
		 &maintenancelist);

  add_cmd ("first_component",
	   class_maintenance,
	   first_component_command,
	   _("Print the first class/namespace component of NAME."),
	   &maint_cplus_cmd_list);

  add_info ("vtbl", info_vtbl_command,
	    _("Show the virtual function table for a C++ object.\n\
Usage: info vtbl EXPRESSION\n\
Evaluate EXPRESSION and display the virtual function table for the\n\
resulting object."));
}
