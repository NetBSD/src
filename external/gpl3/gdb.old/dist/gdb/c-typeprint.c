/* Support for printing C and C++ types for GDB, the GNU debugger.
   Copyright (C) 1986-2016 Free Software Foundation, Inc.

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
#include "gdb_obstack.h"
#include "bfd.h"		/* Binary File Description.  */
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "target.h"
#include "language.h"
#include "demangle.h"
#include "c-lang.h"
#include "typeprint.h"
#include "cp-abi.h"
#include "jv-lang.h"
#include "cp-support.h"

static void c_type_print_varspec_prefix (struct type *,
					 struct ui_file *,
					 int, int, int,
					 const struct type_print_options *);

/* Print "const", "volatile", or address space modifiers.  */
static void c_type_print_modifier (struct type *,
				   struct ui_file *,
				   int, int);


/* A callback function for cp_canonicalize_string_full that uses
   find_typedef_in_hash.  */

static const char *
find_typedef_for_canonicalize (struct type *t, void *data)
{
  return find_typedef_in_hash ((const struct type_print_options *) data, t);
}

/* Print NAME on STREAM.  If the 'raw' field of FLAGS is not set,
   canonicalize NAME using the local typedefs first.  */

static void
print_name_maybe_canonical (const char *name,
			    const struct type_print_options *flags,
			    struct ui_file *stream)
{
  char *s = NULL;

  if (!flags->raw)
    s = cp_canonicalize_string_full (name,
				     find_typedef_for_canonicalize,
				     (void *) flags);

  fputs_filtered (s ? s : name, stream);
  xfree (s);
}



/* LEVEL is the depth to indent lines by.  */

void
c_print_type (struct type *type,
	      const char *varstring,
	      struct ui_file *stream,
	      int show, int level,
	      const struct type_print_options *flags)
{
  enum type_code code;
  int demangled_args;
  int need_post_space;
  const char *local_name;

  if (show > 0)
    type = check_typedef (type);

  local_name = find_typedef_in_hash (flags, type);
  if (local_name != NULL)
    {
      fputs_filtered (local_name, stream);
      if (varstring != NULL && *varstring != '\0')
	fputs_filtered (" ", stream);
    }
  else
    {
      c_type_print_base (type, stream, show, level, flags);
      code = TYPE_CODE (type);
      if ((varstring != NULL && *varstring != '\0')
	  /* Need a space if going to print stars or brackets;
	     but not if we will print just a type name.  */
	  || ((show > 0 || TYPE_NAME (type) == 0)
	      && (code == TYPE_CODE_PTR || code == TYPE_CODE_FUNC
		  || code == TYPE_CODE_METHOD
		  || (code == TYPE_CODE_ARRAY
		      && !TYPE_VECTOR (type))
		  || code == TYPE_CODE_MEMBERPTR
		  || code == TYPE_CODE_METHODPTR
		  || code == TYPE_CODE_REF)))
	fputs_filtered (" ", stream);
      need_post_space = (varstring != NULL && strcmp (varstring, "") != 0);
      c_type_print_varspec_prefix (type, stream, show, 0, need_post_space,
				   flags);
    }

  if (varstring != NULL)
    {
      fputs_filtered (varstring, stream);

      /* For demangled function names, we have the arglist as part of
         the name, so don't print an additional pair of ()'s.  */
      if (local_name == NULL)
	{
	  demangled_args = strchr (varstring, '(') != NULL;
	  c_type_print_varspec_suffix (type, stream, show,
				       0, demangled_args,
				       flags);
	}
    }
}

/* Print a typedef using C syntax.  TYPE is the underlying type.
   NEW_SYMBOL is the symbol naming the type.  STREAM is the stream on
   which to print.  */

void
c_print_typedef (struct type *type,
		 struct symbol *new_symbol,
		 struct ui_file *stream)
{
  type = check_typedef (type);
  fprintf_filtered (stream, "typedef ");
  type_print (type, "", stream, 0);
  if (TYPE_NAME ((SYMBOL_TYPE (new_symbol))) == 0
      || strcmp (TYPE_NAME ((SYMBOL_TYPE (new_symbol))),
		 SYMBOL_LINKAGE_NAME (new_symbol)) != 0
      || TYPE_CODE (SYMBOL_TYPE (new_symbol)) == TYPE_CODE_TYPEDEF)
    fprintf_filtered (stream, " %s", SYMBOL_PRINT_NAME (new_symbol));
  fprintf_filtered (stream, ";\n");
}

/* If TYPE is a derived type, then print out derivation information.
   Print only the actual base classes of this type, not the base
   classes of the base classes.  I.e. for the derivation hierarchy:

   class A { int a; };
   class B : public A {int b; };
   class C : public B {int c; };

   Print the type of class C as:

   class C : public B {
   int c;
   }

   Not as the following (like gdb used to), which is not legal C++
   syntax for derived types and may be confused with the multiple
   inheritance form:

   class C : public B : public A {
   int c;
   }

   In general, gdb should try to print the types as closely as
   possible to the form that they appear in the source code.  */

static void
cp_type_print_derivation_info (struct ui_file *stream,
			       struct type *type,
			       const struct type_print_options *flags)
{
  const char *name;
  int i;

  for (i = 0; i < TYPE_N_BASECLASSES (type); i++)
    {
      wrap_here ("        ");
      fputs_filtered (i == 0 ? ": " : ", ", stream);
      fprintf_filtered (stream, "%s%s ",
			BASETYPE_VIA_PUBLIC (type, i)
			? "public" : (TYPE_FIELD_PROTECTED (type, i)
				      ? "protected" : "private"),
			BASETYPE_VIA_VIRTUAL (type, i) ? " virtual" : "");
      name = type_name_no_tag (TYPE_BASECLASS (type, i));
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	fprintf_filtered (stream, "(null)");
    }
  if (i > 0)
    {
      fputs_filtered (" ", stream);
    }
}

