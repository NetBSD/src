/* Scheme interface to lazy strings.

   Copyright (C) 2010-2016 Free Software Foundation, Inc.

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

/* See README file in this directory for implementation notes, coding
   conventions, et.al.  */

#include "defs.h"
#include "charset.h"
#include "value.h"
#include "valprint.h"
#include "language.h"
#include "guile-internal.h"

/* The <gdb:lazy-string> smob.  */

typedef struct
{
  /* This always appears first.  */
  gdb_smob base;

  /*  Holds the address of the lazy string.  */
  CORE_ADDR address;

  /*  Holds the encoding that will be applied to the string when the string
      is printed by GDB.  If the encoding is set to NULL then GDB will select
      the most appropriate encoding when the sting is printed.
      Space for this is malloc'd and will be freed when the object is
      freed.  */
  char *encoding;

  /* Holds the length of the string in characters.  If the length is -1,
     then the string will be fetched and encoded up to the first null of
     appropriate width.  */
  int length;

  /*  This attribute holds the type that is represented by the lazy
      string's type.  */
  struct type *type;
} lazy_string_smob;

static const char lazy_string_smob_name[] = "gdb:lazy-string";

/* The tag Guile knows the lazy string smob by.  */
static scm_t_bits lazy_string_smob_tag;

/* Administrivia for lazy string smobs.  */

/* The smob "free" function for <gdb:lazy-string>.  */

static size_t
lsscm_free_lazy_string_smob (SCM self)
{
  lazy_string_smob *v_smob = (lazy_string_smob *) SCM_SMOB_DATA (self);

  xfree (v_smob->encoding);

  return 0;
}

/* The smob "print" function for <gdb:lazy-string>.  */

static int
lsscm_print_lazy_string_smob (SCM self, SCM port, scm_print_state *pstate)
{
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (self);

  gdbscm_printf (port, "#<%s", lazy_string_smob_name);
  gdbscm_printf (port, " @%s", hex_string (ls_smob->address));
  if (ls_smob->length >= 0)
    gdbscm_printf (port, " length %d", ls_smob->length);
  if (ls_smob->encoding != NULL)
    gdbscm_printf (port, " encoding %s", ls_smob->encoding);
  scm_puts (">", port);

  scm_remember_upto_here_1 (self);

  /* Non-zero means success.  */
  return 1;
}

/* Low level routine to create a <gdb:lazy-string> object.
   The caller must verify !(address == 0 && length != 0).  */

static SCM
lsscm_make_lazy_string_smob (CORE_ADDR address, int length,
			     const char *encoding, struct type *type)
{
  lazy_string_smob *ls_smob = (lazy_string_smob *)
    scm_gc_malloc (sizeof (lazy_string_smob), lazy_string_smob_name);
  SCM ls_scm;

  /* Caller must verify this.  */
  gdb_assert (!(address == 0 && length != 0));
  gdb_assert (type != NULL);

  ls_smob->address = address;
  /* Coerce all values < 0 to -1.  */
  ls_smob->length = length < 0 ? -1 : length;
  if (encoding == NULL || strcmp (encoding, "") == 0)
    ls_smob->encoding = NULL;
  else
    ls_smob->encoding = xstrdup (encoding);
  ls_smob->type = type;

  ls_scm = scm_new_smob (lazy_string_smob_tag, (scm_t_bits) ls_smob);
  gdbscm_init_gsmob (&ls_smob->base);

  return ls_scm;
}

/* Return non-zero if SCM is a <gdb:lazy-string> object.  */

int
lsscm_is_lazy_string (SCM scm)
{
  return SCM_SMOB_PREDICATE (lazy_string_smob_tag, scm);
}

/* (lazy-string? object) -> boolean */

static SCM
gdbscm_lazy_string_p (SCM scm)
{
  return scm_from_bool (lsscm_is_lazy_string (scm));
}

/* Main entry point to create a <gdb:lazy-string> object.
   If there's an error a <gdb:exception> object is returned.  */

SCM
lsscm_make_lazy_string (CORE_ADDR address, int length,
			const char *encoding, struct type *type)
{
  if (address == 0 && length != 0)
    {
      return gdbscm_make_out_of_range_error
	(NULL, 0, scm_from_int (length),
	 _("cannot create a lazy string with address 0x0"
	   " and a non-zero length"));
    }

  if (type == NULL)
    {
      return gdbscm_make_out_of_range_error
	(NULL, 0, scm_from_int (0), _("a lazy string's type cannot be NULL"));
    }

  return lsscm_make_lazy_string_smob (address, length, encoding, type);
}

/* Returns the <gdb:lazy-string> smob in SELF.
   Throws an exception if SELF is not a <gdb:lazy-string> object.  */

static SCM
lsscm_get_lazy_string_arg_unsafe (SCM self, int arg_pos, const char *func_name)
{
  SCM_ASSERT_TYPE (lsscm_is_lazy_string (self), self, arg_pos, func_name,
		   lazy_string_smob_name);

  return self;
}

/* Lazy string methods.  */

