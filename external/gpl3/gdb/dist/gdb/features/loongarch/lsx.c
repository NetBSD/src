/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: lsx.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_loongarch_lsx (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.loongarch.lsx");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f32", element_type, 4);

  element_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v2f64", element_type, 2);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", element_type, 16);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", element_type, 8);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", element_type, 4);

  element_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v2i64", element_type, 2);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "lsxv");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "v4f32");
  tdesc_add_field (type_with_fields, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v2f64");
  tdesc_add_field (type_with_fields, "v2_double", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type_with_fields, "v16_int8", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type_with_fields, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type_with_fields, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v2i64");
  tdesc_add_field (type_with_fields, "v2_int64", field_type);
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type_with_fields, "uint128", field_type);

  tdesc_create_reg (feature, "vr0", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr1", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr2", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr3", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr4", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr5", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr6", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr7", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr8", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr9", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr10", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr11", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr12", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr13", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr14", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr15", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr16", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr17", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr18", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr19", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr20", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr21", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr22", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr23", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr24", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr25", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr26", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr27", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr28", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr29", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr30", regnum++, 1, "lsx", 128, "lsxv");
  tdesc_create_reg (feature, "vr31", regnum++, 1, "lsx", 128, "lsxv");
  return regnum;
}
