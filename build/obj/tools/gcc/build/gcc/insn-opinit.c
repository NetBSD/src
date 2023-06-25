/* Generated automatically by the program `genopinit'
   from the machine description file `md'.  */

#define IN_TARGET_CODE 1
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "predict.h"
#include "tree.h"
#include "rtl.h"
#include "alias.h"
#include "varasm.h"
#include "stor-layout.h"
#include "calls.h"
#include "memmodel.h"
#include "tm_p.h"
#include "flags.h"
#include "insn-config.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "emit-rtl.h"
#include "stmt.h"
#include "expr.h"
#include "insn-codes.h"
#include "optabs.h"

struct optab_pat {
  unsigned scode;
  enum insn_code icode;
};

static const struct optab_pat pats[NUM_OPTAB_PATTERNS] = {
  { 0x010405, CODE_FOR_extendqihi2 },
  { 0x010406, CODE_FOR_extendqisi2 },
  { 0x010407, CODE_FOR_extendqidi2 },
  { 0x010506, CODE_FOR_extendhisi2 },
  { 0x010507, CODE_FOR_extendhidi2 },
  { 0x010607, CODE_FOR_extendsidi2 },
  { 0x011b1c, CODE_FOR_extendsfdf2 },
  { 0x011b1d, CODE_FOR_extendsfxf2 },
  { 0x011c1d, CODE_FOR_extenddfxf2 },
  { 0x020504, CODE_FOR_trunchiqi2 },
  { 0x020604, CODE_FOR_truncsiqi2 },
  { 0x020605, CODE_FOR_truncsihi2 },
  { 0x021c1b, CODE_FOR_truncdfsf2 },
  { 0x021d1b, CODE_FOR_truncxfsf2 },
  { 0x021d1c, CODE_FOR_truncxfdf2 },
  { 0x030405, CODE_FOR_zero_extendqihi2 },
  { 0x030406, CODE_FOR_zero_extendqisi2 },
  { 0x030407, CODE_FOR_zero_extendqidi2 },
  { 0x030506, CODE_FOR_zero_extendhisi2 },
  { 0x030507, CODE_FOR_zero_extendhidi2 },
  { 0x030607, CODE_FOR_zero_extendsidi2 },
  { 0x041b04, CODE_FOR_fixsfqi2 },
  { 0x041b05, CODE_FOR_fixsfhi2 },
  { 0x041b06, CODE_FOR_fixsfsi2 },
  { 0x041c04, CODE_FOR_fixdfqi2 },
  { 0x041c05, CODE_FOR_fixdfhi2 },
  { 0x041c06, CODE_FOR_fixdfsi2 },
  { 0x041d04, CODE_FOR_fixxfqi2 },
  { 0x041d05, CODE_FOR_fixxfhi2 },
  { 0x041d06, CODE_FOR_fixxfsi2 },
  { 0x06041b, CODE_FOR_floatqisf2 },
  { 0x06041c, CODE_FOR_floatqidf2 },
  { 0x06041d, CODE_FOR_floatqixf2 },
  { 0x06051b, CODE_FOR_floathisf2 },
  { 0x06051c, CODE_FOR_floathidf2 },
  { 0x06051d, CODE_FOR_floathixf2 },
  { 0x06061b, CODE_FOR_floatsisf2 },
  { 0x06061c, CODE_FOR_floatsidf2 },
  { 0x06061d, CODE_FOR_floatsixf2 },
  { 0x101c04, CODE_FOR_fix_truncdfqi2 },
  { 0x101c05, CODE_FOR_fix_truncdfhi2 },
  { 0x101c06, CODE_FOR_fix_truncdfsi2 },
  { 0x120506, CODE_FOR_mulhisi3 },
  { 0x120607, CODE_FOR_mulsidi3 },
  { 0x130506, CODE_FOR_umulhisi3 },
  { 0x130607, CODE_FOR_umulsidi3 },
  { 0x310004, CODE_FOR_addqi3 },
  { 0x310005, CODE_FOR_addhi3 },
  { 0x310006, CODE_FOR_addsi3 },
  { 0x310007, CODE_FOR_adddi3 },
  { 0x31001b, CODE_FOR_addsf3 },
  { 0x31001c, CODE_FOR_adddf3 },
  { 0x31001d, CODE_FOR_addxf3 },
  { 0x350004, CODE_FOR_subqi3 },
  { 0x350005, CODE_FOR_subhi3 },
  { 0x350006, CODE_FOR_subsi3 },
  { 0x350007, CODE_FOR_subdi3 },
  { 0x35001b, CODE_FOR_subsf3 },
  { 0x35001c, CODE_FOR_subdf3 },
  { 0x35001d, CODE_FOR_subxf3 },
  { 0x390005, CODE_FOR_mulhi3 },
  { 0x390006, CODE_FOR_mulsi3 },
  { 0x39001b, CODE_FOR_mulsf3 },
  { 0x39001c, CODE_FOR_muldf3 },
  { 0x39001d, CODE_FOR_mulxf3 },
  { 0x3d001b, CODE_FOR_divsf3 },
  { 0x3d001c, CODE_FOR_divdf3 },
  { 0x3d001d, CODE_FOR_divxf3 },
  { 0x420005, CODE_FOR_divmodhi4 },
  { 0x420006, CODE_FOR_divmodsi4 },
  { 0x430005, CODE_FOR_udivmodhi4 },
  { 0x430006, CODE_FOR_udivmodsi4 },
  { 0x46001b, CODE_FOR_ftruncsf2 },
  { 0x46001c, CODE_FOR_ftruncdf2 },
  { 0x46001d, CODE_FOR_ftruncxf2 },
  { 0x470004, CODE_FOR_andqi3 },
  { 0x470005, CODE_FOR_andhi3 },
  { 0x470006, CODE_FOR_andsi3 },
  { 0x480004, CODE_FOR_iorqi3 },
  { 0x480005, CODE_FOR_iorhi3 },
  { 0x480006, CODE_FOR_iorsi3 },
  { 0x490004, CODE_FOR_xorqi3 },
  { 0x490005, CODE_FOR_xorhi3 },
  { 0x490006, CODE_FOR_xorsi3 },
  { 0x4a0004, CODE_FOR_ashlqi3 },
  { 0x4a0005, CODE_FOR_ashlhi3 },
  { 0x4a0006, CODE_FOR_ashlsi3 },
  { 0x4a0007, CODE_FOR_ashldi3 },
  { 0x4d0004, CODE_FOR_ashrqi3 },
  { 0x4d0005, CODE_FOR_ashrhi3 },
  { 0x4d0006, CODE_FOR_ashrsi3 },
  { 0x4d0007, CODE_FOR_ashrdi3 },
  { 0x4e0004, CODE_FOR_lshrqi3 },
  { 0x4e0005, CODE_FOR_lshrhi3 },
  { 0x4e0006, CODE_FOR_lshrsi3 },
  { 0x4e0007, CODE_FOR_lshrdi3 },
  { 0x4f0004, CODE_FOR_rotlqi3 },
  { 0x4f0005, CODE_FOR_rotlhi3 },
  { 0x4f0006, CODE_FOR_rotlsi3 },
  { 0x500004, CODE_FOR_rotrqi3 },
  { 0x500005, CODE_FOR_rotrhi3 },
  { 0x500006, CODE_FOR_rotrsi3 },
  { 0x5a0004, CODE_FOR_negqi2 },
  { 0x5a0005, CODE_FOR_neghi2 },
  { 0x5a0006, CODE_FOR_negsi2 },
  { 0x5a0007, CODE_FOR_negdi2 },
  { 0x5a001b, CODE_FOR_negsf2 },
  { 0x5a001c, CODE_FOR_negdf2 },
  { 0x5a001d, CODE_FOR_negxf2 },
  { 0x5e001b, CODE_FOR_abssf2 },
  { 0x5e001c, CODE_FOR_absdf2 },
  { 0x5e001d, CODE_FOR_absxf2 },
  { 0x600004, CODE_FOR_one_cmplqi2 },
  { 0x600005, CODE_FOR_one_cmplhi2 },
  { 0x600006, CODE_FOR_one_cmplsi2 },
  { 0x610006, CODE_FOR_bswapsi2 },
  { 0x630006, CODE_FOR_clzsi2 },
  { 0x72001b, CODE_FOR_sqrtsf2 },
  { 0x72001c, CODE_FOR_sqrtdf2 },
  { 0x72001d, CODE_FOR_sqrtxf2 },
  { 0x810004, CODE_FOR_movqi },
  { 0x810005, CODE_FOR_movhi },
  { 0x810006, CODE_FOR_movsi },
  { 0x810007, CODE_FOR_movdi },
  { 0x81001b, CODE_FOR_movsf },
  { 0x81001c, CODE_FOR_movdf },
  { 0x81001d, CODE_FOR_movxf },
  { 0x820004, CODE_FOR_movstrictqi },
  { 0x820005, CODE_FOR_movstricthi },
  { 0x8b0004, CODE_FOR_pushqi1 },
  { 0x8c001b, CODE_FOR_reload_insf },
  { 0x8c001c, CODE_FOR_reload_indf },
  { 0x8d001b, CODE_FOR_reload_outsf },
  { 0x8d001c, CODE_FOR_reload_outdf },
  { 0x8e0004, CODE_FOR_cbranchqi4 },
  { 0x8e0005, CODE_FOR_cbranchhi4 },
  { 0x8e0006, CODE_FOR_cbranchsi4 },
  { 0x8e0007, CODE_FOR_cbranchdi4 },
  { 0x8e001b, CODE_FOR_cbranchsf4 },
  { 0x8e001c, CODE_FOR_cbranchdf4 },
  { 0x8e001d, CODE_FOR_cbranchxf4 },
  { 0xa90004, CODE_FOR_cstoreqi4 },
  { 0xa90005, CODE_FOR_cstorehi4 },
  { 0xa90006, CODE_FOR_cstoresi4 },
  { 0xa90007, CODE_FOR_cstoredi4 },
  { 0xa9001b, CODE_FOR_cstoresf4 },
  { 0xa9001c, CODE_FOR_cstoredf4 },
  { 0xa9001d, CODE_FOR_cstorexf4 },
  { 0xaa0004, CODE_FOR_ctrapqi4 },
  { 0xaa0005, CODE_FOR_ctraphi4 },
  { 0xaa0006, CODE_FOR_ctrapsi4 },
  { 0xb30006, CODE_FOR_smulsi3_highpart },
  { 0xb40006, CODE_FOR_umulsi3_highpart },
  { 0xd0001b, CODE_FOR_cossf2 },
  { 0xd0001c, CODE_FOR_cosdf2 },
  { 0xd0001d, CODE_FOR_cosxf2 },
  { 0xe6001b, CODE_FOR_sinsf2 },
  { 0xe6001c, CODE_FOR_sindf2 },
  { 0xe6001d, CODE_FOR_sinxf2 },
  { 0x13d0004, CODE_FOR_atomic_compare_and_swapqi },
  { 0x13d0005, CODE_FOR_atomic_compare_and_swaphi },
  { 0x13d0006, CODE_FOR_atomic_compare_and_swapsi },
};

