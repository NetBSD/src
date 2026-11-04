/* Object attributes parsing.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street - Fifth Floor, Boston, MA
   02110-1301, USA.  */

#include "obj-elf-attr.h"

#ifdef TC_OBJ_ATTR

#include "obstack.h"
#include "safe-ctype.h"

/* A variant type to store information about known OAv2 identifiers.  */
typedef union {
  uint8_t u8;
  bool b;
} oav2_identifier_variant_value_t;

typedef enum {
  OAv2_ASM_ID_VALUE_UNDEFINED = 0,
  OAv2_ASM_ID_VALUE_U8,
  OAv2_ASM_ID_VALUE_BOOL,
} oav2_identifier_variant_type_info_t;

typedef struct {
  oav2_identifier_variant_value_t val;
  oav2_identifier_variant_type_info_t vtype;
} oav2_identifier_variant_t;

typedef struct {
  const char *const name;
  const oav2_identifier_variant_t value;
} oav2_identifier_t;

typedef struct {
  size_t len;
  struct arg_variant_t *elts;
} arg_variant_list_t;

typedef union {
  const char *string;
  uint64_t u64;
  int64_t i64;
  arg_variant_list_t list;
} arg_variant_value_t;

typedef enum {
  VALUE_UNDEFINED = 0,
  VALUE_U64,
  VALUE_I64,
  VALUE_UNSIGNED_INTEGER = VALUE_U64,
  VALUE_SIGNED_INTEGER = VALUE_I64,
  VALUE_STRING,
  VALUE_LIST,
  VALUE_OPTIONAL_ABSENT,
} arg_variant_type_info_t;

/* A variant type to store the argument values of an assembly directive.  */
typedef struct arg_variant_t {
  arg_variant_value_t val;
  arg_variant_type_info_t vtype;
} arg_variant_t;

typedef arg_variant_t arg_t;

#define skip_whitespace(str)  do { if (is_whitespace (*(str))) ++(str); } while (0)

static inline bool
skip_past_char (char **str, char c)
{
  if (**str == c)
    {
      (*str)++;
      return true;
    }
  return false;
}
#define skip_past_comma(str) skip_past_char (str, ',')

#if (TC_OBJ_ATTR_v1)

/* A list of attributes that have been explicitly set by the assembly code.
   VENDOR is the vendor id, BASE is the tag shifted right by the number
   of bits in MASK, and bit N of MASK is set if tag BASE+N has been set.  */
typedef struct recorded_attribute_info {
  struct recorded_attribute_info *next;
  obj_attr_vendor_t vendor;
  unsigned int base;
  unsigned long mask;
} recorded_attribute_info_t;
static recorded_attribute_info_t *recorded_attributes;

static void
oav1_attr_info_free (recorded_attribute_info_t *node)
{
  while (node != NULL)
    {
      recorded_attribute_info_t *next = node->next;
      free (node);
      node = next;
    }
}

void
oav1_attr_info_init (void)
{
  /* Note: this "constructor" was added for symmetry with oav1_attr_info_exit.
     recorded_attributes is a static variable which is automatically initialized
     to NULL.  There is no need to initialize it another time except for a
     cosmetic reason and to possibly help fuzzing.  */
  recorded_attributes = NULL;
}

void
oav1_attr_info_exit (void)
{
  oav1_attr_info_free (recorded_attributes);
}

/* Record that we have seen an explicit specification of attribute TAG
   for vendor VENDOR.  */

static void
oav1_attr_record_seen (obj_attr_vendor_t vendor, obj_attr_tag_t tag)
{
  unsigned int base;
  unsigned long mask;
  recorded_attribute_info_t *rai;

  base = tag / (8 * sizeof (rai->mask));
  mask = 1UL << (tag % (8 * sizeof (rai->mask)));
  for (rai = recorded_attributes; rai; rai = rai->next)
    if (rai->vendor == vendor && rai->base == base)
      {
	rai->mask |= mask;
	return;
      }

  rai = XNEW (recorded_attribute_info_t);
  rai->next = recorded_attributes;
  rai->vendor = vendor;
  rai->base = base;
  rai->mask = mask;
  recorded_attributes = rai;
}

/* Return true if we have seen an explicit specification of attribute TAG
   for vendor VENDOR.  */

bool
oav1_attr_seen (obj_attr_vendor_t vendor, obj_attr_tag_t tag)
{
  unsigned int base;
  unsigned long mask;
  recorded_attribute_info_t *rai;

  base = tag / (8 * sizeof (rai->mask));
  mask = 1UL << (tag % (8 * sizeof (rai->mask)));
  for (rai = recorded_attributes; rai; rai = rai->next)
    if (rai->vendor == vendor && rai->base == base)
      return (rai->mask & mask) != 0;
  return false;
}

#endif /* TC_OBJ_ATTR_v1 */

/* Expected argument tokens for object attribute directives.  */
typedef enum {
  /* Base types.  */
  IDENTIFIER = 0x1,
  UNSIGNED_INTEGER = 0x2,
  SIGNED_INTEGER = 0x4,
  STRING = 0x8,
  LIST = 0x10,
  LT_MASK = 0xFF,
  /* Higher types.  */
  SUBSECTION_NAME = 0x100,
  SUBSECTION_OPTION_1 = 0x200,
  SUBSECTION_OPTION_2 = 0x400,
  ATTRIBUTE_KEY = 0x800,
  ATTRIBUTE_VALUE = 0x1000,
  HT_MASK = 0xFF00,
} arg_token_t;

