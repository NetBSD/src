/* Language-dependent hooks for LTO.
   Copyright 2009 Free Software Foundation, Inc.
   Contributed by CodeSourcery, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "flags.h"
#include "tm.h"
#include "tree.h"
#include "expr.h"
#include "target.h"
#include "langhooks.h"
#include "langhooks-def.h"
#include "debug.h"
#include "lto-tree.h"
#include "lto.h"
#include "tree-inline.h"
#include "gimple.h"
#include "toplev.h"

static tree handle_noreturn_attribute (tree *, tree, tree, int, bool *);
static tree handle_const_attribute (tree *, tree, tree, int, bool *);
static tree handle_malloc_attribute (tree *, tree, tree, int, bool *);
static tree handle_pure_attribute (tree *, tree, tree, int, bool *);
static tree handle_novops_attribute (tree *, tree, tree, int, bool *);
static tree handle_nonnull_attribute (tree *, tree, tree, int, bool *);
static tree handle_nothrow_attribute (tree *, tree, tree, int, bool *);
static tree handle_sentinel_attribute (tree *, tree, tree, int, bool *);
static tree handle_type_generic_attribute (tree *, tree, tree, int, bool *);
static tree handle_format_attribute (tree *, tree, tree, int, bool *);
static tree handle_format_arg_attribute (tree *, tree, tree, int, bool *);

/* Table of machine-independent attributes supported in GIMPLE.  */
const struct attribute_spec lto_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  { "noreturn",               0, 0, true,  false, false,
			      handle_noreturn_attribute },
  /* The same comments as for noreturn attributes apply to const ones.  */
  { "const",                  0, 0, true,  false, false,
			      handle_const_attribute },
  { "malloc",                 0, 0, true,  false, false,
			      handle_malloc_attribute },
  { "pure",                   0, 0, true,  false, false,
			      handle_pure_attribute },
  { "no vops",                0, 0, true,  false, false,
			      handle_novops_attribute },
  { "nonnull",                0, -1, false, true, true,
			      handle_nonnull_attribute },
  { "nothrow",                0, 0, true,  false, false,
			      handle_nothrow_attribute },
  { "sentinel",               0, 1, false, true, true,
			      handle_sentinel_attribute },
  { "type generic",           0, 0, false, true, true,
			      handle_type_generic_attribute },
  { NULL,                     0, 0, false, false, false, NULL }
};

/* Give the specifications for the format attributes, used by C and all
   descendants.  */

const struct attribute_spec lto_format_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  { "format",                 3, 3, false, true,  true,
			      handle_format_attribute },
  { "format_arg",             1, 1, false, true,  true,
			      handle_format_arg_attribute },
  { NULL,                     0, 0, false, false, false, NULL }
};

enum built_in_attribute
{
#define DEF_ATTR_NULL_TREE(ENUM) ENUM,
#define DEF_ATTR_INT(ENUM, VALUE) ENUM,
#define DEF_ATTR_IDENT(ENUM, STRING) ENUM,
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN) ENUM,
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
  ATTR_LAST
};

static GTY(()) tree built_in_attributes[(int) ATTR_LAST];

/* Builtin types.  */

enum lto_builtin_type
{
#define DEF_PRIMITIVE_TYPE(NAME, VALUE) NAME,
#define DEF_FUNCTION_TYPE_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) NAME,
#define DEF_FUNCTION_TYPE_6(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) NAME,
#define DEF_FUNCTION_TYPE_7(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) NAME,
#define DEF_FUNCTION_TYPE_VAR_0(NAME, RETURN) NAME,
#define DEF_FUNCTION_TYPE_VAR_1(NAME, RETURN, ARG1) NAME,
#define DEF_FUNCTION_TYPE_VAR_2(NAME, RETURN, ARG1, ARG2) NAME,
#define DEF_FUNCTION_TYPE_VAR_3(NAME, RETURN, ARG1, ARG2, ARG3) NAME,
#define DEF_FUNCTION_TYPE_VAR_4(NAME, RETURN, ARG1, ARG2, ARG3, ARG4) NAME,
#define DEF_FUNCTION_TYPE_VAR_5(NAME, RETURN, ARG1, ARG2, ARG3, ARG4, ARG6) \
  NAME,
#define DEF_POINTER_TYPE(NAME, TYPE) NAME,
#include "builtin-types.def"
#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_0
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_7
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_POINTER_TYPE
  BT_LAST
};

typedef enum lto_builtin_type builtin_type;

static GTY(()) tree builtin_types[(int) BT_LAST + 1];

static GTY(()) tree string_type_node;
static GTY(()) tree const_string_type_node;
static GTY(()) tree wint_type_node;
static GTY(()) tree intmax_type_node;
static GTY(()) tree uintmax_type_node;
static GTY(()) tree signed_size_type_node;

/* Flags needed to process builtins.def.  */
int flag_no_builtin;
int flag_no_nonansi_builtin;
int flag_isoc94;
int flag_isoc99;

/* Attribute handlers.  */