void
init_all_optabs (struct target_optabs *optabs)
{
  bool *ena = optabs->pat_enable;
  ena[0] = HAVE_extendqihi2;
  ena[1] = HAVE_extendqisi2;
  ena[2] = HAVE_extendqidi2;
  ena[3] = HAVE_extendhisi2;
  ena[4] = HAVE_extendhidi2;
  ena[5] = HAVE_extendsidi2;
  ena[6] = HAVE_extendsfdf2;
  ena[7] = HAVE_extendsfxf2;
  ena[8] = HAVE_extenddfxf2;
  ena[9] = HAVE_trunchiqi2;
  ena[10] = HAVE_truncsiqi2;
  ena[11] = HAVE_truncsihi2;
  ena[12] = HAVE_truncdfsf2;
  ena[13] = HAVE_truncxfsf2;
  ena[14] = HAVE_truncxfdf2;
  ena[15] = HAVE_zero_extendqihi2;
  ena[16] = HAVE_zero_extendqisi2;
  ena[17] = HAVE_zero_extendqidi2;
  ena[18] = HAVE_zero_extendhisi2;
  ena[19] = HAVE_zero_extendhidi2;
  ena[20] = HAVE_zero_extendsidi2;
  ena[21] = HAVE_fixsfqi2;
  ena[22] = HAVE_fixsfhi2;
  ena[23] = HAVE_fixsfsi2;
  ena[24] = HAVE_fixdfqi2;
  ena[25] = HAVE_fixdfhi2;
  ena[26] = HAVE_fixdfsi2;
  ena[27] = HAVE_fixxfqi2;
  ena[28] = HAVE_fixxfhi2;
  ena[29] = HAVE_fixxfsi2;
  ena[30] = HAVE_floatqisf2;
  ena[31] = HAVE_floatqidf2;
  ena[32] = HAVE_floatqixf2;
  ena[33] = HAVE_floathisf2;
  ena[34] = HAVE_floathidf2;
  ena[35] = HAVE_floathixf2;
  ena[36] = HAVE_floatsisf2;
  ena[37] = HAVE_floatsidf2;
  ena[38] = HAVE_floatsixf2;
  ena[39] = HAVE_fix_truncdfqi2;
  ena[40] = HAVE_fix_truncdfhi2;
  ena[41] = HAVE_fix_truncdfsi2;
  ena[42] = HAVE_mulhisi3;
  ena[43] = HAVE_mulsidi3;
  ena[44] = HAVE_umulhisi3;
  ena[45] = HAVE_umulsidi3;
  ena[46] = HAVE_addqi3;
  ena[47] = HAVE_addhi3;
  ena[48] = HAVE_addsi3;
  ena[49] = HAVE_adddi3;
  ena[50] = HAVE_addsf3;
  ena[51] = HAVE_adddf3;
  ena[52] = HAVE_addxf3;
  ena[53] = HAVE_subqi3;
  ena[54] = HAVE_subhi3;
  ena[55] = HAVE_subsi3;
  ena[56] = HAVE_subdi3;
  ena[57] = HAVE_subsf3;
  ena[58] = HAVE_subdf3;
  ena[59] = HAVE_subxf3;
  ena[60] = HAVE_mulhi3;
  ena[61] = HAVE_mulsi3;
  ena[62] = HAVE_mulsf3;
  ena[63] = HAVE_muldf3;
  ena[64] = HAVE_mulxf3;
  ena[65] = HAVE_divsf3;
  ena[66] = HAVE_divdf3;
  ena[67] = HAVE_divxf3;
  ena[68] = HAVE_divmodhi4;
  ena[69] = HAVE_divmodsi4;
  ena[70] = HAVE_udivmodhi4;
  ena[71] = HAVE_udivmodsi4;
  ena[72] = HAVE_ftruncsf2;
  ena[73] = HAVE_ftruncdf2;
  ena[74] = HAVE_ftruncxf2;
  ena[75] = HAVE_andqi3;
  ena[76] = HAVE_andhi3;
  ena[77] = HAVE_andsi3;
  ena[78] = HAVE_iorqi3;
  ena[79] = HAVE_iorhi3;
  ena[80] = HAVE_iorsi3;
  ena[81] = HAVE_xorqi3;
  ena[82] = HAVE_xorhi3;
  ena[83] = HAVE_xorsi3;
  ena[84] = HAVE_ashlqi3;
  ena[85] = HAVE_ashlhi3;
  ena[86] = HAVE_ashlsi3;
  ena[87] = HAVE_ashldi3;
  ena[88] = HAVE_ashrqi3;
  ena[89] = HAVE_ashrhi3;
  ena[90] = HAVE_ashrsi3;
  ena[91] = HAVE_ashrdi3;
  ena[92] = HAVE_lshrqi3;
  ena[93] = HAVE_lshrhi3;
  ena[94] = HAVE_lshrsi3;
  ena[95] = HAVE_lshrdi3;
  ena[96] = HAVE_rotlqi3;
  ena[97] = HAVE_rotlhi3;
  ena[98] = HAVE_rotlsi3;
  ena[99] = HAVE_rotrqi3;
  ena[100] = HAVE_rotrhi3;
  ena[101] = HAVE_rotrsi3;
  ena[102] = HAVE_negqi2;
  ena[103] = HAVE_neghi2;
  ena[104] = HAVE_negsi2;
  ena[105] = HAVE_negdi2;
  ena[106] = HAVE_negsf2;
  ena[107] = HAVE_negdf2;
  ena[108] = HAVE_negxf2;
  ena[109] = HAVE_abssf2;
  ena[110] = HAVE_absdf2;
  ena[111] = HAVE_absxf2;
  ena[112] = HAVE_one_cmplqi2;
  ena[113] = HAVE_one_cmplhi2;
  ena[114] = HAVE_one_cmplsi2;
  ena[115] = HAVE_bswapsi2;
  ena[116] = HAVE_clzsi2;
  ena[117] = HAVE_sqrtsf2;
  ena[118] = HAVE_sqrtdf2;
  ena[119] = HAVE_sqrtxf2;
  ena[120] = HAVE_movqi;
  ena[121] = HAVE_movhi;
  ena[122] = HAVE_movsi;
  ena[123] = HAVE_movdi;
  ena[124] = HAVE_movsf;
  ena[125] = HAVE_movdf;
  ena[126] = HAVE_movxf;
  ena[127] = HAVE_movstrictqi;
  ena[128] = HAVE_movstricthi;
  ena[129] = HAVE_pushqi1;
  ena[130] = HAVE_reload_insf;
  ena[131] = HAVE_reload_indf;
  ena[132] = HAVE_reload_outsf;
  ena[133] = HAVE_reload_outdf;
  ena[134] = HAVE_cbranchqi4;
  ena[135] = HAVE_cbranchhi4;
  ena[136] = HAVE_cbranchsi4;
  ena[137] = HAVE_cbranchdi4;
  ena[138] = HAVE_cbranchsf4;
  ena[139] = HAVE_cbranchdf4;
  ena[140] = HAVE_cbranchxf4;
  ena[141] = HAVE_cstoreqi4;
  ena[142] = HAVE_cstorehi4;
  ena[143] = HAVE_cstoresi4;
  ena[144] = HAVE_cstoredi4;
  ena[145] = HAVE_cstoresf4;
  ena[146] = HAVE_cstoredf4;
  ena[147] = HAVE_cstorexf4;
  ena[148] = HAVE_ctrapqi4;
  ena[149] = HAVE_ctraphi4;
  ena[150] = HAVE_ctrapsi4;
  ena[151] = HAVE_smulsi3_highpart;
  ena[152] = HAVE_umulsi3_highpart;
  ena[153] = HAVE_cossf2;
  ena[154] = HAVE_cosdf2;
  ena[155] = HAVE_cosxf2;
  ena[156] = HAVE_sinsf2;
  ena[157] = HAVE_sindf2;
  ena[158] = HAVE_sinxf2;
  ena[159] = HAVE_atomic_compare_and_swapqi;
  ena[160] = HAVE_atomic_compare_and_swaphi;
  ena[161] = HAVE_atomic_compare_and_swapsi;
}

