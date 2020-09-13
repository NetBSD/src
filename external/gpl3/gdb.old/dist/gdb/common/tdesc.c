/* Target description support for GDB.

   Copyright (C) 2018-2019 Free Software Foundation, Inc.

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

#include "common-defs.h"
#include "common/tdesc.h"

tdesc_reg::tdesc_reg (struct tdesc_feature *feature, const std::string &name_,
		      int regnum, int save_restore_, const char *group_,
		      int bitsize_, const char *type_)
  : name (name_), target_regnum (regnum),
    save_restore (save_restore_),
    group (group_ != NULL ? group_ : ""),
    bitsize (bitsize_),
    type (type_ != NULL ? type_ : "<unknown>")
{
  /* If the register's type is target-defined, look it up now.  We may not
     have easy access to the containing feature when we want it later.  */
  tdesc_type = tdesc_named_type (feature, type.c_str ());
}

/* Predefined types.  */
static tdesc_type_builtin tdesc_predefined_types[] =
{
  { "bool", TDESC_TYPE_BOOL },
  { "int8", TDESC_TYPE_INT8 },
  { "int16", TDESC_TYPE_INT16 },
  { "int32", TDESC_TYPE_INT32 },
  { "int64", TDESC_TYPE_INT64 },
  { "int128", TDESC_TYPE_INT128 },
  { "uint8", TDESC_TYPE_UINT8 },
  { "uint16", TDESC_TYPE_UINT16 },
  { "uint32", TDESC_TYPE_UINT32 },
  { "uint64", TDESC_TYPE_UINT64 },
  { "uint128", TDESC_TYPE_UINT128 },
  { "code_ptr", TDESC_TYPE_CODE_PTR },
  { "data_ptr", TDESC_TYPE_DATA_PTR },
  { "ieee_single", TDESC_TYPE_IEEE_SINGLE },
  { "ieee_double", TDESC_TYPE_IEEE_DOUBLE },
  { "arm_fpa_ext", TDESC_TYPE_ARM_FPA_EXT },
  { "i387_ext", TDESC_TYPE_I387_EXT }
};

void tdesc_feature::accept (tdesc_element_visitor &v) const
{
  v.visit_pre (this);

  for (const tdesc_type_up &type : types)
    type->accept (v);

  for (const tdesc_reg_up &reg : registers)
    reg->accept (v);

  v.visit_post (this);
}

bool tdesc_feature::operator== (const tdesc_feature &other) const
{
  if (name != other.name)
    return false;

  if (registers.size () != other.registers.size ())
    return false;

  for (int ix = 0; ix < registers.size (); ix++)
    {
      const tdesc_reg_up &reg1 = registers[ix];
      const tdesc_reg_up &reg2 = other.registers[ix];

      if (reg1 != reg2 && *reg1 != *reg2)
	return false;
      }

  if (types.size () != other.types.size ())
    return false;

  for (int ix = 0; ix < types.size (); ix++)
    {
      const tdesc_type_up &type1 = types[ix];
      const tdesc_type_up &type2 = other.types[ix];

      if (type1 != type2 && *type1 != *type2)
	return false;
    }

  return true;
}

/* Lookup a predefined type.  */

static struct tdesc_type *
tdesc_predefined_type (enum tdesc_type_kind kind)
{
  for (int ix = 0; ix < ARRAY_SIZE (tdesc_predefined_types); ix++)
    if (tdesc_predefined_types[ix].kind == kind)
      return &tdesc_predefined_types[ix];

  gdb_assert_not_reached ("bad predefined tdesc type");
}

/* See common/tdesc.h.  */

struct tdesc_type *
tdesc_named_type (const struct tdesc_feature *feature, const char *id)
{
  /* First try target-defined types.  */
  for (const tdesc_type_up &type : feature->types)
    if (type->name == id)
      return type.get ();

  /* Next try the predefined types.  */
  for (int ix = 0; ix < ARRAY_SIZE (tdesc_predefined_types); ix++)
    if (tdesc_predefined_types[ix].name == id)
      return &tdesc_predefined_types[ix];

  return NULL;
}

/* See common/tdesc.h.  */

