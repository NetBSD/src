/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-without-fpu.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_aarch64_without_fpu;
static void
initialize_tdesc_aarch64_without_fpu (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;

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

  tdesc_aarch64_without_fpu = result;
}