/* Free an arguments list of size N.  */
static void
args_list_free (arg_t *args, size_t n)
{
  for (size_t i = 0; i < n; ++i)
    if (args[i].vtype == VALUE_STRING)
      free ((void *) args[i].val.string);
    else if (args[i].vtype == VALUE_LIST)
      args_list_free (args[i].val.list.elts, args[i].val.list.len);
  free (args);
}

/* Extract a string literal from the input.  */
static bool
extract_string_literal (arg_t *arg_out)
{
  int len;
  char *obstack_buf = demand_copy_C_string (&len);
  if (obstack_buf != NULL)
    {
      arg_out->val.string = xstrdup (obstack_buf);
      obstack_free (&notes, obstack_buf);
      arg_out->vtype = VALUE_STRING;
      return true;
    }

  arg_out->val.string = NULL;
  return false;
}

/* Extract an integer literal from the input.
   Anything matched by O_constant is considered an integer literal (see the
   usage of O_constant in expr.c to see all the matches.
   Return true on success, false otherwise. If a signedness issue is detected,
   'signedness_issue' is also set to true.  */
static bool
extract_integer_literal (arg_t *arg_out,
			 bool want_unsigned,
			 bool *signedness_issue)
{
  const char *cursor_begin = input_line_pointer;
  expressionS exp;
  expression_and_evaluate (&exp);
  if (exp.X_op != O_constant)
    {
      char backup_c = *input_line_pointer;
      *input_line_pointer = '\0';
      as_bad (_("expression '%s' does not resolve to an integer"),
	      cursor_begin);
      /* Restore the character pointed by the current cursor position,
	 otherwise '\0' misleads ignore_rest_of_line().  */
      *input_line_pointer = backup_c;
      return false;
    }

  int64_t val = (int64_t) exp.X_add_number;
  if (want_unsigned)
    {
      if (! exp.X_unsigned && val < 0)
	{
	  as_bad (_("unexpected value '%" PRId64 "', expected `unsigned integer' instead"),
		  val);
	  *signedness_issue = true;
	  return false;
	}
      arg_out->val.u64 = val;
      arg_out->vtype = VALUE_UNSIGNED_INTEGER;
    }
  else
    {
      arg_out->val.i64 = val;
      arg_out->vtype = VALUE_SIGNED_INTEGER;
    }
  return true;
}

/* Extract an identifier based on the provided character matcher.  */
static bool
extract_identifier (bool (*char_predicate) (char), arg_t *arg_out)
{
  const char *s = input_line_pointer;
  unsigned int i = 0;
  for (; char_predicate (*input_line_pointer); ++input_line_pointer)
    i++;
  if (i == 0)
    {
      as_bad (_("invalid character '%c' in identifier"),
	      *input_line_pointer);
      return false;
    }

  arg_out->vtype = VALUE_STRING;
  arg_out->val.string = xmemdup0 (s, i);
  return true;
}

#if (TC_OBJ_ATTR_v2)
/* Resolve the identifier if it matches a known tag.  */
static bool
resolve_if_matching_known_tag (const char *identifier,
			       const obj_attr_tag_info_t *known_identifier,
			       arg_t *val_out)
{
  if (strcmp (known_identifier->name, identifier) != 0)
    return false;

  val_out->val.u64 = known_identifier->value;
  val_out->vtype = VALUE_UNSIGNED_INTEGER;
  return true;
}

/* Resolve the identifier if it matches the known one.  */
static bool
resolve_if_matching (const char *identifier,
		     const oav2_identifier_t *known_identifier,
		     arg_t *val_out)
{
  /* Lowercase the identifier before comparison.  */
  char normalized_identifier[100];
  unsigned int i = 0;
  for (; i < sizeof (normalized_identifier) && identifier[i] != '\0'; ++i)
    normalized_identifier[i] = TOLOWER (identifier[i]);
  if (i >= sizeof (normalized_identifier))
    /* Identifier is too long.  */
    return false;
  gas_assert (identifier[i] == '\0');
  normalized_identifier[i] = '\0';

  /* Comparison with normalized identifier.  */
  if (strcmp (known_identifier->name, normalized_identifier) != 0)
    return false;

  /* We only need U8 and Bool for now.  */
  switch (known_identifier->value.vtype)
    {
    case OAv2_ASM_ID_VALUE_U8:
      val_out->val.u64 = known_identifier->value.val.b;
      val_out->vtype = VALUE_UNSIGNED_INTEGER;
      break;
    case OAv2_ASM_ID_VALUE_BOOL:
      val_out->val.u64 = known_identifier->value.val.u8;
      val_out->vtype = VALUE_UNSIGNED_INTEGER;
      break;
    default:
      abort ();
    }

  return true;
}
#endif /* TC_OBJ_ATTR_v2 */