static int
lookup_handler (unsigned scode)
{
  int l = 0, h = ARRAY_SIZE (pats), m;
  while (h > l)
    {
      m = (h + l) / 2;
      if (scode == pats[m].scode)
        return m;
      else if (scode < pats[m].scode)
        h = m;
      else
        l = m + 1;
    }
  return -1;
}

enum insn_code
raw_optab_handler (unsigned scode)
{
  int i = lookup_handler (scode);
  return (i >= 0 && this_fn_optabs->pat_enable[i]
          ? pats[i].icode : CODE_FOR_nothing);
}

bool
swap_optab_enable (optab op, machine_mode m, bool set)
{
  unsigned scode = (op << 16) | m;
  int i = lookup_handler (scode);
  if (i >= 0)
    {
      bool ret = this_fn_optabs->pat_enable[i];
      this_fn_optabs->pat_enable[i] = set;
      return ret;
    }
  else
    {
      gcc_assert (!set);
      return false;
    }
}

const struct convert_optab_libcall_d convlib_def[NUM_CONVLIB_OPTABS] = {
  { "extend", gen_extend_conv_libfunc },
  { "trunc", gen_trunc_conv_libfunc },
  { NULL, NULL },
  { "fix", gen_fp_to_int_conv_libfunc },
  { "fixuns", gen_fp_to_int_conv_libfunc },
  { "float", gen_int_to_fp_conv_libfunc },
  { NULL, gen_ufloat_conv_libfunc },
  { "lrint", gen_int_to_fp_nondecimal_conv_libfunc },
  { "lround", gen_int_to_fp_nondecimal_conv_libfunc },
  { "lfloor", gen_int_to_fp_nondecimal_conv_libfunc },
  { "lceil", gen_int_to_fp_nondecimal_conv_libfunc },
  { "fract", gen_fract_conv_libfunc },
  { "fractuns", gen_fractuns_conv_libfunc },
  { "satfract", gen_satfract_conv_libfunc },
  { "satfractuns", gen_satfractuns_conv_libfunc },
};

