/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: aarch64-gcs-linux.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_aarch64_gcs_linux (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.aarch64.gcs.linux");
  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_flags (feature, "features_flags", 8);
  tdesc_add_flag (type_with_fields, 0, "PR_SHADOW_STACK_ENABLE");
  tdesc_add_flag (type_with_fields, 1, "PR_SHADOW_STACK_WRITE");
  tdesc_add_flag (type_with_fields, 2, "PR_SHADOW_STACK_PUSH");

  tdesc_create_reg (feature, "gcs_features_enabled", regnum++, 1, "system", 64, "features_flags");
  tdesc_create_reg (feature, "gcs_features_locked", regnum++, 1, "system", 64, "features_flags");
  return regnum;
}