/* Handle a "noreturn" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_noreturn_attribute (tree *node, tree ARG_UNUSED (name),
			   tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			   bool * ARG_UNUSED (no_add_attrs))
{
  tree type = TREE_TYPE (*node);

  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_THIS_VOLATILE (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type),
			     TYPE_READONLY (TREE_TYPE (type)), 1));
  else
    gcc_unreachable ();

  return NULL_TREE;
}


/* Handle a "const" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_const_attribute (tree *node, tree ARG_UNUSED (name),
			tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			bool * ARG_UNUSED (no_add_attrs))
{
  tree type = TREE_TYPE (*node);

  /* See FIXME comment on noreturn in c_common_attribute_table.  */
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_READONLY (*node) = 1;
  else if (TREE_CODE (type) == POINTER_TYPE
	   && TREE_CODE (TREE_TYPE (type)) == FUNCTION_TYPE)
    TREE_TYPE (*node)
      = build_pointer_type
	(build_type_variant (TREE_TYPE (type), 1,
			     TREE_THIS_VOLATILE (TREE_TYPE (type))));
  else
    gcc_unreachable ();

  return NULL_TREE;
}


/* Handle a "malloc" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_malloc_attribute (tree *node, tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL
      && POINTER_TYPE_P (TREE_TYPE (TREE_TYPE (*node))))
    DECL_IS_MALLOC (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}


/* Handle a "pure" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_pure_attribute (tree *node, tree ARG_UNUSED (name),
		       tree ARG_UNUSED (args), int ARG_UNUSED (flags),
		       bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    DECL_PURE_P (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}


/* Handle a "no vops" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_novops_attribute (tree *node, tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool *ARG_UNUSED (no_add_attrs))
{
  gcc_assert (TREE_CODE (*node) == FUNCTION_DECL);
  DECL_IS_NOVOPS (*node) = 1;
  return NULL_TREE;
}


/* Helper for nonnull attribute handling; fetch the operand number
   from the attribute argument list.  */

static bool
get_nonnull_operand (tree arg_num_expr, unsigned HOST_WIDE_INT *valp)
{
  /* Verify the arg number is a constant.  */
  if (TREE_CODE (arg_num_expr) != INTEGER_CST
      || TREE_INT_CST_HIGH (arg_num_expr) != 0)
    return false;

  *valp = TREE_INT_CST_LOW (arg_num_expr);
  return true;
}

/* Handle the "nonnull" attribute.  */

static tree
handle_nonnull_attribute (tree *node, tree ARG_UNUSED (name),
			  tree args, int ARG_UNUSED (flags),
			  bool * ARG_UNUSED (no_add_attrs))
{
  tree type = *node;

  /* If no arguments are specified, all pointer arguments should be
     non-null.  Verify a full prototype is given so that the arguments
     will have the correct types when we actually check them later.  */
  if (!args)
    {
      gcc_assert (TYPE_ARG_TYPES (type));
      return NULL_TREE;
    }

  /* Argument list specified.  Verify that each argument number references
     a pointer argument.  */
  for (; args; args = TREE_CHAIN (args))
    {
      tree argument;
      unsigned HOST_WIDE_INT arg_num = 0, ck_num;

      if (!get_nonnull_operand (TREE_VALUE (args), &arg_num))
	gcc_unreachable ();

      argument = TYPE_ARG_TYPES (type);
      if (argument)
	{
	  for (ck_num = 1; ; ck_num++)
	    {
	      if (!argument || ck_num == arg_num)
		break;
	      argument = TREE_CHAIN (argument);
	    }

	  gcc_assert (argument
		      && TREE_CODE (TREE_VALUE (argument)) == POINTER_TYPE);
	}
    }

  return NULL_TREE;
}


/* Handle a "nothrow" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_nothrow_attribute (tree *node, tree ARG_UNUSED (name),
			  tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			  bool * ARG_UNUSED (no_add_attrs))
{
  if (TREE_CODE (*node) == FUNCTION_DECL)
    TREE_NOTHROW (*node) = 1;
  else
    gcc_unreachable ();

  return NULL_TREE;
}


/* Handle a "sentinel" attribute.  */

static tree
handle_sentinel_attribute (tree *node, tree ARG_UNUSED (name), tree args,
			   int ARG_UNUSED (flags),
			   bool * ARG_UNUSED (no_add_attrs))
{
  tree params = TYPE_ARG_TYPES (*node);
  gcc_assert (params);

  while (TREE_CHAIN (params))
    params = TREE_CHAIN (params);

  gcc_assert (!VOID_TYPE_P (TREE_VALUE (params)));

  if (args)
    {
      tree position = TREE_VALUE (args);
      gcc_assert (TREE_CODE (position) == INTEGER_CST);
      if (tree_int_cst_lt (position, integer_zero_node))
	gcc_unreachable ();
    }

  return NULL_TREE;
}

/* Handle a "type_generic" attribute.  */

static tree
handle_type_generic_attribute (tree *node, tree ARG_UNUSED (name),
			       tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			       bool * ARG_UNUSED (no_add_attrs))
{
  tree params;
  
  /* Ensure we have a function type.  */
  gcc_assert (TREE_CODE (*node) == FUNCTION_TYPE);
  
  params = TYPE_ARG_TYPES (*node);
  while (params && ! VOID_TYPE_P (TREE_VALUE (params)))
    params = TREE_CHAIN (params);

  /* Ensure we have a variadic function.  */
  gcc_assert (!params);

  return NULL_TREE;
}