/* Print the C++ method arguments ARGS to the file STREAM.  */

static void
cp_type_print_method_args (struct type *mtype, const char *prefix,
			   const char *varstring, int staticp,
			   struct ui_file *stream,
			   const struct type_print_options *flags)
{
  struct field *args = TYPE_FIELDS (mtype);
  int nargs = TYPE_NFIELDS (mtype);
  int varargs = TYPE_VARARGS (mtype);
  int i;

  fprintf_symbol_filtered (stream, prefix,
			   language_cplus, DMGL_ANSI);
  fprintf_symbol_filtered (stream, varstring,
			   language_cplus, DMGL_ANSI);
  fputs_filtered ("(", stream);

  /* Skip the class variable.  */
  i = staticp ? 0 : 1;
  if (nargs > i)
    {
      while (i < nargs)
	{
	  c_print_type (args[i++].type, "", stream, 0, 0, flags);

	  if (i == nargs && varargs)
	    fprintf_filtered (stream, ", ...");
	  else if (i < nargs)
	    {
	      fprintf_filtered (stream, ", ");
	      wrap_here ("        ");
	    }
	}
    }
  else if (varargs)
    fprintf_filtered (stream, "...");
  else if (current_language->la_language == language_cplus)
    fprintf_filtered (stream, "void");

  fprintf_filtered (stream, ")");

  /* For non-static methods, read qualifiers from the type of
     THIS.  */
  if (!staticp)
    {
      struct type *domain;

      gdb_assert (nargs > 0);
      gdb_assert (TYPE_CODE (args[0].type) == TYPE_CODE_PTR);
      domain = TYPE_TARGET_TYPE (args[0].type);

      if (TYPE_CONST (domain))
	fprintf_filtered (stream, " const");

      if (TYPE_VOLATILE (domain))
	fprintf_filtered (stream, " volatile");

      if (TYPE_RESTRICT (domain))
	fprintf_filtered (stream, " restrict");

      if (TYPE_ATOMIC (domain))
	fprintf_filtered (stream, " _Atomic");
    }
}


/* Print any asterisks or open-parentheses needed before the
   variable name (to describe its type).

   On outermost call, pass 0 for PASSED_A_PTR.
   On outermost call, SHOW > 0 means should ignore
   any typename for TYPE and show its details.
   SHOW is always zero on recursive calls.
   
   NEED_POST_SPACE is non-zero when a space will be be needed
   between a trailing qualifier and a field, variable, or function
   name.  */

static void
c_type_print_varspec_prefix (struct type *type,
			     struct ui_file *stream,
			     int show, int passed_a_ptr,
			     int need_post_space,
			     const struct type_print_options *flags)
{
  const char *name;

  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_PTR:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 1, 1, flags);
      fprintf_filtered (stream, "*");
      c_type_print_modifier (type, stream, 1, need_post_space);
      break;

    case TYPE_CODE_MEMBERPTR:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 0, 0, flags);
      name = type_name_no_tag (TYPE_SELF_TYPE (type));
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	c_type_print_base (TYPE_SELF_TYPE (type),
			   stream, -1, passed_a_ptr, flags);
      fprintf_filtered (stream, "::*");
      break;

    case TYPE_CODE_METHODPTR:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 0, 0, flags);
      fprintf_filtered (stream, "(");
      name = type_name_no_tag (TYPE_SELF_TYPE (type));
      if (name)
	print_name_maybe_canonical (name, flags, stream);
      else
	c_type_print_base (TYPE_SELF_TYPE (type),
			   stream, -1, passed_a_ptr, flags);
      fprintf_filtered (stream, "::*");
      break;

    case TYPE_CODE_REF:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 1, 0, flags);
      fprintf_filtered (stream, "&");
      c_type_print_modifier (type, stream, 1, need_post_space);
      break;

    case TYPE_CODE_METHOD:
    case TYPE_CODE_FUNC:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 0, 0, flags);
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
      break;

    case TYPE_CODE_ARRAY:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, 0, 0, flags);
      if (passed_a_ptr)
	fprintf_filtered (stream, "(");
      break;

    case TYPE_CODE_TYPEDEF:
      c_type_print_varspec_prefix (TYPE_TARGET_TYPE (type),
				   stream, show, passed_a_ptr, 0, flags);
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_NAMESPACE:
    case TYPE_CODE_DECFLOAT:
      /* These types need no prefix.  They are listed here so that
         gcc -Wall will reveal any types that haven't been handled.  */
      break;
    default:
      error (_("type not handled in c_type_print_varspec_prefix()"));
      break;
    }
}

/* Print out "const" and "volatile" attributes,
   and address space id if present.
   TYPE is a pointer to the type being printed out.
   STREAM is the output destination.
   NEED_PRE_SPACE = 1 indicates an initial white space is needed.
   NEED_POST_SPACE = 1 indicates a final white space is needed.  */

