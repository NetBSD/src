/* Implementation of the GDB variable objects API.

   Copyright (C) 1999-2017 Free Software Foundation, Inc.

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
#include "value.h"
#include "expression.h"
#include "frame.h"
#include "language.h"
#include "gdbcmd.h"
#include "block.h"
#include "valprint.h"
#include "gdb_regex.h"

#include "varobj.h"
#include "vec.h"
#include "gdbthread.h"
#include "inferior.h"
#include "varobj-iter.h"

#if HAVE_PYTHON
#include "python/python.h"
#include "python/python-internal.h"
#include "python/py-ref.h"
#else
typedef int PyObject;
#endif

/* Non-zero if we want to see trace of varobj level stuff.  */

unsigned int varobjdebug = 0;
static void
show_varobjdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Varobj debugging is %s.\n"), value);
}

/* String representations of gdb's format codes.  */
const char *varobj_format_string[] =
  { "natural", "binary", "decimal", "hexadecimal", "octal", "zero-hexadecimal" };

/* True if we want to allow Python-based pretty-printing.  */
static int pretty_printing = 0;

void
varobj_enable_pretty_printing (void)
{
  pretty_printing = 1;
}

/* Data structures */

/* Every root variable has one of these structures saved in its
   varobj.  */
struct varobj_root
{

  /* The expression for this parent.  */
  expression_up exp;

  /* Block for which this expression is valid.  */
  const struct block *valid_block;

  /* The frame for this expression.  This field is set iff valid_block is
     not NULL.  */
  struct frame_id frame;

  /* The global thread ID that this varobj_root belongs to.  This field
     is only valid if valid_block is not NULL.
     When not 0, indicates which thread 'frame' belongs to.
     When 0, indicates that the thread list was empty when the varobj_root
     was created.  */
  int thread_id;

  /* If 1, the -var-update always recomputes the value in the
     current thread and frame.  Otherwise, variable object is
     always updated in the specific scope/thread/frame.  */
  int floating;

  /* Flag that indicates validity: set to 0 when this varobj_root refers 
     to symbols that do not exist anymore.  */
  int is_valid;

  /* Language-related operations for this variable and its
     children.  */
  const struct lang_varobj_ops *lang_ops;

  /* The varobj for this root node.  */
  struct varobj *rootvar;

  /* Next root variable */
  struct varobj_root *next;
};

/* Dynamic part of varobj.  */

struct varobj_dynamic
{
  /* Whether the children of this varobj were requested.  This field is
     used to decide if dynamic varobj should recompute their children.
     In the event that the frontend never asked for the children, we
     can avoid that.  */
  int children_requested;

  /* The pretty-printer constructor.  If NULL, then the default
     pretty-printer will be looked up.  If None, then no
     pretty-printer will be installed.  */
  PyObject *constructor;

  /* The pretty-printer that has been constructed.  If NULL, then a
     new printer object is needed, and one will be constructed.  */
  PyObject *pretty_printer;

  /* The iterator returned by the printer's 'children' method, or NULL
     if not available.  */
  struct varobj_iter *child_iter;

  /* We request one extra item from the iterator, so that we can
     report to the caller whether there are more items than we have
     already reported.  However, we don't want to install this value
     when we read it, because that will mess up future updates.  So,
     we stash it here instead.  */
  varobj_item *saved_item;
};

/* A list of varobjs */

struct vlist
{
  struct varobj *var;
  struct vlist *next;
};

/* Private function prototypes */

/* Helper functions for the above subcommands.  */

static int delete_variable (struct varobj *, int);

static void delete_variable_1 (int *, struct varobj *, int, int);

static int install_variable (struct varobj *);

static void uninstall_variable (struct varobj *);

static struct varobj *create_child (struct varobj *, int, std::string &);

static struct varobj *
create_child_with_value (struct varobj *parent, int index,
			 struct varobj_item *item);

/* Utility routines */

static struct varobj *new_variable (void);

static struct varobj *new_root_variable (void);

static void free_variable (struct varobj *var);

static struct cleanup *make_cleanup_free_variable (struct varobj *var);

static enum varobj_display_formats variable_default_display (struct varobj *);

static int update_type_if_necessary (struct varobj *var,
				     struct value *new_value);

static int install_new_value (struct varobj *var, struct value *value, 
			      int initial);

/* Language-specific routines.  */

static int number_of_children (const struct varobj *);

static std::string name_of_variable (const struct varobj *);

static std::string name_of_child (struct varobj *, int);

static struct value *value_of_root (struct varobj **var_handle, int *);

static struct value *value_of_child (const struct varobj *parent, int index);

static std::string my_value_of_variable (struct varobj *var,
					 enum varobj_display_formats format);

static int is_root_p (const struct varobj *var);

static struct varobj *varobj_add_child (struct varobj *var,
					struct varobj_item *item);

/* Private data */

/* Mappings of varobj_display_formats enums to gdb's format codes.  */
static int format_code[] = { 0, 't', 'd', 'x', 'o', 'z' };

/* Header of the list of root variable objects.  */
static struct varobj_root *rootlist;

/* Prime number indicating the number of buckets in the hash table.  */
/* A prime large enough to avoid too many collisions.  */
#define VAROBJ_TABLE_SIZE 227

/* Pointer to the varobj hash table (built at run time).  */
static struct vlist **varobj_table;



/* API Implementation */
static int
is_root_p (const struct varobj *var)
{
  return (var->root->rootvar == var);
}

#ifdef HAVE_PYTHON

/* See python-internal.h.  */
gdbpy_enter_varobj::gdbpy_enter_varobj (const struct varobj *var)
: gdbpy_enter (var->root->exp->gdbarch, var->root->exp->language_defn)
{
}

#endif

/* Return the full FRAME which corresponds to the given CORE_ADDR
   or NULL if no FRAME on the chain corresponds to CORE_ADDR.  */

static struct frame_info *
find_frame_addr_in_frame_chain (CORE_ADDR frame_addr)
{
  struct frame_info *frame = NULL;

  if (frame_addr == (CORE_ADDR) 0)
    return NULL;

  for (frame = get_current_frame ();
       frame != NULL;
       frame = get_prev_frame (frame))
    {
      /* The CORE_ADDR we get as argument was parsed from a string GDB
	 output as $fp.  This output got truncated to gdbarch_addr_bit.
	 Truncate the frame base address in the same manner before
	 comparing it against our argument.  */
      CORE_ADDR frame_base = get_frame_base_address (frame);
      int addr_bit = gdbarch_addr_bit (get_frame_arch (frame));

      if (addr_bit < (sizeof (CORE_ADDR) * HOST_CHAR_BIT))
	frame_base &= ((CORE_ADDR) 1 << addr_bit) - 1;

      if (frame_base == frame_addr)
	return frame;
    }

  return NULL;
}

/* Creates a varobj (not its children).  */