#if (TC_OBJ_ATTR_v1)
/* Look up attribute tags defined in the backend (object attribute v1).  */
static bool
obj_attr_v1_lookup_known_attr_tag_symbol
  (const char *identifier ATTRIBUTE_UNUSED,
   arg_token_t token_type,
   arg_t *val_out)
{
  gas_assert (token_type & UNSIGNED_INTEGER);

#ifndef CONVERT_SYMBOLIC_ATTRIBUTE
# define CONVERT_SYMBOLIC_ATTRIBUTE(a) -1
#endif
  int tag = CONVERT_SYMBOLIC_ATTRIBUTE (identifier);
  if (tag < 0)
    return false;
  val_out->val.u64 = tag;
  val_out->vtype = VALUE_UNSIGNED_INTEGER;
  return true;
}
#endif /* TC_OBJ_ATTR_v1 */

#if (TC_OBJ_ATTR_v2)
/* Look up attribute tags defined in the backend (object attribute v2).  */
static bool
obj_attr_v2_lookup_known_attr_tag_symbol (const char *identifier,
					  arg_token_t token_type,
					  arg_t *val_out)
{
  obj_attr_subsection_v2_t *subsec = elf_obj_attr_subsections (stdoutput).last;
  /* This function is called when setting a value for an attribute, and it
     requires an active subsection.  Calling this function without setting
     'subsec' is a logical error.  Invalid user code, where the subsection
     has not been selected must be handled by the caller.  We require 'subsec'
     to be non-NULL here.  */
  gas_assert (subsec != NULL);

  /* An attribute tag is an unsigned integer, so the expected token type should
     always have the base type UNSIGNED_INTEGER.  Otherwise, this function was
     called incorrectly.  */
  gas_assert (token_type & UNSIGNED_INTEGER);

  bool resolved = false;
  const struct elf_backend_data *be = get_elf_backend_data (stdoutput);
  const known_subsection_v2_t *known_subsec
    = bfd_obj_attr_v2_identify_subsection (be, subsec->name);
  if (known_subsec != NULL)
    {
      for (size_t i = 0; i < known_subsec->len && ! resolved; ++i)
	resolved
	  = resolve_if_matching_known_tag (identifier,
					   &known_subsec->known_attrs[i].tag,
					   val_out);
    }

  if (resolved)
    /* An attribute tag is an unsigned integer, so the type of the found value
       should be VALUE_UNSIGNED_INTEGER.  Otherwise, check if you set correctly
       the type of the value associated to the symbol.  */
    gas_assert (val_out->vtype == VALUE_UNSIGNED_INTEGER);

  return resolved;
}
#endif /* TC_OBJ_ATTR_v2 */

/* Look up known symbols, and try to resolve the given identifier.  */
static bool
lookup_known_symbols (const char *identifier,
		      arg_token_t token_type,
		      arg_t *val_out)
{
  if (identifier == NULL)
    return false;

  arg_token_t high_ttype = (token_type & HT_MASK);

#if (TC_OBJ_ATTR_v2)
  static const oav2_identifier_t known_identifiers_subsection_optional[] = {
    { "optional", {
	.val.b = true,
	.vtype = OAv2_ASM_ID_VALUE_BOOL
      }
    },
    { "required", {
	.val.b = false,
	.vtype = OAv2_ASM_ID_VALUE_BOOL
      }
    },
  };

  static const oav2_identifier_t known_identifiers_subsection_encoding[] = {
    { "uleb128", {
	.val.u8 = obj_attr_encoding_v2_to_u8 (OA_ENC_ULEB128),
	.vtype = OAv2_ASM_ID_VALUE_U8
      }
    },
    { "ntbs", {
	.val.u8 = obj_attr_encoding_v2_to_u8 (OA_ENC_NTBS),
	.vtype = OAv2_ASM_ID_VALUE_U8
      }
    },
  };
#endif /* TC_OBJ_ATTR_v2 */

  bool resolved = false;

#if (TC_OBJ_ATTR_v2)
  if (high_ttype == SUBSECTION_OPTION_1 || high_ttype == SUBSECTION_OPTION_2)
    {
      const oav2_identifier_t *known_identifiers
	= (high_ttype == SUBSECTION_OPTION_1
	   ? known_identifiers_subsection_optional
	   : known_identifiers_subsection_encoding);
      const size_t N_identifiers
	= (high_ttype == SUBSECTION_OPTION_1
	   ? ARRAY_SIZE (known_identifiers_subsection_optional)
	   : ARRAY_SIZE (known_identifiers_subsection_encoding));

      for (size_t i = 0; i < N_identifiers && ! resolved; ++i)
	resolved = resolve_if_matching (identifier,
					&known_identifiers[i],
					val_out);
    }
  else
#endif /* TC_OBJ_ATTR_v2 */
  if (high_ttype == ATTRIBUTE_KEY)
    {
      obj_attr_version_t version = elf_obj_attr_version (stdoutput);
      switch (version)
	{
#if (TC_OBJ_ATTR_v1)
	case OBJ_ATTR_V1:
	  resolved = obj_attr_v1_lookup_known_attr_tag_symbol
	    (identifier, token_type, val_out);
	  break;
#endif /* TC_OBJ_ATTR_v1 */
#if (TC_OBJ_ATTR_v2)
	case OBJ_ATTR_V2:
	  resolved = obj_attr_v2_lookup_known_attr_tag_symbol
	    (identifier, token_type, val_out);
	  break;
#endif /* TC_OBJ_ATTR_v2 */
	default:
	  abort ();
	}
    }
  else
    abort ();

  return resolved;
}

/* Look up the symbol table of this compilation unit, and try to resolve the
   given identifier.  */
