/* XML target description support for GDB.

   Copyright (C) 2006-2017 Free Software Foundation, Inc.

   Contributed by CodeSourcery.

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

#include "defs.h"
#include "target.h"
#include "target-descriptions.h"
#include "xml-support.h"
#include "xml-tdesc.h"
#include "osabi.h"
#include "filenames.h"

/* Maximum sizes.
   This is just to catch obviously wrong values.  */
#define MAX_FIELD_SIZE 65536
#define MAX_FIELD_BITSIZE (MAX_FIELD_SIZE * TARGET_CHAR_BIT)
#define MAX_VECTOR_SIZE 65536

#if !defined(HAVE_LIBEXPAT)

/* Parse DOCUMENT into a target description.  Or don't, since we don't have
   an XML parser.  */

static struct target_desc *
tdesc_parse_xml (const char *document, xml_fetch_another fetcher,
		 void *fetcher_baton)
{
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not parse XML target description; XML support was "
		 "disabled at compile time"));
    }

  return NULL;
}

#else /* HAVE_LIBEXPAT */

/* A record of every XML description we have parsed.  We never discard
   old descriptions, because we never discard gdbarches.  As long as we
   have a gdbarch referencing this description, we want to have a copy
   of it here, so that if we parse the same XML document again we can
   return the same "struct target_desc *"; if they are not singletons,
   then we will create unnecessary duplicate gdbarches.  See
   gdbarch_list_lookup_by_info.  */

struct tdesc_xml_cache
{
  const char *xml_document;
  struct target_desc *tdesc;
};
typedef struct tdesc_xml_cache tdesc_xml_cache_s;
DEF_VEC_O(tdesc_xml_cache_s);

static VEC(tdesc_xml_cache_s) *xml_cache;

/* Callback data for target description parsing.  */

struct tdesc_parsing_data
{
  /* The target description we are building.  */
  struct target_desc *tdesc;

  /* The target feature we are currently parsing, or last parsed.  */
  struct tdesc_feature *current_feature;

  /* The register number to use for the next register we see, if
     it does not have its own.  This starts at zero.  */
  int next_regnum;

  /* The struct or union we are currently parsing, or last parsed.  */
  struct tdesc_type *current_type;

  /* The byte size of the current struct/flags type, if specified.  Zero
     if not specified.  Flags values must specify a size.  */
  int current_type_size;
};

/* Handle the end of an <architecture> element and its value.  */

static void
tdesc_end_arch (struct gdb_xml_parser *parser,
		const struct gdb_xml_element *element,
		void *user_data, const char *body_text)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  const struct bfd_arch_info *arch;

  arch = bfd_scan_arch (body_text);
  if (arch == NULL)
    gdb_xml_error (parser, _("Target description specified unknown "
			     "architecture \"%s\""), body_text);
  set_tdesc_architecture (data->tdesc, arch);
}

/* Handle the end of an <osabi> element and its value.  */

static void
tdesc_end_osabi (struct gdb_xml_parser *parser,
		 const struct gdb_xml_element *element,
		 void *user_data, const char *body_text)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  enum gdb_osabi osabi;

  osabi = osabi_from_tdesc_string (body_text);
  if (osabi == GDB_OSABI_UNKNOWN)
    warning (_("Target description specified unknown osabi \"%s\""),
	     body_text);
  else
    set_tdesc_osabi (data->tdesc, osabi);
}

/* Handle the end of a <compatible> element and its value.  */

static void
tdesc_end_compatible (struct gdb_xml_parser *parser,
		      const struct gdb_xml_element *element,
		      void *user_data, const char *body_text)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  const struct bfd_arch_info *arch;

  arch = bfd_scan_arch (body_text);
  tdesc_add_compatible (data->tdesc, arch);
}

/* Handle the start of a <target> element.  */

static void
tdesc_start_target (struct gdb_xml_parser *parser,
		    const struct gdb_xml_element *element,
		    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  char *version = (char *) xml_find_attribute (attributes, "version")->value;

  if (strcmp (version, "1.0") != 0)
    gdb_xml_error (parser,
		   _("Target description has unsupported version \"%s\""),
		   version);
}

/* Handle the start of a <feature> element.  */

static void
tdesc_start_feature (struct gdb_xml_parser *parser,
		     const struct gdb_xml_element *element,
		     void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  char *name = (char *) xml_find_attribute (attributes, "name")->value;

  data->current_feature = tdesc_create_feature (data->tdesc, name);
}

/* Handle the start of a <reg> element.  Fill in the optional
   attributes and attach it to the containing feature.  */