struct varobj *
varobj_create (const char *objname,
	       const char *expression, CORE_ADDR frame, enum varobj_type type)
{
  struct varobj *var;
  struct cleanup *old_chain;

  /* Fill out a varobj structure for the (root) variable being constructed.  */
  var = new_root_variable ();
  old_chain = make_cleanup_free_variable (var);

  if (expression != NULL)
    {
      struct frame_info *fi;
      struct frame_id old_id = null_frame_id;
      const struct block *block;
      const char *p;
      struct value *value = NULL;
      CORE_ADDR pc;

      /* Parse and evaluate the expression, filling in as much of the
         variable's data as possible.  */

      if (has_stack_frames ())
	{
	  /* Allow creator to specify context of variable.  */
	  if ((type == USE_CURRENT_FRAME) || (type == USE_SELECTED_FRAME))
	    fi = get_selected_frame (NULL);
	  else
	    /* FIXME: cagney/2002-11-23: This code should be doing a
	       lookup using the frame ID and not just the frame's
	       ``address''.  This, of course, means an interface
	       change.  However, with out that interface change ISAs,
	       such as the ia64 with its two stacks, won't work.
	       Similar goes for the case where there is a frameless
	       function.  */
	    fi = find_frame_addr_in_frame_chain (frame);
	}
      else
	fi = NULL;

      /* frame = -2 means always use selected frame.  */
      if (type == USE_SELECTED_FRAME)
	var->root->floating = 1;

      pc = 0;
      block = NULL;
      if (fi != NULL)
	{
	  block = get_frame_block (fi, 0);
	  pc = get_frame_pc (fi);
	}

      p = expression;
      innermost_block = NULL;
      /* Wrap the call to parse expression, so we can 
         return a sensible error.  */
      TRY
	{
	  var->root->exp = parse_exp_1 (&p, pc, block, 0);
	}

      CATCH (except, RETURN_MASK_ERROR)
	{
	  do_cleanups (old_chain);
	  return NULL;
	}
      END_CATCH

      /* Don't allow variables to be created for types.  */
      if (var->root->exp->elts[0].opcode == OP_TYPE
	  || var->root->exp->elts[0].opcode == OP_TYPEOF
	  || var->root->exp->elts[0].opcode == OP_DECLTYPE)
	{
	  do_cleanups (old_chain);
	  fprintf_unfiltered (gdb_stderr, "Attempt to use a type name"
			      " as an expression.\n");
	  return NULL;
	}

      var->format = variable_default_display (var);
      var->root->valid_block = innermost_block;
      var->name = expression;
      /* For a root var, the name and the expr are the same.  */
      var->path_expr = expression;

      /* When the frame is different from the current frame, 
         we must select the appropriate frame before parsing
         the expression, otherwise the value will not be current.
         Since select_frame is so benign, just call it for all cases.  */
      if (innermost_block)
	{
	  /* User could specify explicit FRAME-ADDR which was not found but
	     EXPRESSION is frame specific and we would not be able to evaluate
	     it correctly next time.  With VALID_BLOCK set we must also set
	     FRAME and THREAD_ID.  */
	  if (fi == NULL)
	    error (_("Failed to find the specified frame"));

	  var->root->frame = get_frame_id (fi);
	  var->root->thread_id = ptid_to_global_thread_id (inferior_ptid);
	  old_id = get_frame_id (get_selected_frame (NULL));
	  select_frame (fi);	 
	}

      /* We definitely need to catch errors here.
         If evaluate_expression succeeds we got the value we wanted.
         But if it fails, we still go on with a call to evaluate_type().  */
      TRY
	{
	  value = evaluate_expression (var->root->exp.get ());
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  /* Error getting the value.  Try to at least get the
	     right type.  */
	  struct value *type_only_value = evaluate_type (var->root->exp.get ());

	  var->type = value_type (type_only_value);
	}
      END_CATCH

      if (value != NULL)
	{
	  int real_type_found = 0;

	  var->type = value_actual_type (value, 0, &real_type_found);
	  if (real_type_found)
	    value = value_cast (var->type, value);
	}

      /* Set language info */
      var->root->lang_ops = var->root->exp->language_defn->la_varobj_ops;

      install_new_value (var, value, 1 /* Initial assignment */);

      /* Set ourselves as our root.  */
      var->root->rootvar = var;

      /* Reset the selected frame.  */
      if (frame_id_p (old_id))
	select_frame (frame_find_by_id (old_id));
    }

  /* If the variable object name is null, that means this
     is a temporary variable, so don't install it.  */

  if ((var != NULL) && (objname != NULL))
    {
      var->obj_name = objname;

      /* If a varobj name is duplicated, the install will fail so
         we must cleanup.  */
      if (!install_variable (var))
	{
	  do_cleanups (old_chain);
	  return NULL;
	}
    }

  discard_cleanups (old_chain);
  return var;
}

/* Generates an unique name that can be used for a varobj.  */

char *
varobj_gen_name (void)
{
  static int id = 0;
  char *obj_name;

  /* Generate a name for this object.  */
  id++;
  obj_name = xstrprintf ("var%d", id);

  return obj_name;
}

/* Given an OBJNAME, returns the pointer to the corresponding varobj.  Call
   error if OBJNAME cannot be found.  */

struct varobj *
varobj_get_handle (const char *objname)
{
  struct vlist *cv;
  const char *chp;
  unsigned int index = 0;
  unsigned int i = 1;

  for (chp = objname; *chp; chp++)
    {
      index = (index + (i++ * (unsigned int) *chp)) % VAROBJ_TABLE_SIZE;
    }

  cv = *(varobj_table + index);
  while (cv != NULL && cv->var->obj_name != objname)
    cv = cv->next;

  if (cv == NULL)
    error (_("Variable object not found"));

  return cv->var;
}

/* Given the handle, return the name of the object.  */

const char *
varobj_get_objname (const struct varobj *var)
{
  return var->obj_name.c_str ();
}

/* Given the handle, return the expression represented by the
   object.  */

std::string
varobj_get_expression (const struct varobj *var)
{
  return name_of_variable (var);
}

/* See varobj.h.  */

int
varobj_delete (struct varobj *var, int only_children)
{
  return delete_variable (var, only_children);
}

#if HAVE_PYTHON

/* Convenience function for varobj_set_visualizer.  Instantiate a
   pretty-printer for a given value.  */
static PyObject *
instantiate_pretty_printer (PyObject *constructor, struct value *value)
{
  PyObject *val_obj = NULL; 
  PyObject *printer;

  val_obj = value_to_value_object (value);
  if (! val_obj)
    return NULL;

  printer = PyObject_CallFunctionObjArgs (constructor, val_obj, NULL);
  Py_DECREF (val_obj);
  return printer;
}

#endif

/* Set/Get variable object display format.  */

enum varobj_display_formats
varobj_set_display_format (struct varobj *var,
			   enum varobj_display_formats format)
{
  switch (format)
    {
    case FORMAT_NATURAL:
    case FORMAT_BINARY:
    case FORMAT_DECIMAL:
    case FORMAT_HEXADECIMAL:
    case FORMAT_OCTAL:
    case FORMAT_ZHEXADECIMAL:
      var->format = format;
      break;

    default:
      var->format = variable_default_display (var);
    }

  if (varobj_value_is_changeable_p (var) 
      && var->value && !value_lazy (var->value))
    {
      var->print_value = varobj_value_get_print_value (var->value,
						       var->format, var);
    }

  return var->format;
}

enum varobj_display_formats
varobj_get_display_format (const struct varobj *var)
{
  return var->format;
}

gdb::unique_xmalloc_ptr<char>
varobj_get_display_hint (const struct varobj *var)
{
  gdb::unique_xmalloc_ptr<char> result;

#if HAVE_PYTHON
  if (!gdb_python_initialized)
    return NULL;

  gdbpy_enter_varobj enter_py (var);

  if (var->dynamic->pretty_printer != NULL)
    result = gdbpy_get_display_hint (var->dynamic->pretty_printer);
#endif

  return result;
}

/* Return true if the varobj has items after TO, false otherwise.  */

int
varobj_has_more (const struct varobj *var, int to)
{
  if (VEC_length (varobj_p, var->children) > to)
    return 1;
  return ((to == -1 || VEC_length (varobj_p, var->children) == to)
	  && (var->dynamic->saved_item != NULL));
}

/* If the variable object is bound to a specific thread, that
   is its evaluation can always be done in context of a frame
   inside that thread, returns GDB id of the thread -- which
   is always positive.  Otherwise, returns -1.  */
int
varobj_get_thread_id (const struct varobj *var)
{
  if (var->root->valid_block && var->root->thread_id > 0)
    return var->root->thread_id;
  else
    return -1;
}

void
varobj_set_frozen (struct varobj *var, int frozen)
{
  /* When a variable is unfrozen, we don't fetch its value.
     The 'not_fetched' flag remains set, so next -var-update
     won't complain.

     We don't fetch the value, because for structures the client
     should do -var-update anyway.  It would be bad to have different
     client-size logic for structure and other types.  */
  var->frozen = frozen;
}

int
varobj_get_frozen (const struct varobj *var)
{
  return var->frozen;
}

/* A helper function that restricts a range to what is actually
   available in a VEC.  This follows the usual rules for the meaning
   of FROM and TO -- if either is negative, the entire range is
   used.  */

void
varobj_restrict_range (VEC (varobj_p) *children, int *from, int *to)
{
  if (*from < 0 || *to < 0)
    {
      *from = 0;
      *to = VEC_length (varobj_p, children);
    }
  else
    {
      if (*from > VEC_length (varobj_p, children))
	*from = VEC_length (varobj_p, children);
      if (*to > VEC_length (varobj_p, children))
	*to = VEC_length (varobj_p, children);
      if (*from > *to)
	*from = *to;
    }
}

/* A helper for update_dynamic_varobj_children that installs a new
   child when needed.  */

static void
install_dynamic_child (struct varobj *var,
		       VEC (varobj_p) **changed,
		       VEC (varobj_p) **type_changed,
		       VEC (varobj_p) **newobj,
		       VEC (varobj_p) **unchanged,
		       int *cchanged,
		       int index,
		       struct varobj_item *item)
{
  if (VEC_length (varobj_p, var->children) < index + 1)
    {
      /* There's no child yet.  */
      struct varobj *child = varobj_add_child (var, item);

      if (newobj)
	{
	  VEC_safe_push (varobj_p, *newobj, child);
	  *cchanged = 1;
	}
    }
  else
    {
      varobj_p existing = VEC_index (varobj_p, var->children, index);
      int type_updated = update_type_if_necessary (existing, item->value);

      if (type_updated)
	{
	  if (type_changed)
	    VEC_safe_push (varobj_p, *type_changed, existing);
	}
      if (install_new_value (existing, item->value, 0))
	{
	  if (!type_updated && changed)
	    VEC_safe_push (varobj_p, *changed, existing);
	}
      else if (!type_updated && unchanged)
	VEC_safe_push (varobj_p, *unchanged, existing);
    }
}

#if HAVE_PYTHON