void
tdesc_create_reg (struct tdesc_feature *feature, const char *name,
		  int regnum, int save_restore, const char *group,
		  int bitsize, const char *type)
{
  tdesc_reg *reg = new tdesc_reg (feature, name, regnum, save_restore,
				  group, bitsize, type);

  feature->registers.emplace_back (reg);
}

/* See common/tdesc.h.  */

struct tdesc_type *
tdesc_create_vector (struct tdesc_feature *feature, const char *name,
		     struct tdesc_type *field_type, int count)
{
  tdesc_type_vector *type = new tdesc_type_vector (name, field_type, count);
  feature->types.emplace_back (type);

  return type;
}

/* See common/tdesc.h.  */

tdesc_type_with_fields *
tdesc_create_struct (struct tdesc_feature *feature, const char *name)
{
  tdesc_type_with_fields *type
    = new tdesc_type_with_fields (name, TDESC_TYPE_STRUCT);
  feature->types.emplace_back (type);

  return type;
}

/* See common/tdesc.h.  */

void
tdesc_set_struct_size (tdesc_type_with_fields *type, int size)
{
  gdb_assert (type->kind == TDESC_TYPE_STRUCT);
  gdb_assert (size > 0);
  type->size = size;
}

/* See common/tdesc.h.  */

tdesc_type_with_fields *
tdesc_create_union (struct tdesc_feature *feature, const char *name)
{
  tdesc_type_with_fields *type
    = new tdesc_type_with_fields (name, TDESC_TYPE_UNION);
  feature->types.emplace_back (type);

  return type;
}

/* See common/tdesc.h.  */

tdesc_type_with_fields *
tdesc_create_flags (struct tdesc_feature *feature, const char *name,
		    int size)
{
  gdb_assert (size > 0);

  tdesc_type_with_fields *type
    = new tdesc_type_with_fields (name, TDESC_TYPE_FLAGS, size);
  feature->types.emplace_back (type);

  return type;
}

/* See common/tdesc.h.  */

tdesc_type_with_fields *
tdesc_create_enum (struct tdesc_feature *feature, const char *name,
		   int size)
{
  gdb_assert (size > 0);

  tdesc_type_with_fields *type
    = new tdesc_type_with_fields (name, TDESC_TYPE_ENUM, size);
  feature->types.emplace_back (type);

  return type;
}

/* See common/tdesc.h.  */

void
tdesc_add_field (tdesc_type_with_fields *type, const char *field_name,
		 struct tdesc_type *field_type)
{
  gdb_assert (type->kind == TDESC_TYPE_UNION
	      || type->kind == TDESC_TYPE_STRUCT);

  /* Initialize start and end so we know this is not a bit-field
     when we print-c-tdesc.  */
  type->fields.emplace_back (field_name, field_type, -1, -1);
}

/* See common/tdesc.h.  */

void
tdesc_add_typed_bitfield (tdesc_type_with_fields *type, const char *field_name,
			  int start, int end, struct tdesc_type *field_type)
{
  gdb_assert (type->kind == TDESC_TYPE_STRUCT
	      || type->kind == TDESC_TYPE_FLAGS);
  gdb_assert (start >= 0 && end >= start);

  type->fields.emplace_back (field_name, field_type, start, end);
}

/* See common/tdesc.h.  */

void
tdesc_add_bitfield (tdesc_type_with_fields *type, const char *field_name,
		    int start, int end)
{
  struct tdesc_type *field_type;

  gdb_assert (start >= 0 && end >= start);

  if (type->size > 4)
    field_type = tdesc_predefined_type (TDESC_TYPE_UINT64);
  else
    field_type = tdesc_predefined_type (TDESC_TYPE_UINT32);

  tdesc_add_typed_bitfield (type, field_name, start, end, field_type);
}

/* See common/tdesc.h.  */

void
tdesc_add_flag (tdesc_type_with_fields *type, int start,
		const char *flag_name)
{
  gdb_assert (type->kind == TDESC_TYPE_FLAGS
	      || type->kind == TDESC_TYPE_STRUCT);

  type->fields.emplace_back (flag_name,
			     tdesc_predefined_type (TDESC_TYPE_BOOL),
			     start, start);
}

/* See common/tdesc.h.  */

void
tdesc_add_enum_value (tdesc_type_with_fields *type, int value,
		      const char *name)
{
  gdb_assert (type->kind == TDESC_TYPE_ENUM);
  type->fields.emplace_back (name,
			     tdesc_predefined_type (TDESC_TYPE_INT32),
			     value, -1);
}

