/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: x32-avx-linux.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_x32_avx_linux;
static void
initialize_tdesc_x32_avx_linux (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("i386:x64-32"));

  set_tdesc_osabi (result, osabi_from_tdesc_string ("GNU/Linux"));

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.core");
  type = tdesc_create_flags (feature, "i386_eflags", 4);
  tdesc_add_flag (type, 0, "CF");
  tdesc_add_flag (type, 1, "");
  tdesc_add_flag (type, 2, "PF");
  tdesc_add_flag (type, 4, "AF");
  tdesc_add_flag (type, 6, "ZF");
  tdesc_add_flag (type, 7, "SF");
  tdesc_add_flag (type, 8, "TF");
  tdesc_add_flag (type, 9, "IF");
  tdesc_add_flag (type, 10, "DF");
  tdesc_add_flag (type, 11, "OF");
  tdesc_add_flag (type, 14, "NT");
  tdesc_add_flag (type, 16, "RF");
  tdesc_add_flag (type, 17, "VM");
  tdesc_add_flag (type, 18, "AC");
  tdesc_add_flag (type, 19, "VIF");
  tdesc_add_flag (type, 20, "VIP");
  tdesc_add_flag (type, 21, "ID");

  tdesc_create_reg (feature, "rax", 0, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rbx", 1, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rcx", 2, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rdx", 3, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rsi", 4, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rdi", 5, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rbp", 6, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rsp", 7, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rip", 16, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "eflags", 17, 1, NULL, 32, "i386_eflags");
  tdesc_create_reg (feature, "cs", 18, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ss", 19, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "ds", 20, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "es", 21, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "fs", 22, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "gs", 23, 1, NULL, 32, "int32");
  tdesc_create_reg (feature, "st0", 24, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st1", 25, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st2", 26, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st3", 27, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st4", 28, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st5", 29, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st6", 30, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "st7", 31, 1, NULL, 80, "i387_ext");
  tdesc_create_reg (feature, "fctrl", 32, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fstat", 33, 1, "float", 32, "int");
  tdesc_create_reg (feature, "ftag", 34, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fiseg", 35, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fioff", 36, 1, "float", 32, "int");
  tdesc_create_reg (feature, "foseg", 37, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fooff", 38, 1, "float", 32, "int");
  tdesc_create_reg (feature, "fop", 39, 1, "float", 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.sse");
  field_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", field_type, 4);

  field_type = tdesc_named_type (feature, "ieee_double");
  tdesc_create_vector (feature, "v2d", field_type, 2);

  field_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", field_type, 16);

  field_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", field_type, 8);

  field_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", field_type, 4);

  field_type = tdesc_named_type (feature, "int64");
  tdesc_create_vector (feature, "v2i64", field_type, 2);

  type = tdesc_create_union (feature, "vec128");
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v2d");
  tdesc_add_field (type, "v2_double", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type, "v16_int8", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v2i64");
  tdesc_add_field (type, "v2_int64", field_type);
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type, "uint128", field_type);

  type = tdesc_create_flags (feature, "i386_mxcsr", 4);
  tdesc_add_flag (type, 0, "IE");
  tdesc_add_flag (type, 1, "DE");
  tdesc_add_flag (type, 2, "ZE");
  tdesc_add_flag (type, 3, "OE");
  tdesc_add_flag (type, 4, "UE");
  tdesc_add_flag (type, 5, "PE");
  tdesc_add_flag (type, 6, "DAZ");
  tdesc_add_flag (type, 7, "IM");
  tdesc_add_flag (type, 8, "DM");
  tdesc_add_flag (type, 9, "ZM");
  tdesc_add_flag (type, 10, "OM");
  tdesc_add_flag (type, 11, "UM");
  tdesc_add_flag (type, 12, "PM");
  tdesc_add_flag (type, 15, "FZ");

  tdesc_create_reg (feature, "xmm0", 40, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm1", 41, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm2", 42, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm3", 43, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm4", 44, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm5", 45, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm6", 46, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm7", 47, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm8", 48, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm9", 49, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm10", 50, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm11", 51, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm12", 52, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm13", 53, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm14", 54, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm15", 55, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "mxcsr", 56, 1, "vector", 32, "i386_mxcsr");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.linux");
  tdesc_create_reg (feature, "orig_rax", 57, 1, NULL, 64, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.segments");
  tdesc_create_reg (feature, "fs_base", 58, 1, NULL, 64, "int");
  tdesc_create_reg (feature, "gs_base", 59, 1, NULL, 64, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.avx");
  tdesc_create_reg (feature, "ymm0h", 60, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm1h", 61, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm2h", 62, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm3h", 63, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm4h", 64, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm5h", 65, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm6h", 66, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm7h", 67, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm8h", 68, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm9h", 69, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm10h", 70, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm11h", 71, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm12h", 72, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm13h", 73, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm14h", 74, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm15h", 75, 1, NULL, 128, "uint128");

  tdesc_x32_avx_linux = result;
}