static int
dynamic_varobj_has_child_method (const struct varobj *var)
{
  PyObject *printer = var->dynamic->pretty_printer;

  if (!gdb_python_initialized)
    return 0;

  gdbpy_enter_varobj enter_py (var);
  return PyObject_HasAttr (printer, gdbpy_children_cst);
}
#endif

/* A factory for creating dynamic varobj's iterators.  Returns an
   iterator object suitable for iterating over VAR's children.  */

static struct varobj_iter *
varobj_get_iterator (struct varobj *var)
{
#if HAVE_PYTHON
  if (var->dynamic->pretty_printer)
    return py_varobj_get_iterator (var, var->dynamic->pretty_printer);
#endif

  gdb_assert_not_reached (_("\
requested an iterator from a non-dynamic varobj"));
}

/* Release and clear VAR's saved item, if any.  */

static void
varobj_clear_saved_item (struct varobj_dynamic *var)
{
  if (var->saved_item != NULL)
    {
      value_free (var->saved_item->value);
      delete var->saved_item;
      var->saved_item = NULL;
    }
}

static int
update_dynamic_varobj_children (struct varobj *var,
				VEC (varobj_p) **changed,
				VEC (varobj_p) **type_changed,
				VEC (varobj_p) **newobj,
				VEC (varobj_p) **unchanged,
				int *cchanged,
				int update_children,
				int from,
				int to)
{
  int i;

  *cchanged = 0;

  if (update_children || var->dynamic->child_iter == NULL)
    {
      varobj_iter_delete (var->dynamic->child_iter);
      var->dynamic->child_iter = varobj_get_iterator (var);

      varobj_clear_saved_item (var->dynamic);

      i = 0;

      if (var->dynamic->child_iter == NULL)
	return 0;
    }
  else
    i = VEC_length (varobj_p, var->children);

  /* We ask for one extra child, so that MI can report whether there
     are more children.  */
  for (; to < 0 || i < to + 1; ++i)
    {
      varobj_item *item;

      /* See if there was a leftover from last time.  */
      if (var->dynamic->saved_item != NULL)
	{
	  item = var->dynamic->saved_item;
	  var->dynamic->saved_item = NULL;
	}
      else
	{
	  item = varobj_iter_next (var->dynamic->child_iter);
	  /* Release vitem->value so its lifetime is not bound to the
	     execution of a command.  */
	  if (item != NULL && item->value != NULL)
	    release_value_or_incref (item->value);
	}

      if (item == NULL)
	{
	  /* Iteration is done.  Remove iterator from VAR.  */
	  varobj_iter_delete (var->dynamic->child_iter);
	  var->dynamic->child_iter = NULL;
	  break;
	}
      /* We don't want to push the extra child on any report list.  */
      if (to < 0 || i < to)
	{
	  int can_mention = from < 0 || i >= from;

	  install_dynamic_child (var, can_mention ? changed : NULL,
				 can_mention ? type_changed : NULL,
				 can_mention ? newobj : NULL,
				 can_mention ? unchanged : NULL,
				 can_mention ? cchanged : NULL, i,
				 item);

	  delete item;
	}
      else
	{
	  var->dynamic->saved_item = item;

	  /* We want to truncate the child list just before this
	     element.  */
	  break;
	}
    }

  if (i < VEC_length (varobj_p, var->children))
    {
      int j;

      *cchanged = 1;
      for (j = i; j < VEC_length (varobj_p, var->children); ++j)
	varobj_delete (VEC_index (varobj_p, var->children, j), 0);
      VEC_truncate (varobj_p, var->children, i);
    }

  /* If there are fewer children than requested, note that the list of
     children changed.  */
  if (to >= 0 && VEC_length (varobj_p, var->children) < to)
    *cchanged = 1;

  var->num_children = VEC_length (varobj_p, var->children);

  return 1;
}

int
varobj_get_num_children (struct varobj *var)
{
  if (var->num_children == -1)
    {
      if (varobj_is_dynamic_p (var))
	{
	  int dummy;

	  /* If we have a dynamic varobj, don't report -1 children.
	     So, try to fetch some children first.  */
	  update_dynamic_varobj_children (var, NULL, NULL, NULL, NULL, &dummy,
					  0, 0, 0);
	}
      else
	var->num_children = number_of_children (var);
    }

  return var->num_children >= 0 ? var->num_children : 0;
}

/* Creates a list of the immediate children of a variable object;
   the return code is the number of such children or -1 on error.  */

VEC (varobj_p)*
varobj_list_children (struct varobj *var, int *from, int *to)
{
  int i, children_changed;

  var->dynamic->children_requested = 1;

  if (varobj_is_dynamic_p (var))
    {
      /* This, in theory, can result in the number of children changing without
	 frontend noticing.  But well, calling -var-list-children on the same
	 varobj twice is not something a sane frontend would do.  */
      update_dynamic_varobj_children (var, NULL, NULL, NULL, NULL,
				      &children_changed, 0, 0, *to);
      varobj_restrict_range (var->children, from, to);
      return var->children;
    }

  if (var->num_children == -1)
    var->num_children = number_of_children (var);

  /* If that failed, give up.  */
  if (var->num_children == -1)
    return var->children;

  /* If we're called when the list of children is not yet initialized,
     allocate enough elements in it.  */
  while (VEC_length (varobj_p, var->children) < var->num_children)
    VEC_safe_push (varobj_p, var->children, NULL);

  for (i = 0; i < var->num_children; i++)
    {
      varobj_p existing = VEC_index (varobj_p, var->children, i);

      if (existing == NULL)
	{
	  /* Either it's the first call to varobj_list_children for
	     this variable object, and the child was never created,
	     or it was explicitly deleted by the client.  */
	  std::string name = name_of_child (var, i);
	  existing = create_child (var, i, name);
	  VEC_replace (varobj_p, var->children, i, existing);
	}
    }

  varobj_restrict_range (var->children, from, to);
  return var->children;
}

static struct varobj *
varobj_add_child (struct varobj *var, struct varobj_item *item)
{
  varobj_p v = create_child_with_value (var,
					VEC_length (varobj_p, var->children), 
					item);

  VEC_safe_push (varobj_p, var->children, v);
  return v;
}

/* Obtain the type of an object Variable as a string similar to the one gdb
   prints on the console.  The caller is responsible for freeing the string.
   */

std::string
varobj_get_type (struct varobj *var)
{
  /* For the "fake" variables, do not return a type.  (Its type is
     NULL, too.)
     Do not return a type for invalid variables as well.  */
  if (CPLUS_FAKE_CHILD (var) || !var->root->is_valid)
    return std::string ();

  return type_to_string (var->type);
}

/* Obtain the type of an object variable.  */

struct type *
varobj_get_gdb_type (const struct varobj *var)
{
  return var->type;
}

/* Is VAR a path expression parent, i.e., can it be used to construct
   a valid path expression?  */

static int
is_path_expr_parent (const struct varobj *var)
{
  gdb_assert (var->root->lang_ops->is_path_expr_parent != NULL);
  return var->root->lang_ops->is_path_expr_parent (var);
}

/* Is VAR a path expression parent, i.e., can it be used to construct
   a valid path expression?  By default we assume any VAR can be a path
   parent.  */

int
varobj_default_is_path_expr_parent (const struct varobj *var)
{
  return 1;
}

/* Return the path expression parent for VAR.  */

const struct varobj *
varobj_get_path_expr_parent (const struct varobj *var)
{
  const struct varobj *parent = var;

  while (!is_root_p (parent) && !is_path_expr_parent (parent))
    parent = parent->parent;

  return parent;
}

/* Return a pointer to the full rooted expression of varobj VAR.
   If it has not been computed yet, compute it.  */

const char *
varobj_get_path_expr (const struct varobj *var)
{
  if (var->path_expr.empty ())
    {
      /* For root varobjs, we initialize path_expr
	 when creating varobj, so here it should be
	 child varobj.  */
      struct varobj *mutable_var = (struct varobj *) var;
      gdb_assert (!is_root_p (var));

      mutable_var->path_expr = (*var->root->lang_ops->path_expr_of_child) (var);
    }

  return var->path_expr.c_str ();
}

const struct language_defn *
varobj_get_language (const struct varobj *var)
{
  return var->root->exp->language_defn;
}

int
varobj_get_attributes (const struct varobj *var)
{
  int attributes = 0;

  if (varobj_editable_p (var))
    /* FIXME: define masks for attributes.  */
    attributes |= 0x00000001;	/* Editable */

  return attributes;
}

/* Return true if VAR is a dynamic varobj.  */

int
varobj_is_dynamic_p (const struct varobj *var)
{
  return var->dynamic->pretty_printer != NULL;
}

std::string
varobj_get_formatted_value (struct varobj *var,
			    enum varobj_display_formats format)
{
  return my_value_of_variable (var, format);
}

std::string
varobj_get_value (struct varobj *var)
{
  return my_value_of_variable (var, var->format);
}

/* Set the value of an object variable (if it is editable) to the
   value of the given expression.  */
