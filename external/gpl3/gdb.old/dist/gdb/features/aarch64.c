/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_aarch64;
static void
initialize_tdesc_aarch64 (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("aarch64"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.core");
  tdesc_create_reg (feature, "x0", 0, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x1", 1, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x2", 2, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x3", 3, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x4", 4, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x5", 5, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x6", 6, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x7", 7, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x8", 8, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x9", 9, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x10", 10, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x11", 11, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x12", 12, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x13", 13, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x14", 14, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x15", 15, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x16", 16, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x17", 17, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x18", 18, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x19", 19, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x20", 20, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x21", 21, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x22", 22, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x23", 23, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x24", 24, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x25", 25, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x26", 26, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x27", 27, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x28", 28, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x29", 29, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "x30", 30, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "sp", 31, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "pc", 32, 1, NULL, 64, "code_ptr");
  tdesc_create_reg (feature, "cpsr", 33, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.fpu");
  field_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v2d", field_type, 2);

  field_type = tdesc_named_type (feature, "uint64");
  tdesc_create_vector (feature, "v2u", field_type, 2);

  field_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v2i", field_type, 2);

  field_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", field_type, 4);

  field_type = tdesc_named_type (feature, "uint32");
  tdesc_create_vector (feature, "v4u", field_type, 4);

  field_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i", field_type, 4);

  field_type = tdesc_named_type (feature, "uint16");
  tdesc_create_vector (feature, "v8u", field_type, 8);

  field_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i", field_type, 8);

  field_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "v16u", field_type, 16);

  field_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i", field_type, 16);

  field_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "v1u", field_type, 1);

  field_type = tdesc_named_type (feature, "int128");
  tdesc_create_vector (feature, "v1i", field_type, 1);

  type = tdesc_create_union (feature, "vnd");
  field_type = tdesc_named_type (feature, "v2d");
  tdesc_add_field (type, "f", field_type);
  field_type = tdesc_named_type (feature, "v2u");
  tdesc_add_field (type, "u", field_type);
  field_type = tdesc_named_type (feature, "v2i");
  tdesc_add_field (type, "s", field_type);

  type = tdesc_create_union (feature, "vns");
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type, "f", field_type);
  field_type = tdesc_named_type (feature, "v4u");
  tdesc_add_field (type, "u", field_type);
  field_type = tdesc_named_type (feature, "v4i");
  tdesc_add_field (type, "s", field_type);

  type = tdesc_create_union (feature, "vnh");
  field_type = tdesc_named_type (feature, "v8u");
  tdesc_add_field (type, "u", field_type);
  field_type = tdesc_named_type (feature, "v8i");
  tdesc_add_field (type, "s", field_type);

  type = tdesc_create_union (feature, "vnb");
  field_type = tdesc_named_type (feature, "v16u");
  tdesc_add_field (type, "u", field_type);
  field_type = tdesc_named_type (feature, "v16i");
  tdesc_add_field (type, "s", field_type);

  type = tdesc_create_union (feature, "vnq");
  field_type = tdesc_named_type (feature, "v1u");
  tdesc_add_field (type, "u", field_type);
  field_type = tdesc_named_type (feature, "v1i");
  tdesc_add_field (type, "s", field_type);

  type = tdesc_create_union (feature, "aarch64v");
  field_type = tdesc_named_type (feature, "vnd");
  tdesc_add_field (type, "d", field_type);
  field_type = tdesc_named_type (feature, "vns");
  tdesc_add_field (type, "s", field_type);
  field_type = tdesc_named_type (feature, "vnh");
  tdesc_add_field (type, "h", field_type);
  field_type = tdesc_named_type (feature, "vnb");
  tdesc_add_field (type, "b", field_type);
  field_type = tdesc_named_type (feature, "vnq");
  tdesc_add_field (type, "q", field_type);

  tdesc_create_reg (feature, "v0", 34, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v1", 35, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v2", 36, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v3", 37, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v4", 38, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v5", 39, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v6", 40, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v7", 41, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v8", 42, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v9", 43, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v10", 44, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v11", 45, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v12", 46, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v13", 47, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v14", 48, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v15", 49, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v16", 50, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v17", 51, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v18", 52, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v19", 53, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v20", 54, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v21", 55, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v22", 56, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v23", 57, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v24", 58, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v25", 59, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v26", 60, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v27", 61, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v28", 62, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v29", 63, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v30", 64, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "v31", 65, 1, NULL, 128, "aarch64v");
  tdesc_create_reg (feature, "fpsr", 66, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "fpcr", 67, 1, NULL, 32, "int");

  tdesc_aarch64 = result;
}
