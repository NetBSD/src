/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: lasx.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_loongarch_lasx (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.loongarch.lasx");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v8f32", element_type, 8);

  element_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v4f64", element_type, 4);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v32i8", element_type, 32);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v16i16", element_type, 16);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v8i32", element_type, 8);

  element_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v4i64", element_type, 4);

  element_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "v2ui128", element_type, 2);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "lasxv");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "v8f32");
  tdesc_add_field (type_with_fields, "v8_float", field_type);
  field_type = tdesc_named_type (feature, "v4f64");
  tdesc_add_field (type_with_fields, "v4_double", field_type);
  field_type = tdesc_named_type (feature, "v32i8");
  tdesc_add_field (type_with_fields, "v32_int8", field_type);
  field_type = tdesc_named_type (feature, "v16i16");
  tdesc_add_field (type_with_fields, "v16_int16", field_type);
  field_type = tdesc_named_type (feature, "v8i32");
  tdesc_add_field (type_with_fields, "v8_int32", field_type);
  field_type = tdesc_named_type (feature, "v4i64");
  tdesc_add_field (type_with_fields, "v4_int64", field_type);
  field_type = tdesc_named_type (feature, "v2ui128");
  tdesc_add_field (type_with_fields, "v2_uint128", field_type);

  tdesc_create_reg (feature, "xr0", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr1", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr2", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr3", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr4", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr5", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr6", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr7", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr8", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr9", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr10", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr11", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr12", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr13", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr14", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr15", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr16", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr17", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr18", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr19", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr20", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr21", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr22", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr23", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr24", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr25", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr26", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr27", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr28", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr29", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr30", regnum++, 1, "lasx", 256, "lasxv");
  tdesc_create_reg (feature, "xr31", regnum++, 1, "lasx", 256, "lasxv");
  return regnum;
}