static void
c_type_print_modifier (struct type *type, struct ui_file *stream,
		       int need_pre_space, int need_post_space)
{
  int did_print_modifier = 0;
  const char *address_space_id;

  /* We don't print `const' qualifiers for references --- since all
     operators affect the thing referenced, not the reference itself,
     every reference is `const'.  */
  if (TYPE_CONST (type)
      && TYPE_CODE (type) != TYPE_CODE_REF)
    {
      if (need_pre_space)
	fprintf_filtered (stream, " ");
      fprintf_filtered (stream, "const");
      did_print_modifier = 1;
    }

  if (TYPE_VOLATILE (type))
    {
      if (did_print_modifier || need_pre_space)
	fprintf_filtered (stream, " ");
      fprintf_filtered (stream, "volatile");
      did_print_modifier = 1;
    }

  if (TYPE_RESTRICT (type))
    {
      if (did_print_modifier || need_pre_space)
	fprintf_filtered (stream, " ");
      fprintf_filtered (stream, "restrict");
      did_print_modifier = 1;
    }

  if (TYPE_ATOMIC (type))
    {
      if (did_print_modifier || need_pre_space)
	fprintf_filtered (stream, " ");
      fprintf_filtered (stream, "_Atomic");
      did_print_modifier = 1;
    }

  address_space_id = address_space_int_to_name (get_type_arch (type),
						TYPE_INSTANCE_FLAGS (type));
  if (address_space_id)
    {
      if (did_print_modifier || need_pre_space)
	fprintf_filtered (stream, " ");
      fprintf_filtered (stream, "@%s", address_space_id);
      did_print_modifier = 1;
    }

  if (did_print_modifier && need_post_space)
    fprintf_filtered (stream, " ");
}


/* Print out the arguments of TYPE, which should have TYPE_CODE_METHOD
   or TYPE_CODE_FUNC, to STREAM.  Artificial arguments, such as "this"
   in non-static methods, are displayed if LINKAGE_NAME is zero.  If
   LINKAGE_NAME is non-zero and LANGUAGE is language_cplus the topmost
   parameter types get removed their possible const and volatile qualifiers to
   match demangled linkage name parameters part of such function type.
   LANGUAGE is the language in which TYPE was defined.  This is a necessary
   evil since this code is used by the C, C++, and Java backends.  */

void
c_type_print_args (struct type *type, struct ui_file *stream,
		   int linkage_name, enum language language,
		   const struct type_print_options *flags)
{
  int i;
  int printed_any = 0;

  fprintf_filtered (stream, "(");

  for (i = 0; i < TYPE_NFIELDS (type); i++)
    {
      struct type *param_type;

      if (TYPE_FIELD_ARTIFICIAL (type, i) && linkage_name)
	continue;

      if (printed_any)
	{
	  fprintf_filtered (stream, ", ");
	  wrap_here ("    ");
	}

      param_type = TYPE_FIELD_TYPE (type, i);

      if (language == language_cplus && linkage_name)
	{
	  /* C++ standard, 13.1 Overloadable declarations, point 3, item:
	     - Parameter declarations that differ only in the presence or
	       absence of const and/or volatile are equivalent.

	     And the const/volatile qualifiers are not present in the mangled
	     names as produced by GCC.  */

	  param_type = make_cv_type (0, 0, param_type, NULL);
	}

      if (language == language_java)
	java_print_type (param_type, "", stream, -1, 0, flags);
      else
	c_print_type (param_type, "", stream, -1, 0, flags);
      printed_any = 1;
    }

  if (printed_any && TYPE_VARARGS (type))
    {
      /* Print out a trailing ellipsis for varargs functions.  Ignore
	 TYPE_VARARGS if the function has no named arguments; that
	 represents unprototyped (K&R style) C functions.  */
      if (printed_any && TYPE_VARARGS (type))
	{
	  fprintf_filtered (stream, ", ");
	  wrap_here ("    ");
	  fprintf_filtered (stream, "...");
	}
    }
  else if (!printed_any
	   && ((TYPE_PROTOTYPED (type) && language != language_java)
	       || language == language_cplus))
    fprintf_filtered (stream, "void");

  fprintf_filtered (stream, ")");
}

/* Return true iff the j'th overloading of the i'th method of TYPE
   is a type conversion operator, like `operator int () { ... }'.
   When listing a class's methods, we don't print the return type of
   such operators.  */

static int
is_type_conversion_operator (struct type *type, int i, int j)
{
  /* I think the whole idea of recognizing type conversion operators
     by their name is pretty terrible.  But I don't think our present
     data structure gives us any other way to tell.  If you know of
     some other way, feel free to rewrite this function.  */
  const char *name = TYPE_FN_FIELDLIST_NAME (type, i);

  if (!startswith (name, "operator"))
    return 0;

  name += 8;
  if (! strchr (" \t\f\n\r", *name))
    return 0;

  while (strchr (" \t\f\n\r", *name))
    name++;

  if (!('a' <= *name && *name <= 'z')
      && !('A' <= *name && *name <= 'Z')
      && *name != '_')
    /* If this doesn't look like the start of an identifier, then it
       isn't a type conversion operator.  */
    return 0;
  else if (startswith (name, "new"))
    name += 3;
  else if (startswith (name, "delete"))
    name += 6;
  else
    /* If it doesn't look like new or delete, it's a type conversion
       operator.  */
    return 1;

  /* Is that really the end of the name?  */
  if (('a' <= *name && *name <= 'z')
      || ('A' <= *name && *name <= 'Z')
      || ('0' <= *name && *name <= '9')
      || *name == '_')
    /* No, so the identifier following "operator" must be a type name,
       and this is a type conversion operator.  */
    return 1;

  /* That was indeed the end of the name, so it was `operator new' or
     `operator delete', neither of which are type conversion
     operators.  */
  return 0;
}

/* Given a C++ qualified identifier QID, strip off the qualifiers,
   yielding the unqualified name.  The return value is a pointer into
   the original string.

   It's a pity we don't have this information in some more structured
   form.  Even the author of this function feels that writing little
   parsers like this everywhere is stupid.  */