static void
tdesc_start_reg (struct gdb_xml_parser *parser,
		 const struct gdb_xml_element *element,
		 void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  struct gdb_xml_value *attrs = VEC_address (gdb_xml_value_s, attributes);
  int ix = 0, length;
  char *name, *group;
  const char *type;
  int bitsize, regnum, save_restore;

  length = VEC_length (gdb_xml_value_s, attributes);

  name = (char *) attrs[ix++].value;
  bitsize = * (ULONGEST *) attrs[ix++].value;

  if (ix < length && strcmp (attrs[ix].name, "regnum") == 0)
    regnum = * (ULONGEST *) attrs[ix++].value;
  else
    regnum = data->next_regnum;

  if (ix < length && strcmp (attrs[ix].name, "type") == 0)
    type = (char *) attrs[ix++].value;
  else
    type = "int";

  if (ix < length && strcmp (attrs[ix].name, "group") == 0)
    group = (char *) attrs[ix++].value;
  else
    group = NULL;

  if (ix < length && strcmp (attrs[ix].name, "save-restore") == 0)
    save_restore = * (ULONGEST *) attrs[ix++].value;
  else
    save_restore = 1;

  if (strcmp (type, "int") != 0
      && strcmp (type, "float") != 0
      && tdesc_named_type (data->current_feature, type) == NULL)
    gdb_xml_error (parser, _("Register \"%s\" has unknown type \"%s\""),
		   name, type);

  tdesc_create_reg (data->current_feature, name, regnum, save_restore, group,
		    bitsize, type);

  data->next_regnum = regnum + 1;
}

/* Handle the start of a <union> element.  Initialize the type and
   record it with the current feature.  */

static void
tdesc_start_union (struct gdb_xml_parser *parser,
		   const struct gdb_xml_element *element,
		   void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  char *id = (char *) xml_find_attribute (attributes, "id")->value;

  data->current_type = tdesc_create_union (data->current_feature, id);
  data->current_type_size = 0;
}

/* Handle the start of a <struct> element.  Initialize the type and
   record it with the current feature.  */

static void
tdesc_start_struct (struct gdb_xml_parser *parser,
		   const struct gdb_xml_element *element,
		   void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  char *id = (char *) xml_find_attribute (attributes, "id")->value;
  struct tdesc_type *type;
  struct gdb_xml_value *attr;

  type = tdesc_create_struct (data->current_feature, id);
  data->current_type = type;
  data->current_type_size = 0;

  attr = xml_find_attribute (attributes, "size");
  if (attr != NULL)
    {
      ULONGEST size = * (ULONGEST *) attr->value;

      if (size > MAX_FIELD_SIZE)
	{
	  gdb_xml_error (parser,
			 _("Struct size %s is larger than maximum (%d)"),
			 pulongest (size), MAX_FIELD_SIZE);
	}
      tdesc_set_struct_size (type, size);
      data->current_type_size = size;
    }
}

static void
tdesc_start_flags (struct gdb_xml_parser *parser,
		   const struct gdb_xml_element *element,
		   void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  char *id = (char *) xml_find_attribute (attributes, "id")->value;
  ULONGEST size = * (ULONGEST *)
    xml_find_attribute (attributes, "size")->value;
  struct tdesc_type *type;

  if (size > MAX_FIELD_SIZE)
    {
      gdb_xml_error (parser,
		     _("Flags size %s is larger than maximum (%d)"),
		     pulongest (size), MAX_FIELD_SIZE);
    }
  type = tdesc_create_flags (data->current_feature, id, size);

  data->current_type = type;
  data->current_type_size = size;
}

static void
tdesc_start_enum (struct gdb_xml_parser *parser,
		  const struct gdb_xml_element *element,
		  void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  char *id = (char *) xml_find_attribute (attributes, "id")->value;
  int size = * (ULONGEST *)
    xml_find_attribute (attributes, "size")->value;
  struct tdesc_type *type;

  if (size > MAX_FIELD_SIZE)
    {
      gdb_xml_error (parser,
		     _("Enum size %s is larger than maximum (%d)"),
		     pulongest (size), MAX_FIELD_SIZE);
    }
  type = tdesc_create_enum (data->current_feature, id, size);

  data->current_type = type;
  data->current_type_size = 0;
}

/* Handle the start of a <field> element.  Attach the field to the
   current struct, union or flags.  */