/* Note: Invokes functions that can call error().  */

int
varobj_set_value (struct varobj *var, const char *expression)
{
  struct value *val = NULL; /* Initialize to keep gcc happy.  */
  /* The argument "expression" contains the variable's new value.
     We need to first construct a legal expression for this -- ugh!  */
  /* Does this cover all the bases?  */
  struct value *value = NULL; /* Initialize to keep gcc happy.  */
  int saved_input_radix = input_radix;
  const char *s = expression;

  gdb_assert (varobj_editable_p (var));

  input_radix = 10;		/* ALWAYS reset to decimal temporarily.  */
  expression_up exp = parse_exp_1 (&s, 0, 0, 0);
  TRY
    {
      value = evaluate_expression (exp.get ());
    }

  CATCH (except, RETURN_MASK_ERROR)
    {
      /* We cannot proceed without a valid expression.  */
      return 0;
    }
  END_CATCH

  /* All types that are editable must also be changeable.  */
  gdb_assert (varobj_value_is_changeable_p (var));

  /* The value of a changeable variable object must not be lazy.  */
  gdb_assert (!value_lazy (var->value));

  /* Need to coerce the input.  We want to check if the
     value of the variable object will be different
     after assignment, and the first thing value_assign
     does is coerce the input.
     For example, if we are assigning an array to a pointer variable we
     should compare the pointer with the array's address, not with the
     array's content.  */
  value = coerce_array (value);

  /* The new value may be lazy.  value_assign, or
     rather value_contents, will take care of this.  */
  TRY
    {
      val = value_assign (var->value, value);
    }

  CATCH (except, RETURN_MASK_ERROR)
    {
      return 0;
    }
  END_CATCH

  /* If the value has changed, record it, so that next -var-update can
     report this change.  If a variable had a value of '1', we've set it
     to '333' and then set again to '1', when -var-update will report this
     variable as changed -- because the first assignment has set the
     'updated' flag.  There's no need to optimize that, because return value
     of -var-update should be considered an approximation.  */
  var->updated = install_new_value (var, val, 0 /* Compare values.  */);
  input_radix = saved_input_radix;
  return 1;
}

#if HAVE_PYTHON

/* A helper function to install a constructor function and visualizer
   in a varobj_dynamic.  */

static void
install_visualizer (struct varobj_dynamic *var, PyObject *constructor,
		    PyObject *visualizer)
{
  Py_XDECREF (var->constructor);
  var->constructor = constructor;

  Py_XDECREF (var->pretty_printer);
  var->pretty_printer = visualizer;

  varobj_iter_delete (var->child_iter);
  var->child_iter = NULL;
}

/* Install the default visualizer for VAR.  */

static void
install_default_visualizer (struct varobj *var)
{
  /* Do not install a visualizer on a CPLUS_FAKE_CHILD.  */
  if (CPLUS_FAKE_CHILD (var))
    return;

  if (pretty_printing)
    {
      PyObject *pretty_printer = NULL;

      if (var->value)
	{
	  pretty_printer = gdbpy_get_varobj_pretty_printer (var->value);
	  if (! pretty_printer)
	    {
	      gdbpy_print_stack ();
	      error (_("Cannot instantiate printer for default visualizer"));
	    }
	}
      
      if (pretty_printer == Py_None)
	{
	  Py_DECREF (pretty_printer);
	  pretty_printer = NULL;
	}
  
      install_visualizer (var->dynamic, NULL, pretty_printer);
    }
}

/* Instantiate and install a visualizer for VAR using CONSTRUCTOR to
   make a new object.  */

static void
construct_visualizer (struct varobj *var, PyObject *constructor)
{
  PyObject *pretty_printer;

  /* Do not install a visualizer on a CPLUS_FAKE_CHILD.  */
  if (CPLUS_FAKE_CHILD (var))
    return;

  Py_INCREF (constructor);
  if (constructor == Py_None)
    pretty_printer = NULL;
  else
    {
      pretty_printer = instantiate_pretty_printer (constructor, var->value);
      if (! pretty_printer)
	{
	  gdbpy_print_stack ();
	  Py_DECREF (constructor);
	  constructor = Py_None;
	  Py_INCREF (constructor);
	}

      if (pretty_printer == Py_None)
	{
	  Py_DECREF (pretty_printer);
	  pretty_printer = NULL;
	}
    }

  install_visualizer (var->dynamic, constructor, pretty_printer);
}

#endif /* HAVE_PYTHON */

/* A helper function for install_new_value.  This creates and installs
   a visualizer for VAR, if appropriate.  */

static void
install_new_value_visualizer (struct varobj *var)
{
#if HAVE_PYTHON
  /* If the constructor is None, then we want the raw value.  If VAR
     does not have a value, just skip this.  */
  if (!gdb_python_initialized)
    return;

  if (var->dynamic->constructor != Py_None && var->value != NULL)
    {
      gdbpy_enter_varobj enter_py (var);

      if (var->dynamic->constructor == NULL)
	install_default_visualizer (var);
      else
	construct_visualizer (var, var->dynamic->constructor);
    }
#else
  /* Do nothing.  */
#endif
}

/* When using RTTI to determine variable type it may be changed in runtime when
   the variable value is changed.  This function checks whether type of varobj
   VAR will change when a new value NEW_VALUE is assigned and if it is so
   updates the type of VAR.  */

static int
update_type_if_necessary (struct varobj *var, struct value *new_value)
{
  if (new_value)
    {
      struct value_print_options opts;

      get_user_print_options (&opts);
      if (opts.objectprint)
	{
	  struct type *new_type = value_actual_type (new_value, 0, 0);
	  std::string new_type_str = type_to_string (new_type);
	  std::string curr_type_str = varobj_get_type (var);

	  /* Did the type name change?  */
	  if (curr_type_str != new_type_str)
	    {
	      var->type = new_type;

	      /* This information may be not valid for a new type.  */
	      varobj_delete (var, 1);
	      VEC_free (varobj_p, var->children);
	      var->num_children = -1;
	      return 1;
	    }
	}
    }

  return 0;
}

/* Assign a new value to a variable object.  If INITIAL is non-zero,
   this is the first assignement after the variable object was just
   created, or changed type.  In that case, just assign the value 
   and return 0.
   Otherwise, assign the new value, and return 1 if the value is
   different from the current one, 0 otherwise.  The comparison is
   done on textual representation of value.  Therefore, some types
   need not be compared.  E.g.  for structures the reported value is
   always "{...}", so no comparison is necessary here.  If the old
   value was NULL and new one is not, or vice versa, we always return 1.

   The VALUE parameter should not be released -- the function will
   take care of releasing it when needed.  */
