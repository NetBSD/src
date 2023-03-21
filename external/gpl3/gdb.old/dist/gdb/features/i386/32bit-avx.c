/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 32bit-avx.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_i386_32bit_avx (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.avx");
  tdesc_create_reg (feature, "ymm0h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm1h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm2h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm3h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm4h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm5h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm6h", regnum++, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm7h", regnum++, 1, NULL, 128, "uint128");
  return regnum;
}
