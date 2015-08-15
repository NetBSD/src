/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arm-with-m-fpa-layout.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_arm_with_m_fpa_layout;
static void
initialize_tdesc_arm_with_m_fpa_layout (void)
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
  tdesc_create_reg (feature, "", 16, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 17, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 18, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 19, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 20, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 21, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 22, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 23, 1, NULL, 96, "arm_fpa_ext");
  tdesc_create_reg (feature, "", 24, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "xpsr", 25, 1, NULL, 32, "int");

  tdesc_arm_with_m_fpa_layout = result;
}