static char *
remove_qualifiers (char *qid)
{
  int quoted = 0;	/* Zero if we're not in quotes;
			   '"' if we're in a double-quoted string;
			   '\'' if we're in a single-quoted string.  */
  int depth = 0;	/* Number of unclosed parens we've seen.  */
  char *parenstack = (char *) alloca (strlen (qid));
  char *scan;
  char *last = 0;	/* The character after the rightmost
			   `::' token we've seen so far.  */

  for (scan = qid; *scan; scan++)
    {
      if (quoted)
	{
	  if (*scan == quoted)
	    quoted = 0;
	  else if (*scan == '\\' && *(scan + 1))
	    scan++;
	}
      else if (scan[0] == ':' && scan[1] == ':')
	{
	  /* If we're inside parenthesis (i.e., an argument list) or
	     angle brackets (i.e., a list of template arguments), then
	     we don't record the position of this :: token, since it's
	     not relevant to the top-level structure we're trying to
	     operate on.  */
	  if (depth == 0)
	    {
	      last = scan + 2;
	      scan++;
	    }
	}
      else if (*scan == '"' || *scan == '\'')
	quoted = *scan;
      else if (*scan == '(')
	parenstack[depth++] = ')';
      else if (*scan == '[')
	parenstack[depth++] = ']';
      /* We're going to treat <> as a pair of matching characters,
	 since we're more likely to see those in template id's than
	 real less-than characters.  What a crock.  */
      else if (*scan == '<')
	parenstack[depth++] = '>';
      else if (*scan == ')' || *scan == ']' || *scan == '>')
	{
	  if (depth > 0 && parenstack[depth - 1] == *scan)
	    depth--;
	  else
	    {
	      /* We're going to do a little error recovery here.  If
		 we don't find a match for *scan on the paren stack,
		 but there is something lower on the stack that does
		 match, we pop the stack to that point.  */
	      int i;

	      for (i = depth - 1; i >= 0; i--)
		if (parenstack[i] == *scan)
		  {
		    depth = i;
		    break;
		  }
	    }
	}
    }

  if (last)
    return last;
  else
    /* We didn't find any :: tokens at the top level, so declare the
       whole thing an unqualified identifier.  */
    return qid;
}

/* Print any array sizes, function arguments or close parentheses
   needed after the variable name (to describe its type).
   Args work like c_type_print_varspec_prefix.  */

void
c_type_print_varspec_suffix (struct type *type,
			     struct ui_file *stream,
			     int show, int passed_a_ptr,
			     int demangled_args,
			     const struct type_print_options *flags)
{
  if (type == 0)
    return;

  if (TYPE_NAME (type) && show <= 0)
    return;

  QUIT;

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      {
	LONGEST low_bound, high_bound;
	int is_vector = TYPE_VECTOR (type);

	if (passed_a_ptr)
	  fprintf_filtered (stream, ")");

	fprintf_filtered (stream, (is_vector ?
				   " __attribute__ ((vector_size(" : "["));
	/* Bounds are not yet resolved, print a bounds placeholder instead.  */
	if (TYPE_HIGH_BOUND_KIND (TYPE_INDEX_TYPE (type)) == PROP_LOCEXPR
	    || TYPE_HIGH_BOUND_KIND (TYPE_INDEX_TYPE (type)) == PROP_LOCLIST)
	  fprintf_filtered (stream, "variable length");
	else if (get_array_bounds (type, &low_bound, &high_bound))
	  fprintf_filtered (stream, "%s", 
			    plongest (high_bound - low_bound + 1));
	fprintf_filtered (stream, (is_vector ? ")))" : "]"));

	c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				     show, 0, 0, flags);
      }
      break;

    case TYPE_CODE_MEMBERPTR:
      c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				   show, 0, 0, flags);
      break;

    case TYPE_CODE_METHODPTR:
      fprintf_filtered (stream, ")");
      c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				   show, 0, 0, flags);
      break;

    case TYPE_CODE_PTR:
    case TYPE_CODE_REF:
      c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				   show, 1, 0, flags);
      break;

    case TYPE_CODE_METHOD:
    case TYPE_CODE_FUNC:
      if (passed_a_ptr)
	fprintf_filtered (stream, ")");
      if (!demangled_args)
	c_type_print_args (type, stream, 0, current_language->la_language,
			   flags);
      c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				   show, passed_a_ptr, 0, flags);
      break;

    case TYPE_CODE_TYPEDEF:
      c_type_print_varspec_suffix (TYPE_TARGET_TYPE (type), stream,
				   show, passed_a_ptr, 0, flags);
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FLAGS:
    case TYPE_CODE_ENUM:
    case TYPE_CODE_INT:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_CHAR:
    case TYPE_CODE_BOOL:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_COMPLEX:
    case TYPE_CODE_NAMESPACE:
    case TYPE_CODE_DECFLOAT:
      /* These types do not need a suffix.  They are listed so that
         gcc -Wall will report types that may not have been
         considered.  */
      break;
    default:
      error (_("type not handled in c_type_print_varspec_suffix()"));
      break;
    }
}

/* A helper for c_type_print_base that displays template
   parameters and their bindings, if needed.

   TABLE is the local bindings table to use.  If NULL, no printing is
   done.  Note that, at this point, TABLE won't have any useful
   information in it -- but it is also used as a flag to
   print_name_maybe_canonical to activate searching the global typedef
   table.

   TYPE is the type whose template arguments are being displayed.

   STREAM is the stream on which to print.  */

static void
c_type_print_template_args (const struct type_print_options *flags,
			    struct type *type, struct ui_file *stream)
{
  int first = 1, i;

  if (flags->raw)
    return;

