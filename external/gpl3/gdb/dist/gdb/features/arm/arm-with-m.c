/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-with-m.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_arm_with_m;
static void
initialize_tdesc_arm_with_m (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;

  set_tdesc_architecture (result, bfd_scan_arch ("arm"));

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

  tdesc_arm_with_m = result;
}
