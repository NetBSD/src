/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: i386-avx-mpx.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_i386_avx_mpx;
static void
initialize_tdesc_i386_avx_mpx (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("i386"));

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

  tdesc_create_reg (feature, "xmm0", 32, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm1", 33, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm2", 34, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm3", 35, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm4", 36, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm5", 37, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm6", 38, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm7", 39, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "mxcsr", 40, 1, "vector", 32, "i386_mxcsr");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.avx");
  tdesc_create_reg (feature, "ymm0h", 41, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm1h", 42, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm2h", 43, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm3h", 44, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm4h", 45, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm5h", 46, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm6h", 47, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm7h", 48, 1, NULL, 128, "uint128");

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.mpx");
  type = tdesc_create_struct (feature, "br128");
  field_type = tdesc_named_type (feature, "uint64");
  tdesc_add_field (type, "lbound", field_type);
  field_type = tdesc_named_type (feature, "uint64");
  tdesc_add_field (type, "ubound_raw", field_type);

  type = tdesc_create_struct (feature, "_bndstatus");
  tdesc_set_struct_size (type, 8);
  tdesc_add_bitfield (type, "bde", 2, 31);
  tdesc_add_bitfield (type, "error", 0, 1);

  type = tdesc_create_union (feature, "status");
  field_type = tdesc_named_type (feature, "data_ptr");
  tdesc_add_field (type, "raw", field_type);
  field_type = tdesc_named_type (feature, "_bndstatus");
  tdesc_add_field (type, "status", field_type);

  type = tdesc_create_struct (feature, "_bndcfgu");
  tdesc_set_struct_size (type, 8);
  tdesc_add_bitfield (type, "base", 12, 31);
  tdesc_add_bitfield (type, "reserved", 2, 11);
  tdesc_add_bitfield (type, "preserved", 1, 1);
  tdesc_add_bitfield (type, "enabled", 0, 0);

  type = tdesc_create_union (feature, "cfgu");
  field_type = tdesc_named_type (feature, "data_ptr");
  tdesc_add_field (type, "raw", field_type);
  field_type = tdesc_named_type (feature, "_bndcfgu");
  tdesc_add_field (type, "config", field_type);

  tdesc_create_reg (feature, "bnd0raw", 49, 1, NULL, 128, "br128");
  tdesc_create_reg (feature, "bnd1raw", 50, 1, NULL, 128, "br128");
  tdesc_create_reg (feature, "bnd2raw", 51, 1, NULL, 128, "br128");
  tdesc_create_reg (feature, "bnd3raw", 52, 1, NULL, 128, "br128");
  tdesc_create_reg (feature, "bndcfgu", 53, 1, NULL, 64, "cfgu");
  tdesc_create_reg (feature, "bndstatus", 54, 1, NULL, 64, "status");

  tdesc_i386_avx_mpx = result;
}
