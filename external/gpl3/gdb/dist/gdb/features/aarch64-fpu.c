/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-fpu.xml */

#include "common/tdesc.h"

static int
create_feature_aarch64_fpu (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.fpu");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v2d", element_type, 2);

  element_type = tdesc_named_type (feature, "uint64");
  tdesc_create_vector (feature, "v2u", element_type, 2);

  element_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v2i", element_type, 2);

  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", element_type, 4);

  element_type = tdesc_named_type (feature, "uint32");
  tdesc_create_vector (feature, "v4u", element_type, 4);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i", element_type, 4);

  element_type = tdesc_named_type (feature, "uint16");
  tdesc_create_vector (feature, "v8u", element_type, 8);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i", element_type, 8);

  element_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "v16u", element_type, 16);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i", element_type, 16);

  element_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "v1u", element_type, 1);

  element_type = tdesc_named_type (feature, "int128");
  tdesc_create_vector (feature, "v1i", element_type, 1);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "vnd");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "v2d");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "v2u");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "v2i");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "vns");
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type_with_fields, "f", field_type);
  field_type = tdesc_named_type (feature, "v4u");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "v4i");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "vnh");
  field_type = tdesc_named_type (feature, "v8u");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "v8i");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "vnb");
  field_type = tdesc_named_type (feature, "v16u");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "v16i");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "vnq");
  field_type = tdesc_named_type (feature, "v1u");
  tdesc_add_field (type_with_fields, "u", field_type);
  field_type = tdesc_named_type (feature, "v1i");
  tdesc_add_field (type_with_fields, "s", field_type);

  type_with_fields = tdesc_create_union (feature, "aarch64v");
  field_type = tdesc_named_type (feature, "vnd");
  tdesc_add_field (type_with_fields, "d", field_type);
  field_type = tdesc_named_type (feature, "vns");
  tdesc_add_field (type_with_fields, "s", field_type);
  field_type = tdesc_named_type (feature, "vnh");
  tdesc_add_field (type_with_fields, "h", field_type);
  field_type = tdesc_named_type (feature, "vnb");
  tdesc_add_field (type_with_fields, "b", field_type);
  field_type = tdesc_named_type (feature, "vnq");
  tdesc_add_field (type_with_fields, "q", field_type);

  regnum = 34;
  tdesc_create_reg (feature, "v0", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v1", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v2", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v3", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v4", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v5", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v6", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v7", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v8", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v9", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v10", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v11", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v12", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v13", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v14", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v15", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v16", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v17", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v18", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v19", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v20", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v21", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v22", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v23", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v24", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v25", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v26", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v27", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v28", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v29", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v30", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v31", regnum++, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "fpsr", regnum++, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "fpcr", regnum++, 1, NULL, 32, "int");
  return regnum;
}
