/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-with-iwmmxt.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_arm_with_iwmmxt;
static void
initialize_tdesc_arm_with_iwmmxt (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("iwmmxt"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.core");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "sp", 13, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "lr", 14, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", 15, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "cpsr", 25, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.xscale.iwmmxt");
  field_type = tdesc_named_type (feature, "uint8");
  tdesc_create_vector (feature, "iwmmxt_v8u8", field_type, 8);

  field_type = tdesc_named_type (feature, "uint16");
  tdesc_create_vector (feature, "iwmmxt_v4u16", field_type, 4);

  field_type = tdesc_named_type (feature, "uint32");
  tdesc_create_vector (feature, "iwmmxt_v2u32", field_type, 2);

  type = tdesc_create_union (feature, "iwmmxt_vec64i");
  field_type = tdesc_named_type (feature, "iwmmxt_v8u8");
  tdesc_add_field (type, "u8", field_type);
  field_type = tdesc_named_type (feature, "iwmmxt_v4u16");
  tdesc_add_field (type, "u16", field_type);
  field_type = tdesc_named_type (feature, "iwmmxt_v2u32");
  tdesc_add_field (type, "u32", field_type);
  field_type = tdesc_named_type (feature, "uint64");
  tdesc_add_field (type, "u64", field_type);

  tdesc_create_reg (feature, "wR0", 26, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR1", 27, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR2", 28, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR3", 29, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR4", 30, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR5", 31, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR6", 32, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR7", 33, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR8", 34, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR9", 35, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR10", 36, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR11", 37, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR12", 38, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR13", 39, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR14", 40, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wR15", 41, 1, NULL, 64, "iwmmxt_vec64i");
  tdesc_create_reg (feature, "wCSSF", 42, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCASF", 43, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR0", 44, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR1", 45, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR2", 46, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "wCGR3", 47, 1, "vector", 32, "int");

  tdesc_arm_with_iwmmxt = result;
}
