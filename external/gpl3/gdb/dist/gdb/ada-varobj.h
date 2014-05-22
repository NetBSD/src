/* varobj support for Ada.

   Copyright (C) 2012-2013 Free Software Foundation, Inc.

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

#ifndef ADA_VAROBJ_H
#define ADA_VAROBJ_H

#include "varobj.h"

struct value;
struct value_print_options;

extern int ada_varobj_get_number_of_children (struct value *parent_value,
					      struct type *parent_type);

extern char *ada_varobj_get_name_of_child (struct value *parent_value,
					   struct type *parent_type,
					   const char *parent_name,
					   int child_index);

extern char *ada_varobj_get_path_expr_of_child (struct value *parent_value,
						struct type *parent_type,
						const char *parent_name,
						const char *parent_path_expr,
						int child_index);

extern struct value *ada_varobj_get_value_of_child (struct value *parent_value,
						    struct type *parent_type,
						    const char *parent_name,
						    int child_index);

extern struct type *ada_varobj_get_type_of_child (struct value *parent_value,
						  struct type *parent_type,
						  int child_index);

extern char *ada_varobj_get_value_of_variable
  (struct value *value, struct type *type,
   struct value_print_options *opts);

#endif /* ADA_VAROBJ_H */