static bool
lookup_symbol_table (const char *identifier,
		     const arg_token_t expected_ttype,
		     arg_t *val_out)
{
  if (identifier == NULL)
    return false;

  /* Note: signed integer are unsupported for now.  */
  gas_assert (expected_ttype & UNSIGNED_INTEGER);

  symbolS *symbolP = symbol_find (identifier);
  if (symbolP == NULL)
    return false;

  if (! S_IS_DEFINED (symbolP))
    return false;

  valueT val = S_GET_VALUE (symbolP);
  val_out->val.u64 = val;
  val_out->vtype = VALUE_UNSIGNED_INTEGER;

  return true;
}

/* Function similar to snprintf from the standard library, except that it
   also updates the buffer pointer to point to the last written character,
   and length to match the remaining space in the buffer.
   Return the number of bytes printed.  The function is assumed to always be
   successful, and a failure with vnsprintf() will trigger an assert().  */
static size_t
snprintf_append (char **sbuffer, size_t *length,
		 const char *format, ...)
{
  va_list args;
  va_start (args, format);
  int rval = vsnprintf (*sbuffer, *length, format, args);
  va_end (args);

  gas_assert (rval >= 0);
  size_t written = rval;
  *length -= written;
  *sbuffer += written;
  return written;
}

/* Return the list of comma-separated strings matching the expected types
   (i.e. the flags set in low_ttype).  */
static const char *
expectations_to_string (const arg_token_t low_ttype,
			char *sbuffer, size_t length)
{
  unsigned match_n = 0;
  char *sbuffer_start = sbuffer;
  size_t total = 0;
  if (low_ttype & IDENTIFIER)
    {
      ++match_n;
      total += snprintf_append (&sbuffer, &length, "`%s'", "identifier");
    }

#define EXP_APPEND_TYPE_STR(type_flag, type_str)			\
  if (low_ttype & type_flag)						\
    {									\
      if (match_n >= 1)							\
	total += snprintf_append (&sbuffer, &length, "%s", ", or ");	\
      ++match_n;							\
      total += snprintf_append (&sbuffer, &length, "`%s'", type_str);	\
    }

  EXP_APPEND_TYPE_STR (STRING, "string");
  EXP_APPEND_TYPE_STR (UNSIGNED_INTEGER, "unsigned integer");
  EXP_APPEND_TYPE_STR (SIGNED_INTEGER, "signed integer");
#undef APPEND_TYPE_STR

  gas_assert (total <= length);
  return sbuffer_start;
}

/* In the context of object attributes, an identifier is defined with the
   following lexical constraint: [a-zA-z_][a-zA-Z0-9_]*.  An identifier can
   be used to define the name of a subsection, its optionality, or its
   encoding, as well as for an attribute's tag.  */

static bool
is_identifier_beginner (char c)
{
  return ISALPHA (c) || c == '_';
}

static bool
is_part_of_identifier (char c)
{
  return ISALNUM (c) || c == '_';
}

/* Parse an argument, and set its type accordingly depending on the input
   value, and the constraints on the expected argument.  */
static bool
obj_attr_parse_arg (arg_token_t expected_ttype,
		    bool resolve_identifier,
		    bool optional,
		    arg_t *arg_out)
{
  const arg_token_t low_ttype = (expected_ttype & LT_MASK);

  if (optional && is_end_of_stmt (*input_line_pointer))
    {
      arg_out->vtype = VALUE_OPTIONAL_ABSENT;
      return true;
    }

  /* Check whether this looks like a string literal
     Note: symbol look-up for string literals is not available.  */
  if (*input_line_pointer == '"')
    {
      bool status = extract_string_literal (arg_out);
      if (status && (low_ttype & STRING))
	return true;

      if (status)
	{
	  char sbuffer[100];
	  as_bad (_("unexpected `string' \"%s\", expected %s instead"),
		  arg_out->val.string,
		  expectations_to_string (low_ttype, sbuffer, sizeof(sbuffer)));
	  free ((char *) arg_out->val.string);
	  arg_out->val.string = NULL;
	  arg_out->vtype = VALUE_UNDEFINED;
	}
      return false;
    }

  /* Check whether this looks like an identifier.  */
  if (is_identifier_beginner (*input_line_pointer))
    {
      bool status = extract_identifier (is_part_of_identifier, arg_out);
      /* is_identifier_beginner() confirmed that it was the beginning of an
	 identifier, so we don't expect the extraction to fail.  */
      gas_assert (status);
      gas_assert (arg_out->vtype == VALUE_STRING);

      if (! (low_ttype & IDENTIFIER))
	{
	  char sbuffer[100];
	  as_bad (_("unexpected `identifier' \"%s\", expected %s instead"),
		  arg_out->val.string,
		  expectations_to_string (low_ttype, sbuffer, sizeof(sbuffer)));
	  free ((char *) arg_out->val.string);
	  arg_out->val.string = NULL;
	  arg_out->vtype = VALUE_UNDEFINED;
	  return false;
	}

      /* In some cases, we don't want to resolve the identifier because it is the
	 actual value.  */
      if (! resolve_identifier)
	return true;

      /* Move the identifier out of arg_out.  */
      const char *identifier = arg_out->val.string;
      arg_out->val.string = NULL;
      arg_out->vtype = VALUE_UNDEFINED;
      bool resolved = true;

      /* The identifier is a symbol, let's try to resolve it by:
	 1. using the provided list of known symbols.
	   a) backend-independent
	   b) backend-specific.  */
      if (lookup_known_symbols (identifier, expected_ttype, arg_out))
	goto free_identifier;

      /* 2. using the symbol table for this compilation unit.
	 Note: this is the last attempt before failure.  */
      if (lookup_symbol_table (identifier, low_ttype, arg_out))
	goto free_identifier;

      as_bad (_("unknown identifier '%s' in this context"), identifier);
      resolved = false;

 free_identifier:
      free ((char *) identifier);
      return resolved;
    }

  /* If it is neither a string nor an identifier, it must be an expression.  */
  bool signedness_issue = false;
  bool success = extract_integer_literal (arg_out,
					  (low_ttype & UNSIGNED_INTEGER),
					  &signedness_issue);
  if (success && (low_ttype & (UNSIGNED_INTEGER | SIGNED_INTEGER)))
    return true;

  char sbuffer[100];
  if (success)
    as_bad (_("unexpected integer '%"PRIu64"', expected %s instead"),
	    arg_out->val.u64,
	    expectations_to_string (low_ttype, sbuffer, sizeof(sbuffer)));
  else if ((low_ttype & UNSIGNED_INTEGER) && signedness_issue)
    {
      /* Already handled by extract_integer_literal(), nothing to do.  */
    }
  else
    as_bad (_("fell back to integer literal extraction from expression, "
	      "but expected %s instead"),
	    expectations_to_string (low_ttype, sbuffer, sizeof(sbuffer)));

  arg_out->vtype = VALUE_UNDEFINED;
  return false;
}

