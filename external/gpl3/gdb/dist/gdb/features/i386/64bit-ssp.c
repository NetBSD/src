/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: 64bit-ssp.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_i386_64bit_ssp (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.pl3_ssp");
  tdesc_create_reg (feature, "pl3_ssp", regnum++, 1, NULL, 64, "data_ptr");
  return regnum;
}