  for (i = 0; i < TYPE_N_TEMPLATE_ARGUMENTS (type); ++i)
    {
      struct symbol *sym = TYPE_TEMPLATE_ARGUMENT (type, i);

      if (SYMBOL_CLASS (sym) != LOC_TYPEDEF)
	continue;

      if (first)
	{
	  wrap_here ("    ");
	  fprintf_filtered (stream, _("[with %s = "),
			    SYMBOL_LINKAGE_NAME (sym));
	  first = 0;
	}
      else
	{
	  fputs_filtered (", ", stream);
	  wrap_here ("         ");
	  fprintf_filtered (stream, "%s = ", SYMBOL_LINKAGE_NAME (sym));
	}

      c_print_type (SYMBOL_TYPE (sym), "", stream, -1, 0, flags);
    }

  if (!first)
    fputs_filtered (_("] "), stream);
}

/* Print the name of the type (or the ultimate pointer target,
   function value or array element), or the description of a structure
   or union.

   SHOW positive means print details about the type (e.g. enum
   values), and print structure elements passing SHOW - 1 for show.

   SHOW negative means just print the type name or struct tag if there
   is one.  If there is no name, print something sensible but concise
   like "struct {...}".

   SHOW zero means just print the type name or struct tag if there is
   one.  If there is no name, print something sensible but not as
   concise like "struct {int x; int y;}".

   LEVEL is the number of spaces to indent by.
   We increase it for some recursive calls.  */