/* Trim white space before a parameter.
   Error if it meets a parameter separator before a parameter.  */
static bool
trim_whitespace_before_param (void)
{
  skip_whitespace (input_line_pointer);
  if (*input_line_pointer == ',')
    {
      as_bad (_("syntax error, comma not expected here"));
      return false;
    }
  return true;
}

/* Skip white space + parameter separator after a parameter.
   Error if it does not meet a parameter separator after a parameter.  */
static bool
skip_whitespace_past_comma (void)
{
  skip_whitespace (input_line_pointer);
  if (! skip_past_comma (&input_line_pointer))
    {
      as_bad (_("syntax error, comma missing here"));
      return false;
    }
  return true;
}

/* Can parse a list of arguments with variable length.  */
static bool
obj_attr_parse_args (arg_token_t expected_ttype,
		     bool resolve_identifier,
		     arg_t *arg_out)
{
  if ((expected_ttype & LIST) == 0)
    return obj_attr_parse_arg (expected_ttype, resolve_identifier, false,
			       arg_out);

  static const size_t LIST_MAX_SIZE = 2;
  arg_t *arg_list = xcalloc (LIST_MAX_SIZE, sizeof (*arg_list));

  /* We don't want to support recursive lists.  */
  expected_ttype &= ~LIST;

  size_t n = 0;
  do {
    if (! trim_whitespace_before_param ())
      goto bad;

    if (! obj_attr_parse_arg (expected_ttype, resolve_identifier, false,
			      &arg_list[n]))
      goto bad;

    ++n;
    skip_whitespace (input_line_pointer);
    if (is_end_of_stmt (*input_line_pointer))
      break;

    if (! skip_whitespace_past_comma ())
      goto bad;

    if (n >= LIST_MAX_SIZE)
      {
	as_bad ("too many arguments for a list (max: %zu)", LIST_MAX_SIZE);
	goto bad;
      }
  } while (n < LIST_MAX_SIZE);

  arg_out->vtype = VALUE_LIST;
  arg_out->val.list.len = n;
  arg_out->val.list.elts = arg_list;
  return true;

 bad:
  args_list_free (arg_list, n);
  return false;
}

#if (TC_OBJ_ATTR_v2)
static bool
is_valid_boolean (uint64_t value)
{
  return value <= 1;
}

#define is_valid_comprehension is_valid_boolean

static bool
is_valid_encoding (uint64_t value)
{
  value = obj_attr_encoding_v2_from_u8 (value);
  return OA_ENC_UNSET < value && value <= OA_ENC_MAX;
}

#endif /* TC_OBJ_ATTR_v2 */

#if (TC_OBJ_ATTR_v1)
/* Determine the expected argument type based on the tag ID.  */
static arg_token_t
obj_attr_v1_get_arg_type (bfd *abfd,
			  obj_attr_vendor_t vendor,
			  obj_attr_tag_t tag)
{
  int attr_type = bfd_elf_obj_attrs_arg_type (abfd, vendor, tag);
  arg_token_t arg_type;
  if (attr_type == (ATTR_TYPE_FLAG_STR_VAL | ATTR_TYPE_FLAG_INT_VAL))
    arg_type = LIST | UNSIGNED_INTEGER | STRING;
  else if (attr_type == ATTR_TYPE_FLAG_STR_VAL)
    arg_type = STRING;
  else
    /* Covers the remaning cases:
       - ATTR_TYPE_FLAG_INT_VAL.
       - ATTR_TYPE_FLAG_INT_VAL | ATTR_TYPE_FLAG_NO_DEFAULT.  */
    arg_type = UNSIGNED_INTEGER;
  return arg_type;
}
#endif /* TC_OBJ_ATTR_v1 */