/* Handle a "format" attribute; arguments as in
   struct attribute_spec.handler.  */

static tree
handle_format_attribute (tree * ARG_UNUSED (node), tree ARG_UNUSED (name),
			 tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			 bool *no_add_attrs)
{
  *no_add_attrs = true;
  return NULL_TREE;
}


/* Handle a "format_arg" attribute; arguments as in
   struct attribute_spec.handler.  */

tree
handle_format_arg_attribute (tree * ARG_UNUSED (node), tree ARG_UNUSED (name),
			     tree ARG_UNUSED (args), int ARG_UNUSED (flags),
			     bool *no_add_attrs)
{
  *no_add_attrs = true;
  return NULL_TREE;
}


/* Cribbed from c-common.c.  */

static void
def_fn_type (builtin_type def, builtin_type ret, bool var, int n, ...)
{
  tree args = NULL, t;
  va_list list;
  int i;

  va_start (list, n);
  for (i = 0; i < n; ++i)
    {
      builtin_type a = (builtin_type) va_arg (list, int);
      t = builtin_types[a];
      if (t == error_mark_node)
	goto egress;
      args = tree_cons (NULL_TREE, t, args);
    }
  va_end (list);

  args = nreverse (args);
  if (!var)
    args = chainon (args, void_list_node);

  t = builtin_types[ret];
  if (t == error_mark_node)
    goto egress;
  t = build_function_type (t, args);

 egress:
  builtin_types[def] = t;
}

/* Used to help initialize the builtin-types.def table.  When a type of
   the correct size doesn't exist, use error_mark_node instead of NULL.
   The later results in segfaults even when a decl using the type doesn't
   get invoked.  */

static tree
builtin_type_for_size (int size, bool unsignedp)
{
  tree type = lang_hooks.types.type_for_size (size, unsignedp);
  return type ? type : error_mark_node;
}

/* Support for DEF_BUILTIN.  */

static void
def_builtin_1 (enum built_in_function fncode, const char *name,
	       enum built_in_class fnclass, tree fntype, tree libtype,
	       bool both_p, bool fallback_p, bool nonansi_p,
	       tree fnattrs, bool implicit_p)
{
  tree decl;
  const char *libname;

  if (fntype == error_mark_node)
    return;

  libname = name + strlen ("__builtin_");
  decl = add_builtin_function (name, fntype, fncode, fnclass,
			       (fallback_p ? libname : NULL),
			       fnattrs);

  if (both_p
      && !flag_no_builtin
      && !(nonansi_p && flag_no_nonansi_builtin))
    add_builtin_function (libname, libtype, fncode, fnclass,
			  NULL, fnattrs);

  built_in_decls[(int) fncode] = decl;
  if (implicit_p)
    implicit_built_in_decls[(int) fncode] = decl;
}


/* Initialize the attribute table for all the supported builtins.  */

static void
lto_init_attributes (void)
{
  /* Fill in the built_in_attributes array.  */
#define DEF_ATTR_NULL_TREE(ENUM)				\
  built_in_attributes[(int) ENUM] = NULL_TREE;
#define DEF_ATTR_INT(ENUM, VALUE)				\
  built_in_attributes[(int) ENUM] = build_int_cst (NULL_TREE, VALUE);
#define DEF_ATTR_IDENT(ENUM, STRING)				\
  built_in_attributes[(int) ENUM] = get_identifier (STRING);
#define DEF_ATTR_TREE_LIST(ENUM, PURPOSE, VALUE, CHAIN)	\
  built_in_attributes[(int) ENUM]			\
    = tree_cons (built_in_attributes[(int) PURPOSE],	\
		 built_in_attributes[(int) VALUE],	\
		 built_in_attributes[(int) CHAIN]);
#include "builtin-attrs.def"
#undef DEF_ATTR_NULL_TREE
#undef DEF_ATTR_INT
#undef DEF_ATTR_IDENT
#undef DEF_ATTR_TREE_LIST
}

/* Create builtin types and functions.  VA_LIST_REF_TYPE_NODE and
   VA_LIST_ARG_TYPE_NODE are used in builtin-types.def.  */