const struct optab_libcall_d normlib_def[NUM_NORMLIB_OPTABS] = {
  { '3', "add", gen_int_fp_fixed_libfunc },
  { '3', "add", gen_intv_fp_libfunc },
  { '3', "ssadd", gen_signed_fixed_libfunc },
  { '3', "usadd", gen_unsigned_fixed_libfunc },
  { '3', "sub", gen_int_fp_fixed_libfunc },
  { '3', "sub", gen_intv_fp_libfunc },
  { '3', "sssub", gen_signed_fixed_libfunc },
  { '3', "ussub", gen_unsigned_fixed_libfunc },
  { '3', "mul", gen_int_fp_fixed_libfunc },
  { '3', "mul", gen_intv_fp_libfunc },
  { '3', "ssmul", gen_signed_fixed_libfunc },
  { '3', "usmul", gen_unsigned_fixed_libfunc },
  { '3', "div", gen_int_fp_signed_fixed_libfunc },
  { '3', "divv", gen_int_libfunc },
  { '3', "ssdiv", gen_signed_fixed_libfunc },
  { '3', "udiv", gen_int_unsigned_fixed_libfunc },
  { '3', "usdiv", gen_unsigned_fixed_libfunc },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '3', "mod", gen_int_libfunc },
  { '3', "umod", gen_int_libfunc },
  { '2', "ftrunc", gen_fp_libfunc },
  { '3', "and", gen_int_libfunc },
  { '3', "ior", gen_int_libfunc },
  { '3', "xor", gen_int_libfunc },
  { '3', "ashl", gen_int_fixed_libfunc },
  { '3', "ssashl", gen_signed_fixed_libfunc },
  { '3', "usashl", gen_unsigned_fixed_libfunc },
  { '3', "ashr", gen_int_signed_fixed_libfunc },
  { '3', "lshr", gen_int_unsigned_fixed_libfunc },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '3', "min", gen_int_fp_libfunc },
  { '3', "max", gen_int_fp_libfunc },
  { '3', "umin", gen_int_libfunc },
  { '3', "umax", gen_int_libfunc },
  { '2', "neg", gen_int_fp_fixed_libfunc },
  { '2', "neg", gen_intv_fp_libfunc },
  { '2', "ssneg", gen_signed_fixed_libfunc },
  { '2', "usneg", gen_unsigned_fixed_libfunc },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '2', "one_cmpl", gen_int_libfunc },
  { '\0', NULL, NULL },
  { '2', "ffs", gen_int_libfunc },
  { '2', "clz", gen_int_libfunc },
  { '2', "ctz", gen_int_libfunc },
  { '2', "clrsb", gen_int_libfunc },
  { '2', "popcount", gen_int_libfunc },
  { '2', "parity", gen_int_libfunc },
  { '2', "cmp", gen_int_fp_fixed_libfunc },
  { '2', "ucmp", gen_int_libfunc },
  { '2', "eq", gen_fp_libfunc },
  { '2', "ne", gen_fp_libfunc },
  { '2', "gt", gen_fp_libfunc },
  { '2', "ge", gen_fp_libfunc },
  { '2', "lt", gen_fp_libfunc },
  { '2', "le", gen_fp_libfunc },
  { '2', "unord", gen_fp_libfunc },
  { '2', "powi", gen_fp_libfunc },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
  { '\0', NULL, NULL },
};

