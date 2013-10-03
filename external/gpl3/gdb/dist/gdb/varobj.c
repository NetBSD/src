/* Implementation of the GDB variable objects API.

   Copyright (C) 1999-2013 Free Software Foundation, Inc.

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
#include "exceptions.h"
#include "value.h"
#include "expression.h"
#include "frame.h"
#include "language.h"
#include "gdbcmd.h"
#include "block.h"
#include "valprint.h"

#include "gdb_assert.h"
#include "gdb_string.h"
#include "gdb_regex.h"

#include "varobj.h"
#include "vec.h"
#include "gdbthread.h"
#include "inferior.h"
#include "ada-varobj.h"
#include "ada-lang.h"

#if HAVE_PYTHON
#include "python/python.h"
#include "python/python-internal.h"
#else
typedef int PyObject;
#endif

/* The names of varobjs representing anonymous structs or unions.  */
#define ANONYMOUS_STRUCT_NAME _("<anonymous struct>")
#define ANONYMOUS_UNION_NAME _("<anonymous union>")

/* Non-zero if we want to see trace of varobj level stuff.  */

unsigned int varobjdebug = 0;
static void
show_varobjdebug (struct ui_file *file, int from_tty,
		  struct cmd_list_element *c, const char *value)
{
  fprintf_filtered (file, _("Varobj debugging is %s.\n"), value);
}

/* String representations of gdb's format codes.  */
char *varobj_format_string[] =
  { "natural", "binary", "decimal", "hexadecimal", "octal" };

/* String representations of gdb's known languages.  */
char *varobj_language_string[] = { "unknown", "C", "C++", "Java" };

/* True if we want to allow Python-based pretty-printing.  */
static int pretty_printing = 0;

void
varobj_enable_pretty_printing (void)
{
  pretty_printing = 1;
}

/* Data structures */

/* Every root variable has one of these structures saved in its
   varobj.  Members which must be free'd are noted.  */
struct varobj_root
{

  /* Alloc'd expression for this parent.  */
  struct expression *exp;

  /* Block for which this expression is valid.  */
  const struct block *valid_block;

  /* The frame for this expression.  This field is set iff valid_block is
     not NULL.  */
  struct frame_id frame;

  /* The thread ID that this varobj_root belong to.  This field
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

  /* Language info for this variable and its children.  */
  struct language_specific *lang;

  /* The varobj for this root node.  */
  struct varobj *rootvar;

  /* Next root variable */
  struct varobj_root *next;
};

/* Every variable in the system has a structure of this type defined
   for it.  This structure holds all information necessary to manipulate
   a particular object variable.  Members which must be freed are noted.  */
struct varobj
{

  /* Alloc'd name of the variable for this object.  If this variable is a
     child, then this name will be the child's source name.
     (bar, not foo.bar).  */
  /* NOTE: This is the "expression".  */
  char *name;

  /* Alloc'd expression for this child.  Can be used to create a
     root variable corresponding to this child.  */
  char *path_expr;

  /* The alloc'd name for this variable's object.  This is here for
     convenience when constructing this object's children.  */
  char *obj_name;

  /* Index of this variable in its parent or -1.  */
  int index;

  /* The type of this variable.  This can be NULL
     for artifial variable objects -- currently, the "accessibility" 
     variable objects in C++.  */
  struct type *type;

  /* The value of this expression or subexpression.  A NULL value
     indicates there was an error getting this value.
     Invariant: if varobj_value_is_changeable_p (this) is non-zero, 
     the value is either NULL, or not lazy.  */
  struct value *value;

  /* The number of (immediate) children this variable has.  */
  int num_children;

  /* If this object is a child, this points to its immediate parent.  */
  struct varobj *parent;

  /* Children of this object.  */
  VEC (varobj_p) *children;

  /* Whether the children of this varobj were requested.  This field is
     used to decide if dynamic varobj should recompute their children.
     In the event that the frontend never asked for the children, we
     can avoid that.  */
  int children_requested;

  /* Description of the root variable.  Points to root variable for
     children.  */
  struct varobj_root *root;

  /* The format of the output for this object.  */
  enum varobj_display_formats format;

  /* Was this variable updated via a varobj_set_value operation.  */
  int updated;

  /* Last print value.  */
  char *print_value;

  /* Is this variable frozen.  Frozen variables are never implicitly
     updated by -var-update * 
     or -var-update <direct-or-indirect-parent>.  */
  int frozen;

  /* Is the value of this variable intentionally not fetched?  It is
     not fetched if either the variable is frozen, or any parents is
     frozen.  */
  int not_fetched;

  /* Sub-range of children which the MI consumer has requested.  If
     FROM < 0 or TO < 0, means that all children have been
     requested.  */
  int from;
  int to;

  /* The pretty-printer constructor.  If NULL, then the default
     pretty-printer will be looked up.  If None, then no
     pretty-printer will be installed.  */
  PyObject *constructor;

  /* The pretty-printer that has been constructed.  If NULL, then a
     new printer object is needed, and one will be constructed.  */
  PyObject *pretty_printer;

  /* The iterator returned by the printer's 'children' method, or NULL
     if not available.  */
  PyObject *child_iter;

  /* We request one extra item from the iterator, so that we can
     report to the caller whether there are more items than we have
     already reported.  However, we don't want to install this value
     when we read it, because that will mess up future updates.  So,
     we stash it here instead.  */
  PyObject *saved_item;
};

struct cpstack
{
  char *name;
  struct cpstack *next;
};

/* A list of varobjs */

struct vlist
{
  struct varobj *var;
  struct vlist *next;
};

/* Private function prototypes */

/* Helper functions for the above subcommands.  */

static int delete_variable (struct cpstack **, struct varobj *, int);

static void delete_variable_1 (struct cpstack **, int *,
			       struct varobj *, int, int);

static int install_variable (struct varobj *);

static void uninstall_variable (struct varobj *);

static struct varobj *create_child (struct varobj *, int, char *);

static struct varobj *
create_child_with_value (struct varobj *parent, int index, const char *name,
			 struct value *value);

/* Utility routines */

static struct varobj *new_variable (void);

static struct varobj *new_root_variable (void);

static void free_variable (struct varobj *var);

static struct cleanup *make_cleanup_free_variable (struct varobj *var);

static struct type *get_type (struct varobj *var);

static struct type *get_value_type (struct varobj *var);

static struct type *get_target_type (struct type *);

static enum varobj_display_formats variable_default_display (struct varobj *);

static void cppush (struct cpstack **pstack, char *name);

static char *cppop (struct cpstack **pstack);

static int update_type_if_necessary (struct varobj *var,
				     struct value *new_value);

static int install_new_value (struct varobj *var, struct value *value, 
			      int initial);

/* Language-specific routines.  */

static enum varobj_languages variable_language (struct varobj *var);

static int number_of_children (struct varobj *);

static char *name_of_variable (struct varobj *);

static char *name_of_child (struct varobj *, int);

static struct value *value_of_root (struct varobj **var_handle, int *);

static struct value *value_of_child (struct varobj *parent, int index);

static char *my_value_of_variable (struct varobj *var,
				   enum varobj_display_formats format);

static char *value_get_print_value (struct value *value,
				    enum varobj_display_formats format,
				    struct varobj *var);

static int varobj_value_is_changeable_p (struct varobj *var);

static int is_root_p (struct varobj *var);

#if HAVE_PYTHON

static struct varobj *varobj_add_child (struct varobj *var,
					const char *name,
					struct value *value);

#endif /* HAVE_PYTHON */

static int default_value_is_changeable_p (struct varobj *var);

/* C implementation */

static int c_number_of_children (struct varobj *var);

static char *c_name_of_variable (struct varobj *parent);

static char *c_name_of_child (struct varobj *parent, int index);

static char *c_path_expr_of_child (struct varobj *child);

static struct value *c_value_of_root (struct varobj **var_handle);

static struct value *c_value_of_child (struct varobj *parent, int index);

static struct type *c_type_of_child (struct varobj *parent, int index);

static char *c_value_of_variable (struct varobj *var,
				  enum varobj_display_formats format);

/* C++ implementation */

static int cplus_number_of_children (struct varobj *var);

static void cplus_class_num_children (struct type *type, int children[3]);

static char *cplus_name_of_variable (struct varobj *parent);

static char *cplus_name_of_child (struct varobj *parent, int index);

static char *cplus_path_expr_of_child (struct varobj *child);

static struct value *cplus_value_of_root (struct varobj **var_handle);

static struct value *cplus_value_of_child (struct varobj *parent, int index);

static struct type *cplus_type_of_child (struct varobj *parent, int index);

static char *cplus_value_of_variable (struct varobj *var,
				      enum varobj_display_formats format);

/* Java implementation */

static int java_number_of_children (struct varobj *var);

static char *java_name_of_variable (struct varobj *parent);

static char *java_name_of_child (struct varobj *parent, int index);

static char *java_path_expr_of_child (struct varobj *child);

static struct value *java_value_of_root (struct varobj **var_handle);

static struct value *java_value_of_child (struct varobj *parent, int index);

static struct type *java_type_of_child (struct varobj *parent, int index);

static char *java_value_of_variable (struct varobj *var,
				     enum varobj_display_formats format);

/* Ada implementation */

static int ada_number_of_children (struct varobj *var);

static char *ada_name_of_variable (struct varobj *parent);

static char *ada_name_of_child (struct varobj *parent, int index);

static char *ada_path_expr_of_child (struct varobj *child);

static struct value *ada_value_of_root (struct varobj **var_handle);

static struct value *ada_value_of_child (struct varobj *parent, int index);

static struct type *ada_type_of_child (struct varobj *parent, int index);

static char *ada_value_of_variable (struct varobj *var,
				    enum varobj_display_formats format);

static int ada_value_is_changeable_p (struct varobj *var);

static int ada_value_has_mutated (struct varobj *var, struct value *new_val,
				  struct type *new_type);

/* The language specific vector */

struct language_specific
{

  /* The language of this variable.  */
  enum varobj_languages language;

  /* The number of children of PARENT.  */
  int (*number_of_children) (struct varobj * parent);

  /* The name (expression) of a root varobj.  */
  char *(*name_of_variable) (struct varobj * parent);