static int
install_new_value (struct varobj *var, struct value *value, int initial)
{ 
  int changeable;
  int need_to_fetch;
  int changed = 0;
  int intentionally_not_fetched = 0;

  /* We need to know the varobj's type to decide if the value should
     be fetched or not.  C++ fake children (public/protected/private)
     don't have a type.  */
  gdb_assert (var->type || CPLUS_FAKE_CHILD (var));
  changeable = varobj_value_is_changeable_p (var);

  /* If the type has custom visualizer, we consider it to be always
     changeable.  FIXME: need to make sure this behaviour will not
     mess up read-sensitive values.  */
  if (var->dynamic->pretty_printer != NULL)
    changeable = 1;

  need_to_fetch = changeable;

  /* We are not interested in the address of references, and given
     that in C++ a reference is not rebindable, it cannot
     meaningfully change.  So, get hold of the real value.  */
  if (value)
    value = coerce_ref (value);

  if (var->type && TYPE_CODE (var->type) == TYPE_CODE_UNION)
    /* For unions, we need to fetch the value implicitly because
       of implementation of union member fetch.  When gdb
       creates a value for a field and the value of the enclosing
       structure is not lazy,  it immediately copies the necessary
       bytes from the enclosing values.  If the enclosing value is
       lazy, the call to value_fetch_lazy on the field will read
       the data from memory.  For unions, that means we'll read the
       same memory more than once, which is not desirable.  So
       fetch now.  */
    need_to_fetch = 1;

  /* The new value might be lazy.  If the type is changeable,
     that is we'll be comparing values of this type, fetch the
     value now.  Otherwise, on the next update the old value
     will be lazy, which means we've lost that old value.  */
  if (need_to_fetch && value && value_lazy (value))
    {
      const struct varobj *parent = var->parent;
      int frozen = var->frozen;

      for (; !frozen && parent; parent = parent->parent)
	frozen |= parent->frozen;

      if (frozen && initial)
	{
	  /* For variables that are frozen, or are children of frozen
	     variables, we don't do fetch on initial assignment.
	     For non-initial assignemnt we do the fetch, since it means we're
	     explicitly asked to compare the new value with the old one.  */
	  intentionally_not_fetched = 1;
	}
      else
	{

	  TRY
	    {
	      value_fetch_lazy (value);
	    }

	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      /* Set the value to NULL, so that for the next -var-update,
		 we don't try to compare the new value with this value,
		 that we couldn't even read.  */
	      value = NULL;
	    }
	  END_CATCH
	}
    }

  /* Get a reference now, before possibly passing it to any Python
     code that might release it.  */
  if (value != NULL)
    value_incref (value);

  /* Below, we'll be comparing string rendering of old and new
     values.  Don't get string rendering if the value is
     lazy -- if it is, the code above has decided that the value
     should not be fetched.  */
  std::string print_value;
  if (value != NULL && !value_lazy (value)
      && var->dynamic->pretty_printer == NULL)
    print_value = varobj_value_get_print_value (value, var->format, var);

  /* If the type is changeable, compare the old and the new values.
     If this is the initial assignment, we don't have any old value
     to compare with.  */
  if (!initial && changeable)
    {
      /* If the value of the varobj was changed by -var-set-value,
	 then the value in the varobj and in the target is the same.
	 However, that value is different from the value that the
	 varobj had after the previous -var-update.  So need to the
	 varobj as changed.  */
      if (var->updated)
	{
	  changed = 1;
	}
      else if (var->dynamic->pretty_printer == NULL)
	{
	  /* Try to compare the values.  That requires that both
	     values are non-lazy.  */
	  if (var->not_fetched && value_lazy (var->value))
	    {
	      /* This is a frozen varobj and the value was never read.
		 Presumably, UI shows some "never read" indicator.
		 Now that we've fetched the real value, we need to report
		 this varobj as changed so that UI can show the real
		 value.  */
	      changed = 1;
	    }
          else  if (var->value == NULL && value == NULL)
	    /* Equal.  */
	    ;
	  else if (var->value == NULL || value == NULL)
	    {
	      changed = 1;
	    }
	  else
	    {
	      gdb_assert (!value_lazy (var->value));
	      gdb_assert (!value_lazy (value));

	      gdb_assert (!var->print_value.empty () && !print_value.empty ());
	      if (var->print_value != print_value)
		changed = 1;
	    }
	}
    }

  if (!initial && !changeable)
    {
      /* For values that are not changeable, we don't compare the values.
	 However, we want to notice if a value was not NULL and now is NULL,
	 or vise versa, so that we report when top-level varobjs come in scope
	 and leave the scope.  */
      changed = (var->value != NULL) != (value != NULL);
    }

  /* We must always keep the new value, since children depend on it.  */
  if (var->value != NULL && var->value != value)
    value_free (var->value);
  var->value = value;
  if (value && value_lazy (value) && intentionally_not_fetched)
    var->not_fetched = 1;
  else
    var->not_fetched = 0;
  var->updated = 0;

  install_new_value_visualizer (var);

  /* If we installed a pretty-printer, re-compare the printed version
     to see if the variable changed.  */
  if (var->dynamic->pretty_printer != NULL)
    {
      print_value = varobj_value_get_print_value (var->value, var->format,
						  var);
      if ((var->print_value.empty () && !print_value.empty ())
	  || (!var->print_value.empty () && print_value.empty ())
	  || (!var->print_value.empty () && !print_value.empty ()
	      && var->print_value != print_value))
	  changed = 1;
    }
  var->print_value = print_value;

  gdb_assert (!var->value || value_type (var->value));

  return changed;
}

/* Return the requested range for a varobj.  VAR is the varobj.  FROM
   and TO are out parameters; *FROM and *TO will be set to the
   selected sub-range of VAR.  If no range was selected using
   -var-set-update-range, then both will be -1.  */
void
varobj_get_child_range (const struct varobj *var, int *from, int *to)
{
  *from = var->from;
  *to = var->to;
}

/* Set the selected sub-range of children of VAR to start at index
   FROM and end at index TO.  If either FROM or TO is less than zero,
   this is interpreted as a request for all children.  */
void
varobj_set_child_range (struct varobj *var, int from, int to)
{
  var->from = from;
  var->to = to;
}

void 
varobj_set_visualizer (struct varobj *var, const char *visualizer)
{
#if HAVE_PYTHON
  PyObject *mainmod;

  if (!gdb_python_initialized)
    return;

  gdbpy_enter_varobj enter_py (var);

  mainmod = PyImport_AddModule ("__main__");
  gdbpy_ref<> globals (PyModule_GetDict (mainmod));
  Py_INCREF (globals.get ());

  gdbpy_ref<> constructor (PyRun_String (visualizer, Py_eval_input,
					 globals.get (), globals.get ()));

  if (constructor == NULL)
    {
      gdbpy_print_stack ();
      error (_("Could not evaluate visualizer expression: %s"), visualizer);
    }

  construct_visualizer (var, constructor.get ());

  /* If there are any children now, wipe them.  */
  varobj_delete (var, 1 /* children only */);
  var->num_children = -1;
#else
  error (_("Python support required"));
#endif
}

/* If NEW_VALUE is the new value of the given varobj (var), return
   non-zero if var has mutated.  In other words, if the type of
   the new value is different from the type of the varobj's old
   value.

   NEW_VALUE may be NULL, if the varobj is now out of scope.  */

static int
varobj_value_has_mutated (const struct varobj *var, struct value *new_value,
			  struct type *new_type)
{
  /* If we haven't previously computed the number of children in var,
     it does not matter from the front-end's perspective whether
     the type has mutated or not.  For all intents and purposes,
     it has not mutated.  */
  if (var->num_children < 0)
    return 0;

  if (var->root->lang_ops->value_has_mutated)
    {
      /* The varobj module, when installing new values, explicitly strips
	 references, saying that we're not interested in those addresses.
	 But detection of mutation happens before installing the new
	 value, so our value may be a reference that we need to strip
	 in order to remain consistent.  */
      if (new_value != NULL)
	new_value = coerce_ref (new_value);
      return var->root->lang_ops->value_has_mutated (var, new_value, new_type);
    }
  else
    return 0;
}

/* Update the values for a variable and its children.  This is a
   two-pronged attack.  First, re-parse the value for the root's
   expression to see if it's changed.  Then go all the way
   through its children, reconstructing them and noting if they've
   changed.

   The EXPLICIT parameter specifies if this call is result
   of MI request to update this specific variable, or 
   result of implicit -var-update *.  For implicit request, we don't
   update frozen variables.

   NOTE: This function may delete the caller's varobj.  If it
   returns TYPE_CHANGED, then it has done this and VARP will be modified
   to point to the new varobj.  */