static void
lto_define_builtins (tree va_list_ref_type_node ATTRIBUTE_UNUSED,
		     tree va_list_arg_type_node ATTRIBUTE_UNUSED)
{
#define DEF_PRIMITIVE_TYPE(ENUM, VALUE) \
  builtin_types[ENUM] = VALUE;
#define DEF_FUNCTION_TYPE_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 0, 0);
#define DEF_FUNCTION_TYPE_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 0, 1, ARG1);
#define DEF_FUNCTION_TYPE_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 0, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 0, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 0, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5)	\
  def_fn_type (ENUM, RETURN, 0, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_FUNCTION_TYPE_6(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6)					\
  def_fn_type (ENUM, RETURN, 0, 6, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6);
#define DEF_FUNCTION_TYPE_7(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5, \
			    ARG6, ARG7)					\
  def_fn_type (ENUM, RETURN, 0, 7, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7);
#define DEF_FUNCTION_TYPE_VAR_0(ENUM, RETURN) \
  def_fn_type (ENUM, RETURN, 1, 0);
#define DEF_FUNCTION_TYPE_VAR_1(ENUM, RETURN, ARG1) \
  def_fn_type (ENUM, RETURN, 1, 1, ARG1);
#define DEF_FUNCTION_TYPE_VAR_2(ENUM, RETURN, ARG1, ARG2) \
  def_fn_type (ENUM, RETURN, 1, 2, ARG1, ARG2);
#define DEF_FUNCTION_TYPE_VAR_3(ENUM, RETURN, ARG1, ARG2, ARG3) \
  def_fn_type (ENUM, RETURN, 1, 3, ARG1, ARG2, ARG3);
#define DEF_FUNCTION_TYPE_VAR_4(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4) \
  def_fn_type (ENUM, RETURN, 1, 4, ARG1, ARG2, ARG3, ARG4);
#define DEF_FUNCTION_TYPE_VAR_5(ENUM, RETURN, ARG1, ARG2, ARG3, ARG4, ARG5) \
  def_fn_type (ENUM, RETURN, 1, 5, ARG1, ARG2, ARG3, ARG4, ARG5);
#define DEF_POINTER_TYPE(ENUM, TYPE) \
  builtin_types[(int) ENUM] = build_pointer_type (builtin_types[(int) TYPE]);

#include "builtin-types.def"

#undef DEF_PRIMITIVE_TYPE
#undef DEF_FUNCTION_TYPE_1
#undef DEF_FUNCTION_TYPE_2
#undef DEF_FUNCTION_TYPE_3
#undef DEF_FUNCTION_TYPE_4
#undef DEF_FUNCTION_TYPE_5
#undef DEF_FUNCTION_TYPE_6
#undef DEF_FUNCTION_TYPE_VAR_0
#undef DEF_FUNCTION_TYPE_VAR_1
#undef DEF_FUNCTION_TYPE_VAR_2
#undef DEF_FUNCTION_TYPE_VAR_3
#undef DEF_FUNCTION_TYPE_VAR_4
#undef DEF_FUNCTION_TYPE_VAR_5
#undef DEF_POINTER_TYPE
  builtin_types[(int) BT_LAST] = NULL_TREE;

  lto_init_attributes ();

#define DEF_BUILTIN(ENUM, NAME, CLASS, TYPE, LIBTYPE, BOTH_P, FALLBACK_P,\
		    NONANSI_P, ATTRS, IMPLICIT, COND)			\
    if (NAME && COND)							\
      def_builtin_1 (ENUM, NAME, CLASS, builtin_types[(int) TYPE],	\
		     builtin_types[(int) LIBTYPE], BOTH_P, FALLBACK_P,	\
		     NONANSI_P, built_in_attributes[(int) ATTRS], IMPLICIT);
#include "builtins.def"
#undef DEF_BUILTIN
}

static GTY(()) tree registered_builtin_types;

/* A chain of builtin functions that we need to recognize.  We will
   assume that all other function names we see will be defined by the
   user's program.  */
static GTY(()) tree registered_builtin_fndecls;

/* Language hooks.  */

static unsigned int
lto_init_options (unsigned int argc ATTRIBUTE_UNUSED,
		  const char **argv ATTRIBUTE_UNUSED)
{
  /* By default, C99-like requirements for complex multiply and divide.
     ???  Until the complex method is encoded in the IL this is the only
     safe choice.  This will pessimize Fortran code with LTO unless
     people specify a complex method manually or use -ffast-math.  */
  flag_complex_method = 2;

  return CL_LTO;
}

/* Handle command-line option SCODE.  If the option takes an argument, it is
   stored in ARG, which is otherwise NULL.  VALUE holds either a numerical
   argument or a binary value indicating whether the positive or negative form
   of the option was supplied.  */

const char *resolution_file_name;
static int
lto_handle_option (size_t scode, const char *arg, int value ATTRIBUTE_UNUSED)
{
  enum opt_code code = (enum opt_code) scode;
  int result = 1;

  switch (code)
    {
    case OPT_fresolution:
      resolution_file_name = arg;
      result = 1;
      break;

    case OPT_Wabi:
      warn_psabi = value;
      break;

    case OPT_fsigned_char:
      flag_signed_char = value;
      break;

    case OPT_funsigned_char:
      flag_signed_char = !value;
      break;

    default:
      break;
    }

  return result;
}

/* Perform post-option processing.  Does additional initialization based on
   command-line options.  PFILENAME is the main input filename.  Returns false
   to enable subsequent back-end initialization.  */

static bool
lto_post_options (const char **pfilename ATTRIBUTE_UNUSED)
{
  /* -fltrans and -fwpa are mutually exclusive.  Check for that here.  */
  if (flag_wpa && flag_ltrans)
    error ("-fwpa and -fltrans are mutually exclusive");

  if (flag_ltrans)
    {
      flag_generate_lto = 0;

      /* During LTRANS, we are not looking at the whole program, only
	 a subset of the whole callgraph.  */
      flag_whole_program = 0;
    }

  if (flag_wpa)
    flag_generate_lto = 1;

  /* Excess precision other than "fast" requires front-end
     support.  */
  flag_excess_precision_cmdline = EXCESS_PRECISION_FAST;

  lto_read_all_file_options ();

  /* Initialize the compiler back end.  */
  return false;
}