void print_xml_feature::visit_pre (const tdesc_feature *e)
{
  string_appendf (*m_buffer, "<feature name=\"%s\">\n", e->name.c_str ());
}

void print_xml_feature::visit_post (const tdesc_feature *e)
{
  string_appendf (*m_buffer, "</feature>\n");
}

void print_xml_feature::visit (const tdesc_type_builtin *t)
{
  error (_("xml output is not supported for type \"%s\"."), t->name.c_str ());
}

void print_xml_feature::visit (const tdesc_type_vector *t)
{
  string_appendf (*m_buffer, "<vector id=\"%s\" type=\"%s\" count=\"%d\"/>\n",
		  t->name.c_str (), t->element_type->name.c_str (), t->count);
}

void print_xml_feature::visit (const tdesc_type_with_fields *t)
{
  const static char *types[] = { "struct", "union", "flags", "enum" };

  gdb_assert (t->kind >= TDESC_TYPE_STRUCT && t->kind <= TDESC_TYPE_ENUM);

  string_appendf (*m_buffer,
		  "<%s id=\"%s\"", types[t->kind - TDESC_TYPE_STRUCT],
		  t->name.c_str ());

  switch (t->kind)
    {
    case TDESC_TYPE_STRUCT:
    case TDESC_TYPE_FLAGS:
      if (t->size > 0)
	string_appendf (*m_buffer, " size=\"%d\"", t->size);
      string_appendf (*m_buffer, ">\n");

      for (const tdesc_type_field &f : t->fields)
	{
	  string_appendf (*m_buffer, "  <field name=\"%s\" ", f.name.c_str ());
	  if (f.start == -1)
	    string_appendf (*m_buffer, "type=\"%s\"/>\n",
			    f.type->name.c_str ());
	  else
	    string_appendf (*m_buffer, "start=\"%d\" end=\"%d\"/>\n", f.start,
			    f.end);
	}
      break;

    case TDESC_TYPE_ENUM:
      string_appendf (*m_buffer, ">\n");
      for (const tdesc_type_field &f : t->fields)
	string_appendf (*m_buffer, "  <field name=\"%s\" start=\"%d\"/>\n",
			f.name.c_str (), f.start);
      break;

    case TDESC_TYPE_UNION:
      string_appendf (*m_buffer, ">\n");
      for (const tdesc_type_field &f : t->fields)
	string_appendf (*m_buffer, "  <field name=\"%s\" type=\"%s\"/>\n",
			f.name.c_str (), f.type->name.c_str ());
      break;

    default:
      error (_("xml output is not supported for type \"%s\"."),
	     t->name.c_str ());
    }

  string_appendf (*m_buffer, "</%s>\n", types[t->kind - TDESC_TYPE_STRUCT]);
}

void print_xml_feature::visit (const tdesc_reg *r)
{
  string_appendf (*m_buffer,
		  "<reg name=\"%s\" bitsize=\"%d\" type=\"%s\" regnum=\"%ld\"",
		  r->name.c_str (), r->bitsize, r->type.c_str (),
		  r->target_regnum);

  if (r->group.length () > 0)
    string_appendf (*m_buffer, " group=\"%s\"", r->group.c_str ());

  if (r->save_restore == 0)
    string_appendf (*m_buffer, " save-restore=\"no\"");

  string_appendf (*m_buffer, "/>\n");
}

void print_xml_feature::visit_pre (const target_desc *e)
{
#ifndef IN_PROCESS_AGENT
  string_appendf (*m_buffer, "<?xml version=\"1.0\"?>\n");
  string_appendf (*m_buffer, "<!DOCTYPE target SYSTEM \"gdb-target.dtd\">\n");
  string_appendf (*m_buffer, "<target>\n<architecture>%s</architecture>\n",
		  tdesc_architecture_name (e));

  const char *osabi = tdesc_osabi_name (e);
  if (osabi != nullptr)
    string_appendf (*m_buffer, "<osabi>%s</osabi>", osabi);
#endif
}

void print_xml_feature::visit_post (const target_desc *e)
{
  string_appendf (*m_buffer, "</target>\n");
}