VEC(varobj_update_result) *
varobj_update (struct varobj **varp, int is_explicit)
{
  int type_changed = 0;
  int i;
  struct value *newobj;
  VEC (varobj_update_result) *stack = NULL;
  VEC (varobj_update_result) *result = NULL;

  /* Frozen means frozen -- we don't check for any change in
     this varobj, including its going out of scope, or
     changing type.  One use case for frozen varobjs is
     retaining previously evaluated expressions, and we don't
     want them to be reevaluated at all.  */
  if (!is_explicit && (*varp)->frozen)
    return result;

  if (!(*varp)->root->is_valid)
    {
      varobj_update_result r = {0};

      r.varobj = *varp;
      r.status = VAROBJ_INVALID;
      VEC_safe_push (varobj_update_result, result, &r);
      return result;
    }

  if ((*varp)->root->rootvar == *varp)
    {
      varobj_update_result r = {0};

      r.varobj = *varp;
      r.status = VAROBJ_IN_SCOPE;

      /* Update the root variable.  value_of_root can return NULL
	 if the variable is no longer around, i.e. we stepped out of
	 the frame in which a local existed.  We are letting the 
	 value_of_root variable dispose of the varobj if the type
	 has changed.  */
      newobj = value_of_root (varp, &type_changed);
      if (update_type_if_necessary(*varp, newobj))
	  type_changed = 1;
      r.varobj = *varp;
      r.type_changed = type_changed;
      if (install_new_value ((*varp), newobj, type_changed))
	r.changed = 1;
      
      if (newobj == NULL)
	r.status = VAROBJ_NOT_IN_SCOPE;
      r.value_installed = 1;

      if (r.status == VAROBJ_NOT_IN_SCOPE)
	{
	  if (r.type_changed || r.changed)
	    VEC_safe_push (varobj_update_result, result, &r);
	  return result;
	}
            
      VEC_safe_push (varobj_update_result, stack, &r);
    }
  else
    {
      varobj_update_result r = {0};

      r.varobj = *varp;
      VEC_safe_push (varobj_update_result, stack, &r);
    }

  /* Walk through the children, reconstructing them all.  */
  while (!VEC_empty (varobj_update_result, stack))
    {
      varobj_update_result r = *(VEC_last (varobj_update_result, stack));
      struct varobj *v = r.varobj;

      VEC_pop (varobj_update_result, stack);

      /* Update this variable, unless it's a root, which is already
	 updated.  */
      if (!r.value_installed)
	{
	  struct type *new_type;

	  newobj = value_of_child (v->parent, v->index);
	  if (update_type_if_necessary(v, newobj))
	    r.type_changed = 1;
	  if (newobj)
	    new_type = value_type (newobj);
	  else
	    new_type = v->root->lang_ops->type_of_child (v->parent, v->index);

	  if (varobj_value_has_mutated (v, newobj, new_type))
	    {
	      /* The children are no longer valid; delete them now.
	         Report the fact that its type changed as well.  */
	      varobj_delete (v, 1 /* only_children */);
	      v->num_children = -1;
	      v->to = -1;
	      v->from = -1;
	      v->type = new_type;
	      r.type_changed = 1;
	    }

	  if (install_new_value (v, newobj, r.type_changed))
	    {
	      r.changed = 1;
	      v->updated = 0;
	    }
	}

      /* We probably should not get children of a dynamic varobj, but
	 for which -var-list-children was never invoked.  */
      if (varobj_is_dynamic_p (v))
	{
	  VEC (varobj_p) *changed = 0, *type_changed = 0, *unchanged = 0;
	  VEC (varobj_p) *newobj = 0;
	  int i, children_changed = 0;

	  if (v->frozen)
	    continue;

	  if (!v->dynamic->children_requested)
	    {
	      int dummy;

	      /* If we initially did not have potential children, but
		 now we do, consider the varobj as changed.
		 Otherwise, if children were never requested, consider
		 it as unchanged -- presumably, such varobj is not yet
		 expanded in the UI, so we need not bother getting
		 it.  */
	      if (!varobj_has_more (v, 0))
		{
		  update_dynamic_varobj_children (v, NULL, NULL, NULL, NULL,
						  &dummy, 0, 0, 0);
		  if (varobj_has_more (v, 0))
		    r.changed = 1;
		}

	      if (r.changed)
		VEC_safe_push (varobj_update_result, result, &r);

	      continue;
	    }

	  /* If update_dynamic_varobj_children returns 0, then we have
	     a non-conforming pretty-printer, so we skip it.  */
	  if (update_dynamic_varobj_children (v, &changed, &type_changed, &newobj,
					      &unchanged, &children_changed, 1,
					      v->from, v->to))
	    {
	      if (children_changed || newobj)
		{
		  r.children_changed = 1;
		  r.newobj = newobj;
		}
	      /* Push in reverse order so that the first child is
		 popped from the work stack first, and so will be
		 added to result first.  This does not affect
		 correctness, just "nicer".  */
	      for (i = VEC_length (varobj_p, type_changed) - 1; i >= 0; --i)
		{
		  varobj_p tmp = VEC_index (varobj_p, type_changed, i);
		  varobj_update_result r = {0};

		  /* Type may change only if value was changed.  */
		  r.varobj = tmp;
		  r.changed = 1;
		  r.type_changed = 1;
		  r.value_installed = 1;
		  VEC_safe_push (varobj_update_result, stack, &r);
		}
	      for (i = VEC_length (varobj_p, changed) - 1; i >= 0; --i)
		{
		  varobj_p tmp = VEC_index (varobj_p, changed, i);
		  varobj_update_result r = {0};

		  r.varobj = tmp;
		  r.changed = 1;
		  r.value_installed = 1;
		  VEC_safe_push (varobj_update_result, stack, &r);
		}
	      for (i = VEC_length (varobj_p, unchanged) - 1; i >= 0; --i)
	      	{
		  varobj_p tmp = VEC_index (varobj_p, unchanged, i);

	      	  if (!tmp->frozen)
	      	    {
	      	      varobj_update_result r = {0};

		      r.varobj = tmp;
	      	      r.value_installed = 1;
	      	      VEC_safe_push (varobj_update_result, stack, &r);
	      	    }
	      	}
	      if (r.changed || r.children_changed)
		VEC_safe_push (varobj_update_result, result, &r);

	      /* Free CHANGED, TYPE_CHANGED and UNCHANGED, but not NEW,
		 because NEW has been put into the result vector.  */
	      VEC_free (varobj_p, changed);
	      VEC_free (varobj_p, type_changed);
	      VEC_free (varobj_p, unchanged);

	      continue;
	    }
	}

      /* Push any children.  Use reverse order so that the first
	 child is popped from the work stack first, and so
	 will be added to result first.  This does not
	 affect correctness, just "nicer".  */
      for (i = VEC_length (varobj_p, v->children)-1; i >= 0; --i)
	{
	  varobj_p c = VEC_index (varobj_p, v->children, i);

	  /* Child may be NULL if explicitly deleted by -var-delete.  */
	  if (c != NULL && !c->frozen)
	    {
	      varobj_update_result r = {0};

	      r.varobj = c;
	      VEC_safe_push (varobj_update_result, stack, &r);
	    }
	}

      if (r.changed || r.type_changed)
	VEC_safe_push (varobj_update_result, result, &r);
    }

  VEC_free (varobj_update_result, stack);

  return result;
}


/* Helper functions */

/*
 * Variable object construction/destruction
 */

static int
delete_variable (struct varobj *var, int only_children_p)
{
  int delcount = 0;

  delete_variable_1 (&delcount, var, only_children_p,
		     1 /* remove_from_parent_p */ );

  return delcount;
}

/* Delete the variable object VAR and its children.  */
/* IMPORTANT NOTE: If we delete a variable which is a child
   and the parent is not removed we dump core.  It must be always
   initially called with remove_from_parent_p set.  */
static void
delete_variable_1 (int *delcountp, struct varobj *var, int only_children_p,
		   int remove_from_parent_p)
{
  int i;

  /* Delete any children of this variable, too.  */
  for (i = 0; i < VEC_length (varobj_p, var->children); ++i)
    {   
      varobj_p child = VEC_index (varobj_p, var->children, i);

      if (!child)
	continue;
      if (!remove_from_parent_p)
	child->parent = NULL;
      delete_variable_1 (delcountp, child, 0, only_children_p);
    }
  VEC_free (varobj_p, var->children);

  /* if we were called to delete only the children we are done here.  */
  if (only_children_p)
    return;

  /* Otherwise, add it to the list of deleted ones and proceed to do so.  */
  /* If the name is empty, this is a temporary variable, that has not
     yet been installed, don't report it, it belongs to the caller...  */
  if (!var->obj_name.empty ())
    {
      *delcountp = *delcountp + 1;
    }

  /* If this variable has a parent, remove it from its parent's list.  */
  /* OPTIMIZATION: if the parent of this variable is also being deleted, 
     (as indicated by remove_from_parent_p) we don't bother doing an
     expensive list search to find the element to remove when we are
     discarding the list afterwards.  */
  if ((remove_from_parent_p) && (var->parent != NULL))
    {
      VEC_replace (varobj_p, var->parent->children, var->index, NULL);
    }

  if (!var->obj_name.empty ())
    uninstall_variable (var);

  /* Free memory associated with this variable.  */
  free_variable (var);
}

/* Install the given variable VAR with the object name VAR->OBJ_NAME.  */
static int
install_variable (struct varobj *var)
{
  struct vlist *cv;
  struct vlist *newvl;
  const char *chp;
  unsigned int index = 0;
  unsigned int i = 1;

  for (chp = var->obj_name.c_str (); *chp; chp++)
    {
      index = (index + (i++ * (unsigned int) *chp)) % VAROBJ_TABLE_SIZE;
    }

  cv = *(varobj_table + index);
  while (cv != NULL && cv->var->obj_name != var->obj_name)
    cv = cv->next;

  if (cv != NULL)
    error (_("Duplicate variable object name"));

  /* Add varobj to hash table.  */
  newvl = XNEW (struct vlist);
  newvl->next = *(varobj_table + index);
  newvl->var = var;
  *(varobj_table + index) = newvl;

  /* If root, add varobj to root list.  */
  if (is_root_p (var))
    {
      /* Add to list of root variables.  */
      if (rootlist == NULL)
	var->root->next = NULL;
      else
	var->root->next = rootlist;
      rootlist = var->root;
    }

  return 1;			/* OK */
}

/* Unistall the object VAR.  */
static void
uninstall_variable (struct varobj *var)
{
  struct vlist *cv;
  struct vlist *prev;
  struct varobj_root *cr;
  struct varobj_root *prer;
  const char *chp;
  unsigned int index = 0;
  unsigned int i = 1;

  /* Remove varobj from hash table.  */
  for (chp = var->obj_name.c_str (); *chp; chp++)
    {
      index = (index + (i++ * (unsigned int) *chp)) % VAROBJ_TABLE_SIZE;
    }

  cv = *(varobj_table + index);
  prev = NULL;
  while (cv != NULL && cv->var->obj_name != var->obj_name)
    {
      prev = cv;
      cv = cv->next;
    }

  if (varobjdebug)
    fprintf_unfiltered (gdb_stdlog, "Deleting %s\n", var->obj_name.c_str ());

  if (cv == NULL)
    {
      warning
	("Assertion failed: Could not find variable object \"%s\" to delete",
	 var->obj_name.c_str ());
      return;
    }

  if (prev == NULL)
    *(varobj_table + index) = cv->next;
  else
    prev->next = cv->next;

  xfree (cv);

  /* If root, remove varobj from root list.  */
  if (is_root_p (var))
    {
      /* Remove from list of root variables.  */
      if (rootlist == var->root)
	rootlist = var->root->next;
      else
	{
	  prer = NULL;
	  cr = rootlist;
	  while ((cr != NULL) && (cr->rootvar != var))
	    {
	      prer = cr;
	      cr = cr->next;
	    }
	  if (cr == NULL)
	    {
	      warning (_("Assertion failed: Could not find "
		         "varobj \"%s\" in root list"),
		       var->obj_name.c_str ());
	      return;
	    }
	  if (prer == NULL)
	    rootlist = NULL;
	  else
	    prer->next = cr->next;
	}
    }

}

