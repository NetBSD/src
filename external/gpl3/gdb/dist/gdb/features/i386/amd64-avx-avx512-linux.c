/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: amd64-avx-avx512-linux.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_amd64_avx_avx512_linux;
static void
initialize_tdesc_amd64_avx_avx512_linux (void)
{
  struct target_desc *result = allocate_target_description ();
  struct tdesc_feature *feature;
  struct tdesc_type *field_type;
  struct tdesc_type *type;

  set_tdesc_architecture (result, bfd_scan_arch ("i386:x86-64"));

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
  tdesc_create_reg (feature, "rbp", 6, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "rsp", 7, 1, NULL, 64, "data_ptr");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 64, "int64");
  tdesc_create_reg (feature, "rip", 16, 1, NULL, 64, "code_ptr");
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

  feature = tdesc_create_feature (result, "org.gnu.gdb.i386.avx512");
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

  field_type = tdesc_named_type (feature, "uint128");
  tdesc_create_vector (feature, "v2ui128", field_type, 2);

  tdesc_create_reg (feature, "xmm16", 76, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm17", 77, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm18", 78, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm19", 79, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm20", 80, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm21", 81, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm22", 82, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm23", 83, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm24", 84, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm25", 85, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm26", 86, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm27", 87, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm28", 88, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm29", 89, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm30", 90, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "xmm31", 91, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "ymm16h", 92, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm17h", 93, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm18h", 94, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm19h", 95, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm20h", 96, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm21h", 97, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm22h", 98, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm23h", 99, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm24h", 100, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm25h", 101, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm26h", 102, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm27h", 103, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm28h", 104, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm29h", 105, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm30h", 106, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "ymm31h", 107, 1, NULL, 128, "uint128");
  tdesc_create_reg (feature, "k0", 108, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k1", 109, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k2", 110, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k3", 111, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k4", 112, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k5", 113, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k6", 114, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "k7", 115, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "zmm0h", 116, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm1h", 117, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm2h", 118, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm3h", 119, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm4h", 120, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm5h", 121, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm6h", 122, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm7h", 123, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm8h", 124, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm9h", 125, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm10h", 126, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm11h", 127, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm12h", 128, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm13h", 129, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm14h", 130, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm15h", 131, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm16h", 132, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm17h", 133, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm18h", 134, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm19h", 135, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm20h", 136, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm21h", 137, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm22h", 138, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm23h", 139, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm24h", 140, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm25h", 141, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm26h", 142, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm27h", 143, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm28h", 144, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm29h", 145, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm30h", 146, 1, NULL, 256, "v2ui128");
  tdesc_create_reg (feature, "zmm31h", 146, 1, NULL, 256, "v2ui128");

  tdesc_amd64_avx_avx512_linux = result;
}