#if (TC_OBJ_ATTR_v2)
/* Determine the expected argument type based on the subsection encoding.  */
static arg_token_t
obj_attr_v2_get_arg_type (obj_attr_encoding_v2_t subsec_encoding)
{
  arg_token_t arg_type;
  switch (subsec_encoding)
    {
    case OA_ENC_ULEB128:
      arg_type = UNSIGNED_INTEGER;
      break;
    case OA_ENC_NTBS:
      arg_type = STRING;
      break;
    case OA_ENC_UNSET:
    default:
      abort ();
    }
  return arg_type;
}
#endif /* TC_OBJ_ATTR_v2 */

/* Parse the arguments of [vendor]_attribute directive.  */
static arg_t *
vendor_attribute_parse_args (obj_attr_vendor_t vendor ATTRIBUTE_UNUSED,
			     const obj_attr_subsection_v2_t *subsec ATTRIBUTE_UNUSED,
			     unsigned int nargs, ...)
{
  va_list args;
  va_start (args, nargs);

  arg_t *args_out = xcalloc (nargs, sizeof (arg_t));

  for (unsigned int n = 0; n < nargs; ++n)
    {
      if (! trim_whitespace_before_param ())
	goto bad;

      arg_t *arg_out = &args_out[n];

      arg_token_t expected_ttype = va_arg (args, arg_token_t);
      arg_token_t high_ttype = (expected_ttype & HT_MASK);
      /* Make sure that we called the right parse_args().  */
      gas_assert (high_ttype == ATTRIBUTE_KEY
	       || high_ttype == ATTRIBUTE_VALUE);

      if (high_ttype == ATTRIBUTE_VALUE)
	{
	  arg_token_t type_attr_value
#if (TC_OBJ_ATTR_v1 && TC_OBJ_ATTR_v2)
	    = (subsec != NULL)
	      ? obj_attr_v2_get_arg_type (subsec->encoding)
	      : obj_attr_v1_get_arg_type (stdoutput, vendor,
					  args_out[n - 1].val.u64);
#elif (TC_OBJ_ATTR_v1)
	    = obj_attr_v1_get_arg_type (stdoutput, vendor,
					args_out[n - 1].val.u64);
#else /* TC_OBJ_ATTR_v2 */
	    = obj_attr_v2_get_arg_type (subsec->encoding);
#endif
	  expected_ttype |= type_attr_value;
	}

      if (! obj_attr_parse_args (expected_ttype, true, arg_out))
	{
	  if (high_ttype == ATTRIBUTE_KEY)
	    {
	      as_bad (_("could not parse attribute tag"));
	      goto bad;
	    }
	  else
	    {
	      as_bad (_("could not parse attribute value"));
	      goto bad;
	    }
	}

      if (n + 1 < nargs && !skip_whitespace_past_comma ())
	goto bad;
    }

  demand_empty_rest_of_line ();
  va_end (args);
  return args_out;

 bad:
  ignore_rest_of_line ();
  args_list_free (args_out, nargs);
  va_end (args);
  return NULL;
}

#if (TC_OBJ_ATTR_v1)
/* Record an attribute (object attribute v1 only).  */
static obj_attribute *
obj_attr_v1_record (bfd *abfd,
		    const obj_attr_vendor_t vendor,
		    const obj_attr_tag_t tag,
		    arg_t *parsed_arg)
{
  obj_attribute *attr = bfd_elf_new_obj_attr (abfd, vendor, tag);
  if (attr != NULL)
    {
      attr->type = bfd_elf_obj_attrs_arg_type (abfd, vendor, tag);
      if (parsed_arg->vtype == VALUE_LIST)
	{
	  arg_variant_list_t *plist = &parsed_arg->val.list;
	  gas_assert (plist->len == 2
		      && plist->elts[0].vtype == VALUE_UNSIGNED_INTEGER
		      && plist->elts[1].vtype == VALUE_STRING);
	  attr->i = plist->elts[0].val.u64;
	  attr->s = (char *) plist->elts[1].val.string;
	  plist->elts[1].val.string = NULL;
	}
      else if (parsed_arg->vtype == VALUE_STRING)
	{
	  attr->s = (char *) parsed_arg->val.string;
	  parsed_arg->val.string = NULL;
	}
      else
	{
	  attr->i = parsed_arg->val.u64;
	}
    }
  return attr;
}
#endif /* TC_OBJ_ATTR_v1 */