/* Create and install a child of the parent of the given name.

   The created VAROBJ takes ownership of the allocated NAME.  */

static struct varobj *
create_child (struct varobj *parent, int index, std::string &name)
{
  struct varobj_item item;

  std::swap (item.name, name);
  item.value = value_of_child (parent, index);

  return create_child_with_value (parent, index, &item);
}

static struct varobj *
create_child_with_value (struct varobj *parent, int index,
			 struct varobj_item *item)
{
  struct varobj *child;

  child = new_variable ();

  /* NAME is allocated by caller.  */
  std::swap (child->name, item->name);
  child->index = index;
  child->parent = parent;
  child->root = parent->root;

  if (varobj_is_anonymous_child (child))
    child->obj_name = string_printf ("%s.%d_anonymous",
				     parent->obj_name.c_str (), index);
  else
    child->obj_name = string_printf ("%s.%s",
				     parent->obj_name.c_str (),
				     child->name.c_str ());

  install_variable (child);

  /* Compute the type of the child.  Must do this before
     calling install_new_value.  */
  if (item->value != NULL)
    /* If the child had no evaluation errors, var->value
       will be non-NULL and contain a valid type.  */
    child->type = value_actual_type (item->value, 0, NULL);
  else
    /* Otherwise, we must compute the type.  */
    child->type = (*child->root->lang_ops->type_of_child) (child->parent,
							   child->index);
  install_new_value (child, item->value, 1);

  return child;
}


/*
 * Miscellaneous utility functions.
 */

/* Allocate memory and initialize a new variable.  */
static struct varobj *
new_variable (void)
{
  struct varobj *var;

  var = new varobj ();
  var->index = -1;
  var->type = NULL;
  var->value = NULL;
  var->num_children = -1;
  var->parent = NULL;
  var->children = NULL;
  var->format = FORMAT_NATURAL;
  var->root = NULL;
  var->updated = 0;
  var->frozen = 0;
  var->not_fetched = 0;
  var->dynamic = XNEW (struct varobj_dynamic);
  var->dynamic->children_requested = 0;
  var->from = -1;
  var->to = -1;
  var->dynamic->constructor = 0;
  var->dynamic->pretty_printer = 0;
  var->dynamic->child_iter = 0;
  var->dynamic->saved_item = 0;

  return var;
}

/* Allocate memory and initialize a new root variable.  */
static struct varobj *
new_root_variable (void)
{
  struct varobj *var = new_variable ();

  var->root = new varobj_root ();
  var->root->lang_ops = NULL;
  var->root->exp = NULL;
  var->root->valid_block = NULL;
  var->root->frame = null_frame_id;
  var->root->floating = 0;
  var->root->rootvar = NULL;
  var->root->is_valid = 1;

  return var;
}

/* Free any allocated memory associated with VAR.  */
static void
free_variable (struct varobj *var)
{
#if HAVE_PYTHON
  if (var->dynamic->pretty_printer != NULL)
    {
      gdbpy_enter_varobj enter_py (var);

      Py_XDECREF (var->dynamic->constructor);
      Py_XDECREF (var->dynamic->pretty_printer);
    }
#endif

  varobj_iter_delete (var->dynamic->child_iter);
  varobj_clear_saved_item (var->dynamic);
  value_free (var->value);

  if (is_root_p (var))
    delete var->root;

  xfree (var->dynamic);
  delete var;
}

static void
do_free_variable_cleanup (void *var)
{
  free_variable ((struct varobj *) var);
}

static struct cleanup *
make_cleanup_free_variable (struct varobj *var)
{
  return make_cleanup (do_free_variable_cleanup, var);
}

/* Return the type of the value that's stored in VAR,
   or that would have being stored there if the
   value were accessible.

   This differs from VAR->type in that VAR->type is always
   the true type of the expession in the source language.
   The return value of this function is the type we're
   actually storing in varobj, and using for displaying
   the values and for comparing previous and new values.

   For example, top-level references are always stripped.  */
struct type *
varobj_get_value_type (const struct varobj *var)
{
  struct type *type;

  if (var->value)
    type = value_type (var->value);
  else
    type = var->type;

  type = check_typedef (type);

  if (TYPE_IS_REFERENCE (type))
    type = get_target_type (type);

  type = check_typedef (type);

  return type;
}

/* What is the default display for this variable? We assume that
   everything is "natural".  Any exceptions?  */
static enum varobj_display_formats
variable_default_display (struct varobj *var)
{
  return FORMAT_NATURAL;
}

/*
 * Language-dependencies
 */

/* Common entry points */

/* Return the number of children for a given variable.
   The result of this function is defined by the language
   implementation.  The number of children returned by this function
   is the number of children that the user will see in the variable
   display.  */
static int
number_of_children (const struct varobj *var)
{
  return (*var->root->lang_ops->number_of_children) (var);
}

/* What is the expression for the root varobj VAR? */

static std::string
name_of_variable (const struct varobj *var)
{
  return (*var->root->lang_ops->name_of_variable) (var);
}

/* What is the name of the INDEX'th child of VAR?  */

static std::string
name_of_child (struct varobj *var, int index)
{
  return (*var->root->lang_ops->name_of_child) (var, index);
}

/* If frame associated with VAR can be found, switch
   to it and return 1.  Otherwise, return 0.  */

static int
check_scope (const struct varobj *var)
{
  struct frame_info *fi;
  int scope;

  fi = frame_find_by_id (var->root->frame);
  scope = fi != NULL;

  if (fi)
    {
      CORE_ADDR pc = get_frame_pc (fi);

      if (pc <  BLOCK_START (var->root->valid_block) ||
	  pc >= BLOCK_END (var->root->valid_block))
	scope = 0;
      else
	select_frame (fi);
    }
  return scope;
}

/* Helper function to value_of_root.  */

static struct value *
value_of_root_1 (struct varobj **var_handle)
{
  struct value *new_val = NULL;
  struct varobj *var = *var_handle;
  int within_scope = 0;
  struct cleanup *back_to;
								 
  /*  Only root variables can be updated...  */
  if (!is_root_p (var))
    /* Not a root var.  */
    return NULL;

  back_to = make_cleanup_restore_current_thread ();

  /* Determine whether the variable is still around.  */
  if (var->root->valid_block == NULL || var->root->floating)
    within_scope = 1;
  else if (var->root->thread_id == 0)
    {
      /* The program was single-threaded when the variable object was
	 created.  Technically, it's possible that the program became
	 multi-threaded since then, but we don't support such
	 scenario yet.  */
      within_scope = check_scope (var);	  
    }
  else
    {
      ptid_t ptid = global_thread_id_to_ptid (var->root->thread_id);

      if (!ptid_equal (minus_one_ptid, ptid))
	{
	  switch_to_thread (ptid);
	  within_scope = check_scope (var);
	}
    }

  if (within_scope)
    {

      /* We need to catch errors here, because if evaluate
         expression fails we want to just return NULL.  */
      TRY
	{
	  new_val = evaluate_expression (var->root->exp.get ());
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	}
      END_CATCH
    }

  do_cleanups (back_to);

  return new_val;
}

/* What is the ``struct value *'' of the root variable VAR?
   For floating variable object, evaluation can get us a value
   of different type from what is stored in varobj already.  In
   that case:
   - *type_changed will be set to 1
   - old varobj will be freed, and new one will be
   created, with the same name.
   - *var_handle will be set to the new varobj 
   Otherwise, *type_changed will be set to 0.  */