static void
tdesc_start_field (struct gdb_xml_parser *parser,
		   const struct gdb_xml_element *element,
		   void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  struct gdb_xml_value *attr;
  struct tdesc_type *field_type;
  char *field_name, *field_type_id;
  int start, end;

  field_name = (char *) xml_find_attribute (attributes, "name")->value;

  attr = xml_find_attribute (attributes, "type");
  if (attr != NULL)
    {
      field_type_id = (char *) attr->value;
      field_type = tdesc_named_type (data->current_feature, field_type_id);
    }
  else
    {
      field_type_id = NULL;
      field_type = NULL;
    }

  attr = xml_find_attribute (attributes, "start");
  if (attr != NULL)
    {
      ULONGEST ul_start = * (ULONGEST *) attr->value;

      if (ul_start > MAX_FIELD_BITSIZE)
	{
	  gdb_xml_error (parser,
			 _("Field start %s is larger than maximum (%d)"),
			 pulongest (ul_start), MAX_FIELD_BITSIZE);
	}
      start = ul_start;
    }
  else
    start = -1;

  attr = xml_find_attribute (attributes, "end");
  if (attr != NULL)
    {
      ULONGEST ul_end = * (ULONGEST *) attr->value;

      if (ul_end > MAX_FIELD_BITSIZE)
	{
	  gdb_xml_error (parser,
			 _("Field end %s is larger than maximum (%d)"),
			 pulongest (ul_end), MAX_FIELD_BITSIZE);
	}
      end = ul_end;
    }
  else
    end = -1;

  if (start != -1)
    {
      struct tdesc_type *t = data->current_type;

      /* Older versions of gdb can't handle elided end values.
         Stick with that for now, to help ensure backward compatibility.
	 E.g., If a newer gdbserver is talking to an older gdb.  */
      if (end == -1)
	gdb_xml_error (parser, _("Missing end value"));

      if (data->current_type_size == 0)
	gdb_xml_error (parser,
		       _("Bitfields must live in explicitly sized types"));

      if (field_type_id != NULL
	  && strcmp (field_type_id, "bool") == 0
	  && start != end)
	{
	  gdb_xml_error (parser,
			 _("Boolean fields must be one bit in size"));
	}

      if (end >= 64)
	gdb_xml_error (parser,
		       _("Bitfield \"%s\" goes past "
			 "64 bits (unsupported)"),
		       field_name);

      /* Assume that the bit numbering in XML is "lsb-zero".  Most
	 architectures other than PowerPC use this ordering.  In the
	 future, we can add an XML tag to indicate "msb-zero" numbering.  */
      if (start > end)
	gdb_xml_error (parser, _("Bitfield \"%s\" has start after end"),
		       field_name);
      if (end >= data->current_type_size * TARGET_CHAR_BIT)
	gdb_xml_error (parser, _("Bitfield \"%s\" does not fit in struct"),
		       field_name);

      if (field_type != NULL)
	tdesc_add_typed_bitfield (t, field_name, start, end, field_type);
      else if (start == end)
	tdesc_add_flag (t, start, field_name);
      else
	tdesc_add_bitfield (t, field_name, start, end);
    }
  else if (start == -1 && end != -1)
    gdb_xml_error (parser, _("End specified but not start"));
  else if (field_type_id != NULL)
    {
      /* TDESC_TYPE_FLAGS values are explicitly sized, so the following test
	 catches adding non-bitfield types to flags as well.  */
      if (data->current_type_size != 0)
	gdb_xml_error (parser,
		       _("Explicitly sized type cannot "
			 "contain non-bitfield \"%s\""),
		       field_name);

      if (field_type == NULL)
	gdb_xml_error (parser, _("Field \"%s\" references undefined "
				 "type \"%s\""),
		       field_name, field_type_id);

      tdesc_add_field (data->current_type, field_name, field_type);
    }
  else
    gdb_xml_error (parser, _("Field \"%s\" has neither type nor bit position"),
		   field_name);
}

/* Handle the start of an <evalue> element.  Attach the value to the
   current enum.  */

static void
tdesc_start_enum_value (struct gdb_xml_parser *parser,
			const struct gdb_xml_element *element,
			void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  struct gdb_xml_value *attr;
  char *field_name;
  ULONGEST ul_value;
  int value;

  field_name = (char *) xml_find_attribute (attributes, "name")->value;

  attr = xml_find_attribute (attributes, "value");
  ul_value = * (ULONGEST *) attr->value;
  if (ul_value > INT_MAX)
    {
      gdb_xml_error (parser,
		     _("Enum value %s is larger than maximum (%d)"),
		     pulongest (ul_value), INT_MAX);
    }
  value = ul_value;

  tdesc_add_enum_value (data->current_type, value, field_name);
}

/* Handle the start of a <vector> element.  Initialize the type and
   record it with the current feature.  */