/* Return an integer type with PRECISION bits of precision,
   that is unsigned if UNSIGNEDP is nonzero, otherwise signed.  */

static tree
lto_type_for_size (unsigned precision, int unsignedp)
{
  if (precision == TYPE_PRECISION (integer_type_node))
    return unsignedp ? unsigned_type_node : integer_type_node;

  if (precision == TYPE_PRECISION (signed_char_type_node))
    return unsignedp ? unsigned_char_type_node : signed_char_type_node;

  if (precision == TYPE_PRECISION (short_integer_type_node))
    return unsignedp ? short_unsigned_type_node : short_integer_type_node;

  if (precision == TYPE_PRECISION (long_integer_type_node))
    return unsignedp ? long_unsigned_type_node : long_integer_type_node;

  if (precision == TYPE_PRECISION (long_long_integer_type_node))
    return unsignedp
	   ? long_long_unsigned_type_node
	   : long_long_integer_type_node;

  if (precision <= TYPE_PRECISION (intQI_type_node))
    return unsignedp ? unsigned_intQI_type_node : intQI_type_node;

  if (precision <= TYPE_PRECISION (intHI_type_node))
    return unsignedp ? unsigned_intHI_type_node : intHI_type_node;

  if (precision <= TYPE_PRECISION (intSI_type_node))
    return unsignedp ? unsigned_intSI_type_node : intSI_type_node;

  if (precision <= TYPE_PRECISION (intDI_type_node))
    return unsignedp ? unsigned_intDI_type_node : intDI_type_node;

  if (precision <= TYPE_PRECISION (intTI_type_node))
    return unsignedp ? unsigned_intTI_type_node : intTI_type_node;

  return NULL_TREE;
}


/* Return a data type that has machine mode MODE.
   If the mode is an integer,
   then UNSIGNEDP selects between signed and unsigned types.
   If the mode is a fixed-point mode,
   then UNSIGNEDP selects between saturating and nonsaturating types.  */

