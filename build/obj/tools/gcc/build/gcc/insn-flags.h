/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_pushdi 1
#define HAVE_beq0_di 1
#define HAVE_bne0_di 1
#define HAVE_cbranchdi4_insn 1
#define HAVE_cbranchqi4_insn (!TARGET_COLDFIRE)
#define HAVE_cbranchhi4_insn (!TARGET_COLDFIRE)
#define HAVE_cbranchsi4_insn (!TARGET_COLDFIRE)
#define HAVE_cbranchqi4_insn_rev (!TARGET_COLDFIRE)
#define HAVE_cbranchhi4_insn_rev (!TARGET_COLDFIRE)
#define HAVE_cbranchsi4_insn_rev (!TARGET_COLDFIRE)
#define HAVE_cbranchqi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cbranchhi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cbranchsi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cbranchqi4_insn_cf_rev (TARGET_COLDFIRE)
#define HAVE_cbranchhi4_insn_cf_rev (TARGET_COLDFIRE)
#define HAVE_cbranchsi4_insn_cf_rev (TARGET_COLDFIRE)
#define HAVE_cstoreqi4_insn (!TARGET_COLDFIRE)
#define HAVE_cstorehi4_insn (!TARGET_COLDFIRE)
#define HAVE_cstoresi4_insn (!TARGET_COLDFIRE)
#define HAVE_cstoreqi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cstorehi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cstoresi4_insn_cf (TARGET_COLDFIRE)
#define HAVE_cbranchsi4_btst_mem_insn 1
#define HAVE_cbranchsi4_btst_reg_insn (!(CONST_INT_P (operands[1]) && !IN_RANGE (INTVAL (operands[1]), 0, 31)))
#define HAVE_cbranchsi4_btst_mem_insn_1 (!TARGET_COLDFIRE && (unsigned) INTVAL (operands[2]) < 8)
#define HAVE_cbranchsi4_btst_reg_insn_1 (!(REG_P (operands[1]) && !IN_RANGE (INTVAL (operands[2]), 0, 31)))
#define HAVE_cbranch_bftstqi_insn (TARGET_68020 && TARGET_BITFIELD \
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3]) \
       || IN_RANGE (INTVAL (operands[3]), 0, 31)))
#define HAVE_cbranch_bftstsi_insn (TARGET_68020 && TARGET_BITFIELD \
   && (!REG_P (operands[1]) || !CONST_INT_P (operands[3]) \
       || IN_RANGE (INTVAL (operands[3]), 0, 31)))
#define HAVE_cstore_bftstqi_insn (TARGET_68020 && TARGET_BITFIELD \
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4]) \
       || IN_RANGE (INTVAL (operands[4]), 0, 31)))
#define HAVE_cstore_bftstsi_insn (TARGET_68020 && TARGET_BITFIELD \
   && (!REG_P (operands[2]) || !CONST_INT_P (operands[4]) \
       || IN_RANGE (INTVAL (operands[4]), 0, 31)))
#define HAVE_cbranchsf4_insn_68881 (TARGET_68881 \
   && (register_operand (operands[1], SFmode) \
       || register_operand (operands[2], SFmode) \
       || const0_operand (operands[2], SFmode)))
#define HAVE_cbranchdf4_insn_68881 (TARGET_68881 \
   && (register_operand (operands[1], DFmode) \
       || register_operand (operands[2], DFmode) \
       || const0_operand (operands[2], DFmode)))
#define HAVE_cbranchxf4_insn_68881 ((TARGET_68881 \
   && (register_operand (operands[1], XFmode) \
       || register_operand (operands[2], XFmode) \
       || const0_operand (operands[2], XFmode))) && (TARGET_68881))
#define HAVE_cbranchsf4_insn_cf (TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], SFmode) \
       || register_operand (operands[2], SFmode) \
       || const0_operand (operands[2], SFmode)))
#define HAVE_cbranchdf4_insn_cf (TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], DFmode) \
       || register_operand (operands[2], DFmode) \
       || const0_operand (operands[2], DFmode)))
#define HAVE_cbranchxf4_insn_cf ((TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], XFmode) \
       || register_operand (operands[2], XFmode) \
       || const0_operand (operands[2], XFmode))) && (TARGET_68881))
#define HAVE_cbranchsf4_insn_rev_68881 (TARGET_68881 \
   && (register_operand (operands[1], SFmode) \
       || register_operand (operands[2], SFmode) \
       || const0_operand (operands[2], SFmode)))
#define HAVE_cbranchdf4_insn_rev_68881 (TARGET_68881 \
   && (register_operand (operands[1], DFmode) \
       || register_operand (operands[2], DFmode) \
       || const0_operand (operands[2], DFmode)))
#define HAVE_cbranchxf4_insn_rev_68881 ((TARGET_68881 \
   && (register_operand (operands[1], XFmode) \
       || register_operand (operands[2], XFmode) \
       || const0_operand (operands[2], XFmode))) && (TARGET_68881))
#define HAVE_cbranchsf4_insn_rev_cf (TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], SFmode) \
       || register_operand (operands[2], SFmode) \
       || const0_operand (operands[2], SFmode)))
#define HAVE_cbranchdf4_insn_rev_cf (TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], DFmode) \
       || register_operand (operands[2], DFmode) \
       || const0_operand (operands[2], DFmode)))
#define HAVE_cbranchxf4_insn_rev_cf ((TARGET_COLDFIRE_FPU \
   && (register_operand (operands[1], XFmode) \
       || register_operand (operands[2], XFmode) \
       || const0_operand (operands[2], XFmode))) && (TARGET_68881))
#define HAVE_cstoresf4_insn_68881 (TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU) \
   && (register_operand (operands[2], SFmode) \
       || register_operand (operands[3], SFmode) \
       || const0_operand (operands[3], SFmode)))
#define HAVE_cstoredf4_insn_68881 (TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU) \
   && (register_operand (operands[2], DFmode) \
       || register_operand (operands[3], DFmode) \
       || const0_operand (operands[3], DFmode)))
#define HAVE_cstorexf4_insn_68881 ((TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU) \
   && (register_operand (operands[2], XFmode) \
       || register_operand (operands[3], XFmode) \
       || const0_operand (operands[3], XFmode))) && (TARGET_68881))