enum rtx_code const optab_to_code_[NUM_OPTABS] = {
  UNKNOWN,
  SIGN_EXTEND,
  TRUNCATE,
  ZERO_EXTEND,
  FIX,
  UNSIGNED_FIX,
  FLOAT,
  UNSIGNED_FLOAT,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  FRACT_CONVERT,
  UNSIGNED_FRACT_CONVERT,
  SAT_FRACT,
  UNSIGNED_SAT_FRACT,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  PLUS,
  PLUS,
  SS_PLUS,
  US_PLUS,
  MINUS,
  MINUS,
  SS_MINUS,
  US_MINUS,
  MULT,
  MULT,
  SS_MULT,
  US_MULT,
  DIV,
  DIV,
  SS_DIV,
  UDIV,
  US_DIV,
  UNKNOWN,
  UNKNOWN,
  MOD,
  UMOD,
  UNKNOWN,
  AND,
  IOR,
  XOR,
  ASHIFT,
  SS_ASHIFT,
  US_ASHIFT,
  ASHIFTRT,
  LSHIFTRT,
  ROTATE,
  ROTATERT,
  ASHIFT,
  ASHIFTRT,
  LSHIFTRT,
  ROTATE,
  ROTATERT,
  SMIN,
  SMAX,
  UMIN,
  UMAX,
  NEG,
  NEG,
  SS_NEG,
  US_NEG,
  ABS,
  ABS,
  NOT,
  BSWAP,
  FFS,
  CLZ,
  CTZ,
  CLRSB,
  POPCOUNT,
  PARITY,
  UNKNOWN,
  UNKNOWN,
  EQ,
  NE,
  GT,
  GE,
  LT,
  LE,
  UNORDERED,
  UNKNOWN,
  SQRT,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  SET,
  STRICT_LOW_PART,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  COMPARE,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  FMA,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  UNKNOWN,
  VEC_DUPLICATE,
  VEC_SERIES,
  UNKNOWN,
};