static tree
lto_type_for_mode (enum machine_mode mode, int unsigned_p)
{
  tree t;

  if (mode == TYPE_MODE (integer_type_node))
    return unsigned_p ? unsigned_type_node : integer_type_node;

  if (mode == TYPE_MODE (signed_char_type_node))
    return unsigned_p ? unsigned_char_type_node : signed_char_type_node;

  if (mode == TYPE_MODE (short_integer_type_node))
    return unsigned_p ? short_unsigned_type_node : short_integer_type_node;

  if (mode == TYPE_MODE (long_integer_type_node))
    return unsigned_p ? long_unsigned_type_node : long_integer_type_node;

  if (mode == TYPE_MODE (long_long_integer_type_node))
    return unsigned_p ? long_long_unsigned_type_node : long_long_integer_type_node;

  if (mode == QImode)
    return unsigned_p ? unsigned_intQI_type_node : intQI_type_node;

  if (mode == HImode)
    return unsigned_p ? unsigned_intHI_type_node : intHI_type_node;

  if (mode == SImode)
    return unsigned_p ? unsigned_intSI_type_node : intSI_type_node;

  if (mode == DImode)
    return unsigned_p ? unsigned_intDI_type_node : intDI_type_node;

#if HOST_BITS_PER_WIDE_INT >= 64
  if (mode == TYPE_MODE (intTI_type_node))
    return unsigned_p ? unsigned_intTI_type_node : intTI_type_node;
#endif

  if (mode == TYPE_MODE (float_type_node))
    return float_type_node;

  if (mode == TYPE_MODE (double_type_node))
    return double_type_node;

  if (mode == TYPE_MODE (long_double_type_node))
    return long_double_type_node;

  if (mode == TYPE_MODE (void_type_node))
    return void_type_node;

  if (mode == TYPE_MODE (build_pointer_type (char_type_node)))
    return (unsigned_p
	    ? make_unsigned_type (GET_MODE_PRECISION (mode))
	    : make_signed_type (GET_MODE_PRECISION (mode)));

  if (mode == TYPE_MODE (build_pointer_type (integer_type_node)))
    return (unsigned_p
	    ? make_unsigned_type (GET_MODE_PRECISION (mode))
	    : make_signed_type (GET_MODE_PRECISION (mode)));

  if (COMPLEX_MODE_P (mode))
    {
      enum machine_mode inner_mode;
      tree inner_type;

      if (mode == TYPE_MODE (complex_float_type_node))
	return complex_float_type_node;
      if (mode == TYPE_MODE (complex_double_type_node))
	return complex_double_type_node;
      if (mode == TYPE_MODE (complex_long_double_type_node))
	return complex_long_double_type_node;

      if (mode == TYPE_MODE (complex_integer_type_node) && !unsigned_p)
	return complex_integer_type_node;

      inner_mode = GET_MODE_INNER (mode);
      inner_type = lto_type_for_mode (inner_mode, unsigned_p);
      if (inner_type != NULL_TREE)
	return build_complex_type (inner_type);
    }
  else if (VECTOR_MODE_P (mode))
    {
      enum machine_mode inner_mode = GET_MODE_INNER (mode);
      tree inner_type = lto_type_for_mode (inner_mode, unsigned_p);
      if (inner_type != NULL_TREE)
	return build_vector_type_for_mode (inner_type, mode);
    }

  if (mode == TYPE_MODE (dfloat32_type_node))
    return dfloat32_type_node;
  if (mode == TYPE_MODE (dfloat64_type_node))
    return dfloat64_type_node;
  if (mode == TYPE_MODE (dfloat128_type_node))
    return dfloat128_type_node;

  if (ALL_SCALAR_FIXED_POINT_MODE_P (mode))
    {
      if (mode == TYPE_MODE (short_fract_type_node))
	return unsigned_p ? sat_short_fract_type_node : short_fract_type_node;
      if (mode == TYPE_MODE (fract_type_node))
	return unsigned_p ? sat_fract_type_node : fract_type_node;
      if (mode == TYPE_MODE (long_fract_type_node))
	return unsigned_p ? sat_long_fract_type_node : long_fract_type_node;
      if (mode == TYPE_MODE (long_long_fract_type_node))
	return unsigned_p ? sat_long_long_fract_type_node
			 : long_long_fract_type_node;

      if (mode == TYPE_MODE (unsigned_short_fract_type_node))
	return unsigned_p ? sat_unsigned_short_fract_type_node
			 : unsigned_short_fract_type_node;
      if (mode == TYPE_MODE (unsigned_fract_type_node))
	return unsigned_p ? sat_unsigned_fract_type_node
			 : unsigned_fract_type_node;
      if (mode == TYPE_MODE (unsigned_long_fract_type_node))
	return unsigned_p ? sat_unsigned_long_fract_type_node
			 : unsigned_long_fract_type_node;
      if (mode == TYPE_MODE (unsigned_long_long_fract_type_node))
	return unsigned_p ? sat_unsigned_long_long_fract_type_node
			 : unsigned_long_long_fract_type_node;

      if (mode == TYPE_MODE (short_accum_type_node))
	return unsigned_p ? sat_short_accum_type_node : short_accum_type_node;
      if (mode == TYPE_MODE (accum_type_node))
	return unsigned_p ? sat_accum_type_node : accum_type_node;
      if (mode == TYPE_MODE (long_accum_type_node))
	return unsigned_p ? sat_long_accum_type_node : long_accum_type_node;
      if (mode == TYPE_MODE (long_long_accum_type_node))
	return unsigned_p ? sat_long_long_accum_type_node
			 : long_long_accum_type_node;

      if (mode == TYPE_MODE (unsigned_short_accum_type_node))
	return unsigned_p ? sat_unsigned_short_accum_type_node
			 : unsigned_short_accum_type_node;
      if (mode == TYPE_MODE (unsigned_accum_type_node))
	return unsigned_p ? sat_unsigned_accum_type_node
			 : unsigned_accum_type_node;
      if (mode == TYPE_MODE (unsigned_long_accum_type_node))
	return unsigned_p ? sat_unsigned_long_accum_type_node
			 : unsigned_long_accum_type_node;
      if (mode == TYPE_MODE (unsigned_long_long_accum_type_node))
	return unsigned_p ? sat_unsigned_long_long_accum_type_node
			 : unsigned_long_long_accum_type_node;

      if (mode == QQmode)
	return unsigned_p ? sat_qq_type_node : qq_type_node;
      if (mode == HQmode)
	return unsigned_p ? sat_hq_type_node : hq_type_node;
      if (mode == SQmode)
	return unsigned_p ? sat_sq_type_node : sq_type_node;
      if (mode == DQmode)
	return unsigned_p ? sat_dq_type_node : dq_type_node;
      if (mode == TQmode)
	return unsigned_p ? sat_tq_type_node : tq_type_node;

      if (mode == UQQmode)
	return unsigned_p ? sat_uqq_type_node : uqq_type_node;
      if (mode == UHQmode)
	return unsigned_p ? sat_uhq_type_node : uhq_type_node;
      if (mode == USQmode)
	return unsigned_p ? sat_usq_type_node : usq_type_node;
      if (mode == UDQmode)
	return unsigned_p ? sat_udq_type_node : udq_type_node;
      if (mode == UTQmode)
	return unsigned_p ? sat_utq_type_node : utq_type_node;

      if (mode == HAmode)
	return unsigned_p ? sat_ha_type_node : ha_type_node;
      if (mode == SAmode)
	return unsigned_p ? sat_sa_type_node : sa_type_node;
      if (mode == DAmode)
	return unsigned_p ? sat_da_type_node : da_type_node;
      if (mode == TAmode)
	return unsigned_p ? sat_ta_type_node : ta_type_node;

      if (mode == UHAmode)
	return unsigned_p ? sat_uha_type_node : uha_type_node;
      if (mode == USAmode)
	return unsigned_p ? sat_usa_type_node : usa_type_node;
      if (mode == UDAmode)
	return unsigned_p ? sat_uda_type_node : uda_type_node;
      if (mode == UTAmode)
	return unsigned_p ? sat_uta_type_node : uta_type_node;
    }

  for (t = registered_builtin_types; t; t = TREE_CHAIN (t))
    if (TYPE_MODE (TREE_VALUE (t)) == mode)
      return TREE_VALUE (t);

  return NULL_TREE;
}