#if (TC_OBJ_ATTR_v2)
/* Parse the arguments of [vendor]_subsection directive (v2 only).  */
static arg_t *
vendor_subsection_parse_args (unsigned int nargs, ...)
{
  va_list args;
  va_start (args, nargs);

  arg_t *args_out = xcalloc (nargs, sizeof (arg_t));

  for (unsigned int n = 0; n < nargs; ++n)
    {
      if (! trim_whitespace_before_param ())
	goto bad;

      arg_t *arg_out = &args_out[n];

      arg_token_t expected_ttype = va_arg (args, arg_token_t);
      arg_token_t high_ttype = (expected_ttype & HT_MASK);
      /* Make sure that we called the right parse_args().  */
      gas_assert (high_ttype == SUBSECTION_NAME
		  || high_ttype == SUBSECTION_OPTION_1
		  || high_ttype == SUBSECTION_OPTION_2);

      if (high_ttype == SUBSECTION_NAME)
	{
	  if (! obj_attr_parse_arg (expected_ttype, false, false, arg_out))
	    {
	      as_bad (_("expected <subsection_name>, <comprehension>, "
			"<encoding>"));
	      goto bad;
	    }
	}
      else if (high_ttype == SUBSECTION_OPTION_1
	    || high_ttype == SUBSECTION_OPTION_2)
	{
	  if (! obj_attr_parse_arg (expected_ttype, true, true, arg_out))
	    goto bad;

	  if (arg_out->vtype == VALUE_OPTIONAL_ABSENT)
	    continue;

	  if (high_ttype == SUBSECTION_OPTION_1
	      && ! is_valid_comprehension (arg_out->val.u64))
	    {
	      as_bad
		(_("invalid value '%" PRIu64 "', expected values for "
		   "<comprehension> are 0 (=`required') or 1 (=`optional')"),
		 arg_out->val.u64);
	      goto bad;
	    }
	  else if (high_ttype == SUBSECTION_OPTION_2
		&& ! is_valid_encoding (arg_out->val.u64))
	    {
	      as_bad
		(_("invalid value '%" PRIu64 "', expected values for <encoding>"
		   " are 0 (=`ULEB128') or 1 (=`NTBS')"),
		 arg_out->val.u64);
	      goto bad;
	    }
	}
      else
	abort ();

      if (n + 1 < nargs
	  && ! is_end_of_stmt (*input_line_pointer)
	  && ! skip_whitespace_past_comma ())
	goto bad;
    }

  va_end (args);
  demand_empty_rest_of_line ();
  return args_out;

 bad:
  ignore_rest_of_line ();
  args_list_free (args_out, nargs);
  va_end (args);
  return NULL;
}

/* Record an attribute (object attribute v2 only).  */
static void
obj_attr_v2_record (obj_attr_tag_t key, arg_t *arg_val)
{
  /* An OAv2 cannot be recorded unless a subsection has been recorded.  */
  gas_assert (elf_obj_attr_subsections (stdoutput).last != NULL);

  union obj_attr_value_v2 obj_attr_val;
  if (arg_val->vtype == VALUE_UNSIGNED_INTEGER)
    obj_attr_val.uint = arg_val->val.u64;
  else
    {
      /* Move the string.  */
      obj_attr_val.string = arg_val->val.string;
      arg_val->val.string = NULL;
    }

  obj_attr_v2_t *obj_attr = bfd_elf_obj_attr_v2_init (key, obj_attr_val);
  gas_assert (obj_attr != NULL);

  /* Go over the list of already recorded attributes and check for
     redefinitions (which are forbidden).  */
  bool skip_recording = false;
  obj_attr_v2_t *recorded_attr = bfd_obj_attr_v2_find_by_tag
    (elf_obj_attr_subsections (stdoutput).last, obj_attr->tag, false);
  if (recorded_attr != NULL)
    {
      if ((arg_val->vtype == VALUE_UNSIGNED_INTEGER
	   && recorded_attr->val.uint != obj_attr->val.uint)
	  || (arg_val->vtype == VALUE_STRING
	      && strcmp (recorded_attr->val.string, obj_attr->val.string) != 0))
	as_bad (_("attribute '%" PRIu64 "' cannot be redefined"),
		recorded_attr->tag);
      skip_recording = true;
    }

  if (skip_recording)
    {
      if (arg_val->vtype == VALUE_STRING)
	free ((void *) obj_attr->val.string);
      free (obj_attr);
      return;
    }

  bfd_obj_attr_subsection_v2_append
    (elf_obj_attr_subsections (stdoutput).last, obj_attr);
}

/* Record a subsection (object attribute v2 only).
   Note: this function takes the ownership of 'name', so is responsible to free
   it if an issue occurs.  */
static void
obj_attr_v2_subsection_record (const char *name,
			       arg_t *arg_comprehension,
			       arg_t *arg_encoding)
{
  obj_attr_subsection_v2_t *already_recorded_subsec
    = bfd_obj_attr_subsection_v2_find_by_name
      (elf_obj_attr_subsections (stdoutput).first, name, false);

  bool comprehension_optional = arg_comprehension->val.u64;
  obj_attr_encoding_v2_t encoding
    = obj_attr_encoding_v2_from_u8 (arg_encoding->val.u64);

  if (already_recorded_subsec != NULL)
    {
      bool error_redeclaration = false;

      if (arg_comprehension->vtype == VALUE_OPTIONAL_ABSENT)
	gas_assert (arg_encoding->vtype == VALUE_OPTIONAL_ABSENT);
      else if (comprehension_optional != already_recorded_subsec->optional)
	error_redeclaration = true;

      if (arg_encoding->vtype != VALUE_OPTIONAL_ABSENT
	  && encoding != already_recorded_subsec->encoding)
	error_redeclaration = true;

      /* Check for mismatching redefinition of the subsection, i.e. the names
	 match but the properties are different.  */
      if (error_redeclaration)
	{
	  const char *prev_comprehension = bfd_oav2_comprehension_to_string (
	    already_recorded_subsec->optional);
	  const char *prev_encoding = bfd_oav2_encoding_to_string (
	    already_recorded_subsec->encoding);
	  as_bad (_("incompatible redeclaration of subsection %s"), name);
	  as_info (1, _("previous declaration had properties: %s=%s, %s=%s"),
		   "comprehension", prev_comprehension,
		   "encoding", prev_encoding);
	  goto free_name;
	}

      /* Move the existing subsection to the last position.  */
      bfd_obj_attr_subsection_v2_list_remove
	(&elf_obj_attr_subsections (stdoutput), already_recorded_subsec);
      bfd_obj_attr_subsection_v2_list_append
	(&elf_obj_attr_subsections (stdoutput), already_recorded_subsec);
      /* Note: 'name' was unused, and will be freed on exit.  */
    }
  else
    {
      if (arg_comprehension->vtype == VALUE_OPTIONAL_ABSENT
	  || arg_encoding->vtype == VALUE_OPTIONAL_ABSENT)
	{
	  as_bad (_("comprehension and encoding of a subsection cannot be "
		    "omitted on the first declaration"));
	  goto free_name;
	}

      obj_attr_subsection_scope_v2_t scope
	= bfd_elf_obj_attr_subsection_v2_scope (stdoutput, name);

      /* Note: ownership of 'name' is transfered to the callee when initializing
	 the subsection.  That is why we skip free() at the end.  */
      obj_attr_subsection_v2_t *new_subsection
	= bfd_elf_obj_attr_subsection_v2_init (name, scope,
					       comprehension_optional,
					       encoding);
      bfd_obj_attr_subsection_v2_list_append
	(&elf_obj_attr_subsections (stdoutput), new_subsection);
      return;
    }

 free_name:
  free ((void *) name);
}
#endif /* TC_OBJ_ATTR_v2 */