void
c_type_print_base (struct type *type, struct ui_file *stream,
		   int show, int level, const struct type_print_options *flags)
{
  int i;
  int len, real_len;
  enum
    {
      s_none, s_public, s_private, s_protected
    }
  section_type;
  int need_access_label = 0;
  int j, len2;

  QUIT;

  if (type == NULL)
    {
      fputs_filtered (_("<type unknown>"), stream);
      return;
    }

  /* When SHOW is zero or less, and there is a valid type name, then
     always just print the type name directly from the type.  */
  /* If we have "typedef struct foo {. . .} bar;" do we want to print
     it as "struct foo" or as "bar"?  Pick the latter, because C++
     folk tend to expect things like "class5 *foo" rather than "struct
     class5 *foo".  */

  if (show <= 0
      && TYPE_NAME (type) != NULL)
    {
      c_type_print_modifier (type, stream, 0, 1);
      print_name_maybe_canonical (TYPE_NAME (type), flags, stream);
      return;
    }

  type = check_typedef (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_TYPEDEF:
      /* If we get here, the typedef doesn't have a name, and we
	 couldn't resolve TYPE_TARGET_TYPE.  Not much we can do.  */
      gdb_assert (TYPE_NAME (type) == NULL);
      gdb_assert (TYPE_TARGET_TYPE (type) == NULL);
      fprintf_filtered (stream, _("<unnamed typedef>"));
      break;

    case TYPE_CODE_ARRAY:
    case TYPE_CODE_PTR:
    case TYPE_CODE_MEMBERPTR:
    case TYPE_CODE_REF:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_METHODPTR:
      c_type_print_base (TYPE_TARGET_TYPE (type),
			 stream, show, level, flags);
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	struct type_print_options local_flags = *flags;
	struct type_print_options semi_local_flags = *flags;
	struct cleanup *local_cleanups = make_cleanup (null_cleanup, NULL);

	local_flags.local_typedefs = NULL;
	semi_local_flags.local_typedefs = NULL;

	if (!flags->raw)
	  {
	    if (flags->local_typedefs)
	      local_flags.local_typedefs
		= copy_typedef_hash (flags->local_typedefs);
	    else
	      local_flags.local_typedefs = create_typedef_hash ();

	    make_cleanup_free_typedef_hash (local_flags.local_typedefs);
	  }

	c_type_print_modifier (type, stream, 0, 1);
	if (TYPE_CODE (type) == TYPE_CODE_UNION)
	  fprintf_filtered (stream, "union ");
	else if (TYPE_DECLARED_CLASS (type))
	  fprintf_filtered (stream, "class ");
	else
	  fprintf_filtered (stream, "struct ");

	/* Print the tag if it exists.  The HP aCC compiler emits a
	   spurious "{unnamed struct}"/"{unnamed union}"/"{unnamed
	   enum}" tag for unnamed struct/union/enum's, which we don't
	   want to print.  */
	if (TYPE_TAG_NAME (type) != NULL
	    && !startswith (TYPE_TAG_NAME (type), "{unnamed"))
	  {
	    /* When printing the tag name, we are still effectively
	       printing in the outer context, hence the use of FLAGS
	       here.  */
	    print_name_maybe_canonical (TYPE_TAG_NAME (type), flags, stream);
	    if (show > 0)
	      fputs_filtered (" ", stream);
	  }

	if (show < 0)
	  {
	    /* If we just printed a tag name, no need to print anything
	       else.  */
	    if (TYPE_TAG_NAME (type) == NULL)
	      fprintf_filtered (stream, "{...}");
	  }
	else if (show > 0 || TYPE_TAG_NAME (type) == NULL)
	  {
	    struct type *basetype;
	    int vptr_fieldno;

	    c_type_print_template_args (&local_flags, type, stream);

	    /* Add in template parameters when printing derivation info.  */
	    add_template_parameters (local_flags.local_typedefs, type);
	    cp_type_print_derivation_info (stream, type, &local_flags);

	    /* This holds just the global typedefs and the template
	       parameters.  */
	    semi_local_flags.local_typedefs
	      = copy_typedef_hash (local_flags.local_typedefs);
	    if (semi_local_flags.local_typedefs)
	      make_cleanup_free_typedef_hash (semi_local_flags.local_typedefs);

	    /* Now add in the local typedefs.  */
	    recursively_update_typedef_hash (local_flags.local_typedefs, type);

	    fprintf_filtered (stream, "{\n");
	    if (TYPE_NFIELDS (type) == 0 && TYPE_NFN_FIELDS (type) == 0
		&& TYPE_TYPEDEF_FIELD_COUNT (type) == 0)
	      {
		if (TYPE_STUB (type))
		  fprintfi_filtered (level + 4, stream,
				     _("<incomplete type>\n"));
		else
		  fprintfi_filtered (level + 4, stream,
				     _("<no data fields>\n"));
	      }

	    /* Start off with no specific section type, so we can print
	       one for the first field we find, and use that section type
	       thereafter until we find another type.  */

	    section_type = s_none;

	    /* For a class, if all members are private, there's no need
	       for a "private:" label; similarly, for a struct or union
	       masquerading as a class, if all members are public, there's
	       no need for a "public:" label.  */

	    if (TYPE_DECLARED_CLASS (type))
	      {
		QUIT;
		len = TYPE_NFIELDS (type);
		for (i = TYPE_N_BASECLASSES (type); i < len; i++)
		  if (!TYPE_FIELD_PRIVATE (type, i))
		    {
		      need_access_label = 1;
		      break;
		    }
		QUIT;
		if (!need_access_label)
		  {
		    len2 = TYPE_NFN_FIELDS (type);
		    for (j = 0; j < len2; j++)
		      {
			len = TYPE_FN_FIELDLIST_LENGTH (type, j);
			for (i = 0; i < len; i++)
			  if (!TYPE_FN_FIELD_PRIVATE (TYPE_FN_FIELDLIST1 (type,
									  j), i))
			    {
			      need_access_label = 1;
			      break;
			    }
			if (need_access_label)
			  break;
		      }
		  }
	      }
	    else
	      {
		QUIT;
		len = TYPE_NFIELDS (type);
		for (i = TYPE_N_BASECLASSES (type); i < len; i++)
		  if (TYPE_FIELD_PRIVATE (type, i)
		      || TYPE_FIELD_PROTECTED (type, i))
		    {
		      need_access_label = 1;
		      break;
		    }
		QUIT;
		if (!need_access_label)
		  {
		    len2 = TYPE_NFN_FIELDS (type);
		    for (j = 0; j < len2; j++)
		      {
			QUIT;
			len = TYPE_FN_FIELDLIST_LENGTH (type, j);
			for (i = 0; i < len; i++)
			  if (TYPE_FN_FIELD_PROTECTED (TYPE_FN_FIELDLIST1 (type,
									   j), i)
			      || TYPE_FN_FIELD_PRIVATE (TYPE_FN_FIELDLIST1 (type,
									    j),
							i))
			    {
			      need_access_label = 1;
			      break;
			    }
			if (need_access_label)
			  break;
		      }
		  }
	      }

	    /* If there is a base class for this type,
	       do not print the field that it occupies.  */

	    len = TYPE_NFIELDS (type);
	    vptr_fieldno = get_vptr_fieldno (type, &basetype);
	    for (i = TYPE_N_BASECLASSES (type); i < len; i++)
	      {
		QUIT;

		/* If we have a virtual table pointer, omit it.  Even if
		   virtual table pointers are not specifically marked in
		   the debug info, they should be artificial.  */
		if ((i == vptr_fieldno && type == basetype)
		    || TYPE_FIELD_ARTIFICIAL (type, i))
		  continue;

		if (need_access_label)
		  {
		    if (TYPE_FIELD_PROTECTED (type, i))
		      {
			if (section_type != s_protected)
			  {
			    section_type = s_protected;
			    fprintfi_filtered (level + 2, stream,
					       "protected:\n");
			  }
		      }
		    else if (TYPE_FIELD_PRIVATE (type, i))
		      {
			if (section_type != s_private)
			  {
			    section_type = s_private;
			    fprintfi_filtered (level + 2, stream,
					       "private:\n");
			  }
		      }
		    else
		      {
			if (section_type != s_public)
			  {
			    section_type = s_public;
			    fprintfi_filtered (level + 2, stream,
					       "public:\n");
			  }
		      }
		  }

		print_spaces_filtered (level + 4, stream);
		if (field_is_static (&TYPE_FIELD (type, i)))
		  fprintf_filtered (stream, "static ");
		c_print_type (TYPE_FIELD_TYPE (type, i),
			      TYPE_FIELD_NAME (type, i),
			      stream, show - 1, level + 4,
			      &local_flags);
		if (!field_is_static (&TYPE_FIELD (type, i))
		    && TYPE_FIELD_PACKED (type, i))
		  {
		    /* It is a bitfield.  This code does not attempt
		       to look at the bitpos and reconstruct filler,
		       unnamed fields.  This would lead to misleading
		       results if the compiler does not put out fields
		       for such things (I don't know what it does).  */
		    fprintf_filtered (stream, " : %d",
				      TYPE_FIELD_BITSIZE (type, i));
		  }
		fprintf_filtered (stream, ";\n");
	      }

	  /* If there are both fields and methods, put a blank line
	     between them.  Make sure to count only method that we
	     will display; artificial methods will be hidden.  */
	  len = TYPE_NFN_FIELDS (type);
	  if (!flags->print_methods)
	    len = 0;
	  real_len = 0;
	  for (i = 0; i < len; i++)
	    {
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	      int len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);
	      int j;

	      for (j = 0; j < len2; j++)
		if (!TYPE_FN_FIELD_ARTIFICIAL (f, j))
		  real_len++;
	    }
	  if (real_len > 0 && section_type != s_none)
	    fprintf_filtered (stream, "\n");

	  /* C++: print out the methods.  */
	  for (i = 0; i < len; i++)
	    {
	      struct fn_field *f = TYPE_FN_FIELDLIST1 (type, i);
	      int j, len2 = TYPE_FN_FIELDLIST_LENGTH (type, i);
	      const char *method_name = TYPE_FN_FIELDLIST_NAME (type, i);
	      const char *name = type_name_no_tag (type);
	      int is_constructor = name && strcmp (method_name,
						   name) == 0;

	      for (j = 0; j < len2; j++)
		{
		  const char *mangled_name;
		  char *demangled_name;
		  struct cleanup *inner_cleanup;
		  const char *physname = TYPE_FN_FIELD_PHYSNAME (f, j);
		  int is_full_physname_constructor =
		    TYPE_FN_FIELD_CONSTRUCTOR (f, j)
		    || is_constructor_name (physname)
		    || is_destructor_name (physname)
		    || method_name[0] == '~';

		  /* Do not print out artificial methods.  */
		  if (TYPE_FN_FIELD_ARTIFICIAL (f, j))
		    continue;

		  inner_cleanup = make_cleanup (null_cleanup, NULL);

		  QUIT;
		  if (TYPE_FN_FIELD_PROTECTED (f, j))
		    {
		      if (section_type != s_protected)
			{
			  section_type = s_protected;
			  fprintfi_filtered (level + 2, stream,
					     "protected:\n");
			}
		    }
		  else if (TYPE_FN_FIELD_PRIVATE (f, j))
		    {
		      if (section_type != s_private)
			{
			  section_type = s_private;
			  fprintfi_filtered (level + 2, stream,
					     "private:\n");
			}
		    }
		  else
		    {
		      if (section_type != s_public)
			{
			  section_type = s_public;
			  fprintfi_filtered (level + 2, stream,
					     "public:\n");
			}
		    }

		  print_spaces_filtered (level + 4, stream);
		  if (TYPE_FN_FIELD_VIRTUAL_P (f, j))
		    fprintf_filtered (stream, "virtual ");
		  else if (TYPE_FN_FIELD_STATIC_P (f, j))
		    fprintf_filtered (stream, "static ");
		  if (TYPE_TARGET_TYPE (TYPE_FN_FIELD_TYPE (f, j)) == 0)
		    {
		      /* Keep GDB from crashing here.  */
		      fprintf_filtered (stream,
					_("<undefined type> %s;\n"),
					TYPE_FN_FIELD_PHYSNAME (f, j));
		      break;
		    }
		  else if (!is_constructor	/* Constructors don't
						   have declared
						   types.  */
			   && !is_full_physname_constructor  /* " " */
			   && !is_type_conversion_operator (type, i, j))
		    {
		      c_print_type (TYPE_TARGET_TYPE (TYPE_FN_FIELD_TYPE (f, j)),
				    "", stream, -1, 0,
				    &local_flags);
		      fputs_filtered (" ", stream);
		    }
		  if (TYPE_FN_FIELD_STUB (f, j))
		    {
		      char *tem;

		      /* Build something we can demangle.  */
		      tem = gdb_mangle_name (type, i, j);
		      make_cleanup (xfree, tem);
		      mangled_name = tem;
		    }
		  else
		    mangled_name = TYPE_FN_FIELD_PHYSNAME (f, j);

		  demangled_name =
		    gdb_demangle (mangled_name,
				  DMGL_ANSI | DMGL_PARAMS);
		  if (demangled_name == NULL)
		    {
		      /* In some cases (for instance with the HP
			 demangling), if a function has more than 10
			 arguments, the demangling will fail.
			 Let's try to reconstruct the function
			 signature from the symbol information.  */
		      if (!TYPE_FN_FIELD_STUB (f, j))
			{
			  int staticp = TYPE_FN_FIELD_STATIC_P (f, j);
			  struct type *mtype = TYPE_FN_FIELD_TYPE (f, j);

			  cp_type_print_method_args (mtype,
						     "",
						     method_name,
						     staticp,
						     stream, &local_flags);
			}
		      else
			fprintf_filtered (stream,
					  _("<badly mangled name '%s'>"),
					  mangled_name);
		    }
		  else
		    {
		      char *p;
		      char *demangled_no_class
			= remove_qualifiers (demangled_name);

		      /* Get rid of the `static' appended by the
			 demangler.  */
		      p = strstr (demangled_no_class, " static");
		      if (p != NULL)
			{
			  int length = p - demangled_no_class;
			  char *demangled_no_static;

			  demangled_no_static
			    = (char *) xmalloc (length + 1);
			  strncpy (demangled_no_static,
				   demangled_no_class, length);
			  *(demangled_no_static + length) = '\0';
			  fputs_filtered (demangled_no_static, stream);
			  xfree (demangled_no_static);
			}
		      else
			fputs_filtered (demangled_no_class, stream);
		      xfree (demangled_name);
		    }

		  do_cleanups (inner_cleanup);

		  fprintf_filtered (stream, ";\n");
		}
	    }

	  /* Print typedefs defined in this class.  */

	  if (TYPE_TYPEDEF_FIELD_COUNT (type) != 0 && flags->print_typedefs)
	    {
	      if (TYPE_NFIELDS (type) != 0 || TYPE_NFN_FIELDS (type) != 0)
		fprintf_filtered (stream, "\n");

	      for (i = 0; i < TYPE_TYPEDEF_FIELD_COUNT (type); i++)
		{
		  struct type *target = TYPE_TYPEDEF_FIELD_TYPE (type, i);

		  /* Dereference the typedef declaration itself.  */
		  gdb_assert (TYPE_CODE (target) == TYPE_CODE_TYPEDEF);
		  target = TYPE_TARGET_TYPE (target);

		  print_spaces_filtered (level + 4, stream);
		  fprintf_filtered (stream, "typedef ");

		  /* We want to print typedefs with substitutions
		     from the template parameters or globally-known
		     typedefs but not local typedefs.  */
		  c_print_type (target,
				TYPE_TYPEDEF_FIELD_NAME (type, i),
				stream, show - 1, level + 4,
				&semi_local_flags);
		  fprintf_filtered (stream, ";\n");
		}
	    }

	    fprintfi_filtered (level, stream, "}");
	  }

	do_cleanups (local_cleanups);
      }
      break;

    case TYPE_CODE_ENUM:
      c_type_print_modifier (type, stream, 0, 1);
      fprintf_filtered (stream, "enum ");
      if (TYPE_DECLARED_CLASS (type))
	fprintf_filtered (stream, "class ");
      /* Print the tag name if it exists.
         The aCC compiler emits a spurious 
         "{unnamed struct}"/"{unnamed union}"/"{unnamed enum}"
         tag for unnamed struct/union/enum's, which we don't
         want to print.  */
      if (TYPE_TAG_NAME (type) != NULL
	  && !startswith (TYPE_TAG_NAME (type), "{unnamed"))
	{
	  print_name_maybe_canonical (TYPE_TAG_NAME (type), flags, stream);
	  if (show > 0)
	    fputs_filtered (" ", stream);
	}

      wrap_here ("    ");
      if (show < 0)
	{
	  /* If we just printed a tag name, no need to print anything
	     else.  */
	  if (TYPE_TAG_NAME (type) == NULL)
	    fprintf_filtered (stream, "{...}");
	}
      else if (show > 0 || TYPE_TAG_NAME (type) == NULL)
	{
	  LONGEST lastval = 0;

	  /* We can't handle this case perfectly, as DWARF does not
	     tell us whether or not the underlying type was specified
	     in the source (and other debug formats don't provide this
	     at all).  We choose to print the underlying type, if it
	     has a name, when in C++ on the theory that it's better to
	     print too much than too little; but conversely not to
	     print something egregiously outside the current
	     language's syntax.  */
	  if (current_language->la_language == language_cplus
	      && TYPE_TARGET_TYPE (type) != NULL)
	    {
	      struct type *underlying = check_typedef (TYPE_TARGET_TYPE (type));

	      if (TYPE_NAME (underlying) != NULL)
		fprintf_filtered (stream, ": %s ", TYPE_NAME (underlying));
	    }

	  fprintf_filtered (stream, "{");
	  len = TYPE_NFIELDS (type);
	  for (i = 0; i < len; i++)
	    {
	      QUIT;
	      if (i)
		fprintf_filtered (stream, ", ");
	      wrap_here ("    ");
	      fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	      if (lastval != TYPE_FIELD_ENUMVAL (type, i))
		{
		  fprintf_filtered (stream, " = %s",
				    plongest (TYPE_FIELD_ENUMVAL (type, i)));
		  lastval = TYPE_FIELD_ENUMVAL (type, i);
		}
	      lastval++;
	    }
	  fprintf_filtered (stream, "}");
	}
      break;

    case TYPE_CODE_FLAGS:
      {
	struct type_print_options local_flags = *flags;

	local_flags.local_typedefs = NULL;

	c_type_print_modifier (type, stream, 0, 1);
	fprintf_filtered (stream, "flag ");
	print_name_maybe_canonical (TYPE_NAME (type), flags, stream);
	if (show > 0)
	  {
	    fputs_filtered (" ", stream);
	    fprintf_filtered (stream, "{\n");
	    if (TYPE_NFIELDS (type) == 0)
	      {
		if (TYPE_STUB (type))
		  fprintfi_filtered (level + 4, stream,
				     _("<incomplete type>\n"));
		else
		  fprintfi_filtered (level + 4, stream,
				     _("<no data fields>\n"));
	      }
	    len = TYPE_NFIELDS (type);
	    for (i = 0; i < len; i++)
	      {
		QUIT;
		print_spaces_filtered (level + 4, stream);
		/* We pass "show" here and not "show - 1" to get enum types
		   printed.  There's no other way to see them.  */
		c_print_type (TYPE_FIELD_TYPE (type, i),
			      TYPE_FIELD_NAME (type, i),
			      stream, show, level + 4,
			      &local_flags);
		fprintf_filtered (stream, " @%s",
				  plongest (TYPE_FIELD_BITPOS (type, i)));
		if (TYPE_FIELD_BITSIZE (type, i) > 1)
		  {
		    fprintf_filtered (stream, "-%s",
				      plongest (TYPE_FIELD_BITPOS (type, i)
						+ TYPE_FIELD_BITSIZE (type, i)
						- 1));
		  }
		fprintf_filtered (stream, ";\n");
	      }
	    fprintfi_filtered (level, stream, "}");
	  }
      }
      break;

    case TYPE_CODE_VOID:
      fprintf_filtered (stream, "void");
      break;

    case TYPE_CODE_UNDEF:
      fprintf_filtered (stream, _("struct <unknown>"));
      break;

    case TYPE_CODE_ERROR:
      fprintf_filtered (stream, "%s", TYPE_ERROR_NAME (type));
      break;

    case TYPE_CODE_RANGE:
      /* This should not occur.  */
      fprintf_filtered (stream, _("<range type>"));
      break;

    case TYPE_CODE_NAMESPACE:
      fputs_filtered ("namespace ", stream);
      fputs_filtered (TYPE_TAG_NAME (type), stream);
      break;

    default:
      /* Handle types not explicitly handled by the other cases, such
         as fundamental types.  For these, just print whatever the
         type name is, as recorded in the type itself.  If there is no
         type name, then complain.  */
      if (TYPE_NAME (type) != NULL)
	{
	  c_type_print_modifier (type, stream, 0, 1);
	  print_name_maybe_canonical (TYPE_NAME (type), flags, stream);
	}
      else
	{
	  /* At least for dump_symtab, it is important that this not
	     be an error ().  */
	  fprintf_filtered (stream, _("<invalid type code %d>"),
			    TYPE_CODE (type));
	}
      break;
    }
}