  /* The name of the INDEX'th child of PARENT.  */
  char *(*name_of_child) (struct varobj * parent, int index);

  /* Returns the rooted expression of CHILD, which is a variable
     obtain that has some parent.  */
  char *(*path_expr_of_child) (struct varobj * child);

  /* The ``struct value *'' of the root variable ROOT.  */
  struct value *(*value_of_root) (struct varobj ** root_handle);

  /* The ``struct value *'' of the INDEX'th child of PARENT.  */
  struct value *(*value_of_child) (struct varobj * parent, int index);

  /* The type of the INDEX'th child of PARENT.  */
  struct type *(*type_of_child) (struct varobj * parent, int index);

  /* The current value of VAR.  */
  char *(*value_of_variable) (struct varobj * var,
			      enum varobj_display_formats format);

  /* Return non-zero if changes in value of VAR must be detected and
     reported by -var-update.  Return zero if -var-update should never
     report changes of such values.  This makes sense for structures
     (since the changes in children values will be reported separately),
     or for artifical objects (like 'public' pseudo-field in C++).

     Return value of 0 means that gdb need not call value_fetch_lazy
     for the value of this variable object.  */
  int (*value_is_changeable_p) (struct varobj *var);

  /* Return nonzero if the type of VAR has mutated.

     VAR's value is still the varobj's previous value, while NEW_VALUE
     is VAR's new value and NEW_TYPE is the var's new type.  NEW_VALUE
     may be NULL indicating that there is no value available (the varobj
     may be out of scope, of may be the child of a null pointer, for
     instance).  NEW_TYPE, on the other hand, must never be NULL.

     This function should also be able to assume that var's number of
     children is set (not < 0).

     Languages where types do not mutate can set this to NULL.  */
  int (*value_has_mutated) (struct varobj *var, struct value *new_value,
			    struct type *new_type);
};

/* Array of known source language routines.  */
static struct language_specific languages[vlang_end] = {
  /* Unknown (try treating as C).  */
  {
   vlang_unknown,
   c_number_of_children,
   c_name_of_variable,
   c_name_of_child,
   c_path_expr_of_child,
   c_value_of_root,
   c_value_of_child,
   c_type_of_child,
   c_value_of_variable,
   default_value_is_changeable_p,
   NULL /* value_has_mutated */}
  ,
  /* C */
  {
   vlang_c,
   c_number_of_children,
   c_name_of_variable,
   c_name_of_child,
   c_path_expr_of_child,
   c_value_of_root,
   c_value_of_child,
   c_type_of_child,
   c_value_of_variable,
   default_value_is_changeable_p,
   NULL /* value_has_mutated */}
  ,
  /* C++ */
  {
   vlang_cplus,
   cplus_number_of_children,
   cplus_name_of_variable,
   cplus_name_of_child,
   cplus_path_expr_of_child,
   cplus_value_of_root,
   cplus_value_of_child,
   cplus_type_of_child,
   cplus_value_of_variable,
   default_value_is_changeable_p,
   NULL /* value_has_mutated */}
  ,
  /* Java */
  {
   vlang_java,
   java_number_of_children,
   java_name_of_variable,
   java_name_of_child,
   java_path_expr_of_child,
   java_value_of_root,
   java_value_of_child,
   java_type_of_child,
   java_value_of_variable,
   default_value_is_changeable_p,
   NULL /* value_has_mutated */},
  /* Ada */
  {
   vlang_ada,
   ada_number_of_children,
   ada_name_of_variable,
   ada_name_of_child,
   ada_path_expr_of_child,
   ada_value_of_root,
   ada_value_of_child,
   ada_type_of_child,
   ada_value_of_variable,
   ada_value_is_changeable_p,
   ada_value_has_mutated}
};

/* A little convenience enum for dealing with C++/Java.  */
enum vsections
{
  v_public = 0, v_private, v_protected
};

/* Private data */

/* Mappings of varobj_display_formats enums to gdb's format codes.  */
static int format_code[] = { 0, 't', 'd', 'x', 'o' };

/* Header of the list of root variable objects.  */
static struct varobj_root *rootlist;

/* Prime number indicating the number of buckets in the hash table.  */
/* A prime large enough to avoid too many colisions.  */
#define VAROBJ_TABLE_SIZE 227

/* Pointer to the varobj hash table (built at run time).  */
static struct vlist **varobj_table;

/* Is the variable X one of our "fake" children?  */
#define CPLUS_FAKE_CHILD(x) \
((x) != NULL && (x)->type == NULL && (x)->value == NULL)


/* API Implementation */
static int
is_root_p (struct varobj *var)
{
  return (var->root->rootvar == var);
}

#ifdef HAVE_PYTHON
/* Helper function to install a Python environment suitable for
   use during operations on VAR.  */
static struct cleanup *
varobj_ensure_python_env (struct varobj *var)
{
  return ensure_python_env (var->root->exp->gdbarch,
			    var->root->exp->language_defn);
}
#endif

/* Creates a varobj (not its children).  */

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

struct varobj *
varobj_create (char *objname,
	       char *expression, CORE_ADDR frame, enum varobj_type type)
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
      struct block *block;
      const char *p;
      enum varobj_languages lang;
      struct value *value = NULL;
      volatile struct gdb_exception except;
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
      TRY_CATCH (except, RETURN_MASK_ERROR)
	{
	  var->root->exp = parse_exp_1 (&p, pc, block, 0);
	}

      if (except.reason < 0)
	{
	  do_cleanups (old_chain);
	  return NULL;
	}

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
      var->name = xstrdup (expression);
      /* For a root var, the name and the expr are the same.  */
      var->path_expr = xstrdup (expression);

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
	  var->root->thread_id = pid_to_thread_id (inferior_ptid);
	  old_id = get_frame_id (get_selected_frame (NULL));
	  select_frame (fi);	 
	}

      /* We definitely need to catch errors here.
         If evaluate_expression succeeds we got the value we wanted.
         But if it fails, we still go on with a call to evaluate_type().  */
      TRY_CATCH (except, RETURN_MASK_ERROR)
	{
	  value = evaluate_expression (var->root->exp);
	}

      if (except.reason < 0)
	{
	  /* Error getting the value.  Try to at least get the
	     right type.  */
	  struct value *type_only_value = evaluate_type (var->root->exp);

	  var->type = value_type (type_only_value);
	}
	else
	  {
	    int real_type_found = 0;

	    var->type = value_actual_type (value, 0, &real_type_found);
	    if (real_type_found)
	      value = value_cast (var->type, value);
	  }

      /* Set language info */
      lang = variable_language (var);
      var->root->lang = &languages[lang];

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
      var->obj_name = xstrdup (objname);

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
varobj_get_handle (char *objname)
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
  while ((cv != NULL) && (strcmp (cv->var->obj_name, objname) != 0))
    cv = cv->next;

  if (cv == NULL)
    error (_("Variable object not found"));

  return cv->var;
}

/* Given the handle, return the name of the object.  */

char *
varobj_get_objname (struct varobj *var)
{
  return var->obj_name;
}

/* Given the handle, return the expression represented by the object.  */

char *
varobj_get_expression (struct varobj *var)
{
  return name_of_variable (var);
}

/* Deletes a varobj and all its children if only_children == 0,
   otherwise deletes only the children; returns a malloc'ed list of
   all the (malloc'ed) names of the variables that have been deleted
   (NULL terminated).  */

int
varobj_delete (struct varobj *var, char ***dellist, int only_children)
{
  int delcount;
  int mycount;
  struct cpstack *result = NULL;
  char **cp;

  /* Initialize a stack for temporary results.  */
  cppush (&result, NULL);

  if (only_children)
    /* Delete only the variable children.  */
    delcount = delete_variable (&result, var, 1 /* only the children */ );
  else
    /* Delete the variable and all its children.  */
    delcount = delete_variable (&result, var, 0 /* parent+children */ );

  /* We may have been asked to return a list of what has been deleted.  */
  if (dellist != NULL)
    {
      *dellist = xmalloc ((delcount + 1) * sizeof (char *));

      cp = *dellist;
      mycount = delcount;
      *cp = cppop (&result);
      while ((*cp != NULL) && (mycount > 0))
	{
	  mycount--;
	  cp++;
	  *cp = cppop (&result);
	}

      if (mycount || (*cp != NULL))
	warning (_("varobj_delete: assertion failed - mycount(=%d) <> 0"),
		 mycount);
    }

  return delcount;
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
      var->format = format;
      break;

    default:
      var->format = variable_default_display (var);
    }

  if (varobj_value_is_changeable_p (var) 
      && var->value && !value_lazy (var->value))
    {
      xfree (var->print_value);
      var->print_value = value_get_print_value (var->value, var->format, var);
    }

  return var->format;
}

enum varobj_display_formats
varobj_get_display_format (struct varobj *var)
{
  return var->format;
}

char *
varobj_get_display_hint (struct varobj *var)
{
  char *result = NULL;

#if HAVE_PYTHON
  struct cleanup *back_to = varobj_ensure_python_env (var);

  if (var->pretty_printer)
    result = gdbpy_get_display_hint (var->pretty_printer);

  do_cleanups (back_to);
#endif

  return result;
}

/* Return true if the varobj has items after TO, false otherwise.  */

int
varobj_has_more (struct varobj *var, int to)
{
  if (VEC_length (varobj_p, var->children) > to)
    return 1;
  return ((to == -1 || VEC_length (varobj_p, var->children) == to)
	  && var->saved_item != NULL);
}

/* If the variable object is bound to a specific thread, that
   is its evaluation can always be done in context of a frame
   inside that thread, returns GDB id of the thread -- which
   is always positive.  Otherwise, returns -1.  */
int
varobj_get_thread_id (struct varobj *var)
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
varobj_get_frozen (struct varobj *var)
{
  return var->frozen;
}

/* A helper function that restricts a range to what is actually
   available in a VEC.  This follows the usual rules for the meaning
   of FROM and TO -- if either is negative, the entire range is
   used.  */

static void
restrict_range (VEC (varobj_p) *children, int *from, int *to)
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

#if HAVE_PYTHON

/* A helper for update_dynamic_varobj_children that installs a new
   child when needed.  */