static struct value *
value_of_root (struct varobj **var_handle, int *type_changed)
{
  struct varobj *var;

  if (var_handle == NULL)
    return NULL;

  var = *var_handle;

  /* This should really be an exception, since this should
     only get called with a root variable.  */

  if (!is_root_p (var))
    return NULL;

  if (var->root->floating)
    {
      struct varobj *tmp_var;

      tmp_var = varobj_create (NULL, var->name.c_str (), (CORE_ADDR) 0,
			       USE_SELECTED_FRAME);
      if (tmp_var == NULL)
	{
	  return NULL;
	}
      std::string old_type = varobj_get_type (var);
      std::string new_type = varobj_get_type (tmp_var);
      if (old_type == new_type)
	{
	  /* The expression presently stored inside var->root->exp
	     remembers the locations of local variables relatively to
	     the frame where the expression was created (in DWARF location
	     button, for example).  Naturally, those locations are not
	     correct in other frames, so update the expression.  */

	  std::swap (var->root->exp, tmp_var->root->exp);

	  varobj_delete (tmp_var, 0);
	  *type_changed = 0;
	}
      else
	{
	  tmp_var->obj_name = var->obj_name;
	  tmp_var->from = var->from;
	  tmp_var->to = var->to;
	  varobj_delete (var, 0);

	  install_variable (tmp_var);
	  *var_handle = tmp_var;
	  var = *var_handle;
	  *type_changed = 1;
	}
    }
  else
    {
      *type_changed = 0;
    }

  {
    struct value *value;

    value = value_of_root_1 (var_handle);
    if (var->value == NULL || value == NULL)
      {
	/* For root varobj-s, a NULL value indicates a scoping issue.
	   So, nothing to do in terms of checking for mutations.  */
      }
    else if (varobj_value_has_mutated (var, value, value_type (value)))
      {
	/* The type has mutated, so the children are no longer valid.
	   Just delete them, and tell our caller that the type has
	   changed.  */
	varobj_delete (var, 1 /* only_children */);
	var->num_children = -1;
	var->to = -1;
	var->from = -1;
	*type_changed = 1;
      }
    return value;
  }
}

/* What is the ``struct value *'' for the INDEX'th child of PARENT?  */
static struct value *
value_of_child (const struct varobj *parent, int index)
{
  struct value *value;

  value = (*parent->root->lang_ops->value_of_child) (parent, index);

  return value;
}

/* GDB already has a command called "value_of_variable".  Sigh.  */
static std::string
my_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  if (var->root->is_valid)
    {
      if (var->dynamic->pretty_printer != NULL)
	return varobj_value_get_print_value (var->value, var->format, var);
      return (*var->root->lang_ops->value_of_variable) (var, format);
    }
  else
    return std::string ();
}

void
varobj_formatted_print_options (struct value_print_options *opts,
				enum varobj_display_formats format)
{
  get_formatted_print_options (opts, format_code[(int) format]);
  opts->deref_ref = 0;
  opts->raw = 1;
}

std::string
varobj_value_get_print_value (struct value *value,
			      enum varobj_display_formats format,
			      const struct varobj *var)
{
  struct value_print_options opts;
  struct type *type = NULL;
  long len = 0;
  gdb::unique_xmalloc_ptr<char> encoding;
  /* Initialize it just to avoid a GCC false warning.  */
  CORE_ADDR str_addr = 0;
  int string_print = 0;

  if (value == NULL)
    return std::string ();

  string_file stb;
  std::string thevalue;

#if HAVE_PYTHON
  if (gdb_python_initialized)
    {
      PyObject *value_formatter =  var->dynamic->pretty_printer;

      gdbpy_enter_varobj enter_py (var);

      if (value_formatter)
	{
	  /* First check to see if we have any children at all.  If so,
	     we simply return {...}.  */
	  if (dynamic_varobj_has_child_method (var))
	    return "{...}";

	  if (PyObject_HasAttr (value_formatter, gdbpy_to_string_cst))
	    {
	      struct value *replacement;

	      gdbpy_ref<> output (apply_varobj_pretty_printer (value_formatter,
							       &replacement,
							       &stb));

	      /* If we have string like output ...  */
	      if (output != NULL)
		{
		  /* If this is a lazy string, extract it.  For lazy
		     strings we always print as a string, so set
		     string_print.  */
		  if (gdbpy_is_lazy_string (output.get ()))
		    {
		      gdbpy_extract_lazy_string (output.get (), &str_addr,
						 &type, &len, &encoding);
		      string_print = 1;
		    }
		  else
		    {
		      /* If it is a regular (non-lazy) string, extract
			 it and copy the contents into THEVALUE.  If the
			 hint says to print it as a string, set
			 string_print.  Otherwise just return the extracted
			 string as a value.  */

		      gdb::unique_xmalloc_ptr<char> s
			= python_string_to_target_string (output.get ());

		      if (s)
			{
			  struct gdbarch *gdbarch;

			  gdb::unique_xmalloc_ptr<char> hint
			    = gdbpy_get_display_hint (value_formatter);
			  if (hint)
			    {
			      if (!strcmp (hint.get (), "string"))
				string_print = 1;
			    }

			  thevalue = std::string (s.get ());
			  len = thevalue.size ();
			  gdbarch = get_type_arch (value_type (value));
			  type = builtin_type (gdbarch)->builtin_char;

			  if (!string_print)
			    return thevalue;
			}
		      else
			gdbpy_print_stack ();
		    }
		}
	      /* If the printer returned a replacement value, set VALUE
		 to REPLACEMENT.  If there is not a replacement value,
		 just use the value passed to this function.  */
	      if (replacement)
		value = replacement;
	    }
	}
    }
#endif

  varobj_formatted_print_options (&opts, format);

  /* If the THEVALUE has contents, it is a regular string.  */
  if (!thevalue.empty ())
    LA_PRINT_STRING (&stb, type, (gdb_byte *) thevalue.c_str (),
		     len, encoding.get (), 0, &opts);
  else if (string_print)
    /* Otherwise, if string_print is set, and it is not a regular
       string, it is a lazy string.  */
    val_print_string (type, encoding.get (), str_addr, len, &stb, &opts);
  else
    /* All other cases.  */
    common_val_print (value, &stb, 0, &opts, current_language);

  return std::move (stb.string ());
}

int
varobj_editable_p (const struct varobj *var)
{
  struct type *type;

  if (!(var->root->is_valid && var->value && VALUE_LVAL (var->value)))
    return 0;

  type = varobj_get_value_type (var);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_METHOD:
      return 0;
      break;

    default:
      return 1;
      break;
    }
}

/* Call VAR's value_is_changeable_p language-specific callback.  */

int
varobj_value_is_changeable_p (const struct varobj *var)
{
  return var->root->lang_ops->value_is_changeable_p (var);
}

/* Return 1 if that varobj is floating, that is is always evaluated in the
   selected frame, and not bound to thread/frame.  Such variable objects
   are created using '@' as frame specifier to -var-create.  */
int
varobj_floating_p (const struct varobj *var)
{
  return var->root->floating;
}

/* Implement the "value_is_changeable_p" varobj callback for most
   languages.  */

int
varobj_default_value_is_changeable_p (const struct varobj *var)
{
  int r;
  struct type *type;

  if (CPLUS_FAKE_CHILD (var))
    return 0;

  type = varobj_get_value_type (var);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_ARRAY:
      r = 0;
      break;

    default:
      r = 1;
    }

  return r;
}

/* Iterate all the existing _root_ VAROBJs and call the FUNC callback for them
   with an arbitrary caller supplied DATA pointer.  */

void
all_root_varobjs (void (*func) (struct varobj *var, void *data), void *data)
{
  struct varobj_root *var_root, *var_root_next;

  /* Iterate "safely" - handle if the callee deletes its passed VAROBJ.  */

  for (var_root = rootlist; var_root != NULL; var_root = var_root_next)
    {
      var_root_next = var_root->next;

      (*func) (var_root->rootvar, data);
    }
}

/* Invalidate varobj VAR if it is tied to locals and re-create it if it is
   defined on globals.  It is a helper for varobj_invalidate.

   This function is called after changing the symbol file, in this case the
   pointers to "struct type" stored by the varobj are no longer valid.  All
   varobj must be either re-evaluated, or marked as invalid here.  */

static void
varobj_invalidate_iter (struct varobj *var, void *unused)
{
  /* global and floating var must be re-evaluated.  */
  if (var->root->floating || var->root->valid_block == NULL)
    {
      struct varobj *tmp_var;

      /* Try to create a varobj with same expression.  If we succeed
	 replace the old varobj, otherwise invalidate it.  */
      tmp_var = varobj_create (NULL, var->name.c_str (), (CORE_ADDR) 0,
			       USE_CURRENT_FRAME);
      if (tmp_var != NULL) 
	{ 
	  tmp_var->obj_name = var->obj_name;
	  varobj_delete (var, 0);
	  install_variable (tmp_var);
	}
      else
	var->root->is_valid = 0;
    }
  else /* locals must be invalidated.  */
    var->root->is_valid = 0;
}

/* Invalidate the varobjs that are tied to locals and re-create the ones that
   are defined on globals.
   Invalidated varobjs will be always printed in_scope="invalid".  */

void 
varobj_invalidate (void)
{
  all_root_varobjs (varobj_invalidate_iter, NULL);
}

extern void _initialize_varobj (void);
void
_initialize_varobj (void)
{
  varobj_table = XCNEWVEC (struct vlist *, VAROBJ_TABLE_SIZE);

  add_setshow_zuinteger_cmd ("varobj", class_maintenance,
			     &varobjdebug,
			     _("Set varobj debugging."),
			     _("Show varobj debugging."),
			     _("When non-zero, varobj debugging is enabled."),
			     NULL, show_varobjdebug,
			     &setdebuglist, &showdebuglist);
}
