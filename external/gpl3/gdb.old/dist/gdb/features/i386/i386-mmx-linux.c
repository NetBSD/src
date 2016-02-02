/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: i386-mmx-linux.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_i386_mmx_linux;
static void
initialize_tdesc_i386_mmx_linux (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;

  set_tdesc_architecture (result, bfd_scan_arch ("i386"));

  set_tdesc_osabi (result, osabi_from_tdesc_string ("GNU/Linux"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.core");
  field_type = tdesc_create_flags (feature, "i386_eflags", 4);
  tdesc_add_flag (field_type, 0, "CF");
  tdesc_add_flag (field_type, 1, "");
  tdesc_add_flag (field_type, 2, "PF");
  tdesc_add_flag (field_type, 4, "AF");
  tdesc_add_flag (field_type, 6, "ZF");
  tdesc_add_flag (field_type, 7, "SF");
  tdesc_add_flag (field_type, 8, "TF");
  tdesc_add_flag (field_type, 9, "IF");
  tdesc_add_flag (field_type, 10, "DF");
  tdesc_add_flag (field_type, 11, "OF");
  tdesc_add_flag (field_type, 14, "NT");
  tdesc_add_flag (field_type, 16, "RF");
  tdesc_add_flag (field_type, 17, "VM");
  tdesc_add_flag (field_type, 18, "AC");
  tdesc_add_flag (field_type, 19, "VIF");
  tdesc_add_flag (field_type, 20, "VIP");
  tdesc_add_flag (field_type, 21, "ID");

  tdesc_create_reg (feature, "eax", 0, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ecx", 1, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "edx", 2, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ebx", 3, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "esp", 4, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "ebp", 5, 1, NULL, 32, "data_ptr");
  tdesc_create_reg (feature, "esi", 6, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "edi", 7, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "eip", 8, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "eflags", 9, 1, NULL, 32, "i386_eflags");
  tdesc_create_reg (feature, "cs", 10, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ss", 11, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ds", 12, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "es", 13, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "fs", 14, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "gs", 15, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "st0", 16, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st1", 17, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st2", 18, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st3", 19, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st4", 20, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st5", 21, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st6", 22, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st7", 23, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "fctrl", 24, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fstat", 25, 1, "float", 32, "int");
  tdesc_create_reg (feature, "ftag", 26, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fiseg", 27, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fioff", 28, 1, "float", 32, "int");
  tdesc_create_reg (feature, "foseg", 29, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fooff", 30, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fop", 31, 1, "float", 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.linux");
  tdesc_create_reg (feature, "orig_eax", 41, 1, NULL, 32, "int");

  tdesc_i386_mmx_linux = result;
}