#define HAVE_cstoresf4_insn_cf (TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU)
#define HAVE_cstoredf4_insn_cf (TARGET_HARD_FLOAT && TARGET_COLDFIRE_FPU)
#define HAVE_pushexthisi_const (INTVAL (operands[1]) >= -0x8000 && INTVAL (operands[1]) < 0x8000)
#define HAVE_movsf_cf_soft (TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU)
#define HAVE_movsf_cf_hard (TARGET_COLDFIRE_FPU)
#define HAVE_movdf_cf_soft (TARGET_COLDFIRE && !TARGET_COLDFIRE_FPU)
#define HAVE_movdf_cf_hard (TARGET_COLDFIRE_FPU)
#define HAVE_pushasi 1
#define HAVE_truncsiqi2 (!TARGET_COLDFIRE)
#define HAVE_trunchiqi2 (!TARGET_COLDFIRE)
#define HAVE_truncsihi2 (!TARGET_COLDFIRE)
#define HAVE_zero_extendqidi2 1
#define HAVE_zero_extendhidi2 1
#define HAVE_zero_extendhisi2 1
#define HAVE_zero_extendqisi2 1
#define HAVE_extendqidi2 1
#define HAVE_extendhidi2 1
#define HAVE_extendsidi2 1
#define HAVE_extendplussidi 1
#define HAVE_extendqihi2 1
#define HAVE_extendsfdf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_truncdfsf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floatsisf2_68881 (TARGET_68881)
#define HAVE_floatsidf2_68881 (TARGET_68881)
#define HAVE_floatsixf2_68881 (TARGET_68881)
#define HAVE_floatsisf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floatsidf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floathisf2_68881 (TARGET_68881)
#define HAVE_floathidf2_68881 (TARGET_68881)
#define HAVE_floathixf2_68881 (TARGET_68881)
#define HAVE_floathisf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floathidf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floatqisf2_68881 (TARGET_68881)
#define HAVE_floatqidf2_68881 (TARGET_68881)
#define HAVE_floatqixf2_68881 (TARGET_68881)
#define HAVE_floatqisf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_floatqidf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fix_truncdfsi2 (TARGET_68881 && TUNE_68040)
#define HAVE_fix_truncdfhi2 (TARGET_68881 && TUNE_68040)
#define HAVE_fix_truncdfqi2 (TARGET_68881 && TUNE_68040)
#define HAVE_ftruncsf2_68881 (TARGET_68881 && !TUNE_68040)
#define HAVE_ftruncdf2_68881 (TARGET_68881 && !TUNE_68040)
#define HAVE_ftruncxf2_68881 ((TARGET_68881 && !TUNE_68040) && (TARGET_68881))
#define HAVE_ftruncsf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_ftruncdf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixsfqi2_68881 (TARGET_68881)
#define HAVE_fixdfqi2_68881 (TARGET_68881)
#define HAVE_fixxfqi2_68881 (TARGET_68881)
#define HAVE_fixsfqi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixdfqi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixsfhi2_68881 (TARGET_68881)
#define HAVE_fixdfhi2_68881 (TARGET_68881)
#define HAVE_fixxfhi2_68881 (TARGET_68881)
#define HAVE_fixsfhi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixdfhi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixsfsi2_68881 (TARGET_68881)
#define HAVE_fixdfsi2_68881 (TARGET_68881)
#define HAVE_fixxfsi2_68881 (TARGET_68881)
#define HAVE_fixsfsi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fixdfsi2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_adddi_lshrdi_63 1
#define HAVE_adddi_sexthishl32 (!TARGET_COLDFIRE)
#define HAVE_adddi_dishl32 1
#define HAVE_adddi3 1
#define HAVE_addsi_lshrsi_31 1
#define HAVE_addhi3 (!TARGET_COLDFIRE)
#define HAVE_addqi3 (!TARGET_COLDFIRE)
#define HAVE_addsf3_floatsi_68881 (TARGET_68881)
#define HAVE_adddf3_floatsi_68881 (TARGET_68881)
#define HAVE_addxf3_floatsi_68881 (TARGET_68881)
#define HAVE_addsf3_floathi_68881 (TARGET_68881)
#define HAVE_adddf3_floathi_68881 (TARGET_68881)
#define HAVE_addxf3_floathi_68881 (TARGET_68881)
#define HAVE_addsf3_floatqi_68881 (TARGET_68881)
#define HAVE_adddf3_floatqi_68881 (TARGET_68881)
#define HAVE_addxf3_floatqi_68881 (TARGET_68881)
#define HAVE_addsf3_68881 (TARGET_68881)
#define HAVE_adddf3_68881 (TARGET_68881)
#define HAVE_addxf3_68881 (TARGET_68881)
#define HAVE_addsf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_adddf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_subdi_sexthishl32 (!TARGET_COLDFIRE)
#define HAVE_subdi_dishl32 1
#define HAVE_subdi3 1
#define HAVE_subsi3 1
#define HAVE_subhi3 (!TARGET_COLDFIRE)
#define HAVE_subqi3 (!TARGET_COLDFIRE)
#define HAVE_subsf3_floatsi_68881 (TARGET_68881)
#define HAVE_subdf3_floatsi_68881 (TARGET_68881)
#define HAVE_subxf3_floatsi_68881 (TARGET_68881)
#define HAVE_subsf3_floathi_68881 (TARGET_68881)
#define HAVE_subdf3_floathi_68881 (TARGET_68881)
#define HAVE_subxf3_floathi_68881 (TARGET_68881)
#define HAVE_subsf3_floatqi_68881 (TARGET_68881)
#define HAVE_subdf3_floatqi_68881 (TARGET_68881)
#define HAVE_subxf3_floatqi_68881 (TARGET_68881)
#define HAVE_subsf3_68881 (TARGET_68881)
#define HAVE_subdf3_68881 (TARGET_68881)
#define HAVE_subxf3_68881 (TARGET_68881)
#define HAVE_subsf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_subdf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_mulhi3 1
#define HAVE_mulhisi3 1
#define HAVE_umulhisi3 1
#define HAVE_const_umulsi3_highpart (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_const_smulsi3_highpart (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_mulsf3_floatsi_68881 (TARGET_68881)
#define HAVE_muldf3_floatsi_68881 (TARGET_68881)
#define HAVE_mulxf3_floatsi_68881 (TARGET_68881)
#define HAVE_mulsf3_floathi_68881 (TARGET_68881)
#define HAVE_muldf3_floathi_68881 (TARGET_68881)
#define HAVE_mulxf3_floathi_68881 (TARGET_68881)
#define HAVE_mulsf3_floatqi_68881 (TARGET_68881)
#define HAVE_muldf3_floatqi_68881 (TARGET_68881)
#define HAVE_mulxf3_floatqi_68881 (TARGET_68881)
#define HAVE_muldf_68881 (TARGET_68881)
#define HAVE_mulsf_68881 (TARGET_68881)
#define HAVE_mulxf3_68881 (TARGET_68881)
#define HAVE_fmulsf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_fmuldf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_divsf3_floatsi_68881 (TARGET_68881)
#define HAVE_divdf3_floatsi_68881 (TARGET_68881)
#define HAVE_divxf3_floatsi_68881 (TARGET_68881)
#define HAVE_divsf3_floathi_68881 (TARGET_68881)
#define HAVE_divdf3_floathi_68881 (TARGET_68881)
#define HAVE_divxf3_floathi_68881 (TARGET_68881)
#define HAVE_divsf3_floatqi_68881 (TARGET_68881)
#define HAVE_divdf3_floatqi_68881 (TARGET_68881)
#define HAVE_divxf3_floatqi_68881 (TARGET_68881)
#define HAVE_divsf3_68881 (TARGET_68881)
#define HAVE_divdf3_68881 (TARGET_68881)
#define HAVE_divxf3_68881 (TARGET_68881)
#define HAVE_divsf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_divdf3_cf (TARGET_COLDFIRE_FPU)
#define HAVE_divmodhi4 (!TARGET_COLDFIRE || TARGET_CF_HWDIV)
#define HAVE_udivmodhi4 (!TARGET_COLDFIRE || TARGET_CF_HWDIV)
#define HAVE_andsi3_internal (!TARGET_COLDFIRE)
#define HAVE_andsi3_5200 (TARGET_COLDFIRE)
#define HAVE_andhi3 (!TARGET_COLDFIRE)
#define HAVE_andqi3 (!TARGET_COLDFIRE)
#define HAVE_iordi_zext (!TARGET_COLDFIRE)
#define HAVE_iorsi3_internal (! TARGET_COLDFIRE)
#define HAVE_iorsi3_5200 (TARGET_COLDFIRE)
#define HAVE_iorhi3 (!TARGET_COLDFIRE)
#define HAVE_iorqi3 (!TARGET_COLDFIRE)
#define HAVE_iorsi_zexthi_ashl16 1
#define HAVE_iorsi_zext (!TARGET_COLDFIRE)
#define HAVE_xorsi3_internal (!TARGET_COLDFIRE)
#define HAVE_xorsi3_5200 (TARGET_COLDFIRE)
#define HAVE_xorhi3 (!TARGET_COLDFIRE)
#define HAVE_xorqi3 (!TARGET_COLDFIRE)
#define HAVE_negdi2_internal (!TARGET_COLDFIRE)
#define HAVE_negdi2_5200 (TARGET_COLDFIRE)
#define HAVE_negsi2_internal (!TARGET_COLDFIRE)
#define HAVE_negsi2_5200 (TARGET_COLDFIRE)
#define HAVE_neghi2 (!TARGET_COLDFIRE)
#define HAVE_negqi2 (!TARGET_COLDFIRE)
#define HAVE_negsf2_68881 (TARGET_68881)
#define HAVE_negdf2_68881 (TARGET_68881)
#define HAVE_negxf2_68881 (TARGET_68881)
#define HAVE_negsf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_negdf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_sqrtsf2_68881 (TARGET_68881)
#define HAVE_sqrtdf2_68881 (TARGET_68881)
#define HAVE_sqrtxf2_68881 (TARGET_68881)
#define HAVE_sqrtsf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_sqrtdf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_abssf2_68881 (TARGET_68881)
#define HAVE_absdf2_68881 (TARGET_68881)
#define HAVE_absxf2_68881 (TARGET_68881)
#define HAVE_abssf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_absdf2_cf (TARGET_COLDFIRE_FPU)
#define HAVE_one_cmplsi2_internal (!TARGET_COLDFIRE)
#define HAVE_one_cmplsi2_5200 (TARGET_COLDFIRE)
#define HAVE_one_cmplhi2 (!TARGET_COLDFIRE)
#define HAVE_one_cmplqi2 (!TARGET_COLDFIRE)
#define HAVE_ashldi_extsi 1
#define HAVE_ashldi_sexthi 1
#define HAVE_ashlsi_16 (!TUNE_68060)
#define HAVE_ashlsi_17_24 (TUNE_68000_10 \
   && INTVAL (operands[2]) > 16 \
   && INTVAL (operands[2]) <= 24)
