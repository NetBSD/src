/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-gcs.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_aarch64_gcs (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.gcs");
  tdesc_create_reg (feature, "gcspr", regnum++, 1, "system", 64, "data_ptr");
  return regnum;
}
