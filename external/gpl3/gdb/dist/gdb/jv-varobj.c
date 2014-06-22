/* varobj support for Java.

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
#include "varobj.h"

/* Java */

static int
java_number_of_children (struct varobj *var)
{
  return cplus_varobj_ops.number_of_children (var);
}

static char *
java_name_of_variable (struct varobj *parent)
{
  char *p, *name;

  name = cplus_varobj_ops.name_of_variable (parent);
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

  name = cplus_varobj_ops.name_of_child (parent, index);
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
java_value_of_child (struct varobj *parent, int index)
{
  return cplus_varobj_ops.value_of_child (parent, index);
}

static struct type *
java_type_of_child (struct varobj *parent, int index)
{
  return cplus_varobj_ops.type_of_child (parent, index);
}

static char *
java_value_of_variable (struct varobj *var, enum varobj_display_formats format)
{
  return cplus_varobj_ops.value_of_variable (var, format);
}

/* varobj operations for java.  */

const struct lang_varobj_ops java_varobj_ops =
{
   java_number_of_children,
   java_name_of_variable,
   java_name_of_child,
   java_path_expr_of_child,
   java_value_of_child,
   java_type_of_child,
   java_value_of_variable,
   varobj_default_value_is_changeable_p,
   NULL /* value_has_mutated */
};