/* Parse an attribute directive (supports both v1 & v2).  */
obj_attr_tag_t
obj_attr_process_attribute (obj_attr_vendor_t vendor)
{
  obj_attr_version_t version = elf_obj_attr_version (stdoutput);
  obj_attr_subsection_v2_t *subsec = NULL;

#if (TC_OBJ_ATTR_v2)
  if (version == OBJ_ATTR_V2)
    {
      subsec = elf_obj_attr_subsections (stdoutput).last;
      if (subsec == NULL)
	{
	  as_bad (_("declaration of an attribute outside the scope of an "
		    "attribute subsection"));
	  ignore_rest_of_line ();
	  return 0;
	}
    }
#endif /* TC_OBJ_ATTR_v2 */

  const size_t N_ARGS = 2;
  arg_t *args = vendor_attribute_parse_args (
    vendor, subsec, N_ARGS,
    ATTRIBUTE_KEY | IDENTIFIER | UNSIGNED_INTEGER,
    ATTRIBUTE_VALUE);

  if (args == NULL)
    return 0;

  obj_attr_tag_t tag = args[0].val.u64;
  switch (version)
    {
#if (TC_OBJ_ATTR_v1)
    case OBJ_ATTR_V1:
      oav1_attr_record_seen (vendor, tag);
      obj_attr_v1_record (stdoutput, vendor, tag, &args[1]);
      break;
#endif /* TC_OBJ_ATTR_v1 */
#if (TC_OBJ_ATTR_v2)
    case OBJ_ATTR_V2:
      obj_attr_v2_record (tag, &args[1]);
      break;
#endif /* TC_OBJ_ATTR_v2 */
    default:
      abort ();
    }

  args_list_free (args, N_ARGS);

  return tag;
}

#if (TC_OBJ_ATTR_v2)
/* Parse an object attribute v2's subsection directive.  */
void
obj_attr_process_subsection (void)
{
  const size_t N_ARGS = 3;
  arg_t *args = vendor_subsection_parse_args (
    N_ARGS,
    SUBSECTION_NAME | IDENTIFIER,
    SUBSECTION_OPTION_1 | IDENTIFIER | UNSIGNED_INTEGER,
    SUBSECTION_OPTION_2 | IDENTIFIER | UNSIGNED_INTEGER);

  if (args == NULL)
    return;

  /* Note: move the value to avoid double free.  */
  const char *name = args[0].val.string;
  args[0].val.string = NULL;

  /* Note: ownership of 'name' is transferred to the callee.  */
  obj_attr_v2_subsection_record (name, &args[1], &args[2]);
  args_list_free (args, N_ARGS);
}
#endif /* TC_OBJ_ATTR_v2 */

/* Parse a .gnu_attribute directive.  */

void
obj_elf_gnu_attribute (int ignored ATTRIBUTE_UNUSED)
{
  obj_attr_process_attribute (OBJ_ATTR_GNU);
}

#if (TC_OBJ_ATTR_v2)
/* Return True if the VERSION of object attributes supports subsections, False
   otherwise.  */

static inline bool
attr_fmt_has_subsections (obj_attr_version_t version)
{
  switch (version)
  {
  case OBJ_ATTR_V1:
    return false;
  case OBJ_ATTR_V2:
    return true;
  default:
    abort ();  /* Unsupported format.  */
  }
}

/* Parse a .gnu_subsection directive.  */

void
obj_elf_gnu_subsection (int ignored ATTRIBUTE_UNUSED)
{
  obj_attr_version_t version = elf_obj_attr_version (stdoutput);
  if (! attr_fmt_has_subsections (version))
    {
      as_bad (_(".gnu_subsection is only available with object attributes v2"));
      ignore_rest_of_line ();
      return;
    }
  obj_attr_process_subsection ();
}
#endif /* TC_OBJ_ATTR_v2 */

#endif /* TC_OBJ_ATTR */
