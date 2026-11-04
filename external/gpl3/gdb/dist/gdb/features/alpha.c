/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: alpha.xml */

#include "osabi.h"
#include "target-descriptions.h"

const struct target_desc *tdesc_alpha;
static void
initialize_tdesc_alpha (void)
{
  target_desc_up result = allocate_target_description ();
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result.get (), "org.gnu.gdb.alpha.core");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_enum (feature, "dyn_rm_enum", 8);
  tdesc_add_enum_value (type_with_fields, 0, "chop");
  tdesc_add_enum_value (type_with_fields, 1, "-inf");
  tdesc_add_enum_value (type_with_fields, 2, "norm");
  tdesc_add_enum_value (type_with_fields, 3, "+inf");

  type_with_fields = tdesc_create_flags (feature, "fpcr_flags", 8);
  tdesc_add_flag (type_with_fields, 47, "DNOD");
  tdesc_add_flag (type_with_fields, 48, "DNZ");
  tdesc_add_flag (type_with_fields, 49, "INVD");
  tdesc_add_flag (type_with_fields, 50, "DZED");
  tdesc_add_flag (type_with_fields, 51, "OVFD");
  tdesc_add_flag (type_with_fields, 52, "INV");
  tdesc_add_flag (type_with_fields, 53, "DZE");
  tdesc_add_flag (type_with_fields, 54, "OVF");
  tdesc_add_flag (type_with_fields, 55, "UNF");
  tdesc_add_flag (type_with_fields, 56, "INE");
  tdesc_add_flag (type_with_fields, 57, "IOV");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "dyn_rm_enum");
  tdesc_add_typed_bitfield (type_with_fields, "DYN_RM", 58, 59, field_type);
  tdesc_add_flag (type_with_fields, 60, "UNDZ");
  tdesc_add_flag (type_with_fields, 61, "UNFD");
  tdesc_add_flag (type_with_fields, 62, "INED");
  tdesc_add_flag (type_with_fields, 63, "SUM");

  tdesc_create_reg (feature, "v0", 0, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t0", 1, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t1", 2, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t2", 3, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t3", 4, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t4", 5, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t5", 6, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t6", 7, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t7", 8, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s0", 9, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s1", 10, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s2", 11, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s3", 12, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s4", 13, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "s5", 14, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "fp", 15, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a0", 16, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a1", 17, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a2", 18, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a3", 19, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a4", 20, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "a5", 21, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t8", 22, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t9", 23, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t10", 24, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t11", 25, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "ra", 26, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "t12", 27, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "at", 28, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "gp", 29, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "sp", 30, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "zero", 31, 0, NULL, 64, "int64");
  tdesc_create_reg (feature, "f0", 32, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f1", 33, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f2", 34, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f3", 35, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f4", 36, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f5", 37, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f6", 38, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f7", 39, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f8", 40, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f9", 41, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f10", 42, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f11", 43, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f12", 44, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f13", 45, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f14", 46, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f15", 47, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f16", 48, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f17", 49, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f18", 50, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f19", 51, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f20", 52, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f21", 53, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f22", 54, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f23", 55, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f24", 56, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f25", 57, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f26", 58, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f27", 59, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f28", 60, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f29", 61, 1, "float", 64, "float");
  tdesc_create_reg (feature, "f30", 62, 1, "float", 64, "float");
  tdesc_create_reg (feature, "fpcr", 63, 1, "float", 64, "fpcr_flags");
  tdesc_create_reg (feature, "pc", 64, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "", 65, 0, NULL, 64, "int64");
  tdesc_create_reg (feature, "unique", 66, 1, "system", 64, "int64");

  tdesc_alpha = result.release ();
}
