/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: lbt.xml */

#include "gdbsupport/tdesc.h"

static int
create_feature_loongarch_lbt (struct target_desc *result, long regnum)
{
  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.loongarch.lbt");
  tdesc_create_reg (feature, "scr0", regnum++, 1, "lbt", 64, "uint64");
  tdesc_create_reg (feature, "scr1", regnum++, 1, "lbt", 64, "uint64");
  tdesc_create_reg (feature, "scr2", regnum++, 1, "lbt", 64, "uint64");
  tdesc_create_reg (feature, "scr3", regnum++, 1, "lbt", 64, "uint64");
  tdesc_create_reg (feature, "eflags", regnum++, 1, "lbt", 32, "uint32");
  tdesc_create_reg (feature, "ftop", regnum++, 1, "lbt", 32, "uint32");
  return regnum;
}