static int
lto_global_bindings_p (void) 
{
  return cfun == NULL;
}

static void
lto_set_decl_assembler_name (tree decl)
{
  /* This is almost the same as lhd_set_decl_assembler_name, except that
     we need to uniquify file-scope names, even if they are not
     TREE_PUBLIC, to avoid conflicts between individual files.  */
  tree id;

  if (TREE_PUBLIC (decl))
    id = targetm.mangle_decl_assembler_name (decl, DECL_NAME (decl));
  else
    {
      const char *name = IDENTIFIER_POINTER (DECL_NAME (decl));
      char *label;

      ASM_FORMAT_PRIVATE_NAME (label, name, DECL_UID (decl));
      id = get_identifier (label);
    }

  SET_DECL_ASSEMBLER_NAME (decl, id);
}

static tree
lto_pushdecl (tree t ATTRIBUTE_UNUSED)
{
  /* Do nothing, since we get all information from DWARF and LTO
     sections.  */
  return NULL_TREE;
}

static tree
lto_getdecls (void)
{
  return registered_builtin_fndecls;
}

static void
lto_write_globals (void)
{
  tree *vec = VEC_address (tree, lto_global_var_decls);
  int len = VEC_length (tree, lto_global_var_decls);
  wrapup_global_declarations (vec, len);
  emit_debug_global_declarations (vec, len);
  VEC_free (tree, gc, lto_global_var_decls);
}

static tree
lto_builtin_function (tree decl)
{
  /* Record it.  */
  TREE_CHAIN (decl) = registered_builtin_fndecls;
  registered_builtin_fndecls = decl;

  return decl;
}

static void
lto_register_builtin_type (tree type, const char *name)
{
  tree decl;

  decl = build_decl (UNKNOWN_LOCATION, TYPE_DECL, get_identifier (name), type);
  DECL_ARTIFICIAL (decl) = 1;
  if (!TYPE_NAME (type))
    TYPE_NAME (type) = decl;

  registered_builtin_types = tree_cons (0, type, registered_builtin_types);
}

/* Build nodes that would have be created by the C front-end; necessary
   for including builtin-types.def and ultimately builtins.def.  */

static void
lto_build_c_type_nodes (void)
{
  gcc_assert (void_type_node);

  void_list_node = build_tree_list (NULL_TREE, void_type_node);
  string_type_node = build_pointer_type (char_type_node);
  const_string_type_node
    = build_pointer_type (build_qualified_type (char_type_node, TYPE_QUAL_CONST));

  if (strcmp (SIZE_TYPE, "unsigned int") == 0)
    {
      intmax_type_node = integer_type_node;
      uintmax_type_node = unsigned_type_node;
      signed_size_type_node = integer_type_node;
    }
  else if (strcmp (SIZE_TYPE, "long unsigned int") == 0)
    {
      intmax_type_node = long_integer_type_node;
      uintmax_type_node = long_unsigned_type_node;
      signed_size_type_node = long_integer_type_node;
    }
  else
    gcc_unreachable ();

  wint_type_node = unsigned_type_node;
  pid_type_node = integer_type_node;
}


/* Perform LTO-specific initialization.  */

static bool
lto_init (void)
{
  /* We need to generate LTO if running in WPA mode.  */
  flag_generate_lto = flag_wpa;

  /* Initialize libcpp line maps for gcc_assert to work.  */
  linemap_add (line_table, LC_RENAME, 0, NULL, 0);
  linemap_add (line_table, LC_RENAME, 0, NULL, 0);

  /* Create the basic integer types.  */
  build_common_tree_nodes (flag_signed_char, /*signed_sizetype=*/false);

  /* Share char_type_node with whatever would be the default for the target.
     char_type_node will be used for internal types such as
     va_list_type_node but will not be present in the lto stream.  */
  /* ???  This breaks the more common case of consistent but non-standard
     setting of flag_signed_char, so share according to flag_signed_char.
     See PR42528.  */
  char_type_node
    = flag_signed_char ? signed_char_type_node : unsigned_char_type_node;

  /* Tell the middle end what type to use for the size of objects.  */
  if (strcmp (SIZE_TYPE, "unsigned int") == 0)
    {
      set_sizetype (unsigned_type_node);
      size_type_node = unsigned_type_node;
    }
  else if (strcmp (SIZE_TYPE, "long unsigned int") == 0)
    {
      set_sizetype (long_unsigned_type_node);
      size_type_node = long_unsigned_type_node;
    }
  else
    gcc_unreachable ();

  /* The global tree for the main identifier is filled in by
     language-specific front-end initialization that is not run in the
     LTO back-end.  It appears that all languages that perform such
     initialization currently do so in the same way, so we do it here.  */
  if (main_identifier_node == NULL_TREE)
    main_identifier_node = get_identifier ("main");

  /* In the C++ front-end, fileptr_type_node is defined as a variant
     copy of of ptr_type_node, rather than ptr_node itself.  The
     distinction should only be relevant to the front-end, so we
     always use the C definition here in lto1.  */
  gcc_assert (fileptr_type_node == ptr_type_node);

  ptrdiff_type_node = integer_type_node;

  /* Create other basic types.  */
  build_common_tree_nodes_2 (/*short_double=*/false);
  lto_build_c_type_nodes ();
  gcc_assert (va_list_type_node);

  if (TREE_CODE (va_list_type_node) == ARRAY_TYPE)
    {
      tree x = build_pointer_type (TREE_TYPE (va_list_type_node));
      lto_define_builtins (x, x);
    }
  else
    {
      lto_define_builtins (va_list_type_node,
			   build_reference_type (va_list_type_node));
    }

  targetm.init_builtins ();
  build_common_builtin_nodes ();

  /* Initialize LTO-specific data structures.  */
  lto_global_var_decls = VEC_alloc (tree, gc, 256);
  in_lto_p = true;

  return true;
}

