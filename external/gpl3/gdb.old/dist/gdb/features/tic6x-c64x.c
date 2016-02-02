/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: tic6x-c64x.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_tic6x_c64x;
static void
initialize_tdesc_tic6x_c64x (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;

  set_tdesc_architecture (result, bfd_scan_arch ("tic6x"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.tic6x.core");
  tdesc_create_reg (feature, "A0", 0, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A1", 1, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A2", 2, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A3", 3, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A4", 4, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A5", 5, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A6", 6, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A7", 7, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A8", 8, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A9", 9, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A10", 10, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A11", 11, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A12", 12, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A13", 13, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A14", 14, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A15", 15, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B0", 16, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B1", 17, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B2", 18, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B3", 19, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B4", 20, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B5", 21, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B6", 22, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B7", 23, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B8", 24, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B9", 25, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B10", 26, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B11", 27, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B12", 28, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B13", 29, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B14", 30, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B15", 31, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "CSR", 32, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "PC", 33, 1, NULL, 32, "code_ptr");

  feature = tdesc_create_feature (result, "org.gnu.gdb.tic6x.gp");
  tdesc_create_reg (feature, "A16", 34, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A17", 35, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A18", 36, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A19", 37, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A20", 38, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A21", 39, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A22", 40, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A23", 41, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A24", 42, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A25", 43, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A26", 44, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A27", 45, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A28", 46, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A29", 47, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A30", 48, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "A31", 49, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B16", 50, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B17", 51, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B18", 52, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B19", 53, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B20", 54, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B21", 55, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B22", 56, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B23", 57, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B24", 58, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B25", 59, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B26", 60, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B27", 61, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B28", 62, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B29", 63, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B30", 64, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "B31", 65, 1, NULL, 32, "uint32");

  tdesc_tic6x_c64x = result;
}