static void
install_dynamic_child (struct varobj *var,
		       VEC (varobj_p) **changed,
		       VEC (varobj_p) **type_changed,
		       VEC (varobj_p) **new,
		       VEC (varobj_p) **unchanged,
		       int *cchanged,
		       int index,
		       const char *name,
		       struct value *value)
{
  if (VEC_length (varobj_p, var->children) < index + 1)
    {
      /* There's no child yet.  */
      struct varobj *child = varobj_add_child (var, name, value);

      if (new)
	{
	  VEC_safe_push (varobj_p, *new, child);
	  *cchanged = 1;
	}
    }
  else 
    {
      varobj_p existing = VEC_index (varobj_p, var->children, index);

      int type_updated = update_type_if_necessary (existing, value);
      if (type_updated)
	{
	  if (type_changed)
	    VEC_safe_push (varobj_p, *type_changed, existing);
	}
      if (install_new_value (existing, value, 0))
	{
	  if (!type_updated && changed)
	    VEC_safe_push (varobj_p, *changed, existing);
	}
      else if (!type_updated && unchanged)
	VEC_safe_push (varobj_p, *unchanged, existing);
    }
}

static int
dynamic_varobj_has_child_method (struct varobj *var)
{
  struct cleanup *back_to;
  PyObject *printer = var->pretty_printer;
  int result;

  back_to = varobj_ensure_python_env (var);
  result = PyObject_HasAttr (printer, gdbpy_children_cst);
  do_cleanups (back_to);
  return result;
}

#endif

static int
update_dynamic_varobj_children (struct varobj *var,
				VEC (varobj_p) **changed,
				VEC (varobj_p) **type_changed,
				VEC (varobj_p) **new,
				VEC (varobj_p) **unchanged,
				int *cchanged,
				int update_children,
				int from,
				int to)
{
#if HAVE_PYTHON
  struct cleanup *back_to;
  PyObject *children;
  int i;
  PyObject *printer = var->pretty_printer;

  back_to = varobj_ensure_python_env (var);

  *cchanged = 0;
  if (!PyObject_HasAttr (printer, gdbpy_children_cst))
    {
      do_cleanups (back_to);
      return 0;
    }

  if (update_children || !var->child_iter)
    {
      children = PyObject_CallMethodObjArgs (printer, gdbpy_children_cst,
					     NULL);

      if (!children)
	{
	  gdbpy_print_stack ();
	  error (_("Null value returned for children"));
	}

      make_cleanup_py_decref (children);

      Py_XDECREF (var->child_iter);
      var->child_iter = PyObject_GetIter (children);
      if (!var->child_iter)
	{
	  gdbpy_print_stack ();
	  error (_("Could not get children iterator"));
	}

      Py_XDECREF (var->saved_item);
      var->saved_item = NULL;

      i = 0;
    }
  else
    i = VEC_length (varobj_p, var->children);

  /* We ask for one extra child, so that MI can report whether there
     are more children.  */
  for (; to < 0 || i < to + 1; ++i)
    {
      PyObject *item;
      int force_done = 0;

      /* See if there was a leftover from last time.  */
      if (var->saved_item)
	{
	  item = var->saved_item;
	  var->saved_item = NULL;
	}
      else
	item = PyIter_Next (var->child_iter);

      if (!item)
	{
	  /* Normal end of iteration.  */
	  if (!PyErr_Occurred ())
	    break;

	  /* If we got a memory error, just use the text as the
	     item.  */
	  if (PyErr_ExceptionMatches (gdbpy_gdb_memory_error))
	    {
	      PyObject *type, *value, *trace;
	      char *name_str, *value_str;

	      PyErr_Fetch (&type, &value, &trace);
	      value_str = gdbpy_exception_to_string (type, value);
	      Py_XDECREF (type);
	      Py_XDECREF (value);
	      Py_XDECREF (trace);
	      if (!value_str)
		{
		  gdbpy_print_stack ();
		  break;
		}

	      name_str = xstrprintf ("<error at %d>", i);
	      item = Py_BuildValue ("(ss)", name_str, value_str);
	      xfree (name_str);
	      xfree (value_str);
	      if (!item)
		{
		  gdbpy_print_stack ();
		  break;
		}

	      force_done = 1;
	    }
	  else
	    {
	      /* Any other kind of error.  */
	      gdbpy_print_stack ();
	      break;
	    }
	}

      /* We don't want to push the extra child on any report list.  */
      if (to < 0 || i < to)
	{
	  PyObject *py_v;
	  const char *name;
	  struct value *v;
	  struct cleanup *inner;
	  int can_mention = from < 0 || i >= from;

	  inner = make_cleanup_py_decref (item);

	  if (!PyArg_ParseTuple (item, "sO", &name, &py_v))
	    {
	      gdbpy_print_stack ();
	      error (_("Invalid item from the child list"));
	    }

	  v = convert_value_from_python (py_v);
	  if (v == NULL)
	    gdbpy_print_stack ();
	  install_dynamic_child (var, can_mention ? changed : NULL,
				 can_mention ? type_changed : NULL,
				 can_mention ? new : NULL,
				 can_mention ? unchanged : NULL,
				 can_mention ? cchanged : NULL, i, name, v);
	  do_cleanups (inner);
	}
      else
	{
	  Py_XDECREF (var->saved_item);
	  var->saved_item = item;

	  /* We want to truncate the child list just before this
	     element.  */
	  break;
	}

      if (force_done)
	break;
    }

  if (i < VEC_length (varobj_p, var->children))
    {
      int j;

      *cchanged = 1;
      for (j = i; j < VEC_length (varobj_p, var->children); ++j)
	varobj_delete (VEC_index (varobj_p, var->children, j), NULL, 0);
      VEC_truncate (varobj_p, var->children, i);
    }

  /* If there are fewer children than requested, note that the list of
     children changed.  */
  if (to >= 0 && VEC_length (varobj_p, var->children) < to)
    *cchanged = 1;

  var->num_children = VEC_length (varobj_p, var->children);
 
  do_cleanups (back_to);

  return 1;
#else
  gdb_assert (0 && "should never be called if Python is not enabled");
#endif
}

int
varobj_get_num_children (struct varobj *var)
{
  if (var->num_children == -1)
    {
      if (var->pretty_printer)
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
  char *name;
  int i, children_changed;

  var->children_requested = 1;

  if (var->pretty_printer)
    {
      /* This, in theory, can result in the number of children changing without
	 frontend noticing.  But well, calling -var-list-children on the same
	 varobj twice is not something a sane frontend would do.  */
      update_dynamic_varobj_children (var, NULL, NULL, NULL, NULL,
				      &children_changed, 0, 0, *to);
      restrict_range (var->children, from, to);
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
	  name = name_of_child (var, i);
	  existing = create_child (var, i, name);
	  VEC_replace (varobj_p, var->children, i, existing);
	}
    }

  restrict_range (var->children, from, to);
  return var->children;
}

#if HAVE_PYTHON

static struct varobj *
varobj_add_child (struct varobj *var, const char *name, struct value *value)
{
  varobj_p v = create_child_with_value (var, 
					VEC_length (varobj_p, var->children), 
					name, value);

  VEC_safe_push (varobj_p, var->children, v);
  return v;
}

#endif /* HAVE_PYTHON */

/* Obtain the type of an object Variable as a string similar to the one gdb
   prints on the console.  */

char *
varobj_get_type (struct varobj *var)
{
  /* For the "fake" variables, do not return a type.  (It's type is
     NULL, too.)
     Do not return a type for invalid variables as well.  */
  if (CPLUS_FAKE_CHILD (var) || !var->root->is_valid)
    return NULL;

  return type_to_string (var->type);
}

/* Obtain the type of an object variable.  */

struct type *
varobj_get_gdb_type (struct varobj *var)
{
  return var->type;
}

/* Is VAR a path expression parent, i.e., can it be used to construct
   a valid path expression?  */

static int
is_path_expr_parent (struct varobj *var)
{
  struct type *type;

  /* "Fake" children are not path_expr parents.  */
  if (CPLUS_FAKE_CHILD (var))
    return 0;

  type = get_value_type (var);

  /* Anonymous unions and structs are also not path_expr parents.  */
  return !((TYPE_CODE (type) == TYPE_CODE_STRUCT
	    || TYPE_CODE (type) == TYPE_CODE_UNION)
	   && TYPE_NAME (type) == NULL);
}

/* Return the path expression parent for VAR.  */

static struct varobj *
get_path_expr_parent (struct varobj *var)
{
  struct varobj *parent = var;

  while (!is_root_p (parent) && !is_path_expr_parent (parent))
    parent = parent->parent;

  return parent;
}

/* Return a pointer to the full rooted expression of varobj VAR.
   If it has not been computed yet, compute it.  */
char *
varobj_get_path_expr (struct varobj *var)
{
  if (var->path_expr != NULL)
    return var->path_expr;
  else 
    {
      /* For root varobjs, we initialize path_expr
	 when creating varobj, so here it should be
	 child varobj.  */
      gdb_assert (!is_root_p (var));
      return (*var->root->lang->path_expr_of_child) (var);
    }
}

enum varobj_languages
varobj_get_language (struct varobj *var)
{
  return variable_language (var);
}

int
varobj_get_attributes (struct varobj *var)
{
  int attributes = 0;

  if (varobj_editable_p (var))
    /* FIXME: define masks for attributes.  */
    attributes |= 0x00000001;	/* Editable */

  return attributes;
}

int
varobj_pretty_printed_p (struct varobj *var)
{
  return var->pretty_printer != NULL;
}

char *
varobj_get_formatted_value (struct varobj *var,
			    enum varobj_display_formats format)
{
  return my_value_of_variable (var, format);
}

char *
varobj_get_value (struct varobj *var)
{
  return my_value_of_variable (var, var->format);
}

/* Set the value of an object variable (if it is editable) to the
   value of the given expression.  */
/* Note: Invokes functions that can call error().  */

