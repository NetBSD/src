/* THIS FILE IS GENERATED.  -*- buffer-read-only: t -*- vi:set ro:
  Original: powerpc-vsx32l.xml */

#include "defs.h"
#include "osabi.h"
#include "target-descriptions.h"

struct target_desc *tdesc_powerpc_vsx32l;
static void
initialize_tdesc_powerpc_vsx32l (void)
{
  struct target_desc *result = allocate_target_description ();
  set_tdesc_architecture (result, bfd_scan_arch ("powerpc:common"));

  struct tdesc_feature *feature;

  feature = tdesc_create_feature (result, "org.gnu.gdb.power.core");
  tdesc_create_reg (feature, "r0", 0, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r1", 1, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r2", 2, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r3", 3, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r4", 4, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r5", 5, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r6", 6, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r7", 7, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r8", 8, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r9", 9, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r10", 10, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r11", 11, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r12", 12, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r13", 13, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r14", 14, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r15", 15, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r16", 16, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r17", 17, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r18", 18, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r19", 19, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r20", 20, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r21", 21, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r22", 22, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r23", 23, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r24", 24, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r25", 25, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r26", 26, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r27", 27, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r28", 28, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r29", 29, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r30", 30, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "r31", 31, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "pc", 64, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "msr", 65, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "cr", 66, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "lr", 67, 1, NULL, 32, "code_ptr");
  tdesc_create_reg (feature, "ctr", 68, 1, NULL, 32, "uint32");
  tdesc_create_reg (feature, "xer", 69, 1, NULL, 32, "uint32");

  feature = tdesc_create_feature (result, "org.gnu.gdb.power.fpu");
  tdesc_create_reg (feature, "f0", 32, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f1", 33, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f2", 34, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f3", 35, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f4", 36, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f5", 37, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f6", 38, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f7", 39, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f8", 40, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f9", 41, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f10", 42, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f11", 43, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f12", 44, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f13", 45, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f14", 46, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f15", 47, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f16", 48, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f17", 49, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f18", 50, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f19", 51, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f20", 52, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f21", 53, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f22", 54, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f23", 55, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f24", 56, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f25", 57, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f26", 58, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f27", 59, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f28", 60, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f29", 61, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f30", 62, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "f31", 63, 1, NULL, 64, "ieee_double");
  tdesc_create_reg (feature, "fpscr", 70, 1, "float", 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.power.linux");
  tdesc_create_reg (feature, "orig_r3", 71, 1, NULL, 32, "int");
  tdesc_create_reg (feature, "trap", 72, 1, NULL, 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.power.altivec");
  tdesc_type *element_type;
  element_type = tdesc_named_type (feature, "ieee_single");
  tdesc_create_vector (feature, "v4f", element_type, 4);

  element_type = tdesc_named_type (feature, "int32");
  tdesc_create_vector (feature, "v4i32", element_type, 4);

  element_type = tdesc_named_type (feature, "int16");
  tdesc_create_vector (feature, "v8i16", element_type, 8);

  element_type = tdesc_named_type (feature, "int8");
  tdesc_create_vector (feature, "v16i8", element_type, 16);

  tdesc_type_with_fields *type_with_fields;
  type_with_fields = tdesc_create_union (feature, "vec128");
  tdesc_type *field_type;
  field_type = tdesc_named_type (feature, "uint128");
  tdesc_add_field (type_with_fields, "uint128", field_type);
  field_type = tdesc_named_type (feature, "v4f");
  tdesc_add_field (type_with_fields, "v4_float", field_type);
  field_type = tdesc_named_type (feature, "v4i32");
  tdesc_add_field (type_with_fields, "v4_int32", field_type);
  field_type = tdesc_named_type (feature, "v8i16");
  tdesc_add_field (type_with_fields, "v8_int16", field_type);
  field_type = tdesc_named_type (feature, "v16i8");
  tdesc_add_field (type_with_fields, "v16_int8", field_type);

  tdesc_create_reg (feature, "vr0", 73, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr1", 74, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr2", 75, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr3", 76, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr4", 77, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr5", 78, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr6", 79, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr7", 80, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr8", 81, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr9", 82, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr10", 83, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr11", 84, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr12", 85, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr13", 86, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr14", 87, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr15", 88, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr16", 89, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr17", 90, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr18", 91, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr19", 92, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr20", 93, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr21", 94, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr22", 95, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr23", 96, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr24", 97, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr25", 98, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr26", 99, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr27", 100, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr28", 101, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr29", 102, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr30", 103, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vr31", 104, 1, NULL, 128, "vec128");
  tdesc_create_reg (feature, "vscr", 105, 1, "vector", 32, "int");
  tdesc_create_reg (feature, "vrsave", 106, 1, "vector", 32, "int");

  feature = tdesc_create_feature (result, "org.gnu.gdb.power.vsx");
  tdesc_create_reg (feature, "vs0h", 107, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs1h", 108, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs2h", 109, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs3h", 110, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs4h", 111, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs5h", 112, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs6h", 113, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs7h", 114, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs8h", 115, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs9h", 116, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs10h", 117, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs11h", 118, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs12h", 119, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs13h", 120, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs14h", 121, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs15h", 122, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs16h", 123, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs17h", 124, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs18h", 125, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs19h", 126, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs20h", 127, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs21h", 128, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs22h", 129, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs23h", 130, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs24h", 131, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs25h", 132, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs26h", 133, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs27h", 134, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs28h", 135, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs29h", 136, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs30h", 137, 1, NULL, 64, "uint64");
  tdesc_create_reg (feature, "vs31h", 138, 1, NULL, 64, "uint64");

  tdesc_powerpc_vsx32l = result;
}