const optab code_to_optab_[NUM_RTX_CODE] = {
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  mov_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  movstrict_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  cbranch_optab,
  add_optab,
  sub_optab,
  neg_optab,
  smul_optab,
  ssmul_optab,
  usmul_optab,
  sdiv_optab,
  ssdiv_optab,
  usdiv_optab,
  smod_optab,
  udiv_optab,
  umod_optab,
  and_optab,
  ior_optab,
  xor_optab,
  one_cmpl_optab,
  ashl_optab,
  rotl_optab,
  ashr_optab,
  lshr_optab,
  rotr_optab,
  smin_optab,
  smax_optab,
  umin_optab,
  umax_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  ne_optab,
  eq_optab,
  ge_optab,
  gt_optab,
  le_optab,
  lt_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unord_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  abs_optab,
  sqrt_optab,
  bswap_optab,
  ffs_optab,
  clrsb_optab,
  clz_optab,
  ctz_optab,
  popcount_optab,
  parity_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  vec_duplicate_optab,
  vec_series_optab,
  ssadd_optab,
  usadd_optab,
  sssub_optab,
  ssneg_optab,
  usneg_optab,
  unknown_optab,
  ssashl_optab,
  usashl_optab,
  ussub_optab,
  unknown_optab,
  unknown_optab,
  fma_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
  unknown_optab,
};