static void
tdesc_start_vector (struct gdb_xml_parser *parser,
		    const struct gdb_xml_element *element,
		    void *user_data, VEC(gdb_xml_value_s) *attributes)
{
  struct tdesc_parsing_data *data = (struct tdesc_parsing_data *) user_data;
  struct gdb_xml_value *attrs = VEC_address (gdb_xml_value_s, attributes);
  struct tdesc_type *field_type;
  char *id, *field_type_id;
  ULONGEST count;

  id = (char *) attrs[0].value;
  field_type_id = (char *) attrs[1].value;
  count = * (ULONGEST *) attrs[2].value;

  if (count > MAX_VECTOR_SIZE)
    {
      gdb_xml_error (parser,
		     _("Vector size %s is larger than maximum (%d)"),
		     pulongest (count), MAX_VECTOR_SIZE);
    }

  field_type = tdesc_named_type (data->current_feature, field_type_id);
  if (field_type == NULL)
    gdb_xml_error (parser, _("Vector \"%s\" references undefined type \"%s\""),
		   id, field_type_id);

  tdesc_create_vector (data->current_feature, id, field_type, count);
}

/* The elements and attributes of an XML target description.  */

static const struct gdb_xml_attribute field_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "type", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "start", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { "end", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute enum_value_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "value", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element struct_union_children[] = {
  { "field", field_attributes, NULL, GDB_XML_EF_REPEATABLE,
    tdesc_start_field, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element enum_children[] = {
  { "evalue", enum_value_attributes, NULL, GDB_XML_EF_REPEATABLE,
    tdesc_start_enum_value, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute reg_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { "bitsize", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { "regnum", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL },
  { "type", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "group", GDB_XML_AF_OPTIONAL, NULL, NULL },
  { "save-restore", GDB_XML_AF_OPTIONAL,
    gdb_xml_parse_attr_enum, gdb_xml_enums_boolean },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute struct_union_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "size", GDB_XML_AF_OPTIONAL, gdb_xml_parse_attr_ulongest, NULL},
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute flags_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL},
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute enum_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "size", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL},
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute vector_attributes[] = {
  { "id", GDB_XML_AF_NONE, NULL, NULL },
  { "type", GDB_XML_AF_NONE, NULL, NULL },
  { "count", GDB_XML_AF_NONE, gdb_xml_parse_attr_ulongest, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute feature_attributes[] = {
  { "name", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element feature_children[] = {
  { "reg", reg_attributes, NULL,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_reg, NULL },
  { "struct", struct_union_attributes, struct_union_children,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_struct, NULL },
  { "union", struct_union_attributes, struct_union_children,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_union, NULL },
  { "flags", flags_attributes, struct_union_children,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_flags, NULL },    
  { "enum", enum_attributes, enum_children,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_enum, NULL },
  { "vector", vector_attributes, NULL,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_vector, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_attribute target_attributes[] = {
  { "version", GDB_XML_AF_NONE, NULL, NULL },
  { NULL, GDB_XML_AF_NONE, NULL, NULL }
};

static const struct gdb_xml_element target_children[] = {
  { "architecture", NULL, NULL, GDB_XML_EF_OPTIONAL,
    NULL, tdesc_end_arch },
  { "osabi", NULL, NULL, GDB_XML_EF_OPTIONAL,
    NULL, tdesc_end_osabi },
  { "compatible", NULL, NULL, GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    NULL, tdesc_end_compatible },
  { "feature", feature_attributes, feature_children,
    GDB_XML_EF_OPTIONAL | GDB_XML_EF_REPEATABLE,
    tdesc_start_feature, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

static const struct gdb_xml_element tdesc_elements[] = {
  { "target", target_attributes, target_children, GDB_XML_EF_NONE,
    tdesc_start_target, NULL },
  { NULL, NULL, NULL, GDB_XML_EF_NONE, NULL, NULL }
};

/* Parse DOCUMENT into a target description and return it.  */

static struct target_desc *
tdesc_parse_xml (const char *document, xml_fetch_another fetcher,
		 void *fetcher_baton)
{
  struct cleanup *back_to, *result_cleanup;
  struct tdesc_parsing_data data;
  struct tdesc_xml_cache *cache;
  char *expanded_text;
  int ix;

  /* Expand all XInclude directives.  */
  expanded_text = xml_process_xincludes (_("target description"),
					 document, fetcher, fetcher_baton, 0);
  if (expanded_text == NULL)
    {
      warning (_("Could not load XML target description; ignoring"));
      return NULL;
    }

  /* Check for an exact match in the list of descriptions we have
     previously parsed.  strcmp is a slightly inefficient way to
     do this; an SHA-1 checksum would work as well.  */
  for (ix = 0; VEC_iterate (tdesc_xml_cache_s, xml_cache, ix, cache); ix++)
    if (strcmp (cache->xml_document, expanded_text) == 0)
      {
       xfree (expanded_text);
       return cache->tdesc;
      }

  back_to = make_cleanup (null_cleanup, NULL);

  memset (&data, 0, sizeof (struct tdesc_parsing_data));
  data.tdesc = allocate_target_description ();
  result_cleanup = make_cleanup_free_target_description (data.tdesc);
  make_cleanup (xfree, expanded_text);

  if (gdb_xml_parse_quick (_("target description"), "gdb-target.dtd",
			   tdesc_elements, expanded_text, &data) == 0)
    {
      /* Parsed successfully.  */
      struct tdesc_xml_cache new_cache;

      new_cache.xml_document = expanded_text;
      new_cache.tdesc = data.tdesc;
      VEC_safe_push (tdesc_xml_cache_s, xml_cache, &new_cache);
      discard_cleanups (result_cleanup);
      do_cleanups (back_to);
      return data.tdesc;
    }
  else
    {
      warning (_("Could not load XML target description; ignoring"));
      do_cleanups (back_to);
      return NULL;
    }
}
#endif /* HAVE_LIBEXPAT */


/* Read an XML target description from FILENAME.  Parse it, and return
   the parsed description.  */

const struct target_desc *
file_read_description_xml (const char *filename)
{
  struct target_desc *tdesc;
  char *tdesc_str;
  struct cleanup *back_to;

  tdesc_str = xml_fetch_content_from_file (filename, NULL);
  if (tdesc_str == NULL)
    {
      warning (_("Could not open \"%s\""), filename);
      return NULL;
    }

  back_to = make_cleanup (xfree, tdesc_str);

  tdesc = tdesc_parse_xml (tdesc_str, xml_fetch_content_from_file,
			   (void *) ldirname (filename).c_str ());
  do_cleanups (back_to);

  return tdesc;
}

/* Read a string representation of available features from the target,
   using TARGET_OBJECT_AVAILABLE_FEATURES.  The returned string is
   malloc allocated and NUL-terminated.  NAME should be a non-NULL
   string identifying the XML document we want; the top level document
   is "target.xml".  Other calls may be performed for the DTD or
   for <xi:include>.  */

static char *
fetch_available_features_from_target (const char *name, void *baton_)
{
  struct target_ops *ops = (struct target_ops *) baton_;

  /* Read this object as a string.  This ensures that a NUL
     terminator is added.  */
  return target_read_stralloc (ops,
			       TARGET_OBJECT_AVAILABLE_FEATURES,
			       name);
}


/* Read an XML target description using OPS.  Parse it, and return the
   parsed description.  */

const struct target_desc *
target_read_description_xml (struct target_ops *ops)
{
  struct target_desc *tdesc;
  char *tdesc_str;
  struct cleanup *back_to;

  tdesc_str = fetch_available_features_from_target ("target.xml", ops);
  if (tdesc_str == NULL)
    return NULL;

  back_to = make_cleanup (xfree, tdesc_str);
  tdesc = tdesc_parse_xml (tdesc_str,
			   fetch_available_features_from_target,
			   ops);
  do_cleanups (back_to);

  return tdesc;
}

/* Fetches an XML target description using OPS,  processing
   includes, but not parsing it.  Used to dump whole tdesc
   as a single XML file.  */

char *
target_fetch_description_xml (struct target_ops *ops)
{
#if !defined(HAVE_LIBEXPAT)
  static int have_warned;

  if (!have_warned)
    {
      have_warned = 1;
      warning (_("Can not fetch XML target description; XML support was "
		 "disabled at compile time"));
    }

  return NULL;
#else
  struct target_desc *tdesc;
  char *tdesc_str;
  char *expanded_text;
  struct cleanup *back_to;

  tdesc_str = fetch_available_features_from_target ("target.xml", ops);
  if (tdesc_str == NULL)
    return NULL;

  back_to = make_cleanup (xfree, tdesc_str);
  expanded_text = xml_process_xincludes (_("target description"),
					 tdesc_str,
					 fetch_available_features_from_target, ops, 0);
  do_cleanups (back_to);
  if (expanded_text == NULL)
    {
      warning (_("Could not load XML target description; ignoring"));
      return NULL;
    }

  return expanded_text;
#endif
}