#define HAVE_ashlsi3 1
#define HAVE_ashlhi3 (!TARGET_COLDFIRE)
#define HAVE_ashlqi3 (!TARGET_COLDFIRE)
#define HAVE_ashrsi_16 (!TUNE_68060)
#define HAVE_subreghi1ashrdi_const32 1
#define HAVE_subregsi1ashrdi_const32 1
#define HAVE_ashrdi_const (!TARGET_COLDFIRE \
    && ((INTVAL (operands[2]) >= 1 && INTVAL (operands[2]) <= 3) \
	|| INTVAL (operands[2]) == 8 || INTVAL (operands[2]) == 16 \
	|| INTVAL (operands[2]) == 31 \
	|| (INTVAL (operands[2]) > 32 && INTVAL (operands[2]) <= 63)))
#define HAVE_ashrsi_31 1
#define HAVE_ashrsi3 1
#define HAVE_ashrhi3 (!TARGET_COLDFIRE)
#define HAVE_ashrqi3 (!TARGET_COLDFIRE)
#define HAVE_subreg1lshrdi_const32 1
#define HAVE_lshrsi_31 1
#define HAVE_lshrsi_16 (!TUNE_68060)
#define HAVE_lshrsi_17_24 (TUNE_68000_10 \
   && INTVAL (operands[2]) > 16 \
   && INTVAL (operands[2]) <= 24)
