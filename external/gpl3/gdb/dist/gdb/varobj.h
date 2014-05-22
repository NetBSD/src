/* GDB variable objects API.
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

#ifndef VAROBJ_H
#define VAROBJ_H 1

#include "symtab.h"
#include "gdbtypes.h"
#include "vec.h"

/* Enumeration for the format types */
enum varobj_display_formats
  {
    FORMAT_NATURAL,		/* What gdb actually calls 'natural' */
    FORMAT_BINARY,		/* Binary display                    */
    FORMAT_DECIMAL,		/* Decimal display                   */
    FORMAT_HEXADECIMAL,		/* Hex display                       */
    FORMAT_OCTAL		/* Octal display                     */
  };

enum varobj_type
  {
    USE_SPECIFIED_FRAME,        /* Use the frame passed to varobj_create.  */
    USE_CURRENT_FRAME,          /* Use the current frame.  */
    USE_SELECTED_FRAME          /* Always reevaluate in selected frame.  */
  };

/* Enumerator describing if a variable object is in scope.  */
enum varobj_scope_status
  {
    VAROBJ_IN_SCOPE = 0,        /* Varobj is scope, value available.  */
    VAROBJ_NOT_IN_SCOPE = 1,    /* Varobj is not in scope, value not
				   available, but varobj can become in
				   scope later.  */
    VAROBJ_INVALID = 2,         /* Varobj no longer has any value, and never
				   will.  */
  };

/* String representations of gdb's format codes (defined in varobj.c).  */
extern char *varobj_format_string[];

/* Languages supported by this variable objects system.  */
enum varobj_languages
  {
    vlang_unknown = 0, vlang_c, vlang_cplus, vlang_java, vlang_ada, vlang_end
  };

/* String representations of gdb's known languages (defined in varobj.c).  */
extern char *varobj_language_string[];

/* Struct thar describes a variable object instance.  */
struct varobj;

typedef struct varobj *varobj_p;
DEF_VEC_P (varobj_p);

typedef struct varobj_update_result_t
{
  struct varobj *varobj;
  int type_changed;
  int children_changed;
  int changed;
  enum varobj_scope_status status;
  /* This variable is used internally by varobj_update to indicate if the
     new value of varobj is already computed and installed, or has to
     be yet installed.  Don't use this outside varobj.c.  */
  int value_installed;  

  /* This will be non-NULL when new children were added to the varobj.
     It lists the new children (which must necessarily come at the end
     of the child list) added during an update.  The caller is
     responsible for freeing this vector.  */
  VEC (varobj_p) *new;
} varobj_update_result;

DEF_VEC_O (varobj_update_result);

/* API functions */

extern struct varobj *varobj_create (char *objname,
				     char *expression, CORE_ADDR frame,
				     enum varobj_type type);

extern char *varobj_gen_name (void);

extern struct varobj *varobj_get_handle (char *name);

extern char *varobj_get_objname (struct varobj *var);

extern char *varobj_get_expression (struct varobj *var);

extern int varobj_delete (struct varobj *var, char ***dellist,
			  int only_children);

extern enum varobj_display_formats varobj_set_display_format (
							 struct varobj *var,
					enum varobj_display_formats format);

extern enum varobj_display_formats varobj_get_display_format (
							struct varobj *var);

extern int varobj_get_thread_id (struct varobj *var);

extern void varobj_set_frozen (struct varobj *var, int frozen);

extern int varobj_get_frozen (struct varobj *var);

extern void varobj_get_child_range (struct varobj *var, int *from, int *to);

extern void varobj_set_child_range (struct varobj *var, int from, int to);

extern char *varobj_get_display_hint (struct varobj *var);

extern int varobj_get_num_children (struct varobj *var);

/* Return the list of children of VAR.  The returned vector should not
   be modified in any way.  FROM and TO are in/out parameters
   indicating the range of children to return.  If either *FROM or *TO
   is less than zero on entry, then all children will be returned.  On
   return, *FROM and *TO will be updated to indicate the real range
   that was returned.  The resulting VEC will contain at least the
   children from *FROM to just before *TO; it might contain more
   children, depending on whether any more were available.  */
extern VEC (varobj_p)* varobj_list_children (struct varobj *var,
					     int *from, int *to);

extern char *varobj_get_type (struct varobj *var);

extern struct type *varobj_get_gdb_type (struct varobj *var);

extern char *varobj_get_path_expr (struct varobj *var);

extern enum varobj_languages varobj_get_language (struct varobj *var);

extern int varobj_get_attributes (struct varobj *var);

extern char *varobj_get_formatted_value (struct varobj *var,
					 enum varobj_display_formats format);

extern char *varobj_get_value (struct varobj *var);

extern int varobj_set_value (struct varobj *var, char *expression);

extern void all_root_varobjs (void (*func) (struct varobj *var, void *data),
			      void *data);

extern VEC(varobj_update_result) *varobj_update (struct varobj **varp, 
						 int explicit);

extern void varobj_invalidate (void);

extern int varobj_editable_p (struct varobj *var);

extern int varobj_floating_p (struct varobj *var);

extern void varobj_set_visualizer (struct varobj *var,
				   const char *visualizer);

extern void varobj_enable_pretty_printing (void);

extern int varobj_has_more (struct varobj *var, int to);

extern int varobj_pretty_printed_p (struct varobj *var);

#endif /* VAROBJ_H */
