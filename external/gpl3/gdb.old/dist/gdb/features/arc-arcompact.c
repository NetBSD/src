/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: arc-arcompact.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_arc_arcompact;
static void
initialize_tdesc_arc_arcompact (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("ARC700"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.arc.core.arcompact");
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
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r16", 16, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r17", 17, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r18", 18, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r19", 19, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r20", 20, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r21", 21, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r22", 22, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r23", 23, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r24", 24, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "r25", 25, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "gp", 26, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "fp", 27, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "sp", 28, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "ilink1", 29, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "ilink2", 30, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "blink", 31, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "lp_count", 32, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "pcl", 33, 1, "", 32, "code_ptr");

  feature = tdesc_create_feature (result, "org.gnu.gdb.arc.aux-minimal");
  type = tdesc_create_flags (feature, "status32_type", 4);
  tdesc_add_flag (type, 0, "H");
  tdesc_add_bitfield (type, "E", 1, 2);
  tdesc_add_bitfield (type, "A", 3, 4);
  tdesc_add_flag (type, 5, "AE");
  tdesc_add_flag (type, 6, "DE");
  tdesc_add_flag (type, 7, "U");
  tdesc_add_flag (type, 8, "V");
  tdesc_add_flag (type, 9, "C");
  tdesc_add_flag (type, 10, "N");
  tdesc_add_flag (type, 11, "Z");
  tdesc_add_flag (type, 12, "L");
  tdesc_add_flag (type, 13, "R");
  tdesc_add_flag (type, 14, "SE");

  tdesc_create_reg (feature, "pc", 34, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "status32", 35, 1, NULL, 32, "status32_type");

  tdesc_arc_arcompact = result;
}