#define HAVE_lshrsi3 1
#define HAVE_lshrhi3 (!TARGET_COLDFIRE)
#define HAVE_lshrqi3 (!TARGET_COLDFIRE)
#define HAVE_rotlsi_16 1
#define HAVE_rotlsi3 (!TARGET_COLDFIRE)
#define HAVE_rotlhi3 (!TARGET_COLDFIRE)
#define HAVE_rotlqi3 (!TARGET_COLDFIRE)
#define HAVE_rotrsi3 (!TARGET_COLDFIRE)
#define HAVE_rotrhi3 (!TARGET_COLDFIRE)
#define HAVE_rotrhi_lowpart (!TARGET_COLDFIRE)
#define HAVE_rotrqi3 (!TARGET_COLDFIRE)
#define HAVE_bsetmemqi 1
#define HAVE_bclrmemqi 1
#define HAVE_scc0_di (! TARGET_COLDFIRE)
#define HAVE_scc0_di_5200 (TARGET_COLDFIRE)
#define HAVE_scc_di (! TARGET_COLDFIRE)
#define HAVE_scc_di_5200 (TARGET_COLDFIRE)
#define HAVE_jump 1
#define HAVE_blockage 1
#define HAVE_nop 1
#define HAVE_load_got 1
#define HAVE_indirect_jump 1
#define HAVE_extendsfxf2 (TARGET_68881)
#define HAVE_extenddfxf2 (TARGET_68881)
#define HAVE_truncxfdf2 (TARGET_68881)
#define HAVE_truncxfsf2 (TARGET_68881)
#define HAVE_sinsf2 (TARGET_68881 && flag_unsafe_math_optimizations)
#define HAVE_sindf2 (TARGET_68881 && flag_unsafe_math_optimizations)
#define HAVE_sinxf2 ((TARGET_68881 && flag_unsafe_math_optimizations) && (TARGET_68881))
#define HAVE_cossf2 (TARGET_68881 && flag_unsafe_math_optimizations)
#define HAVE_cosdf2 (TARGET_68881 && flag_unsafe_math_optimizations)
#define HAVE_cosxf2 ((TARGET_68881 && flag_unsafe_math_optimizations) && (TARGET_68881))
#define HAVE_trap 1
#define HAVE_ctrapqi4 (TARGET_68020 && !TARGET_COLDFIRE)
#define HAVE_ctraphi4 (TARGET_68020 && !TARGET_COLDFIRE)
#define HAVE_ctrapsi4 (TARGET_68020 && !TARGET_COLDFIRE)
#define HAVE_ctrapqi4_cf (TARGET_68020 && TARGET_COLDFIRE)
#define HAVE_ctraphi4_cf (TARGET_68020 && TARGET_COLDFIRE)
#define HAVE_ctrapsi4_cf (TARGET_68020 && TARGET_COLDFIRE)
#define HAVE_stack_tie 1
#define HAVE_ib 1
#define HAVE_atomic_compare_and_swapqi_1 (TARGET_CAS)
#define HAVE_atomic_compare_and_swaphi_1 (TARGET_CAS)
#define HAVE_atomic_compare_and_swapsi_1 (TARGET_CAS)
#define HAVE_atomic_test_and_set_1 (ISA_HAS_TAS)
#define HAVE_cbranchdi4 1
#define HAVE_cstoredi4 1
#define HAVE_cbranchqi4 1
#define HAVE_cbranchhi4 1
#define HAVE_cbranchsi4 1
#define HAVE_cstoreqi4 1
#define HAVE_cstorehi4 1
#define HAVE_cstoresi4 1
#define HAVE_cbranchsf4 (TARGET_HARD_FLOAT)
#define HAVE_cbranchdf4 (TARGET_HARD_FLOAT)
#define HAVE_cbranchxf4 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_cstoresf4 (TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU))
#define HAVE_cstoredf4 (TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU))
#define HAVE_cstorexf4 ((TARGET_HARD_FLOAT && !(TUNE_68060 || TARGET_COLDFIRE_FPU)) && (TARGET_68881))
#define HAVE_movsi 1
#define HAVE_movhi 1
#define HAVE_movstricthi 1
#define HAVE_movqi 1
#define HAVE_movstrictqi 1
#define HAVE_pushqi1 (!TARGET_COLDFIRE)
#define HAVE_reload_insf (TARGET_COLDFIRE_FPU)
#define HAVE_reload_outsf (TARGET_COLDFIRE_FPU)
#define HAVE_movsf 1
#define HAVE_reload_indf (TARGET_COLDFIRE_FPU)
#define HAVE_reload_outdf (TARGET_COLDFIRE_FPU)
#define HAVE_movdf 1
#define HAVE_movxf 1
#define HAVE_movdi 1
#define HAVE_zero_extendsidi2 1
#define HAVE_zero_extendqihi2 (!TARGET_COLDFIRE)
#define HAVE_extendhisi2 1
#define HAVE_extendqisi2 (TARGET_68020 || TARGET_COLDFIRE)
#define HAVE_extendsfdf2 (TARGET_HARD_FLOAT)
#define HAVE_truncdfsf2 (TARGET_HARD_FLOAT)
#define HAVE_floatsisf2 (TARGET_HARD_FLOAT)
#define HAVE_floatsidf2 (TARGET_HARD_FLOAT)
#define HAVE_floatsixf2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_floathisf2 (TARGET_HARD_FLOAT)
#define HAVE_floathidf2 (TARGET_HARD_FLOAT)
#define HAVE_floathixf2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_floatqisf2 (TARGET_HARD_FLOAT)
#define HAVE_floatqidf2 (TARGET_HARD_FLOAT)
#define HAVE_floatqixf2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_ftruncsf2 (TARGET_HARD_FLOAT && !TUNE_68040)
#define HAVE_ftruncdf2 (TARGET_HARD_FLOAT && !TUNE_68040)
#define HAVE_ftruncxf2 ((TARGET_HARD_FLOAT && !TUNE_68040) && (TARGET_68881))
#define HAVE_fixsfqi2 (TARGET_HARD_FLOAT)
#define HAVE_fixdfqi2 (TARGET_HARD_FLOAT)
#define HAVE_fixxfqi2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_fixsfhi2 (TARGET_HARD_FLOAT)
#define HAVE_fixdfhi2 (TARGET_HARD_FLOAT)
#define HAVE_fixxfhi2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_fixsfsi2 (TARGET_HARD_FLOAT)
#define HAVE_fixdfsi2 (TARGET_HARD_FLOAT)
#define HAVE_fixxfsi2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_addsi3 1
#define HAVE_addsf3 (TARGET_HARD_FLOAT)
#define HAVE_adddf3 (TARGET_HARD_FLOAT)
#define HAVE_addxf3 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_subsf3 (TARGET_HARD_FLOAT)
#define HAVE_subdf3 (TARGET_HARD_FLOAT)
#define HAVE_subxf3 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_mulsi3 (TARGET_68020 || TARGET_COLDFIRE)
#define HAVE_umulsidi3 (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_mulsidi3 (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_umulsi3_highpart (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_smulsi3_highpart (TARGET_68020 && !TUNE_68060 && !TARGET_COLDFIRE)
#define HAVE_mulsf3 (TARGET_HARD_FLOAT)
#define HAVE_muldf3 (TARGET_HARD_FLOAT)
#define HAVE_mulxf3 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_divsf3 (TARGET_HARD_FLOAT)
#define HAVE_divdf3 (TARGET_HARD_FLOAT)
#define HAVE_divxf3 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_divmodsi4 (TARGET_68020 || TARGET_CF_HWDIV)
#define HAVE_udivmodsi4 (TARGET_68020 || TARGET_CF_HWDIV)
#define HAVE_andsi3 1
#define HAVE_iorsi3 1
#define HAVE_xorsi3 1
#define HAVE_negdi2 1
#define HAVE_negsi2 1
#define HAVE_negsf2 1
#define HAVE_negdf2 1
#define HAVE_negxf2 1
#define HAVE_sqrtsf2 (TARGET_HARD_FLOAT)
#define HAVE_sqrtdf2 (TARGET_HARD_FLOAT)
#define HAVE_sqrtxf2 ((TARGET_HARD_FLOAT) && (TARGET_68881))
#define HAVE_abssf2 1
#define HAVE_absdf2 1
#define HAVE_absxf2 1
#define HAVE_clzsi2 (ISA_HAS_FF1 || (TARGET_68020 && TARGET_BITFIELD))
#define HAVE_one_cmplsi2 1
#define HAVE_ashldi3 (!TARGET_COLDFIRE)
#define HAVE_ashrdi3 (!TARGET_COLDFIRE)
#define HAVE_lshrdi3 (!TARGET_COLDFIRE)
#define HAVE_bswapsi2 (!TARGET_COLDFIRE)
#define HAVE_extv (TARGET_68020 && TARGET_BITFIELD)
#define HAVE_extzv (TARGET_68020 && TARGET_BITFIELD)
#define HAVE_insv (TARGET_68020 && TARGET_BITFIELD)
#define HAVE_tablejump 1
#define HAVE_decrement_and_branch_until_zero 1
#define HAVE_sibcall 1
#define HAVE_sibcall_value 1
#define HAVE_call 1
#define HAVE_call_value 1
#define HAVE_untyped_call 1
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_sibcall_epilogue 1
#define HAVE_return (m68k_use_return_insn ())
#define HAVE_link (TARGET_68020 || INTVAL (operands[1]) >= -0x8004)
#define HAVE_unlink 1
#define HAVE_atomic_compare_and_swapqi (TARGET_CAS)
#define HAVE_atomic_compare_and_swaphi (TARGET_CAS)
#define HAVE_atomic_compare_and_swapsi (TARGET_CAS)
#define HAVE_atomic_test_and_set (ISA_HAS_TAS)
extern rtx        gen_pushdi                          (rtx, rtx);
extern rtx        gen_beq0_di                         (rtx, rtx);
extern rtx        gen_bne0_di                         (rtx, rtx);
extern rtx        gen_cbranchdi4_insn                 (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchqi4_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchhi4_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_insn                 (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchqi4_insn_rev             (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchhi4_insn_rev             (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_insn_rev             (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchqi4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchhi4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchqi4_insn_cf_rev          (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchhi4_insn_cf_rev          (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_insn_cf_rev          (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoreqi4_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cstorehi4_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4_insn                  (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoreqi4_insn_cf               (rtx, rtx, rtx, rtx);
extern rtx        gen_cstorehi4_insn_cf               (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4_insn_cf               (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_btst_mem_insn        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_btst_reg_insn        (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_btst_mem_insn_1      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4_btst_reg_insn_1      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranch_bftstqi_insn            (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cbranch_bftstsi_insn            (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cstore_bftstqi_insn             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cstore_bftstsi_insn             (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4_insn_68881           (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4_insn_68881           (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchxf4_insn_68881           (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchxf4_insn_cf              (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4_insn_rev_68881       (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4_insn_rev_68881       (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchxf4_insn_rev_68881       (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4_insn_rev_cf          (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4_insn_rev_cf          (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchxf4_insn_rev_cf          (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4_insn_68881            (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4_insn_68881            (rtx, rtx, rtx, rtx);
extern rtx        gen_cstorexf4_insn_68881            (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4_insn_cf               (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4_insn_cf               (rtx, rtx, rtx, rtx);
static inline rtx gen_cstorexf4_insn_cf               (rtx, rtx, rtx, rtx);
static inline rtx
gen_cstorexf4_insn_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c), rtx ARG_UNUSED (d))
{
  return 0;
}
extern rtx        gen_pushexthisi_const               (rtx, rtx);
extern rtx        gen_movsf_cf_soft                   (rtx, rtx);
extern rtx        gen_movsf_cf_hard                   (rtx, rtx);
extern rtx        gen_movdf_cf_soft                   (rtx, rtx);
extern rtx        gen_movdf_cf_hard                   (rtx, rtx);
extern rtx        gen_pushasi                         (rtx, rtx);
extern rtx        gen_truncsiqi2                      (rtx, rtx);
extern rtx        gen_trunchiqi2                      (rtx, rtx);
extern rtx        gen_truncsihi2                      (rtx, rtx);
extern rtx        gen_zero_extendqidi2                (rtx, rtx);
extern rtx        gen_zero_extendhidi2                (rtx, rtx);
extern rtx        gen_zero_extendhisi2                (rtx, rtx);
extern rtx        gen_zero_extendqisi2                (rtx, rtx);
extern rtx        gen_extendqidi2                     (rtx, rtx);
extern rtx        gen_extendhidi2                     (rtx, rtx);
extern rtx        gen_extendsidi2                     (rtx, rtx);
extern rtx        gen_extendplussidi                  (rtx, rtx, rtx);
extern rtx        gen_extendqihi2                     (rtx, rtx);
extern rtx        gen_extendsfdf2_cf                  (rtx, rtx);
extern rtx        gen_truncdfsf2_cf                   (rtx, rtx);
extern rtx        gen_floatsisf2_68881                (rtx, rtx);
extern rtx        gen_floatsidf2_68881                (rtx, rtx);
extern rtx        gen_floatsixf2_68881                (rtx, rtx);
extern rtx        gen_floatsisf2_cf                   (rtx, rtx);
extern rtx        gen_floatsidf2_cf                   (rtx, rtx);
static inline rtx gen_floatsixf2_cf                   (rtx, rtx);
static inline rtx
gen_floatsixf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_floathisf2_68881                (rtx, rtx);
extern rtx        gen_floathidf2_68881                (rtx, rtx);
extern rtx        gen_floathixf2_68881                (rtx, rtx);
extern rtx        gen_floathisf2_cf                   (rtx, rtx);
extern rtx        gen_floathidf2_cf                   (rtx, rtx);
static inline rtx gen_floathixf2_cf                   (rtx, rtx);
static inline rtx
gen_floathixf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_floatqisf2_68881                (rtx, rtx);
extern rtx        gen_floatqidf2_68881                (rtx, rtx);
extern rtx        gen_floatqixf2_68881                (rtx, rtx);
extern rtx        gen_floatqisf2_cf                   (rtx, rtx);
extern rtx        gen_floatqidf2_cf                   (rtx, rtx);
static inline rtx gen_floatqixf2_cf                   (rtx, rtx);
static inline rtx
gen_floatqixf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_fix_truncdfsi2                  (rtx, rtx);
extern rtx        gen_fix_truncdfhi2                  (rtx, rtx);
extern rtx        gen_fix_truncdfqi2                  (rtx, rtx);
extern rtx        gen_ftruncsf2_68881                 (rtx, rtx);
extern rtx        gen_ftruncdf2_68881                 (rtx, rtx);
extern rtx        gen_ftruncxf2_68881                 (rtx, rtx);
extern rtx        gen_ftruncsf2_cf                    (rtx, rtx);
extern rtx        gen_ftruncdf2_cf                    (rtx, rtx);
static inline rtx gen_ftruncxf2_cf                    (rtx, rtx);
static inline rtx
gen_ftruncxf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_fixsfqi2_68881                  (rtx, rtx);
extern rtx        gen_fixdfqi2_68881                  (rtx, rtx);
extern rtx        gen_fixxfqi2_68881                  (rtx, rtx);
extern rtx        gen_fixsfqi2_cf                     (rtx, rtx);
extern rtx        gen_fixdfqi2_cf                     (rtx, rtx);
static inline rtx gen_fixxfqi2_cf                     (rtx, rtx);
static inline rtx
gen_fixxfqi2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_fixsfhi2_68881                  (rtx, rtx);
extern rtx        gen_fixdfhi2_68881                  (rtx, rtx);
extern rtx        gen_fixxfhi2_68881                  (rtx, rtx);
extern rtx        gen_fixsfhi2_cf                     (rtx, rtx);
extern rtx        gen_fixdfhi2_cf                     (rtx, rtx);
static inline rtx gen_fixxfhi2_cf                     (rtx, rtx);
static inline rtx
gen_fixxfhi2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_fixsfsi2_68881                  (rtx, rtx);
extern rtx        gen_fixdfsi2_68881                  (rtx, rtx);
extern rtx        gen_fixxfsi2_68881                  (rtx, rtx);
extern rtx        gen_fixsfsi2_cf                     (rtx, rtx);
extern rtx        gen_fixdfsi2_cf                     (rtx, rtx);
static inline rtx gen_fixxfsi2_cf                     (rtx, rtx);
static inline rtx
gen_fixxfsi2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_adddi_lshrdi_63                 (rtx, rtx);
extern rtx        gen_adddi_sexthishl32               (rtx, rtx, rtx);
extern rtx        gen_adddi_dishl32                   (rtx, rtx, rtx);
extern rtx        gen_adddi3                          (rtx, rtx, rtx);
extern rtx        gen_addsi_lshrsi_31                 (rtx, rtx);
extern rtx        gen_addhi3                          (rtx, rtx, rtx);
extern rtx        gen_addqi3                          (rtx, rtx, rtx);
extern rtx        gen_addsf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_adddf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_addxf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_addsf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_adddf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_addxf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_addsf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_adddf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_addxf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_addsf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_adddf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_addxf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_addsf3_cf                       (rtx, rtx, rtx);
extern rtx        gen_adddf3_cf                       (rtx, rtx, rtx);
static inline rtx gen_addxf3_cf                       (rtx, rtx, rtx);
static inline rtx
gen_addxf3_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c))
{
  return 0;
}
extern rtx        gen_subdi_sexthishl32               (rtx, rtx, rtx);
extern rtx        gen_subdi_dishl32                   (rtx, rtx);
extern rtx        gen_subdi3                          (rtx, rtx, rtx);
extern rtx        gen_subsi3                          (rtx, rtx, rtx);
extern rtx        gen_subhi3                          (rtx, rtx, rtx);
extern rtx        gen_subqi3                          (rtx, rtx, rtx);
extern rtx        gen_subsf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_subdf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_subxf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_subsf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_subdf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_subxf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_subsf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_subdf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_subxf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_subsf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_subdf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_subxf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_subsf3_cf                       (rtx, rtx, rtx);
extern rtx        gen_subdf3_cf                       (rtx, rtx, rtx);
static inline rtx gen_subxf3_cf                       (rtx, rtx, rtx);
static inline rtx
gen_subxf3_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c))
{
  return 0;
}
extern rtx        gen_mulhi3                          (rtx, rtx, rtx);
extern rtx        gen_mulhisi3                        (rtx, rtx, rtx);
extern rtx        gen_umulhisi3                       (rtx, rtx, rtx);
extern rtx        gen_const_umulsi3_highpart          (rtx, rtx, rtx, rtx);
extern rtx        gen_const_smulsi3_highpart          (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_muldf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_mulxf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_mulsf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_muldf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_mulxf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_mulsf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_muldf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_mulxf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_muldf_68881                     (rtx, rtx, rtx);
extern rtx        gen_mulsf_68881                     (rtx, rtx, rtx);
extern rtx        gen_mulxf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_fmulsf3_cf                      (rtx, rtx, rtx);
extern rtx        gen_fmuldf3_cf                      (rtx, rtx, rtx);
static inline rtx gen_fmulxf3_cf                      (rtx, rtx, rtx);
static inline rtx
gen_fmulxf3_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c))
{
  return 0;
}
extern rtx        gen_divsf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_divdf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_divxf3_floatsi_68881            (rtx, rtx, rtx);
extern rtx        gen_divsf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_divdf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_divxf3_floathi_68881            (rtx, rtx, rtx);
extern rtx        gen_divsf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_divdf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_divxf3_floatqi_68881            (rtx, rtx, rtx);
extern rtx        gen_divsf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_divdf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_divxf3_68881                    (rtx, rtx, rtx);
extern rtx        gen_divsf3_cf                       (rtx, rtx, rtx);
extern rtx        gen_divdf3_cf                       (rtx, rtx, rtx);
static inline rtx gen_divxf3_cf                       (rtx, rtx, rtx);
static inline rtx
gen_divxf3_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b), rtx ARG_UNUSED (c))
{
  return 0;
}
extern rtx        gen_divmodhi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_udivmodhi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_andsi3_internal                 (rtx, rtx, rtx);
extern rtx        gen_andsi3_5200                     (rtx, rtx, rtx);
extern rtx        gen_andhi3                          (rtx, rtx, rtx);
extern rtx        gen_andqi3                          (rtx, rtx, rtx);
extern rtx        gen_iordi_zext                      (rtx, rtx, rtx);
extern rtx        gen_iorsi3_internal                 (rtx, rtx, rtx);
extern rtx        gen_iorsi3_5200                     (rtx, rtx, rtx);
extern rtx        gen_iorhi3                          (rtx, rtx, rtx);
extern rtx        gen_iorqi3                          (rtx, rtx, rtx);
extern rtx        gen_iorsi_zexthi_ashl16             (rtx, rtx, rtx);
extern rtx        gen_iorsi_zext                      (rtx, rtx, rtx);
extern rtx        gen_xorsi3_internal                 (rtx, rtx, rtx);
extern rtx        gen_xorsi3_5200                     (rtx, rtx, rtx);
extern rtx        gen_xorhi3                          (rtx, rtx, rtx);
extern rtx        gen_xorqi3                          (rtx, rtx, rtx);
extern rtx        gen_negdi2_internal                 (rtx, rtx);
extern rtx        gen_negdi2_5200                     (rtx, rtx);
extern rtx        gen_negsi2_internal                 (rtx, rtx);
extern rtx        gen_negsi2_5200                     (rtx, rtx);
extern rtx        gen_neghi2                          (rtx, rtx);
extern rtx        gen_negqi2                          (rtx, rtx);
extern rtx        gen_negsf2_68881                    (rtx, rtx);
extern rtx        gen_negdf2_68881                    (rtx, rtx);
extern rtx        gen_negxf2_68881                    (rtx, rtx);
extern rtx        gen_negsf2_cf                       (rtx, rtx);
extern rtx        gen_negdf2_cf                       (rtx, rtx);
static inline rtx gen_negxf2_cf                       (rtx, rtx);
static inline rtx
gen_negxf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_sqrtsf2_68881                   (rtx, rtx);
extern rtx        gen_sqrtdf2_68881                   (rtx, rtx);
extern rtx        gen_sqrtxf2_68881                   (rtx, rtx);
extern rtx        gen_sqrtsf2_cf                      (rtx, rtx);
extern rtx        gen_sqrtdf2_cf                      (rtx, rtx);
static inline rtx gen_sqrtxf2_cf                      (rtx, rtx);
static inline rtx
gen_sqrtxf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_abssf2_68881                    (rtx, rtx);
extern rtx        gen_absdf2_68881                    (rtx, rtx);
extern rtx        gen_absxf2_68881                    (rtx, rtx);
extern rtx        gen_abssf2_cf                       (rtx, rtx);
extern rtx        gen_absdf2_cf                       (rtx, rtx);
static inline rtx gen_absxf2_cf                       (rtx, rtx);
static inline rtx
gen_absxf2_cf(rtx ARG_UNUSED (a), rtx ARG_UNUSED (b))
{
  return 0;
}
extern rtx        gen_one_cmplsi2_internal            (rtx, rtx);
extern rtx        gen_one_cmplsi2_5200                (rtx, rtx);
extern rtx        gen_one_cmplhi2                     (rtx, rtx);
extern rtx        gen_one_cmplqi2                     (rtx, rtx);
extern rtx        gen_ashldi_extsi                    (rtx, rtx, rtx);
extern rtx        gen_ashldi_sexthi                   (rtx, rtx);
extern rtx        gen_ashlsi_16                       (rtx, rtx);
extern rtx        gen_ashlsi_17_24                    (rtx, rtx, rtx);
extern rtx        gen_ashlsi3                         (rtx, rtx, rtx);
extern rtx        gen_ashlhi3                         (rtx, rtx, rtx);
extern rtx        gen_ashlqi3                         (rtx, rtx, rtx);
extern rtx        gen_ashrsi_16                       (rtx, rtx);
extern rtx        gen_subreghi1ashrdi_const32         (rtx, rtx);
extern rtx        gen_subregsi1ashrdi_const32         (rtx, rtx);
extern rtx        gen_ashrdi_const                    (rtx, rtx, rtx);
extern rtx        gen_ashrsi_31                       (rtx, rtx);
extern rtx        gen_ashrsi3                         (rtx, rtx, rtx);
extern rtx        gen_ashrhi3                         (rtx, rtx, rtx);
extern rtx        gen_ashrqi3                         (rtx, rtx, rtx);
extern rtx        gen_subreg1lshrdi_const32           (rtx, rtx);
extern rtx        gen_lshrsi_31                       (rtx, rtx);
extern rtx        gen_lshrsi_16                       (rtx, rtx);
extern rtx        gen_lshrsi_17_24                    (rtx, rtx, rtx);
extern rtx        gen_lshrsi3                         (rtx, rtx, rtx);
extern rtx        gen_lshrhi3                         (rtx, rtx, rtx);
extern rtx        gen_lshrqi3                         (rtx, rtx, rtx);
extern rtx        gen_rotlsi_16                       (rtx, rtx);
extern rtx        gen_rotlsi3                         (rtx, rtx, rtx);
extern rtx        gen_rotlhi3                         (rtx, rtx, rtx);
extern rtx        gen_rotlqi3                         (rtx, rtx, rtx);
extern rtx        gen_rotrsi3                         (rtx, rtx, rtx);
extern rtx        gen_rotrhi3                         (rtx, rtx, rtx);
extern rtx        gen_rotrhi_lowpart                  (rtx, rtx);
extern rtx        gen_rotrqi3                         (rtx, rtx, rtx);
extern rtx        gen_bsetmemqi                       (rtx, rtx);
extern rtx        gen_bclrmemqi                       (rtx, rtx);
extern rtx        gen_scc0_di                         (rtx, rtx, rtx);
extern rtx        gen_scc0_di_5200                    (rtx, rtx, rtx);
extern rtx        gen_scc_di                          (rtx, rtx, rtx, rtx);
extern rtx        gen_scc_di_5200                     (rtx, rtx, rtx, rtx);
extern rtx        gen_jump                            (rtx);
extern rtx        gen_blockage                        (void);
extern rtx        gen_nop                             (void);
extern rtx        gen_load_got                        (rtx);
extern rtx        gen_indirect_jump                   (rtx);
extern rtx        gen_extendsfxf2                     (rtx, rtx);
extern rtx        gen_extenddfxf2                     (rtx, rtx);
extern rtx        gen_truncxfdf2                      (rtx, rtx);
extern rtx        gen_truncxfsf2                      (rtx, rtx);
extern rtx        gen_sinsf2                          (rtx, rtx);
extern rtx        gen_sindf2                          (rtx, rtx);
extern rtx        gen_sinxf2                          (rtx, rtx);
extern rtx        gen_cossf2                          (rtx, rtx);
extern rtx        gen_cosdf2                          (rtx, rtx);
extern rtx        gen_cosxf2                          (rtx, rtx);
extern rtx        gen_trap                            (void);
extern rtx        gen_ctrapqi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_ctraphi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_ctrapsi4                        (rtx, rtx, rtx, rtx);
extern rtx        gen_ctrapqi4_cf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_ctraphi4_cf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_ctrapsi4_cf                     (rtx, rtx, rtx, rtx);
extern rtx        gen_stack_tie                       (rtx, rtx);
extern rtx        gen_ib                              (void);
extern rtx        gen_atomic_compare_and_swapqi_1     (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi_1     (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi_1     (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_test_and_set_1           (rtx, rtx);
extern rtx        gen_cbranchdi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchqi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchhi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoreqi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cstorehi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchdf4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchxf4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoredf4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_cstorexf4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_movsi                           (rtx, rtx);
extern rtx        gen_movhi                           (rtx, rtx);
extern rtx        gen_movstricthi                     (rtx, rtx);
extern rtx        gen_movqi                           (rtx, rtx);
extern rtx        gen_movstrictqi                     (rtx, rtx);
extern rtx        gen_pushqi1                         (rtx);
extern rtx        gen_reload_insf                     (rtx, rtx, rtx);
extern rtx        gen_reload_outsf                    (rtx, rtx, rtx);
extern rtx        gen_movsf                           (rtx, rtx);
extern rtx        gen_reload_indf                     (rtx, rtx, rtx);
extern rtx        gen_reload_outdf                    (rtx, rtx, rtx);
extern rtx        gen_movdf                           (rtx, rtx);
extern rtx        gen_movxf                           (rtx, rtx);
extern rtx        gen_movdi                           (rtx, rtx);
extern rtx        gen_zero_extendsidi2                (rtx, rtx);
extern rtx        gen_zero_extendqihi2                (rtx, rtx);
extern rtx        gen_extendhisi2                     (rtx, rtx);
extern rtx        gen_extendqisi2                     (rtx, rtx);
extern rtx        gen_extendsfdf2                     (rtx, rtx);
extern rtx        gen_truncdfsf2                      (rtx, rtx);
extern rtx        gen_floatsisf2                      (rtx, rtx);
extern rtx        gen_floatsidf2                      (rtx, rtx);
extern rtx        gen_floatsixf2                      (rtx, rtx);
extern rtx        gen_floathisf2                      (rtx, rtx);
extern rtx        gen_floathidf2                      (rtx, rtx);
extern rtx        gen_floathixf2                      (rtx, rtx);
extern rtx        gen_floatqisf2                      (rtx, rtx);
extern rtx        gen_floatqidf2                      (rtx, rtx);
extern rtx        gen_floatqixf2                      (rtx, rtx);
extern rtx        gen_ftruncsf2                       (rtx, rtx);
extern rtx        gen_ftruncdf2                       (rtx, rtx);
extern rtx        gen_ftruncxf2                       (rtx, rtx);
extern rtx        gen_fixsfqi2                        (rtx, rtx);
extern rtx        gen_fixdfqi2                        (rtx, rtx);
extern rtx        gen_fixxfqi2                        (rtx, rtx);
extern rtx        gen_fixsfhi2                        (rtx, rtx);
extern rtx        gen_fixdfhi2                        (rtx, rtx);
extern rtx        gen_fixxfhi2                        (rtx, rtx);
extern rtx        gen_fixsfsi2                        (rtx, rtx);
extern rtx        gen_fixdfsi2                        (rtx, rtx);
extern rtx        gen_fixxfsi2                        (rtx, rtx);
extern rtx        gen_addsi3                          (rtx, rtx, rtx);
extern rtx        gen_addsf3                          (rtx, rtx, rtx);
extern rtx        gen_adddf3                          (rtx, rtx, rtx);
extern rtx        gen_addxf3                          (rtx, rtx, rtx);
extern rtx        gen_subsf3                          (rtx, rtx, rtx);
extern rtx        gen_subdf3                          (rtx, rtx, rtx);
extern rtx        gen_subxf3                          (rtx, rtx, rtx);
extern rtx        gen_mulsi3                          (rtx, rtx, rtx);
extern rtx        gen_umulsidi3                       (rtx, rtx, rtx);
extern rtx        gen_mulsidi3                        (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart                (rtx, rtx, rtx);
extern rtx        gen_smulsi3_highpart                (rtx, rtx, rtx);
extern rtx        gen_mulsf3                          (rtx, rtx, rtx);
extern rtx        gen_muldf3                          (rtx, rtx, rtx);
extern rtx        gen_mulxf3                          (rtx, rtx, rtx);
extern rtx        gen_divsf3                          (rtx, rtx, rtx);
extern rtx        gen_divdf3                          (rtx, rtx, rtx);
extern rtx        gen_divxf3                          (rtx, rtx, rtx);
extern rtx        gen_divmodsi4                       (rtx, rtx, rtx, rtx);
extern rtx        gen_udivmodsi4                      (rtx, rtx, rtx, rtx);
extern rtx        gen_andsi3                          (rtx, rtx, rtx);
extern rtx        gen_iorsi3                          (rtx, rtx, rtx);
extern rtx        gen_xorsi3                          (rtx, rtx, rtx);
extern rtx        gen_negdi2                          (rtx, rtx);
extern rtx        gen_negsi2                          (rtx, rtx);
extern rtx        gen_negsf2                          (rtx, rtx);
extern rtx        gen_negdf2                          (rtx, rtx);
extern rtx        gen_negxf2                          (rtx, rtx);
extern rtx        gen_sqrtsf2                         (rtx, rtx);
extern rtx        gen_sqrtdf2                         (rtx, rtx);
extern rtx        gen_sqrtxf2                         (rtx, rtx);
extern rtx        gen_abssf2                          (rtx, rtx);
extern rtx        gen_absdf2                          (rtx, rtx);
extern rtx        gen_absxf2                          (rtx, rtx);
extern rtx        gen_clzsi2                          (rtx, rtx);
extern rtx        gen_one_cmplsi2                     (rtx, rtx);
extern rtx        gen_ashldi3                         (rtx, rtx, rtx);
extern rtx        gen_ashrdi3                         (rtx, rtx, rtx);
extern rtx        gen_lshrdi3                         (rtx, rtx, rtx);
extern rtx        gen_bswapsi2                        (rtx, rtx);
extern rtx        gen_extv                            (rtx, rtx, rtx, rtx);
extern rtx        gen_extzv                           (rtx, rtx, rtx, rtx);
extern rtx        gen_insv                            (rtx, rtx, rtx, rtx);
extern rtx        gen_tablejump                       (rtx, rtx);
extern rtx        gen_decrement_and_branch_until_zero (rtx, rtx);
extern rtx        gen_sibcall                         (rtx, rtx);
extern rtx        gen_sibcall_value                   (rtx, rtx, rtx);
extern rtx        gen_call                            (rtx, rtx);
extern rtx        gen_call_value                      (rtx, rtx, rtx);
extern rtx        gen_untyped_call                    (rtx, rtx, rtx);
extern rtx        gen_prologue                        (void);
extern rtx        gen_epilogue                        (void);
extern rtx        gen_sibcall_epilogue                (void);
extern rtx        gen_return                          (void);
extern rtx        gen_link                            (rtx, rtx);
extern rtx        gen_unlink                          (rtx);
extern rtx        gen_atomic_compare_and_swapqi       (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swaphi       (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_compare_and_swapsi       (rtx, rtx, rtx, rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_atomic_test_and_set             (rtx, rtx, rtx);

#endif /* GCC_INSN_FLAGS_H */