int
varobj_set_value (struct varobj *var, char *expression)
{
  struct value *val = NULL; /* Initialize to keep gcc happy.  */
  /* The argument "expression" contains the variable's new value.
     We need to first construct a legal expression for this -- ugh!  */
  /* Does this cover all the bases?  */
  struct expression *exp;
  struct value *value = NULL; /* Initialize to keep gcc happy.  */
  int saved_input_radix = input_radix;
  const char *s = expression;
  volatile struct gdb_exception except;

  gdb_assert (varobj_editable_p (var));

  input_radix = 10;		/* ALWAYS reset to decimal temporarily.  */
  exp = parse_exp_1 (&s, 0, 0, 0);
  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      value = evaluate_expression (exp);
    }

  if (except.reason < 0)
    {
      /* We cannot proceed without a valid expression.  */
      xfree (exp);
      return 0;
    }

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
  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      val = value_assign (var->value, value);
    }

  if (except.reason < 0)
    return 0;

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
   in a varobj.  */

static void
install_visualizer (struct varobj *var, PyObject *constructor,
		    PyObject *visualizer)
{
  Py_XDECREF (var->constructor);
  var->constructor = constructor;

  Py_XDECREF (var->pretty_printer);
  var->pretty_printer = visualizer;

  Py_XDECREF (var->child_iter);
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
  
      install_visualizer (var, NULL, pretty_printer);
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

  install_visualizer (var, constructor, pretty_printer);
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
  if (var->constructor != Py_None && var->value)
    {
      struct cleanup *cleanup;

      cleanup = varobj_ensure_python_env (var);

      if (!var->constructor)
	install_default_visualizer (var);
      else
	construct_visualizer (var, var->constructor);

      do_cleanups (cleanup);
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
	  struct type *new_type;
	  char *curr_type_str, *new_type_str;

	  new_type = value_actual_type (new_value, 0, 0);
	  new_type_str = type_to_string (new_type);
	  curr_type_str = varobj_get_type (var);
	  if (strcmp (curr_type_str, new_type_str) != 0)
	    {
	      var->type = new_type;

	      /* This information may be not valid for a new type.  */
	      varobj_delete (var, NULL, 1);
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
  char *print_value = NULL;

  /* We need to know the varobj's type to decide if the value should
     be fetched or not.  C++ fake children (public/protected/private)
     don't have a type.  */
  gdb_assert (var->type || CPLUS_FAKE_CHILD (var));
  changeable = varobj_value_is_changeable_p (var);

  /* If the type has custom visualizer, we consider it to be always
     changeable.  FIXME: need to make sure this behaviour will not
     mess up read-sensitive values.  */
  if (var->pretty_printer)
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
      struct varobj *parent = var->parent;
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
	  volatile struct gdb_exception except;

	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      value_fetch_lazy (value);
	    }

	  if (except.reason < 0)
	    {
	      /* Set the value to NULL, so that for the next -var-update,
		 we don't try to compare the new value with this value,
		 that we couldn't even read.  */
	      value = NULL;
	    }
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
  if (value && !value_lazy (value) && !var->pretty_printer)
    print_value = value_get_print_value (value, var->format, var);

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
      else if (! var->pretty_printer)
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

	      gdb_assert (var->print_value != NULL && print_value != NULL);
	      if (strcmp (var->print_value, print_value) != 0)
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
  if (var->pretty_printer)
    {
      xfree (print_value);
      print_value = value_get_print_value (var->value, var->format, var);
      if ((var->print_value == NULL && print_value != NULL)
	  || (var->print_value != NULL && print_value == NULL)
	  || (var->print_value != NULL && print_value != NULL
	      && strcmp (var->print_value, print_value) != 0))
	changed = 1;
    }
  if (var->print_value)
    xfree (var->print_value);
  var->print_value = print_value;

  gdb_assert (!var->value || value_type (var->value));

  return changed;
}

/* Return the requested range for a varobj.  VAR is the varobj.  FROM
   and TO are out parameters; *FROM and *TO will be set to the
   selected sub-range of VAR.  If no range was selected using
   -var-set-update-range, then both will be -1.  */
void
varobj_get_child_range (struct varobj *var, int *from, int *to)
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
  PyObject *mainmod, *globals, *constructor;
  struct cleanup *back_to;

  back_to = varobj_ensure_python_env (var);

  mainmod = PyImport_AddModule ("__main__");
  globals = PyModule_GetDict (mainmod);
  Py_INCREF (globals);
  make_cleanup_py_decref (globals);

  constructor = PyRun_String (visualizer, Py_eval_input, globals, globals);

  if (! constructor)
    {
      gdbpy_print_stack ();
      error (_("Could not evaluate visualizer expression: %s"), visualizer);
    }

  construct_visualizer (var, constructor);
  Py_XDECREF (constructor);

  /* If there are any children now, wipe them.  */
  varobj_delete (var, NULL, 1 /* children only */);
  var->num_children = -1;

  do_cleanups (back_to);
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
varobj_value_has_mutated (struct varobj *var, struct value *new_value,
			  struct type *new_type)
{
  /* If we haven't previously computed the number of children in var,
     it does not matter from the front-end's perspective whether
     the type has mutated or not.  For all intents and purposes,
     it has not mutated.  */
  if (var->num_children < 0)
    return 0;

  if (var->root->lang->value_has_mutated)
    return var->root->lang->value_has_mutated (var, new_value, new_type);
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
varobj_update (struct varobj **varp, int explicit)
{
  int type_changed = 0;
  int i;
  struct value *new;
  VEC (varobj_update_result) *stack = NULL;
  VEC (varobj_update_result) *result = NULL;

  /* Frozen means frozen -- we don't check for any change in
     this varobj, including its going out of scope, or
     changing type.  One use case for frozen varobjs is
     retaining previously evaluated expressions, and we don't
     want them to be reevaluated at all.  */
  if (!explicit && (*varp)->frozen)
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
      new = value_of_root (varp, &type_changed);
      if (update_type_if_necessary(*varp, new))
	  type_changed = 1;
      r.varobj = *varp;
      r.type_changed = type_changed;
      if (install_new_value ((*varp), new, type_changed))
	r.changed = 1;
      
      if (new == NULL)
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

	  new = value_of_child (v->parent, v->index);
	  if (update_type_if_necessary(v, new))
	    r.type_changed = 1;
	  if (new)
	    new_type = value_type (new);
	  else
	    new_type = v->root->lang->type_of_child (v->parent, v->index);

	  if (varobj_value_has_mutated (v, new, new_type))
	    {
	      /* The children are no longer valid; delete them now.
	         Report the fact that its type changed as well.  */
	      varobj_delete (v, NULL, 1 /* only_children */);
	      v->num_children = -1;
	      v->to = -1;
	      v->from = -1;
	      v->type = new_type;
	      r.type_changed = 1;
	    }

	  if (install_new_value (v, new, r.type_changed))
	    {
	      r.changed = 1;
	      v->updated = 0;
	    }
	}

      /* We probably should not get children of a varobj that has a
	 pretty-printer, but for which -var-list-children was never
	 invoked.  */
      if (v->pretty_printer)
	{
	  VEC (varobj_p) *changed = 0, *type_changed = 0, *unchanged = 0;
	  VEC (varobj_p) *new = 0;
	  int i, children_changed = 0;

	  if (v->frozen)
	    continue;

	  if (!v->children_requested)
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
	  if (update_dynamic_varobj_children (v, &changed, &type_changed, &new,
					      &unchanged, &children_changed, 1,
					      v->from, v->to))
	    {
	      if (children_changed || new)
		{
		  r.children_changed = 1;
		  r.new = new;
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
delete_variable (struct cpstack **resultp, struct varobj *var,
		 int only_children_p)
{
  int delcount = 0;

  delete_variable_1 (resultp, &delcount, var,
		     only_children_p, 1 /* remove_from_parent_p */ );

  return delcount;
}

/* Delete the variable object VAR and its children.  */
/* IMPORTANT NOTE: If we delete a variable which is a child
   and the parent is not removed we dump core.  It must be always
   initially called with remove_from_parent_p set.  */
static void
delete_variable_1 (struct cpstack **resultp, int *delcountp,
		   struct varobj *var, int only_children_p,
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
      delete_variable_1 (resultp, delcountp, child, 0, only_children_p);
    }
  VEC_free (varobj_p, var->children);

  /* if we were called to delete only the children we are done here.  */
  if (only_children_p)
    return;

  /* Otherwise, add it to the list of deleted ones and proceed to do so.  */
  /* If the name is null, this is a temporary variable, that has not
     yet been installed, don't report it, it belongs to the caller...  */
  if (var->obj_name != NULL)
    {
      cppush (resultp, xstrdup (var->obj_name));
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

  if (var->obj_name != NULL)
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

  for (chp = var->obj_name; *chp; chp++)
    {
      index = (index + (i++ * (unsigned int) *chp)) % VAROBJ_TABLE_SIZE;
    }

  cv = *(varobj_table + index);
  while ((cv != NULL) && (strcmp (cv->var->obj_name, var->obj_name) != 0))
    cv = cv->next;

  if (cv != NULL)
    error (_("Duplicate variable object name"));

  /* Add varobj to hash table.  */
  newvl = xmalloc (sizeof (struct vlist));
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
  for (chp = var->obj_name; *chp; chp++)
    {
      index = (index + (i++ * (unsigned int) *chp)) % VAROBJ_TABLE_SIZE;
    }

  cv = *(varobj_table + index);
  prev = NULL;
  while ((cv != NULL) && (strcmp (cv->var->obj_name, var->obj_name) != 0))
    {
      prev = cv;
      cv = cv->next;
    }

  if (varobjdebug)
    fprintf_unfiltered (gdb_stdlog, "Deleting %s\n", var->obj_name);

  if (cv == NULL)
    {
      warning
	("Assertion failed: Could not find variable object \"%s\" to delete",
	 var->obj_name);
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
		       var->obj_name);
	      return;
	    }
	  if (prer == NULL)
	    rootlist = NULL;
	  else
	    prer->next = cr->next;
	}
    }

}

/* Create and install a child of the parent of the given name.  */
static struct varobj *
create_child (struct varobj *parent, int index, char *name)
{
  return create_child_with_value (parent, index, name, 
				  value_of_child (parent, index));
}

/* Does CHILD represent a child with no name?  This happens when
   the child is an anonmous struct or union and it has no field name
   in its parent variable.

   This has already been determined by *_describe_child. The easiest
   thing to do is to compare the child's name with ANONYMOUS_*_NAME.  */

static int
is_anonymous_child (struct varobj *child)
{
  return (strcmp (child->name, ANONYMOUS_STRUCT_NAME) == 0
	  || strcmp (child->name, ANONYMOUS_UNION_NAME) == 0);
}

static struct varobj *
create_child_with_value (struct varobj *parent, int index, const char *name,
			 struct value *value)
{
  struct varobj *child;
  char *childs_name;

  child = new_variable ();

  /* Name is allocated by name_of_child.  */
  /* FIXME: xstrdup should not be here.  */
  child->name = xstrdup (name);
  child->index = index;
  child->parent = parent;
  child->root = parent->root;

  if (is_anonymous_child (child))
    childs_name = xstrprintf ("%s.%d_anonymous", parent->obj_name, index);
  else
    childs_name = xstrprintf ("%s.%s", parent->obj_name, name);
  child->obj_name = childs_name;

  install_variable (child);

  /* Compute the type of the child.  Must do this before
     calling install_new_value.  */
  if (value != NULL)
    /* If the child had no evaluation errors, var->value
       will be non-NULL and contain a valid type.  */
    child->type = value_actual_type (value, 0, NULL);
  else
    /* Otherwise, we must compute the type.  */
    child->type = (*child->root->lang->type_of_child) (child->parent, 
						       child->index);
  install_new_value (child, value, 1);

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

  var = (struct varobj *) xmalloc (sizeof (struct varobj));
  var->name = NULL;
  var->path_expr = NULL;
  var->obj_name = NULL;
  var->index = -1;
  var->type = NULL;
  var->value = NULL;
  var->num_children = -1;
  var->parent = NULL;
  var->children = NULL;
  var->format = 0;
  var->root = NULL;
  var->updated = 0;
  var->print_value = NULL;
  var->frozen = 0;
  var->not_fetched = 0;
  var->children_requested = 0;
  var->from = -1;
  var->to = -1;
  var->constructor = 0;
  var->pretty_printer = 0;
  var->child_iter = 0;
  var->saved_item = 0;

  return var;
}

/* Allocate memory and initialize a new root variable.  */
static struct varobj *
new_root_variable (void)
{
  struct varobj *var = new_variable ();

  var->root = (struct varobj_root *) xmalloc (sizeof (struct varobj_root));
  var->root->lang = NULL;
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
  if (var->pretty_printer)
    {
      struct cleanup *cleanup = varobj_ensure_python_env (var);
      Py_XDECREF (var->constructor);
      Py_XDECREF (var->pretty_printer);
      Py_XDECREF (var->child_iter);
      Py_XDECREF (var->saved_item);
      do_cleanups (cleanup);
    }
#endif

  value_free (var->value);

  /* Free the expression if this is a root variable.  */
  if (is_root_p (var))
    {
      xfree (var->root->exp);
      xfree (var->root);
    }

  xfree (var->name);
  xfree (var->obj_name);
  xfree (var->print_value);
  xfree (var->path_expr);
  xfree (var);
}

static void
do_free_variable_cleanup (void *var)
{
  free_variable (var);
}

static struct cleanup *
make_cleanup_free_variable (struct varobj *var)
{
  return make_cleanup (do_free_variable_cleanup, var);
}

/* This returns the type of the variable.  It also skips past typedefs
   to return the real type of the variable.

   NOTE: TYPE_TARGET_TYPE should NOT be used anywhere in this file
   except within get_target_type and get_type.  */
static struct type *
get_type (struct varobj *var)
{
  struct type *type;

  type = var->type;
  if (type != NULL)
    type = check_typedef (type);

  return type;
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
static struct type *
get_value_type (struct varobj *var)
{
  struct type *type;

  if (var->value)
    type = value_type (var->value);
  else
    type = var->type;

  type = check_typedef (type);

  if (TYPE_CODE (type) == TYPE_CODE_REF)
    type = get_target_type (type);

  type = check_typedef (type);

  return type;
}

/* This returns the target type (or NULL) of TYPE, also skipping
   past typedefs, just like get_type ().

   NOTE: TYPE_TARGET_TYPE should NOT be used anywhere in this file
   except within get_target_type and get_type.  */
static struct type *
get_target_type (struct type *type)
{
  if (type != NULL)
    {
      type = TYPE_TARGET_TYPE (type);
      if (type != NULL)
	type = check_typedef (type);
    }

  return type;
}

/* What is the default display for this variable? We assume that
   everything is "natural".  Any exceptions?  */
static enum varobj_display_formats
variable_default_display (struct varobj *var)
{
  return FORMAT_NATURAL;
}

/* FIXME: The following should be generic for any pointer.  */
static void
cppush (struct cpstack **pstack, char *name)
{
  struct cpstack *s;

  s = (struct cpstack *) xmalloc (sizeof (struct cpstack));
  s->name = name;
  s->next = *pstack;
  *pstack = s;
}

/* FIXME: The following should be generic for any pointer.  */
static char *
cppop (struct cpstack **pstack)
{
  struct cpstack *s;
  char *v;

  if ((*pstack)->name == NULL && (*pstack)->next == NULL)
    return NULL;

  s = *pstack;
  v = s->name;
  *pstack = (*pstack)->next;
  xfree (s);

  return v;
}

/*
 * Language-dependencies
 */

/* Common entry points */

/* Get the language of variable VAR.  */
static enum varobj_languages
variable_language (struct varobj *var)
{
  enum varobj_languages lang;

  switch (var->root->exp->language_defn->la_language)
    {
    default:
    case language_c:
      lang = vlang_c;
      break;
    case language_cplus:
      lang = vlang_cplus;
      break;
    case language_java:
      lang = vlang_java;
      break;
    case language_ada:
      lang = vlang_ada;
      break;
    }

  return lang;
}

/* Return the number of children for a given variable.
   The result of this function is defined by the language
   implementation.  The number of children returned by this function
   is the number of children that the user will see in the variable
   display.  */
static int
number_of_children (struct varobj *var)
{
  return (*var->root->lang->number_of_children) (var);
}

/* What is the expression for the root varobj VAR? Returns a malloc'd
   string.  */
static char *
name_of_variable (struct varobj *var)
{
  return (*var->root->lang->name_of_variable) (var);
}

/* What is the name of the INDEX'th child of VAR? Returns a malloc'd
   string.  */
static char *
name_of_child (struct varobj *var, int index)
{
  return (*var->root->lang->name_of_child) (var, index);
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
      char *old_type, *new_type;

      tmp_var = varobj_create (NULL, var->name, (CORE_ADDR) 0,
			       USE_SELECTED_FRAME);
      if (tmp_var == NULL)
	{
	  return NULL;
	}
      old_type = varobj_get_type (var);
      new_type = varobj_get_type (tmp_var);
      if (strcmp (old_type, new_type) == 0)
	{
	  /* The expression presently stored inside var->root->exp
	     remembers the locations of local variables relatively to
	     the frame where the expression was created (in DWARF location
	     button, for example).  Naturally, those locations are not
	     correct in other frames, so update the expression.  */

         struct expression *tmp_exp = var->root->exp;

         var->root->exp = tmp_var->root->exp;
         tmp_var->root->exp = tmp_exp;

	  varobj_delete (tmp_var, NULL, 0);
	  *type_changed = 0;
	}
      else
	{
	  tmp_var->obj_name = xstrdup (var->obj_name);
	  tmp_var->from = var->from;
	  tmp_var->to = var->to;
	  varobj_delete (var, NULL, 0);

	  install_variable (tmp_var);
	  *var_handle = tmp_var;
	  var = *var_handle;
	  *type_changed = 1;
	}
      xfree (old_type);
      xfree (new_type);
    }
  else
    {
      *type_changed = 0;
    }

  {
    struct value *value;

    value = (*var->root->lang->value_of_root) (var_handle);
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
	varobj_delete (var, NULL, 1 /* only_children */);
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
value_of_child (struct varobj *parent, int index)
{
  struct value *value;

  value = (*parent->root->lang->value_of_child) (parent, index);

  return value;
}

/* GDB already has a command called "value_of_variable".  Sigh.  */
static char *
my_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  if (var->root->is_valid)
    {
      if (var->pretty_printer)
	return value_get_print_value (var->value, var->format, var);
      return (*var->root->lang->value_of_variable) (var, format);
    }
  else
    return NULL;
}

static char *
value_get_print_value (struct value *value, enum varobj_display_formats format,
		       struct varobj *var)
{
  struct ui_file *stb;
  struct cleanup *old_chain;
  char *thevalue = NULL;
  struct value_print_options opts;
  struct type *type = NULL;
  long len = 0;
  char *encoding = NULL;
  struct gdbarch *gdbarch = NULL;
  /* Initialize it just to avoid a GCC false warning.  */
  CORE_ADDR str_addr = 0;
  int string_print = 0;

  if (value == NULL)
    return NULL;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  gdbarch = get_type_arch (value_type (value));
#if HAVE_PYTHON
  {
    PyObject *value_formatter = var->pretty_printer;

    varobj_ensure_python_env (var);

    if (value_formatter)
      {
	/* First check to see if we have any children at all.  If so,
	   we simply return {...}.  */
	if (dynamic_varobj_has_child_method (var))
	  {
	    do_cleanups (old_chain);
	    return xstrdup ("{...}");
	  }

	if (PyObject_HasAttr (value_formatter, gdbpy_to_string_cst))
	  {
	    struct value *replacement;
	    PyObject *output = NULL;

	    output = apply_varobj_pretty_printer (value_formatter,
						  &replacement,
						  stb);

	    /* If we have string like output ...  */
	    if (output)
	      {
		make_cleanup_py_decref (output);

		/* If this is a lazy string, extract it.  For lazy
		   strings we always print as a string, so set
		   string_print.  */
		if (gdbpy_is_lazy_string (output))
		  {
		    gdbpy_extract_lazy_string (output, &str_addr, &type,
					       &len, &encoding);
		    make_cleanup (free_current_contents, &encoding);
		    string_print = 1;
		  }
		else
		  {
		    /* If it is a regular (non-lazy) string, extract
		       it and copy the contents into THEVALUE.  If the
		       hint says to print it as a string, set
		       string_print.  Otherwise just return the extracted
		       string as a value.  */

		    char *s = python_string_to_target_string (output);

		    if (s)
		      {
			char *hint;

			hint = gdbpy_get_display_hint (value_formatter);
			if (hint)
			  {
			    if (!strcmp (hint, "string"))
			      string_print = 1;
			    xfree (hint);
			  }

			len = strlen (s);
			thevalue = xmemdup (s, len + 1, len + 1);
			type = builtin_type (gdbarch)->builtin_char;
			xfree (s);

			if (!string_print)
			  {
			    do_cleanups (old_chain);
			    return thevalue;
			  }

			make_cleanup (xfree, thevalue);
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

  get_formatted_print_options (&opts, format_code[(int) format]);
  opts.deref_ref = 0;
  opts.raw = 1;

  /* If the THEVALUE has contents, it is a regular string.  */
  if (thevalue)
    LA_PRINT_STRING (stb, type, (gdb_byte *) thevalue, len, encoding, 0, &opts);
  else if (string_print)
    /* Otherwise, if string_print is set, and it is not a regular
       string, it is a lazy string.  */
    val_print_string (type, encoding, str_addr, len, stb, &opts);
  else
    /* All other cases.  */
    common_val_print (value, stb, 0, &opts, current_language);

  thevalue = ui_file_xstrdup (stb, NULL);

  do_cleanups (old_chain);
  return thevalue;
}

int
varobj_editable_p (struct varobj *var)
{
  struct type *type;

  if (!(var->root->is_valid && var->value && VALUE_LVAL (var->value)))
    return 0;

  type = get_value_type (var);

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

static int
varobj_value_is_changeable_p (struct varobj *var)
{
  return var->root->lang->value_is_changeable_p (var);
}

/* Return 1 if that varobj is floating, that is is always evaluated in the
   selected frame, and not bound to thread/frame.  Such variable objects
   are created using '@' as frame specifier to -var-create.  */
int
varobj_floating_p (struct varobj *var)
{
  return var->root->floating;
}

/* Given the value and the type of a variable object,
   adjust the value and type to those necessary
   for getting children of the variable object.
   This includes dereferencing top-level references
   to all types and dereferencing pointers to
   structures.

   If LOOKUP_ACTUAL_TYPE is set the enclosing type of the
   value will be fetched and if it differs from static type
   the value will be casted to it.

   Both TYPE and *TYPE should be non-null.  VALUE
   can be null if we want to only translate type.
   *VALUE can be null as well -- if the parent
   value is not known.

   If WAS_PTR is not NULL, set *WAS_PTR to 0 or 1
   depending on whether pointer was dereferenced
   in this function.  */
static void
adjust_value_for_child_access (struct value **value,
				  struct type **type,
				  int *was_ptr,
				  int lookup_actual_type)
{
  gdb_assert (type && *type);

  if (was_ptr)
    *was_ptr = 0;

  *type = check_typedef (*type);
  
  /* The type of value stored in varobj, that is passed
     to us, is already supposed to be
     reference-stripped.  */

  gdb_assert (TYPE_CODE (*type) != TYPE_CODE_REF);

  /* Pointers to structures are treated just like
     structures when accessing children.  Don't
     dererences pointers to other types.  */
  if (TYPE_CODE (*type) == TYPE_CODE_PTR)
    {
      struct type *target_type = get_target_type (*type);
      if (TYPE_CODE (target_type) == TYPE_CODE_STRUCT
	  || TYPE_CODE (target_type) == TYPE_CODE_UNION)
	{
	  if (value && *value)
	    {
	      volatile struct gdb_exception except;

	      TRY_CATCH (except, RETURN_MASK_ERROR)
		{
		  *value = value_ind (*value);
		}

	      if (except.reason < 0)
		*value = NULL;
	    }
	  *type = target_type;
	  if (was_ptr)
	    *was_ptr = 1;
	}
    }

  /* The 'get_target_type' function calls check_typedef on
     result, so we can immediately check type code.  No
     need to call check_typedef here.  */

  /* Access a real type of the value (if necessary and possible).  */
  if (value && *value && lookup_actual_type)
    {
      struct type *enclosing_type;
      int real_type_found = 0;

      enclosing_type = value_actual_type (*value, 1, &real_type_found);
      if (real_type_found)
        {
          *type = enclosing_type;
          *value = value_cast (enclosing_type, *value);
        }
    }
}

/* Implement the "value_is_changeable_p" varobj callback for most
   languages.  */

static int
default_value_is_changeable_p (struct varobj *var)
{
  int r;
  struct type *type;

  if (CPLUS_FAKE_CHILD (var))
    return 0;

  type = get_value_type (var);

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

/* C */

static int
c_number_of_children (struct varobj *var)
{
  struct type *type = get_value_type (var);
  int children = 0;
  struct type *target;

  adjust_value_for_child_access (NULL, &type, NULL, 0);
  target = get_target_type (type);

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (TYPE_LENGTH (type) > 0 && TYPE_LENGTH (target) > 0
	  && !TYPE_ARRAY_UPPER_BOUND_IS_UNDEFINED (type))
	children = TYPE_LENGTH (type) / TYPE_LENGTH (target);
      else
	/* If we don't know how many elements there are, don't display
	   any.  */
	children = 0;
      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      children = TYPE_NFIELDS (type);
      break;

    case TYPE_CODE_PTR:
      /* The type here is a pointer to non-struct.  Typically, pointers
	 have one child, except for function ptrs, which have no children,
	 and except for void*, as we don't know what to show.

         We can show char* so we allow it to be dereferenced.  If you decide
         to test for it, please mind that a little magic is necessary to
         properly identify it: char* has TYPE_CODE == TYPE_CODE_INT and 
         TYPE_NAME == "char".  */
      if (TYPE_CODE (target) == TYPE_CODE_FUNC
	  || TYPE_CODE (target) == TYPE_CODE_VOID)
	children = 0;
      else
	children = 1;
      break;

    default:
      /* Other types have no children.  */
      break;
    }

  return children;
}

static char *
c_name_of_variable (struct varobj *parent)
{
  return xstrdup (parent->name);
}

/* Return the value of element TYPE_INDEX of a structure
   value VALUE.  VALUE's type should be a structure,
   or union, or a typedef to struct/union.

   Returns NULL if getting the value fails.  Never throws.  */
static struct value *
value_struct_element_index (struct value *value, int type_index)
{
  struct value *result = NULL;
  volatile struct gdb_exception e;
  struct type *type = value_type (value);

  type = check_typedef (type);

  gdb_assert (TYPE_CODE (type) == TYPE_CODE_STRUCT
	      || TYPE_CODE (type) == TYPE_CODE_UNION);

  TRY_CATCH (e, RETURN_MASK_ERROR)
    {
      if (field_is_static (&TYPE_FIELD (type, type_index)))
	result = value_static_field (type, type_index);
      else
	result = value_primitive_field (value, 0, type_index, type);
    }
  if (e.reason < 0)
    {
      return NULL;
    }
  else
    {
      return result;
    }
}

/* Obtain the information about child INDEX of the variable
   object PARENT.
   If CNAME is not null, sets *CNAME to the name of the child relative
   to the parent.
   If CVALUE is not null, sets *CVALUE to the value of the child.
   If CTYPE is not null, sets *CTYPE to the type of the child.

   If any of CNAME, CVALUE, or CTYPE is not null, but the corresponding
   information cannot be determined, set *CNAME, *CVALUE, or *CTYPE
   to NULL.  */
static void 
c_describe_child (struct varobj *parent, int index,
		  char **cname, struct value **cvalue, struct type **ctype,
		  char **cfull_expression)
{
  struct value *value = parent->value;
  struct type *type = get_value_type (parent);
  char *parent_expression = NULL;
  int was_ptr;
  volatile struct gdb_exception except;

  if (cname)
    *cname = NULL;
  if (cvalue)
    *cvalue = NULL;
  if (ctype)
    *ctype = NULL;
  if (cfull_expression)
    {
      *cfull_expression = NULL;
      parent_expression = varobj_get_path_expr (get_path_expr_parent (parent));
    }
  adjust_value_for_child_access (&value, &type, &was_ptr, 0);
      
  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_ARRAY:
      if (cname)
	*cname
	  = xstrdup (int_string (index 
				 + TYPE_LOW_BOUND (TYPE_INDEX_TYPE (type)),
				 10, 1, 0, 0));

      if (cvalue && value)
	{
	  int real_index = index + TYPE_LOW_BOUND (TYPE_INDEX_TYPE (type));

	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      *cvalue = value_subscript (value, real_index);
	    }
	}

      if (ctype)
	*ctype = get_target_type (type);

      if (cfull_expression)
	*cfull_expression = 
	  xstrprintf ("(%s)[%s]", parent_expression, 
		      int_string (index
				  + TYPE_LOW_BOUND (TYPE_INDEX_TYPE (type)),
				  10, 1, 0, 0));


      break;

    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      {
	const char *field_name;

	/* If the type is anonymous and the field has no name,
	   set an appropriate name.  */
	field_name = TYPE_FIELD_NAME (type, index);
	if (field_name == NULL || *field_name == '\0')
	  {
	    if (cname)
	      {
		if (TYPE_CODE (TYPE_FIELD_TYPE (type, index))
		    == TYPE_CODE_STRUCT)
		  *cname = xstrdup (ANONYMOUS_STRUCT_NAME);
		else
		  *cname = xstrdup (ANONYMOUS_UNION_NAME);
	      }

	    if (cfull_expression)
	      *cfull_expression = xstrdup ("");
	  }
	else
	  {
	    if (cname)
	      *cname = xstrdup (field_name);

	    if (cfull_expression)
	      {
		char *join = was_ptr ? "->" : ".";

		*cfull_expression = xstrprintf ("(%s)%s%s", parent_expression,
						join, field_name);
	      }
	  }

	if (cvalue && value)
	  {
	    /* For C, varobj index is the same as type index.  */
	    *cvalue = value_struct_element_index (value, index);
	  }

	if (ctype)
	  *ctype = TYPE_FIELD_TYPE (type, index);
      }
      break;

    case TYPE_CODE_PTR:
      if (cname)
	*cname = xstrprintf ("*%s", parent->name);

      if (cvalue && value)
	{
	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      *cvalue = value_ind (value);
	    }

	  if (except.reason < 0)
	    *cvalue = NULL;
	}

      /* Don't use get_target_type because it calls
	 check_typedef and here, we want to show the true
	 declared type of the variable.  */
      if (ctype)
	*ctype = TYPE_TARGET_TYPE (type);

      if (cfull_expression)
	*cfull_expression = xstrprintf ("*(%s)", parent_expression);
      
      break;

    default:
      /* This should not happen.  */
      if (cname)
	*cname = xstrdup ("???");
      if (cfull_expression)
	*cfull_expression = xstrdup ("???");
      /* Don't set value and type, we don't know then.  */
    }
}

static char *
c_name_of_child (struct varobj *parent, int index)
{
  char *name;

  c_describe_child (parent, index, &name, NULL, NULL, NULL);
  return name;
}

static char *
c_path_expr_of_child (struct varobj *child)
{
  c_describe_child (child->parent, child->index, NULL, NULL, NULL, 
		    &child->path_expr);
  return child->path_expr;
}

/* If frame associated with VAR can be found, switch
   to it and return 1.  Otherwise, return 0.  */
static int
check_scope (struct varobj *var)
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

static struct value *
c_value_of_root (struct varobj **var_handle)
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
      ptid_t ptid = thread_id_to_pid (var->root->thread_id);
      if (in_thread_list (ptid))
	{
	  switch_to_thread (ptid);
	  within_scope = check_scope (var);
	}
    }

  if (within_scope)
    {
      volatile struct gdb_exception except;

      /* We need to catch errors here, because if evaluate
         expression fails we want to just return NULL.  */
      TRY_CATCH (except, RETURN_MASK_ERROR)
	{
	  new_val = evaluate_expression (var->root->exp);
	}

      return new_val;
    }

  do_cleanups (back_to);

  return NULL;
}

static struct value *
c_value_of_child (struct varobj *parent, int index)
{
  struct value *value = NULL;

  c_describe_child (parent, index, NULL, &value, NULL, NULL);
  return value;
}

static struct type *
c_type_of_child (struct varobj *parent, int index)
{
  struct type *type = NULL;

  c_describe_child (parent, index, NULL, NULL, &type, NULL);
  return type;
}

static char *
c_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  /* BOGUS: if val_print sees a struct/class, or a reference to one,
     it will print out its children instead of "{...}".  So we need to
     catch that case explicitly.  */
  struct type *type = get_type (var);

  /* Strip top-level references.  */
  while (TYPE_CODE (type) == TYPE_CODE_REF)
    type = check_typedef (TYPE_TARGET_TYPE (type));

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
      return xstrdup ("{...}");
      /* break; */

    case TYPE_CODE_ARRAY:
      {
	char *number;

	number = xstrprintf ("[%d]", var->num_children);
	return (number);
      }
      /* break; */

    default:
      {
	if (var->value == NULL)
	  {
	    /* This can happen if we attempt to get the value of a struct
	       member when the parent is an invalid pointer.  This is an
	       error condition, so we should tell the caller.  */
	    return NULL;
	  }
	else
	  {
	    if (var->not_fetched && value_lazy (var->value))
	      /* Frozen variable and no value yet.  We don't
		 implicitly fetch the value.  MI response will
		 use empty string for the value, which is OK.  */
	      return NULL;

	    gdb_assert (varobj_value_is_changeable_p (var));
	    gdb_assert (!value_lazy (var->value));
	    
	    /* If the specified format is the current one,
	       we can reuse print_value.  */
	    if (format == var->format)
	      return xstrdup (var->print_value);
	    else
	      return value_get_print_value (var->value, format, var);
	  }
      }
    }
}


/* C++ */

static int
cplus_number_of_children (struct varobj *var)
{
  struct value *value = NULL;
  struct type *type;
  int children, dont_know;
  int lookup_actual_type = 0;
  struct value_print_options opts;

  dont_know = 1;
  children = 0;

  get_user_print_options (&opts);

  if (!CPLUS_FAKE_CHILD (var))
    {
      type = get_value_type (var);

      /* It is necessary to access a real type (via RTTI).  */
      if (opts.objectprint)
        {
          value = var->value;
          lookup_actual_type = (TYPE_CODE (var->type) == TYPE_CODE_REF
				|| TYPE_CODE (var->type) == TYPE_CODE_PTR);
        }
      adjust_value_for_child_access (&value, &type, NULL, lookup_actual_type);

      if (((TYPE_CODE (type)) == TYPE_CODE_STRUCT) ||
	  ((TYPE_CODE (type)) == TYPE_CODE_UNION))
	{
	  int kids[3];

	  cplus_class_num_children (type, kids);
	  if (kids[v_public] != 0)
	    children++;
	  if (kids[v_private] != 0)
	    children++;
	  if (kids[v_protected] != 0)
	    children++;

	  /* Add any baseclasses.  */
	  children += TYPE_N_BASECLASSES (type);
	  dont_know = 0;

	  /* FIXME: save children in var.  */
	}
    }
  else
    {
      int kids[3];

      type = get_value_type (var->parent);

      /* It is necessary to access a real type (via RTTI).  */
      if (opts.objectprint)
        {
	  struct varobj *parent = var->parent;

	  value = parent->value;
	  lookup_actual_type = (TYPE_CODE (parent->type) == TYPE_CODE_REF
				|| TYPE_CODE (parent->type) == TYPE_CODE_PTR);
        }
      adjust_value_for_child_access (&value, &type, NULL, lookup_actual_type);

      cplus_class_num_children (type, kids);
      if (strcmp (var->name, "public") == 0)
	children = kids[v_public];
      else if (strcmp (var->name, "private") == 0)
	children = kids[v_private];
      else
	children = kids[v_protected];
      dont_know = 0;
    }

  if (dont_know)
    children = c_number_of_children (var);

  return children;
}

/* Compute # of public, private, and protected variables in this class.
   That means we need to descend into all baseclasses and find out
   how many are there, too.  */
static void
cplus_class_num_children (struct type *type, int children[3])
{
  int i, vptr_fieldno;
  struct type *basetype = NULL;

  children[v_public] = 0;
  children[v_private] = 0;
  children[v_protected] = 0;

  vptr_fieldno = get_vptr_fieldno (type, &basetype);
  for (i = TYPE_N_BASECLASSES (type); i < TYPE_NFIELDS (type); i++)
    {
      /* If we have a virtual table pointer, omit it.  Even if virtual
	 table pointers are not specifically marked in the debug info,
	 they should be artificial.  */
      if ((type == basetype && i == vptr_fieldno)
	  || TYPE_FIELD_ARTIFICIAL (type, i))
	continue;

      if (TYPE_FIELD_PROTECTED (type, i))
	children[v_protected]++;
      else if (TYPE_FIELD_PRIVATE (type, i))
	children[v_private]++;
      else
	children[v_public]++;
    }
}

static char *
cplus_name_of_variable (struct varobj *parent)
{
  return c_name_of_variable (parent);
}

enum accessibility { private_field, protected_field, public_field };

/* Check if field INDEX of TYPE has the specified accessibility.
   Return 0 if so and 1 otherwise.  */
static int 
match_accessibility (struct type *type, int index, enum accessibility acc)
{
  if (acc == private_field && TYPE_FIELD_PRIVATE (type, index))
    return 1;
  else if (acc == protected_field && TYPE_FIELD_PROTECTED (type, index))
    return 1;
  else if (acc == public_field && !TYPE_FIELD_PRIVATE (type, index)
	   && !TYPE_FIELD_PROTECTED (type, index))
    return 1;
  else
    return 0;
}

static void
cplus_describe_child (struct varobj *parent, int index,
		      char **cname, struct value **cvalue, struct type **ctype,
		      char **cfull_expression)
{
  struct value *value;
  struct type *type;
  int was_ptr;
  int lookup_actual_type = 0;
  char *parent_expression = NULL;
  struct varobj *var;
  struct value_print_options opts;

  if (cname)
    *cname = NULL;
  if (cvalue)
    *cvalue = NULL;
  if (ctype)
    *ctype = NULL;
  if (cfull_expression)
    *cfull_expression = NULL;

  get_user_print_options (&opts);

  var = (CPLUS_FAKE_CHILD (parent)) ? parent->parent : parent;
  if (opts.objectprint)
    lookup_actual_type = (TYPE_CODE (var->type) == TYPE_CODE_REF
			  || TYPE_CODE (var->type) == TYPE_CODE_PTR);
  value = var->value;
  type = get_value_type (var);
  if (cfull_expression)
    parent_expression = varobj_get_path_expr (get_path_expr_parent (var));

  adjust_value_for_child_access (&value, &type, &was_ptr, lookup_actual_type);

  if (TYPE_CODE (type) == TYPE_CODE_STRUCT
      || TYPE_CODE (type) == TYPE_CODE_UNION)
    {
      char *join = was_ptr ? "->" : ".";

      if (CPLUS_FAKE_CHILD (parent))
	{
	  /* The fields of the class type are ordered as they
	     appear in the class.  We are given an index for a
	     particular access control type ("public","protected",
	     or "private").  We must skip over fields that don't
	     have the access control we are looking for to properly
	     find the indexed field.  */
	  int type_index = TYPE_N_BASECLASSES (type);
	  enum accessibility acc = public_field;
	  int vptr_fieldno;
	  struct type *basetype = NULL;
	  const char *field_name;

	  vptr_fieldno = get_vptr_fieldno (type, &basetype);
	  if (strcmp (parent->name, "private") == 0)
	    acc = private_field;
	  else if (strcmp (parent->name, "protected") == 0)
	    acc = protected_field;

	  while (index >= 0)
	    {
	      if ((type == basetype && type_index == vptr_fieldno)
		  || TYPE_FIELD_ARTIFICIAL (type, type_index))
		; /* ignore vptr */
	      else if (match_accessibility (type, type_index, acc))
		    --index;
		  ++type_index;
	    }
	  --type_index;

	  /* If the type is anonymous and the field has no name,
	     set an appopriate name.  */
	  field_name = TYPE_FIELD_NAME (type, type_index);
	  if (field_name == NULL || *field_name == '\0')
	    {
	      if (cname)
		{
		  if (TYPE_CODE (TYPE_FIELD_TYPE (type, type_index))
		      == TYPE_CODE_STRUCT)
		    *cname = xstrdup (ANONYMOUS_STRUCT_NAME);
		  else if (TYPE_CODE (TYPE_FIELD_TYPE (type, type_index))
			   == TYPE_CODE_UNION)
		    *cname = xstrdup (ANONYMOUS_UNION_NAME);
		}

	      if (cfull_expression)
		*cfull_expression = xstrdup ("");
	    }
	  else
	    {
	      if (cname)
		*cname = xstrdup (TYPE_FIELD_NAME (type, type_index));

	      if (cfull_expression)
		*cfull_expression
		  = xstrprintf ("((%s)%s%s)", parent_expression, join,
				field_name);
	    }

	  if (cvalue && value)
	    *cvalue = value_struct_element_index (value, type_index);

	  if (ctype)
	    *ctype = TYPE_FIELD_TYPE (type, type_index);
	}
      else if (index < TYPE_N_BASECLASSES (type))
	{
	  /* This is a baseclass.  */
	  if (cname)
	    *cname = xstrdup (TYPE_FIELD_NAME (type, index));

	  if (cvalue && value)
	    *cvalue = value_cast (TYPE_FIELD_TYPE (type, index), value);

	  if (ctype)
	    {
	      *ctype = TYPE_FIELD_TYPE (type, index);
	    }

	  if (cfull_expression)
	    {
	      char *ptr = was_ptr ? "*" : "";

	      /* Cast the parent to the base' type.  Note that in gdb,
		 expression like 
		         (Base1)d
		 will create an lvalue, for all appearences, so we don't
		 need to use more fancy:
		         *(Base1*)(&d)
		 construct.

		 When we are in the scope of the base class or of one
		 of its children, the type field name will be interpreted
		 as a constructor, if it exists.  Therefore, we must
		 indicate that the name is a class name by using the
		 'class' keyword.  See PR mi/11912  */
	      *cfull_expression = xstrprintf ("(%s(class %s%s) %s)", 
					      ptr, 
					      TYPE_FIELD_NAME (type, index),
					      ptr,
					      parent_expression);
	    }
	}
      else
	{
	  char *access = NULL;
	  int children[3];

	  cplus_class_num_children (type, children);

	  /* Everything beyond the baseclasses can
	     only be "public", "private", or "protected"

	     The special "fake" children are always output by varobj in
	     this order.  So if INDEX == 2, it MUST be "protected".  */
	  index -= TYPE_N_BASECLASSES (type);
	  switch (index)
	    {
	    case 0:
	      if (children[v_public] > 0)
	 	access = "public";
	      else if (children[v_private] > 0)
	 	access = "private";
	      else 
	 	access = "protected";
	      break;
	    case 1:
	      if (children[v_public] > 0)
		{
		  if (children[v_private] > 0)
		    access = "private";
		  else
		    access = "protected";
		}
	      else if (children[v_private] > 0)
	 	access = "protected";
	      break;
	    case 2:
	      /* Must be protected.  */
	      access = "protected";
	      break;
	    default:
	      /* error!  */
	      break;
	    }

	  gdb_assert (access);
	  if (cname)
	    *cname = xstrdup (access);

	  /* Value and type and full expression are null here.  */
	}
    }
  else
    {
      c_describe_child (parent, index, cname, cvalue, ctype, cfull_expression);
    }  
}

static char *
cplus_name_of_child (struct varobj *parent, int index)
{
  char *name = NULL;

  cplus_describe_child (parent, index, &name, NULL, NULL, NULL);
  return name;
}

static char *
cplus_path_expr_of_child (struct varobj *child)
{
  cplus_describe_child (child->parent, child->index, NULL, NULL, NULL, 
			&child->path_expr);
  return child->path_expr;
}

static struct value *
cplus_value_of_root (struct varobj **var_handle)
{
  return c_value_of_root (var_handle);
}

static struct value *
cplus_value_of_child (struct varobj *parent, int index)
{
  struct value *value = NULL;

  cplus_describe_child (parent, index, NULL, &value, NULL, NULL);
  return value;
}

static struct type *
cplus_type_of_child (struct varobj *parent, int index)
{
  struct type *type = NULL;

  cplus_describe_child (parent, index, NULL, NULL, &type, NULL);
  return type;
}

static char *
cplus_value_of_variable (struct varobj *var, 
			 enum varobj_display_formats format)
{

  /* If we have one of our special types, don't print out
     any value.  */
  if (CPLUS_FAKE_CHILD (var))
    return xstrdup ("");

  return c_value_of_variable (var, format);
}

/* Java */

static int
java_number_of_children (struct varobj *var)
{
  return cplus_number_of_children (var);
}

static char *
java_name_of_variable (struct varobj *parent)
{
  char *p, *name;

  name = cplus_name_of_variable (parent);
  /* If  the name has "-" in it, it is because we
     needed to escape periods in the name...  */
  p = name;

  while (*p != '\000')
    {
      if (*p == '-')
	*p = '.';
      p++;
    }

  return name;
}

static char *
java_name_of_child (struct varobj *parent, int index)
{
  char *name, *p;

  name = cplus_name_of_child (parent, index);
  /* Escape any periods in the name...  */
  p = name;

  while (*p != '\000')
    {
      if (*p == '.')
	*p = '-';
      p++;
    }

  return name;
}

static char *
java_path_expr_of_child (struct varobj *child)
{
  return NULL;
}

static struct value *
java_value_of_root (struct varobj **var_handle)
{
  return cplus_value_of_root (var_handle);
}

static struct value *
java_value_of_child (struct varobj *parent, int index)
{
  return cplus_value_of_child (parent, index);
}

static struct type *
java_type_of_child (struct varobj *parent, int index)
{
  return cplus_type_of_child (parent, index);
}

static char *
java_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  return cplus_value_of_variable (var, format);
}

/* Ada specific callbacks for VAROBJs.  */

static int
ada_number_of_children (struct varobj *var)
{
  return ada_varobj_get_number_of_children (var->value, var->type);
}

static char *
ada_name_of_variable (struct varobj *parent)
{
  return c_name_of_variable (parent);
}

static char *
ada_name_of_child (struct varobj *parent, int index)
{
  return ada_varobj_get_name_of_child (parent->value, parent->type,
				       parent->name, index);
}

static char*
ada_path_expr_of_child (struct varobj *child)
{
  struct varobj *parent = child->parent;
  const char *parent_path_expr = varobj_get_path_expr (parent);

  return ada_varobj_get_path_expr_of_child (parent->value,
					    parent->type,
					    parent->name,
					    parent_path_expr,
					    child->index);
}

static struct value *
ada_value_of_root (struct varobj **var_handle)
{
  return c_value_of_root (var_handle);
}

static struct value *
ada_value_of_child (struct varobj *parent, int index)
{
  return ada_varobj_get_value_of_child (parent->value, parent->type,
					parent->name, index);
}

static struct type *
ada_type_of_child (struct varobj *parent, int index)
{
  return ada_varobj_get_type_of_child (parent->value, parent->type,
				       index);
}

static char *
ada_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  struct value_print_options opts;

  get_formatted_print_options (&opts, format_code[(int) format]);
  opts.deref_ref = 0;
  opts.raw = 1;

  return ada_varobj_get_value_of_variable (var->value, var->type, &opts);
}

/* Implement the "value_is_changeable_p" routine for Ada.  */

static int
ada_value_is_changeable_p (struct varobj *var)
{
  struct type *type = var->value ? value_type (var->value) : var->type;

  if (ada_is_array_descriptor_type (type)
      && TYPE_CODE (type) == TYPE_CODE_TYPEDEF)
    {
      /* This is in reality a pointer to an unconstrained array.
	 its value is changeable.  */
      return 1;
    }

  if (ada_is_string_type (type))
    {
      /* We display the contents of the string in the array's
	 "value" field.  The contents can change, so consider
	 that the array is changeable.  */
      return 1;
    }

  return default_value_is_changeable_p (var);
}

/* Implement the "value_has_mutated" routine for Ada.  */

static int
ada_value_has_mutated (struct varobj *var, struct value *new_val,
		       struct type *new_type)
{
  int i;
  int from = -1;
  int to = -1;

  /* If the number of fields have changed, then for sure the type
     has mutated.  */
  if (ada_varobj_get_number_of_children (new_val, new_type)
      != var->num_children)
    return 1;

  /* If the number of fields have remained the same, then we need
     to check the name of each field.  If they remain the same,
     then chances are the type hasn't mutated.  This is technically
     an incomplete test, as the child's type might have changed
     despite the fact that the name remains the same.  But we'll
     handle this situation by saying that the child has mutated,
     not this value.

     If only part (or none!) of the children have been fetched,
     then only check the ones we fetched.  It does not matter
     to the frontend whether a child that it has not fetched yet
     has mutated or not. So just assume it hasn't.  */

  restrict_range (var->children, &from, &to);
  for (i = from; i < to; i++)
    if (strcmp (ada_varobj_get_name_of_child (new_val, new_type,
					      var->name, i),
		VEC_index (varobj_p, var->children, i)->name) != 0)
      return 1;

  return 0;
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

extern void _initialize_varobj (void);
void
_initialize_varobj (void)
{
  int sizeof_table = sizeof (struct vlist *) * VAROBJ_TABLE_SIZE;

  varobj_table = xmalloc (sizeof_table);
  memset (varobj_table, 0, sizeof_table);

  add_setshow_zuinteger_cmd ("debugvarobj", class_maintenance,
			     &varobjdebug,
			     _("Set varobj debugging."),
			     _("Show varobj debugging."),
			     _("When non-zero, varobj debugging is enabled."),
			     NULL, show_varobjdebug,
			     &setlist, &showlist);
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
      tmp_var = varobj_create (NULL, var->name, (CORE_ADDR) 0,
			       USE_CURRENT_FRAME);
      if (tmp_var != NULL) 
	{ 
	  tmp_var->obj_name = xstrdup (var->obj_name);
	  varobj_delete (var, NULL, 0);
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