/* (lazy-string-address <gdb:lazy-string>) -> address */

static SCM
gdbscm_lazy_string_address (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return gdbscm_scm_from_ulongest (ls_smob->address);
}

/* (lazy-string-length <gdb:lazy-string>) -> integer */

static SCM
gdbscm_lazy_string_length (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return scm_from_int (ls_smob->length);
}

/* (lazy-string-encoding <gdb:lazy-string>) -> string */

static SCM
gdbscm_lazy_string_encoding (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  /* An encoding can be set to NULL by the user, so check first.
     If NULL return #f.  */
  if (ls_smob != NULL)
    return gdbscm_scm_from_c_string (ls_smob->encoding);
  return SCM_BOOL_F;
}

/* (lazy-string-type <gdb:lazy-string>) -> <gdb:type> */

static SCM
gdbscm_lazy_string_type (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);

  return tyscm_scm_from_type (ls_smob->type);
}

/* (lazy-string->value <gdb:lazy-string>) -> <gdb:value> */

static SCM
gdbscm_lazy_string_to_value (SCM self)
{
  SCM ls_scm = lsscm_get_lazy_string_arg_unsafe (self, SCM_ARG1, FUNC_NAME);
  lazy_string_smob *ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (ls_scm);
  struct value *value = NULL;

  if (ls_smob->address == 0)
    {
      gdbscm_throw (gdbscm_make_out_of_range_error (FUNC_NAME, SCM_ARG1, self,
				_("cannot create a value from NULL")));
    }

  TRY
    {
      value = value_at_lazy (ls_smob->type, ls_smob->address);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      GDBSCM_HANDLE_GDB_EXCEPTION (except);
    }
  END_CATCH

  return vlscm_scm_from_value (value);
}

/* A "safe" version of gdbscm_lazy_string_to_value for use by
   vlscm_convert_typed_value_from_scheme.
   The result, upon success, is the value of <gdb:lazy-string> STRING.
   ARG_POS is the argument position of STRING in the original Scheme
   function call, used in exception text.
   If there's an error, NULL is returned and a <gdb:exception> object
   is stored in *except_scmp.

   Note: The result is still "lazy".  The caller must call value_fetch_lazy
   to actually fetch the value.  */

struct value *
lsscm_safe_lazy_string_to_value (SCM string, int arg_pos,
				 const char *func_name, SCM *except_scmp)
{
  lazy_string_smob *ls_smob;
  struct value *value = NULL;

  gdb_assert (lsscm_is_lazy_string (string));

  ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (string);
  *except_scmp = SCM_BOOL_F;

  if (ls_smob->address == 0)
    {
      *except_scmp
	= gdbscm_make_out_of_range_error (FUNC_NAME, SCM_ARG1, string,
					 _("cannot create a value from NULL"));
      return NULL;
    }

  TRY
    {
      value = value_at_lazy (ls_smob->type, ls_smob->address);
    }
  CATCH (except, RETURN_MASK_ALL)
    {
      *except_scmp = gdbscm_scm_from_gdb_exception (except);
      return NULL;
    }
  END_CATCH

  return value;
}

/* Print a lazy string to STREAM using val_print_string.
   STRING must be a <gdb:lazy-string> object.  */

void
lsscm_val_print_lazy_string (SCM string, struct ui_file *stream,
			     const struct value_print_options *options)
{
  lazy_string_smob *ls_smob;

  gdb_assert (lsscm_is_lazy_string (string));

  ls_smob = (lazy_string_smob *) SCM_SMOB_DATA (string);

  val_print_string (ls_smob->type, ls_smob->encoding,
		    ls_smob->address, ls_smob->length,
		    stream, options);
}

/* Initialize the Scheme lazy-strings code.  */

static const scheme_function lazy_string_functions[] =
{
  { "lazy-string?", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_p),
    "\
Return #t if the object is a <gdb:lazy-string> object." },

  { "lazy-string-address", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_address),
    "\
Return the address of the lazy-string." },

  { "lazy-string-length", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_length),
    "\
Return the length of the lazy-string.\n\
If the length is -1 then the length is determined by the first null\n\
of appropriate width." },

  { "lazy-string-encoding", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_encoding),
    "\
Return the encoding of the lazy-string." },

  { "lazy-string-type", 1, 0, 0, as_a_scm_t_subr (gdbscm_lazy_string_type),
    "\
Return the <gdb:type> of the lazy-string." },

  { "lazy-string->value", 1, 0, 0,
    as_a_scm_t_subr (gdbscm_lazy_string_to_value),
    "\
Return the <gdb:value> representation of the lazy-string." },

  END_FUNCTIONS
};

void
gdbscm_initialize_lazy_strings (void)
{
  lazy_string_smob_tag = gdbscm_make_smob_type (lazy_string_smob_name,
						sizeof (lazy_string_smob));
  scm_set_smob_free (lazy_string_smob_tag, lsscm_free_lazy_string_smob);
  scm_set_smob_print (lazy_string_smob_tag, lsscm_print_lazy_string_smob);

  gdbscm_define_functions (lazy_string_functions, 1);
}
