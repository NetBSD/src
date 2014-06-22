/* varobj support for C and C++.

   Copyright (C) 1999-2014 Free Software Foundation, Inc.

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
#include "varobj.h"
#include "gdbthread.h"
#include "valprint.h"

static void cplus_class_num_children (struct type *type, int children[3]);

/* The names of varobjs representing anonymous structs or unions.  */
#define ANONYMOUS_STRUCT_NAME _("<anonymous struct>")
#define ANONYMOUS_UNION_NAME _("<anonymous union>")

/* Does CHILD represent a child with no name?  This happens when
   the child is an anonmous struct or union and it has no field name
   in its parent variable.

   This has already been determined by *_describe_child. The easiest
   thing to do is to compare the child's name with ANONYMOUS_*_NAME.  */

int
varobj_is_anonymous_child (struct varobj *child)
{
  return (strcmp (child->name, ANONYMOUS_STRUCT_NAME) == 0
	  || strcmp (child->name, ANONYMOUS_UNION_NAME) == 0);
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

/* C */

static int
c_number_of_children (struct varobj *var)
{
  struct type *type = varobj_get_value_type (var);
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
  struct type *type = varobj_get_value_type (parent);
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
      parent_expression
	= varobj_get_path_expr (varobj_get_path_expr_parent (parent));
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

/* This returns the type of the variable.  It also skips past typedefs
   to return the real type of the variable.  */

static struct type *
get_type (struct varobj *var)
{
  struct type *type;

  type = var->type;
  if (type != NULL)
    type = check_typedef (type);

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
	      return varobj_value_get_print_value (var->value, format, var);
	  }
      }
    }
}


/* varobj operations for c.  */

const struct lang_varobj_ops c_varobj_ops =
{
   c_number_of_children,
   c_name_of_variable,
   c_name_of_child,
   c_path_expr_of_child,
   c_value_of_child,
   c_type_of_child,
   c_value_of_variable,
   varobj_default_value_is_changeable_p,
   NULL /* value_has_mutated */
};

/* A little convenience enum for dealing with C++/Java.  */
enum vsections
{
  v_public = 0, v_private, v_protected
};

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
      type = varobj_get_value_type (var);

      /* It is necessary to access a real type (via RTTI).  */
      if (opts.objectprint)
        {
          value = var->value;
          lookup_actual_type = (TYPE_CODE (var->type) == TYPE_CODE_REF
				|| TYPE_CODE (var->type) == TYPE_CODE_PTR);
        }
      adjust_value_for_child_access (&value, &type, NULL, lookup_actual_type);

      if (((TYPE_CODE (type)) == TYPE_CODE_STRUCT)
	  || ((TYPE_CODE (type)) == TYPE_CODE_UNION))
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

      type = varobj_get_value_type (var->parent);

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
  type = varobj_get_value_type (var);
  if (cfull_expression)
    parent_expression
      = varobj_get_path_expr (varobj_get_path_expr_parent (var));

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


/* varobj operations for c++.  */

const struct lang_varobj_ops cplus_varobj_ops =
{
   cplus_number_of_children,
   cplus_name_of_variable,
   cplus_name_of_child,
   cplus_path_expr_of_child,
   cplus_value_of_child,
   cplus_type_of_child,
   cplus_value_of_variable,
   varobj_default_value_is_changeable_p,
   NULL /* value_has_mutated */
};


