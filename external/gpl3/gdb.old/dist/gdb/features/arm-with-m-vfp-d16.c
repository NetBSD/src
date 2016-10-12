/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-with-m-vfp-d16.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_arm_with_m_vfp_d16;
static void
initialize_tdesc_arm_with_m_vfp_d16 (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.m-profile");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "sp", 13, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "lr", 14, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "pc", 15, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "xpsr", 25, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.arm.vfp");
  tdesc_create_reg (feature, "d0", 26, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d1", 27, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d2", 28, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d3", 29, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d4", 30, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d5", 31, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d6", 32, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d7", 33, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d8", 34, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d9", 35, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d10", 36, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d11", 37, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d12", 38, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d13", 39, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d14", 40, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "d15", 41, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fpscr", 42, 1, "float", 32, "int");

  tdesc_arm_with_m_vfp_d16 = result;
}