/* Initialize tree structures required by the LTO front end.  */

static void lto_init_ts (void)
{
  tree_contains_struct[NAMESPACE_DECL][TS_DECL_MINIMAL] = 1;
}

#undef LANG_HOOKS_NAME
#define LANG_HOOKS_NAME "GNU GIMPLE"
#undef LANG_HOOKS_INIT_OPTIONS
#define LANG_HOOKS_INIT_OPTIONS lto_init_options
#undef LANG_HOOKS_HANDLE_OPTION
#define LANG_HOOKS_HANDLE_OPTION lto_handle_option
#undef LANG_HOOKS_POST_OPTIONS
#define LANG_HOOKS_POST_OPTIONS lto_post_options
#undef LANG_HOOKS_GET_ALIAS_SET
#define LANG_HOOKS_GET_ALIAS_SET gimple_get_alias_set
#undef LANG_HOOKS_TYPE_FOR_MODE
#define LANG_HOOKS_TYPE_FOR_MODE lto_type_for_mode
#undef LANG_HOOKS_TYPE_FOR_SIZE
#define LANG_HOOKS_TYPE_FOR_SIZE lto_type_for_size
#undef LANG_HOOKS_SET_DECL_ASSEMBLER_NAME
#define LANG_HOOKS_SET_DECL_ASSEMBLER_NAME lto_set_decl_assembler_name
#undef LANG_HOOKS_GLOBAL_BINDINGS_P
#define LANG_HOOKS_GLOBAL_BINDINGS_P lto_global_bindings_p
#undef LANG_HOOKS_PUSHDECL
#define LANG_HOOKS_PUSHDECL lto_pushdecl
#undef LANG_HOOKS_GETDECLS
#define LANG_HOOKS_GETDECLS lto_getdecls
#undef LANG_HOOKS_WRITE_GLOBALS
#define LANG_HOOKS_WRITE_GLOBALS lto_write_globals
#undef LANG_HOOKS_REGISTER_BUILTIN_TYPE
#define LANG_HOOKS_REGISTER_BUILTIN_TYPE lto_register_builtin_type
#undef LANG_HOOKS_BUILTIN_FUNCTION
#define LANG_HOOKS_BUILTIN_FUNCTION lto_builtin_function
#undef LANG_HOOKS_INIT
#define LANG_HOOKS_INIT lto_init
#undef LANG_HOOKS_PARSE_FILE
#define LANG_HOOKS_PARSE_FILE lto_main
#undef LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION
#define LANG_HOOKS_CALLGRAPH_EXPAND_FUNCTION tree_rest_of_compilation
#undef LANG_HOOKS_REDUCE_BIT_FIELD_OPERATIONS
#define LANG_HOOKS_REDUCE_BIT_FIELD_OPERATIONS true
#undef LANG_HOOKS_TYPES_COMPATIBLE_P
#define LANG_HOOKS_TYPES_COMPATIBLE_P NULL

/* Attribute hooks.  */
#undef LANG_HOOKS_COMMON_ATTRIBUTE_TABLE
#define LANG_HOOKS_COMMON_ATTRIBUTE_TABLE lto_attribute_table
#undef LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE
#define LANG_HOOKS_FORMAT_ATTRIBUTE_TABLE lto_format_attribute_table

#undef LANG_HOOKS_BEGIN_SECTION
#define LANG_HOOKS_BEGIN_SECTION lto_obj_begin_section
#undef LANG_HOOKS_APPEND_DATA
#define LANG_HOOKS_APPEND_DATA lto_obj_append_data
#undef LANG_HOOKS_END_SECTION
#define LANG_HOOKS_END_SECTION lto_obj_end_section

#undef LANG_HOOKS_INIT_TS
#define LANG_HOOKS_INIT_TS lto_init_ts

struct lang_hooks lang_hooks = LANG_HOOKS_INITIALIZER;

/* Language hooks that are not part of lang_hooks.  */

tree
convert (tree type ATTRIBUTE_UNUSED, tree expr ATTRIBUTE_UNUSED)
{
  gcc_unreachable ();
}

/* Tree walking support.  */

static enum lto_tree_node_structure_enum
lto_tree_node_structure (union lang_tree_node *t ATTRIBUTE_UNUSED)
{
  return TS_LTO_GENERIC;
}

#include "ggc.h"
#include "gtype-lto.h"
#include "gt-lto-lto-lang.h"
