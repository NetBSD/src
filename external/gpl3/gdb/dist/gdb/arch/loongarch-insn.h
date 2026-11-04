/* Target-dependent code for LoongArch

   Copyright (C) 2024-2025 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* The LoongArch opcode and mask definitions in this file are obtained from
   https://sourceware.org/git/?p=binutils-gdb.git;a=blob;f=opcodes/loongarch-opc.c  */

#ifndef GDB_ARCH_LOONGARCH_INSN_H
#define GDB_ARCH_LOONGARCH_INSN_H

/* loongarch fix insn opcode  */
#define OP_CLO_W             0x00001000
#define OP_CLZ_W             0x00001400
#define OP_CTO_W             0x00001800
#define OP_CTZ_W             0x00001c00
#define OP_CLO_D             0x00002000
#define OP_CLZ_D             0x00002400
#define OP_CTO_D             0x00002800
#define OP_CTZ_D             0x00002c00
#define OP_REVB_2H           0x00003000
#define OP_REVB_4H           0x00003400
#define OP_REVB_2W           0x00003800
#define OP_REVB_D            0x00003c00
#define OP_REVH_2W           0x00004000
#define OP_REVH_D            0x00004400
#define OP_BITREV_4B         0x00004800
#define OP_BITREV_8B         0x00004c00
#define OP_BITREV_W          0x00005000
#define OP_BITREV_D          0x00005400
#define OP_EXT_W_H           0x00005800
#define OP_EXT_W_B           0x00005c00
#define OP_RDTIMEL_W         0x00006000
#define OP_RDTIMEH_W         0x00006400
#define OP_RDTIME_D          0x00006800
#define OP_CPUCFG            0x00006c00
#define OP_ASRTLE_D          0x00010000
#define OP_ASRTGT_D          0x00018000
#define OP_ALSL_W            0x00040000
#define OP_ALSL_WU           0x00060000
#define OP_BYTEPICK_W        0x00080000
#define OP_BYTEPICK_D        0x000c0000
#define OP_ADD_W             0x00100000
#define OP_ADD_D             0x00108000
#define OP_SUB_W             0x00110000
#define OP_SUB_D             0x00118000
#define OP_SLT               0x00120000
#define OP_SLTU              0x00128000
#define OP_MASKEQZ           0x00130000
#define OP_MASKNEZ           0x00138000
#define OP_NOR               0x00140000
#define OP_AND               0x00148000
#define OP_OR                0x00150000
#define OP_XOR               0x00158000
#define OP_ORN               0x00160000
#define OP_ANDN              0x00168000
#define OP_SLL_W             0x00170000
#define OP_SRL_W             0x00178000
#define OP_SRA_W             0x00180000
#define OP_SLL_D             0x00188000
#define OP_SRL_D             0x00190000
#define OP_SRA_D             0x00198000
#define OP_ROTR_W            0x001b0000
#define OP_ROTR_D            0x001b8000
#define OP_MUL_W             0x001c0000
#define OP_MULH_W            0x001c8000
#define OP_MULH_WU           0x001d0000
#define OP_MUL_D             0x001d8000
#define OP_MULH_D            0x001e0000
#define OP_MULH_DU           0x001e8000
#define OP_MULW_D_W          0x001f0000
#define OP_MULW_D_WU         0x001f8000
#define OP_DIV_W             0x00200000
#define OP_MOD_W             0x00208000
#define OP_DIV_WU            0x00210000
#define OP_MOD_WU            0x00218000
#define OP_DIV_D             0x00220000
#define OP_MOD_D             0x00228000
#define OP_DIV_DU            0x00230000
#define OP_MOD_DU            0x00238000
#define OP_CRC_W_B_W         0x00240000
#define OP_CRC_W_H_W         0x00248000
#define OP_CRC_W_W_W         0x00250000
#define OP_CRC_W_D_W         0x00258000
#define OP_CRCC_W_B_W        0x00260000
#define OP_CRCC_W_H_W        0x00268000
#define OP_CRCC_W_W_W        0x00270000
#define OP_CRCC_W_D_W        0x00278000
#define OP_BREAK             0x002a0000
#define OP_DBCL              0x002a8000
#define OP_SYSCALL           0x002b0000
#define OP_ALSL_D            0x002c0000
#define OP_SLLI_W            0x00408000
#define OP_SLLI_D            0x00410000
#define OP_SRLI_W            0x00448000
#define OP_SRLI_D            0x00450000
#define OP_SRAI_W            0x00488000
#define OP_SRAI_D            0x00490000
#define OP_ROTRI_W           0x004c8000
#define OP_ROTRI_D           0x004d0000
#define OP_BSTRINS_W         0x00600000
#define OP_BSTRPICK_W        0x00608000
#define OP_BSTRINS_D         0x00800000
#define OP_BSTRPICK_D        0x00c00000

/* loongarch single float insn opcode  */
#define OP_FADD_S            0x01008000
#define OP_SUB_S             0x01028000
#define OP_MUL_S             0x01048000
#define OP_FDIV_S            0x01068000
#define OP_FMAX_S            0x01088000
#define OP_FMIN_S            0x010a8000
#define OP_FMAXA_S           0x010c8000
#define OP_FMINA_S           0x010e8000
#define OP_FSCALEB_S         0x01108000
#define OP_FCOPYSIGN_S       0x01128000
#define OP_FABS_S            0x01140400
#define OP_FNEG_S            0x01141400
#define OP_FLOGB_S           0x01142400
#define OP_FCLASS_S          0x01143400
#define OP_FSQRT_S           0x01144400
#define OP_FRECIP_S          0x01145400
#define OP_FRSQRT_S          0x01146400
#define OP_FRECIPE_S         0x01147400
#define OP_FRSQRTE_S         0x01148400
#define OP_FMOV_S            0x01149400
#define OP_MOVGR2FR_W        0x0114a400
#define OP_MOVGR2FRH_W       0x0114ac00
#define OP_MOVFR2GR_S        0x0114b400
#define OP_MOVFRH2GR_S       0x0114bc00
#define OP_MOVGR2FCSR        0x0114c000
#define OP_MOVFCSR2GR        0x0114c800
#define OP_MOVFR2CF          0x0114d000
#define OP_MOVCF2FR          0x0114d400
#define OP_MOVGR2CF          0x0114d800
#define OP_MOVCF2GR          0x0114dc00
#define OP_FTINTRM_W_S       0x011a0400
#define OP_FTINTRM_L_S       0x011a2400
#define OP_FTINTRP_W_S       0x011a4400
#define OP_FTINTRP_L_S       0x011a6400
#define OP_FTINTRZ_W_S       0x011a8400
#define OP_FTINTRZ_L_S       0x011aa400
#define OP_FTINTRNE_W_S      0x011ac400
#define OP_FTINTRNE_L_S      0x011ae400
#define OP_FTINT_W_S         0x011b0400
#define OP_FTINT_L_S         0x011b2400
#define OP_FFINT_S_W         0x011d1000
#define OP_FFINT_S_L         0x011d1800
#define OP_FRINT_S           0x011e4400

/* loongarch double float insn opcode  */
#define OP_FADD_D            0x01010000
#define OP_FSUB_D            0x01030000
#define OP_FMUL_D            0x01050000
#define OP_FDIV_D            0x01070000
#define OP_FMAX_D            0x01090000
#define OP_FMIN_D            0x010b0000
#define OP_FMAXA_D           0x010d0000
#define OP_FMINA_D           0x010f0000
#define OP_FSCALEB_D         0x01110000
#define OP_FCOPYSIGN_D       0x01130000
#define OP_FABS_D            0x01140800
#define OP_FNEG_D            0x01141800
#define OP_FLOGB_D           0x01142800
#define OP_FCLASS_D          0x01143800
#define OP_FSQRT_D           0x01144800
#define OP_FRECIP_D          0x01145800
#define OP_FRSQRT_D          0x01146800
#define OP_FRECIPE_D         0x01147800
#define OP_FRSQRTE_D         0x01148800
#define OP_FMOV_D            0x01149800
#define OP_MOVGR2FR_D        0x0114a800
#define OP_MOVFR2GR_D        0x0114b800
#define OP_FCVT_S_D          0x01191800
#define OP_FCVT_D_S          0x01192400
#define OP_FTINTRM_W_D       0x011a0800
#define OP_FTINTRM_L_D       0x011a2800
#define OP_FTINTRP_W_D       0x011a4800
#define OP_FTINTRP_L_D       0x011a6800
#define OP_FTINTRZ_W_D       0x011a8800
#define OP_FTINTRZ_L_D       0x011aa800
#define OP_FTINTRNE_W_D      0x011ac800
#define OP_FTINTRNE_L_D      0x011ae800
#define OP_FTINT_W_D         0x011b0800
#define OP_FTINT_L_D         0x011b2800
#define OP_FFINT_D_W         0x011d2000
#define OP_FFINT_D_L         0x011d2800
#define OP_FRINT_D           0x011e4800

/* loongarch imm insn opcode  */
#define OP_SLTI              0x02000000
#define OP_SLTUI             0x02400000
#define OP_ADDI_W            0x02800000
#define OP_ADDI_D            0x02c00000
#define OP_LU52I_D           0x03000000
#define OP_ANDI              0x03400000
#define OP_ORI               0x03800000
#define OP_XORI              0x03c00000
#define OP_ADDU16I_D         0x10000000
#define OP_LU12I_W           0x14000000
#define OP_LU32I_D           0x16000000
#define OP_PCADDI            0x18000000
#define OP_PCALAU12I         0x1a000000
#define OP_PCADDU12I         0x1c000000
#define OP_PCADDU18I         0x1e000000

/* loongarch privilege insn opcode  */
#define OP_CSRRD             0x04000000
#define OP_CSRWR             0x04000020
#define OP_CSRXCHG           0x04000000
#define OP_CACOP             0x06000000
#define OP_LDDIR             0x06400000
#define OP_LDPTE             0x06440000
#define OP_IOCSRRD_B         0x06480000
#define OP_IOCSRRD_H         0x06480400
#define OP_IOCSRRD_W         0x06480800
#define OP_IOCSRRD_D         0x06480c00
#define OP_IOCSRWR_B         0x06481000
#define OP_IOCSRWR_H         0x06481400
#define OP_IOCSRWR_W         0x06481800
#define OP_IOCSRWR_D         0x06481c00
#define OP_TLBCLR            0x06482000
#define OP_TLBFLUSH          0x06482400
#define OP_TLBSRCH           0x06482800
#define OP_TLBRD             0x06482c00
#define OP_TLBWR             0x06483000
#define OP_TLBFILL           0x06483400
#define OP_ERTN              0x06483800
#define OP_IDLE              0x06488000
#define OP_INVTLB            0x06498000

/* loongarch 4opt single float insn opcode  */
#define OP_FMADD_S           0x08100000
#define OP_FMSUB_S           0x08500000
#define OP_FNMADD_S          0x08900000
#define OP_FNMSUB_S          0x08d00000
#define OP_FCMP_CAF_S        0x0c100000
#define OP_FCMP_SAF_S        0x0c108000
#define OP_FCMP_CLT_S        0x0c110000
#define OP_FCMP_SLT_S        0x0c118000
#define OP_FCMP_SGT_S        0x0c118000
#define OP_FCMP_CEQ_S        0x0c120000
#define OP_FCMP_SEQ_S        0x0c128000
#define OP_FCMP_CLE_S        0x0c130000
#define OP_FCMP_SLE_S        0x0c138000
#define OP_FCMP_SGE_S        0x0c138000
#define OP_FCMP_CUN_S        0x0c140000
#define OP_FCMP_SUN_S        0x0c148000
#define OP_FCMP_CULT_S       0x0c150000
#define OP_FCMP_CUGT_S       0x0c150000
#define OP_FCMP_SULT_S       0x0c158000
#define OP_FCMP_CUEQ_S       0x0c160000
#define OP_FCMP_SUEQ_S       0x0c168000
#define OP_FCMP_CULE_S       0x0c170000
#define OP_FCMP_CUGE_S       0x0c170000
#define OP_FCMP_SULE_S       0x0c178000
#define OP_FCMP_CNE_S        0x0c180000
#define OP_FCMP_SNE_S        0x0c188000
#define OP_FCMP_COR_S        0x0c1a0000
#define OP_FCMP_SOR_S        0x0c1a8000
#define OP_FCMP_CUNE_S       0x0c1c0000
#define OP_FCMP_SUNE_S       0x0c1c8000
#define OP_FSEL              0x0d000000

/* loongarch 4opt double float insn opcode  */
#define OP_FMADD_D           0x08200000
#define OP_FMSUB_D           0x08600000
#define OP_FNMADD_D          0x08a00000
#define OP_FNMSUB_D          0x08e00000
#define OP_FCMP_CAF_D        0x0c200000
#define OP_FCMP_SAF_D        0x0c208000
#define OP_FCMP_CLT_D        0x0c210000
#define OP_FCMP_SLT_D        0x0c218000
#define OP_FCMP_SGT_D        0x0c218000
#define OP_FCMP_CEQ_D        0x0c220000
#define OP_FCMP_SEQ_D        0x0c228000
#define OP_FCMP_CLE_D        0x0c230000
#define OP_FCMP_SLE_D        0x0c238000
#define OP_FCMP_SGE_D        0x0c238000
#define OP_FCMP_CUN_D        0x0c240000
#define OP_FCMP_SUN_D        0x0c248000
#define OP_FCMP_CULT_D       0x0c250000
#define OP_FCMP_CUGT_D       0x0c250000
#define OP_FCMP_SULT_D       0x0c258000
#define OP_FCMP_CUEQ_D       0x0c260000
#define OP_FCMP_SUEQ_D       0x0c268000
#define OP_FCMP_CULE_D       0x0c270000
#define OP_FCMP_CUGE_D       0x0c270000
#define OP_FCMP_SULE_D       0x0c278000
#define OP_FCMP_CNE_D        0x0c280000
#define OP_FCMP_SNE_D        0x0c288000
#define OP_FCMP_COR_D        0x0c2a0000
#define OP_FCMP_SOR_D        0x0c2a8000
#define OP_FCMP_CUNE_D       0x0c2c0000
#define OP_FCMP_SUNE_D       0x0c2c8000

/* loongarch load store insn opcode  */
#define OP_LL_W              0x20000000
#define OP_SC_W              0x21000000
#define OP_LL_D              0x22000000
#define OP_SC_D              0x23000000
#define OP_LDPTR_W           0x24000000
#define OP_STPTR_W           0x25000000
#define OP_LDPTR_D           0x26000000
#define OP_STPTR_D           0x27000000
#define OP_LD_B              0x28000000
#define OP_LD_H              0x28400000
#define OP_LD_W              0x28800000
#define OP_LD_D              0x28c00000
#define OP_ST_B              0x29000000
#define OP_ST_H              0x29400000
#define OP_ST_W              0x29800000
#define OP_ST_D              0x29c00000
#define OP_LD_BU             0x2a000000
#define OP_LD_HU             0x2a400000
#define OP_LD_WU             0x2a800000
#define OP_PRELD             0x2ac00000
#define OP_LDX_B             0x38000000
#define OP_LDX_H             0x38040000
#define OP_LDX_W             0x38080000
#define OP_LDX_D             0x380c0000
#define OP_STX_B             0x38100000
#define OP_STX_H             0x38140000
#define OP_STX_W             0x38180000
#define OP_STX_D             0x381c0000
#define OP_LDX_BU            0x38200000
#define OP_LDX_HU            0x38240000
#define OP_LDX_WU            0x38280000
#define OP_PRELDX            0x382c0000
#define OP_SC_Q              0x38570000
#define OP_LLACQ_W           0x38578000
#define OP_SCREL_W           0x38578400
#define OP_LLACQ_D           0x38578800
#define OP_SCREL_D           0x38578c00
#define OP_AMCAS_B           0x38580000
#define OP_AMCAS_H           0x38588000
#define OP_AMCAS_W           0x38590000
#define OP_AMCAS_D           0x38598000
#define OP_AMCAS_DB_B        0x385a0000
#define OP_AMCAS_DB_H        0x385a8000
#define OP_AMCAS_DB_W        0x385b0000
#define OP_AMCAS_DB_D        0x385b8000
#define OP_AMSWAP_B          0x385c0000
#define OP_AMSWAP_H          0x385c8000
#define OP_AMADD_B           0x385d0000
#define OP_AMADD_H           0x385d8000
#define OP_AMSWAP_DB_B       0x385e0000
#define OP_AMSWAP_DB_H       0x385e8000
#define OP_AMADD_DB_B        0x385f0000
#define OP_AMADD_DB_H        0x385f8000
#define OP_AMSWAP_W          0x38600000
#define OP_AMSWAP_D          0x38608000
#define OP_AMADD_W           0x38610000
#define OP_AMADD_D           0x38618000
#define OP_AMAND_W           0x38620000
#define OP_AMAND_D           0x38628000
#define OP_AMOR_W            0x38630000
#define OP_AMOR_D            0x38638000
#define OP_AMXOR_W           0x38640000
#define OP_AMXOR_D           0x38648000
#define OP_AMMAX_W           0x38650000
#define OP_AMMAX_D           0x38658000
#define OP_AMMIN_W           0x38660000
#define OP_AMMIN_D           0x38668000
#define OP_AMMAX_WU          0x38670000
#define OP_AMMAX_DU          0x38678000
#define OP_AMMIN_WU          0x38680000
#define OP_AMMIN_DU          0x38688000
#define OP_AMSWAP_DB_W       0x38690000
#define OP_AMSWAP_DB_D       0x38698000
#define OP_AMADD_DB_W        0x386a0000
#define OP_AMADD_DB_D        0x386a8000
#define OP_AMAND_DB_W        0x386b0000
#define OP_AMAND_DB_D        0x386b8000
#define OP_AMOR_DB_W         0x386c0000
#define OP_AMOR_DB_D         0x386c8000
#define OP_AMXOR_DB_W        0x386d0000
#define OP_AMXOR_DB_D        0x386d8000
#define OP_AMMAX_DB_W        0x386e0000
#define OP_AMMAX_DB_D        0x386e8000
#define OP_AMMIN_DB_W        0x386f0000
#define OP_AMMIN_DB_D        0x386f8000
#define OP_AMMAX_DB_WU       0x38700000
#define OP_AMMAX_DB_DU       0x38708000
#define OP_AMMIN_DB_WU       0x38710000
#define OP_AMMIN_DB_DU       0x38718000
#define OP_DBAR              0x38720000
#define OP_IBAR              0x38728000
#define OP_LDGT_B            0x38780000
#define OP_LDGT_H            0x38788000
#define OP_LDGT_W            0x38790000
#define OP_LDGT_D            0x38798000
#define OP_LDLE_B            0x387a0000
#define OP_LDLE_H            0x387a8000
#define OP_LDLE_W            0x387b0000
#define OP_LDLE_D            0x387b8000
#define OP_STGT_B            0x387c0000
#define OP_STGT_H            0x387c8000
#define OP_STGT_W            0x387d0000
#define OP_STGT_D            0x387d8000
#define OP_STLE_B            0x387e0000
#define OP_STLE_H            0x387e8000
#define OP_STLE_W            0x387f0000
#define OP_STLE_D            0x387f8000
#define OP_VLD               0x2c000000
#define OP_VST               0x2c400000
#define OP_XVLD              0x2c800000
#define OP_XVST              0x2cc00000

/* loongarch single float load store insn opcode  */
#define OP_FLD_S             0x2b000000
#define OP_FST_S             0x2b400000
#define OP_FLDX_S            0x38300000
#define OP_FSTX_S            0x38380000
#define OP_FLDGT_S           0x38740000
#define OP_FLDLE_S           0x38750000
#define OP_FSTGT_S           0x38760000
#define OP_FSTLE_S           0x38770000

/* loongarch double float load store insn opcode  */
#define OP_FLD_D             0x2b800000
#define OP_FST_D             0x2bc00000
#define OP_FLDX_D            0x38340000
#define OP_FSTX_D            0x383c0000
#define OP_FLDGT_D           0x38748000
#define OP_FLDLE_D           0x38758000
#define OP_FSTGT_D           0x38768000
#define OP_FSTLE_D           0x38778000

/* loongarch float jmp insn opcode  */
#define OP_BCEQZ             0x48000000
#define OP_BCNEZ             0x48000100

/* loongarch jmp insn opcode  */
#define OP_BEQZ              0x40000000
#define OP_BNEZ              0x44000000
#define OP_JIRL              0x4c000000
#define OP_B                 0x50000000
#define OP_BL                0x54000000
#define OP_BEQ               0x58000000
#define OP_BNE               0x5c000000
#define OP_BLT               0x60000000
#define OP_BGE               0x64000000
#define OP_BLTU              0x68000000
#define OP_BGEU              0x6c000000

/* loongarch fix insn mask  */
#define MASK_CLO_W           0xfffffc00
#define MASK_CLZ_W           0xfffffc00
#define MASK_CTO_W           0xfffffc00
#define MASK_CTZ_W           0xfffffc00
#define MASK_CLO_D           0xfffffc00
#define MASK_CLZ_D           0xfffffc00
#define MASK_CTO_D           0xfffffc00
#define MASK_CTZ_D           0xfffffc00
#define MASK_REVB_2H         0xfffffc00
#define MASK_REVB_4H         0xfffffc00
#define MASK_REVB_2W         0xfffffc00
#define MASK_REVB_D          0xfffffc00
#define MASK_REVH_2W         0xfffffc00
#define MASK_REVH_D          0xfffffc00
#define MASK_BITREV_4B       0xfffffc00
#define MASK_BITREV_8B       0xfffffc00
#define MASK_BITREV_W        0xfffffc00
#define MASK_BITREV_D        0xfffffc00
#define MASK_EXT_W_H         0xfffffc00
#define MASK_EXT_W_B         0xfffffc00
#define MASK_RDTIMEL_W       0xfffffc00
#define MASK_RDTIMEH_W       0xfffffc00
#define MASK_RDTIME_D        0xfffffc00
#define MASK_CPUCFG          0xfffffc00
#define MASK_ASRTLE_D        0xffff801f
#define MASK_ASRTGT_D        0xffff801f
#define MASK_ALSL_W          0xfffe0000
#define MASK_ALSL_WU         0xfffe0000
#define MASK_BYTEPICK_W      0xfffe0000
#define MASK_BYTEPICK_D      0xfffc0000
#define MASK_ADD_W           0xffff8000
#define MASK_ADD_D           0xffff8000
#define MASK_SUB_W           0xffff8000
#define MASK_SUB_D           0xffff8000
#define MASK_SLT             0xffff8000
#define MASK_SLTU            0xffff8000
#define MASK_MASKEQZ         0xffff8000
#define MASK_MASKNEZ         0xffff8000
#define MASK_NOR             0xffff8000
#define MASK_AND             0xffff8000
#define MASK_OR              0xffff8000
#define MASK_XOR             0xffff8000
#define MASK_ORN             0xffff8000
#define MASK_ANDN            0xffff8000
#define MASK_SLL_W           0xffff8000
#define MASK_SRL_W           0xffff8000
#define MASK_SRA_W           0xffff8000
#define MASK_SLL_D           0xffff8000
#define MASK_SRL_D           0xffff8000
#define MASK_SRA_D           0xffff8000
#define MASK_ROTR_W          0xffff8000
#define MASK_ROTR_D          0xffff8000
#define MASK_MUL_W           0xffff8000
#define MASK_MULH_W          0xffff8000
#define MASK_MULH_WU         0xffff8000
#define MASK_MUL_D           0xffff8000
#define MASK_MULH_D          0xffff8000
#define MASK_MULH_DU         0xffff8000
#define MASK_MULW_D_W        0xffff8000
#define MASK_MULW_D_WU       0xffff8000
#define MASK_DIV_W           0xffff8000
#define MASK_MOD_W           0xffff8000
#define MASK_DIV_WU          0xffff8000
#define MASK_MOD_WU          0xffff8000
#define MASK_DIV_D           0xffff8000
#define MASK_MOD_D           0xffff8000
#define MASK_DIV_DU          0xffff8000
#define MASK_MOD_DU          0xffff8000
#define MASK_CRC_W_B_W       0xffff8000
#define MASK_CRC_W_H_W       0xffff8000
#define MASK_CRC_W_W_W       0xffff8000
#define MASK_CRC_W_D_W       0xffff8000
#define MASK_CRCC_W_B_W      0xffff8000
#define MASK_CRCC_W_H_W      0xffff8000
#define MASK_CRCC_W_W_W      0xffff8000
#define MASK_CRCC_W_D_W      0xffff8000
#define MASK_BREAK           0xffff8000
#define MASK_DBCL            0xffff8000
#define MASK_SYSCALL         0xffff8000
#define MASK_ALSL_D          0xfffe0000
#define MASK_SLLI_W          0xffff8000
#define MASK_SLLI_D          0xffff0000
#define MASK_SRLI_W          0xffff8000
#define MASK_SRLI_D          0xffff0000
#define MASK_SRAI_W          0xffff8000
#define MASK_SRAI_D          0xffff0000
#define MASK_ROTRI_W         0xffff8000
#define MASK_ROTRI_D         0xffff0000
#define MASK_BSTRINS_W       0xffe08000
#define MASK_BSTRPICK_W      0xffe08000
#define MASK_BSTRINS_D       0xffc00000
#define MASK_BSTRPICK_D      0xffc00000

/* loongarch single float insn mask  */
#define MASK_FADD_S          0xffff8000
#define MASK_SUB_S           0xffff8000
#define MASK_MUL_S           0xffff8000
#define MASK_FDIV_S          0xffff8000
#define MASK_FMAX_S          0xffff8000
#define MASK_FMIN_S          0xffff8000
#define MASK_FMAXA_S         0xffff8000
#define MASK_FMINA_S         0xffff8000
#define MASK_FSCALEB_S       0xffff8000
#define MASK_FCOPYSIGN_S     0xffff8000
#define MASK_FABS_S          0xfffffc00
#define MASK_FNEG_S          0xfffffc00
#define MASK_FLOGB_S         0xfffffc00
#define MASK_FCLASS_S        0xfffffc00
#define MASK_FSQRT_S         0xfffffc00
#define MASK_FRECIP_S        0xfffffc00
#define MASK_FRSQRT_S        0xfffffc00
#define MASK_FRECIPE_S       0xfffffc00
#define MASK_FRSQRTE_S       0xfffffc00
#define MASK_FMOV_S          0xfffffc00
#define MASK_MOVGR2FR_W      0xfffffc00
#define MASK_MOVGR2FRH_W     0xfffffc00
#define MASK_MOVFR2GR_S      0xfffffc00
#define MASK_MOVFRH2GR_S     0xfffffc00
#define MASK_MOVGR2FCSR      0xfffffc1c
#define MASK_MOVFCSR2GR      0xffffff80
#define MASK_MOVFR2CF        0xfffffc18
#define MASK_MOVCF2FR        0xffffff00
#define MASK_MOVGR2CF        0xfffffc18
#define MASK_MOVCF2GR        0xffffff00
#define MASK_FTINTRM_W_S     0xfffffc00
#define MASK_FTINTRM_L_S     0xfffffc00
#define MASK_FTINTRP_W_S     0xfffffc00
#define MASK_FTINTRP_L_S     0xfffffc00
#define MASK_FTINTRZ_W_S     0xfffffc00
#define MASK_FTINTRZ_L_S     0xfffffc00
#define MASK_FTINTRNE_W_S    0xfffffc00
#define MASK_FTINTRNE_L_S    0xfffffc00
#define MASK_FTINT_W_S       0xfffffc00
#define MASK_FTINT_L_S       0xfffffc00
#define MASK_FFINT_S_W       0xfffffc00
#define MASK_FFINT_S_L       0xfffffc00
#define MASK_FRINT_S         0xfffffc00

/* loongarch double float insn mask  */
#define MASK_FADD_D          0xffff8000
#define MASK_FSUB_D          0xffff8000
#define MASK_FMUL_D          0xffff8000
#define MASK_FDIV_D          0xffff8000
#define MASK_FMAX_D          0xffff8000
#define MASK_FMIN_D          0xffff8000
#define MASK_FMAXA_D         0xffff8000
#define MASK_FMINA_D         0xffff8000
#define MASK_FSCALEB_D       0xffff8000
#define MASK_FCOPYSIGN_D     0xffff8000
#define MASK_FABS_D          0xfffffc00
#define MASK_FNEG_D          0xfffffc00
#define MASK_FLOGB_D         0xfffffc00
#define MASK_FCLASS_D        0xfffffc00
#define MASK_FSQRT_D         0xfffffc00
#define MASK_FRECIP_D        0xfffffc00
#define MASK_FRSQRT_D        0xfffffc00
#define MASK_FRECIPE_D       0xfffffc00
#define MASK_FRSQRTE_D       0xfffffc00
#define MASK_FMOV_D          0xfffffc00
#define MASK_MOVGR2FR_D      0xfffffc00
#define MASK_MOVFR2GR_D      0xfffffc00
#define MASK_FCVT_S_D        0xfffffc00
#define MASK_FCVT_D_S        0xfffffc00
#define MASK_FTINTRM_W_D     0xfffffc00
#define MASK_FTINTRM_L_D     0xfffffc00
#define MASK_FTINTRP_W_D     0xfffffc00
#define MASK_FTINTRP_L_D     0xfffffc00
#define MASK_FTINTRZ_W_D     0xfffffc00
#define MASK_FTINTRZ_L_D     0xfffffc00
#define MASK_FTINTRNE_W_D    0xfffffc00
#define MASK_FTINTRNE_L_D    0xfffffc00
#define MASK_FTINT_W_D       0xfffffc00
#define MASK_FTINT_L_D       0xfffffc00
#define MASK_FFINT_D_W       0xfffffc00
#define MASK_FFINT_D_L       0xfffffc00
#define MASK_FRINT_D         0xfffffc00

/* loongarch imm insn mask  */
#define MASK_SLTI            0xfffffc00
#define MASK_SLTUI           0xffc00000
#define MASK_ADDI_W          0xffc00000
#define MASK_ADDI_D          0xffc00000
#define MASK_LU52I_D         0xffc00000
#define MASK_ANDI            0xffc00000
#define MASK_ORI             0xffc00000
#define MASK_XORI            0xffc00000
#define MASK_ADDU16I_D       0xfc000000
#define MASK_LU12I_W         0xfe000000
#define MASK_LU32I_D         0xfe000000
#define MASK_PCADDI          0xfe000000
#define MASK_PCALAU12I       0xfe000000
#define MASK_PCADDU12I       0xfe000000
#define MASK_PCADDU18I       0xfe000000

/* loongarch privilege insn mask  */
#define MASK_CSRRD           0xff0003e0
#define MASK_CSRWR           0xff0003e0
#define MASK_CSRXCHG         0xff000000
#define MASK_CACOP           0xffc00000
#define MASK_LDDIR           0xfffc0000
#define MASK_LDPTE           0xfffc001f
#define MASK_IOCSRRD_B       0xfffffc00
#define MASK_IOCSRRD_H       0xfffffc00
#define MASK_IOCSRRD_W       0xfffffc00
#define MASK_IOCSRRD_D       0xfffffc00
#define MASK_IOCSRWR_B       0xfffffc00
#define MASK_IOCSRWR_H       0xfffffc00
#define MASK_IOCSRWR_W       0xfffffc00
#define MASK_IOCSRWR_D       0xfffffc00
#define MASK_TLBCLR          0xffffffff
#define MASK_TLBFLUSH        0xffffffff
#define MASK_TLBSRCH         0xffffffff
#define MASK_TLBRD           0xffffffff
#define MASK_TLBWR           0xffffffff
#define MASK_TLBFILL         0xffffffff
#define MASK_ERTN            0xffffffff
#define MASK_IDLE            0xffff8000
#define MASK_INVTLB          0xffff8000

/* loongarch 4opt single float insn mask  */
#define MASK_FMADD_S         0xfff00000
#define MASK_FMSUB_S         0xfff00000
#define MASK_FNMADD_S        0xfff00000
#define MASK_FNMSUB_S        0xfff00000
#define MASK_FCMP_CAF_S      0xffff8018
#define MASK_FCMP_SAF_S      0xffff8018
#define MASK_FCMP_CLT_S      0xffff8018
#define MASK_FCMP_SLT_S      0xffff8018
#define MASK_FCMP_SGT_S      0xffff8018
#define MASK_FCMP_CEQ_S      0xffff8018
#define MASK_FCMP_SEQ_S      0xffff8018
#define MASK_FCMP_CLE_S      0xffff8018
#define MASK_FCMP_SLE_S      0xffff8018
#define MASK_FCMP_SGE_S      0xffff8018
#define MASK_FCMP_CUN_S      0xffff8018
#define MASK_FCMP_SUN_S      0xffff8018
#define MASK_FCMP_CULT_S     0xffff8018
#define MASK_FCMP_CUGT_S     0xffff8018
#define MASK_FCMP_SULT_S     0xffff8018
#define MASK_FCMP_CUEQ_S     0xffff8018
#define MASK_FCMP_SUEQ_S     0xffff8018
#define MASK_FCMP_CULE_S     0xffff8018
#define MASK_FCMP_CUGE_S     0xffff8018
#define MASK_FCMP_SULE_S     0xffff8018
#define MASK_FCMP_CNE_S      0xffff8018
#define MASK_FCMP_SNE_S      0xffff8018
#define MASK_FCMP_COR_S      0xffff8018
#define MASK_FCMP_SOR_S      0xffff8018
#define MASK_FCMP_CUNE_S     0xffff8018
#define MASK_FCMP_SUNE_S     0xffff8018
#define MASK_FSEL            0xfffc0000

/* loongarch 4opt double float insn mask  */
#define MASK_FMADD_D         0xfff00000
#define MASK_FMSUB_D         0xfff00000
#define MASK_FNMADD_D        0xfff00000
#define MASK_FNMSUB_D        0xfff00000
#define MASK_FCMP_CAF_D      0xffff8018
#define MASK_FCMP_SAF_D      0xffff8018
#define MASK_FCMP_CLT_D      0xffff8018
#define MASK_FCMP_SLT_D      0xffff8018
#define MASK_FCMP_SGT_D      0xffff8018
#define MASK_FCMP_CEQ_D      0xffff8018
#define MASK_FCMP_SEQ_D      0xffff8018
#define MASK_FCMP_CLE_D      0xffff8018
#define MASK_FCMP_SLE_D      0xffff8018
#define MASK_FCMP_SGE_D      0xffff8018
#define MASK_FCMP_CUN_D      0xffff8018
#define MASK_FCMP_SUN_D      0xffff8018
#define MASK_FCMP_CULT_D     0xffff8018
#define MASK_FCMP_CUGT_D     0xffff8018
#define MASK_FCMP_SULT_D     0xffff8018
#define MASK_FCMP_CUEQ_D     0xffff8018
#define MASK_FCMP_SUEQ_D     0xffff8018
#define MASK_FCMP_CULE_D     0xffff8018
#define MASK_FCMP_CUGE_D     0xffff8018
#define MASK_FCMP_SULE_D     0xffff8018
#define MASK_FCMP_CNE_D      0xffff8018
#define MASK_FCMP_SNE_D      0xffff8018
#define MASK_FCMP_COR_D      0xffff8018
#define MASK_FCMP_SOR_D      0xffff8018
#define MASK_FCMP_CUNE_D     0xffff8018
#define MASK_FCMP_SUNE_D     0xffff8018

/* loongarch load store insn mask  */
#define MASK_LL_W            0xff000000
#define MASK_SC_W            0xff000000
#define MASK_LL_D            0xff000000
#define MASK_SC_D            0xff000000
#define MASK_LDPTR_W         0xff000000
#define MASK_STPTR_W         0xff000000
#define MASK_LDPTR_D         0xff000000
#define MASK_STPTR_D         0xff000000
#define MASK_LD_B            0xffc00000
#define MASK_LD_H            0xffc00000
#define MASK_LD_W            0xffc00000
#define MASK_LD_D            0xffc00000
#define MASK_ST_B            0xffc00000
#define MASK_ST_H            0xffc00000
#define MASK_ST_W            0xffc00000
#define MASK_ST_D            0xffc00000
#define MASK_LD_BU           0xffc00000
#define MASK_LD_HU           0xffc00000
#define MASK_LD_WU           0xffc00000
#define MASK_PRELD           0xffc00000
#define MASK_LDX_B           0xffff8000
#define MASK_LDX_H           0xffff8000
#define MASK_LDX_W           0xffff8000
#define MASK_LDX_D           0xffff8000
#define MASK_STX_B           0xffff8000
#define MASK_STX_H           0xffff8000
#define MASK_STX_W           0xffff8000
#define MASK_STX_D           0xffff8000
#define MASK_LDX_BU          0xffff8000
#define MASK_LDX_HU          0xffff8000
#define MASK_LDX_WU          0xffff8000
#define MASK_PRELDX          0xffff8000
#define MASK_SC_Q            0xffff8000
#define MASK_LLACQ_W         0xfffffc00
#define MASK_SCREL_W         0xfffffc00
#define MASK_LLACQ_D         0xfffffc00
#define MASK_SCREL_D         0xfffffc00
#define MASK_AMCAS_B         0xffff8000
#define MASK_AMCAS_H         0xffff8000
#define MASK_AMCAS_W         0xffff8000
#define MASK_AMCAS_D         0xffff8000
#define MASK_AMCAS_DB_B      0xffff8000
#define MASK_AMCAS_DB_H      0xffff8000
#define MASK_AMCAS_DB_W      0xffff8000
#define MASK_AMCAS_DB_D      0xffff8000
#define MASK_AMSWAP_B        0xffff8000
#define MASK_AMSWAP_H        0xffff8000
#define MASK_AMADD_B         0xffff8000
#define MASK_AMADD_H         0xffff8000
#define MASK_AMSWAP_DB_B     0xffff8000
#define MASK_AMSWAP_DB_H     0xffff8000
#define MASK_AMADD_DB_B      0xffff8000
#define MASK_AMADD_DB_H      0xffff8000
#define MASK_AMSWAP_W        0xffff8000
#define MASK_AMSWAP_D        0xffff8000
#define MASK_AMADD_W         0xffff8000
#define MASK_AMADD_D         0xffff8000
#define MASK_AMAND_W         0xffff8000
#define MASK_AMAND_D         0xffff8000
#define MASK_AMOR_W          0xffff8000
#define MASK_AMOR_D          0xffff8000
#define MASK_AMXOR_W         0xffff8000
#define MASK_AMXOR_D         0xffff8000
#define MASK_AMMAX_W         0xffff8000
#define MASK_AMMAX_D         0xffff8000
#define MASK_AMMIN_W         0xffff8000
#define MASK_AMMIN_D         0xffff8000
#define MASK_AMMAX_WU        0xffff8000
#define MASK_AMMAX_DU        0xffff8000
#define MASK_AMMIN_WU        0xffff8000
#define MASK_AMMIN_DU        0xffff8000
#define MASK_AMSWAP_DB_W     0xffff8000
#define MASK_AMSWAP_DB_D     0xffff8000
#define MASK_AMADD_DB_W      0xffff8000
#define MASK_AMADD_DB_D      0xffff8000
#define MASK_AMAND_DB_W      0xffff8000
#define MASK_AMAND_DB_D      0xffff8000
#define MASK_AMOR_DB_W       0xffff8000
#define MASK_AMOR_DB_D       0xffff8000
#define MASK_AMXOR_DB_W      0xffff8000
#define MASK_AMXOR_DB_D      0xffff8000
#define MASK_AMMAX_DB_W      0xffff8000
#define MASK_AMMAX_DB_D      0xffff8000
#define MASK_AMMIN_DB_W      0xffff8000
#define MASK_AMMIN_DB_D      0xffff8000
#define MASK_AMMAX_DB_WU     0xffff8000
#define MASK_AMMAX_DB_DU     0xffff8000
#define MASK_AMMIN_DB_WU     0xffff8000
#define MASK_AMMIN_DB_DU     0xffff8000
#define MASK_DBAR            0xffff8000
#define MASK_IBAR            0xffff8000
#define MASK_LDGT_B          0xffff8000
#define MASK_LDGT_H          0xffff8000
#define MASK_LDGT_W          0xffff8000
#define MASK_LDGT_D          0xffff8000
#define MASK_LDLE_B          0xffff8000
#define MASK_LDLE_H          0xffff8000
#define MASK_LDLE_W          0xffff8000
#define MASK_LDLE_D          0xffff8000
#define MASK_STGT_B          0xffff8000
#define MASK_STGT_H          0xffff8000
#define MASK_STGT_W          0xffff8000
#define MASK_STGT_D          0xffff8000
#define MASK_STLE_B          0xffff8000
#define MASK_STLE_H          0xffff8000
#define MASK_STLE_W          0xffff8000
#define MASK_STLE_D          0xffff8000
#define MASK_VLD             0xffc00000
#define MASK_VST             0xffc00000
#define MASK_XVLD            0xffc00000
#define MASK_XVST            0xffc00000

/* loongarch single float load store insn mask  */
#define MASK_FLD_S           0xffc00000
#define MASK_FST_S           0xffc00000
#define MASK_FLDX_S          0xffff8000
#define MASK_FSTX_S          0xffff8000
#define MASK_FLDGT_S         0xffff8000
#define MASK_FLDLE_S         0xffff8000
#define MASK_FSTGT_S         0xffff8000
#define MASK_FSTLE_S         0xffff8000

/* loongarch double float load store insn mask  */
#define MASK_FLD_D           0xffc00000
#define MASK_FST_D           0xffc00000
#define MASK_FLDX_D          0xffff8000
#define MASK_FSTX_D          0xffff8000
#define MASK_FLDGT_D         0xffff8000
#define MASK_FLDLE_D         0xffff8000
#define MASK_FSTGT_D         0xffff8000
#define MASK_FSTLE_D         0xffff8000

/* loongarch float jmp insn mask  */
#define MASK_BCEQZ           0xfc000300
#define MASK_BCNEZ           0xfc000300

/* loongarch jmp insn mask  */
#define MASK_BEQZ            0xfc000000
#define MASK_BNEZ            0xfc000000
#define MASK_JIRL            0xfc000000
#define MASK_B               0xfc000000
#define MASK_BL              0xfc000000
#define MASK_BEQ             0xfc000000
#define MASK_BNE             0xfc000000
#define MASK_BLT             0xfc000000
#define MASK_BGE             0xfc000000
#define MASK_BLTU            0xfc000000
#define MASK_BGEU            0xfc000000

/* Define a series of is_XXX_insn functions to check if the value INSN
   is an instance of instruction XXX.  */
#define DECLARE_INSN(INSN_NAME, INSN_OPCODE, INSN_MASK) \
static inline bool is_ ## INSN_NAME ## _insn (uint32_t insn) \
{ \
  return (insn & INSN_MASK) == INSN_OPCODE; \
}

/* loongarch fix instruction */
DECLARE_INSN(clo_w, OP_CLO_W, MASK_CLO_W)
DECLARE_INSN(clz_w, OP_CLZ_W, MASK_CLZ_W)
DECLARE_INSN(cto_w, OP_CTO_W, MASK_CTO_W)
DECLARE_INSN(ctz_w, OP_CTZ_W, MASK_CTZ_W)
DECLARE_INSN(clo_d, OP_CLO_D, MASK_CLO_D)
DECLARE_INSN(clz_d, OP_CLZ_D, MASK_CLZ_D)
DECLARE_INSN(cto_d, OP_CTO_D, MASK_CTO_D)
DECLARE_INSN(ctz_d, OP_CTZ_D, MASK_CTZ_D)
DECLARE_INSN(revb_2h, OP_REVB_2H, MASK_REVB_2H)
DECLARE_INSN(revb_4h, OP_REVB_4H, MASK_REVB_4H)
DECLARE_INSN(revb_2w, OP_REVB_2W, MASK_REVB_2W)
DECLARE_INSN(revb_d, OP_REVB_D, MASK_REVB_D)
DECLARE_INSN(revh_2w, OP_REVH_2W, MASK_REVH_2W)
DECLARE_INSN(revh_d, OP_REVH_D, MASK_REVH_D)
DECLARE_INSN(bitrev_4b, OP_BITREV_4B, MASK_BITREV_4B)
DECLARE_INSN(bitrev_8b, OP_BITREV_8B, MASK_BITREV_8B)
DECLARE_INSN(bitrev_w, OP_BITREV_W, MASK_BITREV_W)
DECLARE_INSN(bitrev_d, OP_BITREV_D, MASK_BITREV_D)
DECLARE_INSN(ext_w_h, OP_EXT_W_H, MASK_EXT_W_H)
DECLARE_INSN(ext_w_b, OP_EXT_W_B, MASK_EXT_W_B)
DECLARE_INSN(rdtimel_w, OP_RDTIMEL_W, MASK_RDTIMEL_W)
DECLARE_INSN(rdtimeh_w, OP_RDTIMEH_W, MASK_RDTIMEH_W)
DECLARE_INSN(rdtime_d, OP_RDTIME_D, MASK_RDTIME_D)
DECLARE_INSN(cpucfg, OP_CPUCFG, MASK_CPUCFG)
DECLARE_INSN(asrtle_d, OP_ASRTLE_D, MASK_ASRTLE_D)
DECLARE_INSN(asrtgt_d, OP_ASRTGT_D, MASK_ASRTGT_D)
DECLARE_INSN(alsl_w, OP_ALSL_W, MASK_ALSL_W)
DECLARE_INSN(alsl_wu, OP_ALSL_WU, MASK_ALSL_WU)
DECLARE_INSN(bytepick_w, OP_BYTEPICK_W, MASK_BYTEPICK_W)
DECLARE_INSN(bytepick_d, OP_BYTEPICK_D, MASK_BYTEPICK_D)
DECLARE_INSN(add_w, OP_ADD_W, MASK_ADD_W)
DECLARE_INSN(add_d, OP_ADD_D, MASK_ADD_D)
DECLARE_INSN(sub_w, OP_SUB_W, MASK_SUB_W)
DECLARE_INSN(sub_d, OP_SUB_D, MASK_SUB_D)
DECLARE_INSN(slt, OP_SLT, MASK_SLT)
DECLARE_INSN(sltu, OP_SLTU, MASK_SLTU)
DECLARE_INSN(maskeqz, OP_MASKEQZ, MASK_MASKEQZ)
DECLARE_INSN(masknez, OP_MASKNEZ, MASK_MASKNEZ)
DECLARE_INSN(nor, OP_NOR, MASK_NOR)
DECLARE_INSN(and, OP_AND, MASK_AND)
DECLARE_INSN(or, OP_OR, MASK_OR)
DECLARE_INSN(xor, OP_XOR, MASK_XOR)
DECLARE_INSN(orn, OP_ORN, MASK_ORN)
DECLARE_INSN(andn, OP_ANDN, MASK_ANDN)
DECLARE_INSN(sll_w, OP_SLL_W, MASK_SLL_W)
DECLARE_INSN(srl_w, OP_SRL_W, MASK_SRL_W)
DECLARE_INSN(sra_w, OP_SRA_W, MASK_SRA_W)
DECLARE_INSN(sll_d, OP_SLL_D, MASK_SLL_D)
DECLARE_INSN(srl_d, OP_SRL_D, MASK_SRL_D)
DECLARE_INSN(sra_d, OP_SRA_D, MASK_SRA_D)
DECLARE_INSN(rotr_w, OP_ROTR_W, MASK_ROTR_W)
DECLARE_INSN(rotr_d, OP_ROTR_D, MASK_ROTR_D)
DECLARE_INSN(mul_w, OP_MUL_W, MASK_MUL_W)
DECLARE_INSN(mulh_w, OP_MULH_W, MASK_MULH_W)
DECLARE_INSN(mulh_wu, OP_MULH_WU, MASK_MULH_WU)
DECLARE_INSN(mul_d, OP_MUL_D, MASK_MUL_D)
DECLARE_INSN(mulh_d, OP_MULH_D, MASK_MULH_D)
DECLARE_INSN(mulh_du, OP_MULH_DU, MASK_MULH_DU)
DECLARE_INSN(mulw_d_w, OP_MULW_D_W, MASK_MULW_D_W)
DECLARE_INSN(mulw_d_wu, OP_MULW_D_WU, MASK_MULW_D_WU)
DECLARE_INSN(div_w, OP_DIV_W, MASK_DIV_W)
DECLARE_INSN(mod_w, OP_MOD_W, MASK_MOD_W)
DECLARE_INSN(div_wu, OP_DIV_WU, MASK_DIV_WU)
DECLARE_INSN(mod_wu, OP_MOD_WU, MASK_MOD_WU)
DECLARE_INSN(div_d, OP_DIV_D, MASK_DIV_D)
DECLARE_INSN(mod_d, OP_MOD_D, MASK_MOD_D)
DECLARE_INSN(div_du, OP_DIV_DU, MASK_DIV_DU)
DECLARE_INSN(mod_du, OP_MOD_DU, MASK_MOD_DU)
DECLARE_INSN(crc_w_b_w, OP_CRC_W_B_W, MASK_CRC_W_B_W)
DECLARE_INSN(crc_w_h_w, OP_CRC_W_H_W, MASK_CRC_W_H_W)
DECLARE_INSN(crc_w_w_w, OP_CRC_W_W_W, MASK_CRC_W_W_W)
DECLARE_INSN(crc_w_d_w, OP_CRC_W_D_W, MASK_CRC_W_D_W)
DECLARE_INSN(crcc_w_b_w, OP_CRCC_W_B_W, MASK_CRCC_W_B_W)
DECLARE_INSN(crcc_w_h_w, OP_CRCC_W_H_W, MASK_CRCC_W_H_W)
DECLARE_INSN(crcc_w_w_w, OP_CRCC_W_W_W, MASK_CRCC_W_W_W)
DECLARE_INSN(crcc_w_d_w, OP_CRCC_W_D_W, MASK_CRCC_W_D_W)
DECLARE_INSN(break, OP_BREAK, MASK_BREAK)
DECLARE_INSN(dbcl, OP_DBCL, MASK_DBCL)
DECLARE_INSN(syscall, OP_SYSCALL, MASK_SYSCALL)
DECLARE_INSN(alsl_d, OP_ALSL_D, MASK_ALSL_D)
DECLARE_INSN(slli_w, OP_SLLI_W, MASK_SLLI_W)
DECLARE_INSN(slli_d, OP_SLLI_D, MASK_SLLI_D)
DECLARE_INSN(srli_w, OP_SRLI_W, MASK_SRLI_W)
DECLARE_INSN(srli_d, OP_SRLI_D, MASK_SRLI_D)
DECLARE_INSN(srai_w, OP_SRAI_W, MASK_SRAI_W)
DECLARE_INSN(srai_d, OP_SRAI_D, MASK_SRAI_D)
DECLARE_INSN(rotri_w, OP_ROTRI_W, MASK_ROTRI_W)
DECLARE_INSN(rotri_d, OP_ROTRI_D, MASK_ROTRI_D)
DECLARE_INSN(bstrins_w, OP_BSTRINS_W, MASK_BSTRINS_W)
DECLARE_INSN(bstrpick_w, OP_BSTRPICK_W, MASK_BSTRPICK_W)
DECLARE_INSN(bstrins_d, OP_BSTRINS_D, MASK_BSTRINS_D)
DECLARE_INSN(bstrpick_d, OP_BSTRPICK_D, MASK_BSTRPICK_D)

/* loongarch single float instruction  */
DECLARE_INSN(fadd_s, OP_FADD_S, MASK_FADD_S)
DECLARE_INSN(fsub_s, OP_SUB_S, MASK_SUB_S)
DECLARE_INSN(fmul_s, OP_MUL_S, MASK_MUL_S)
DECLARE_INSN(fdiv_s, OP_FDIV_S, MASK_FDIV_S)
DECLARE_INSN(fmax_s, OP_FMAX_S, MASK_FMAX_S)
DECLARE_INSN(fmin_s, OP_FMIN_S, MASK_FMIN_S)
DECLARE_INSN(fmaxa_s, OP_FMAXA_S, MASK_FMAXA_S)
DECLARE_INSN(fmina_s, OP_FMINA_S, MASK_FMINA_S)
DECLARE_INSN(fscaleb_s, OP_FSCALEB_S, MASK_FSCALEB_S)
DECLARE_INSN(fcopysign_s, OP_FCOPYSIGN_S, MASK_FCOPYSIGN_S)
DECLARE_INSN(fabs_s, OP_FABS_S, MASK_FABS_S)
DECLARE_INSN(fneg_s, OP_FNEG_S, MASK_FNEG_S)
DECLARE_INSN(flogb_s, OP_FLOGB_S, MASK_FLOGB_S)
DECLARE_INSN(fclass_s, OP_FCLASS_S, MASK_FCLASS_S)
DECLARE_INSN(fsqrt_s, OP_FSQRT_S, MASK_FSQRT_S)
DECLARE_INSN(frecip_s, OP_FRECIP_S, MASK_FRECIP_S)
DECLARE_INSN(frsqrt_s, OP_FRSQRT_S, MASK_FRSQRT_S)
DECLARE_INSN(frecipe_s, OP_FRECIPE_S, MASK_FRECIPE_S)
DECLARE_INSN(frsqrte_s, OP_FRSQRTE_S, MASK_FRSQRTE_S)
DECLARE_INSN(fmov_s, OP_FMOV_S, MASK_FMOV_S)
DECLARE_INSN(movgr2fr_w, OP_MOVGR2FR_W, MASK_MOVGR2FR_W)
DECLARE_INSN(movgr2frh_w, OP_MOVGR2FRH_W, MASK_MOVGR2FRH_W)
DECLARE_INSN(movfr2gr_s, OP_MOVFR2GR_S, MASK_MOVFR2GR_S)
DECLARE_INSN(movfrh2gr_s, OP_MOVFRH2GR_S, MASK_MOVFRH2GR_S)
DECLARE_INSN(movgr2fcsr, OP_MOVGR2FCSR, MASK_MOVGR2FCSR)
DECLARE_INSN(movfcsr2gr, OP_MOVFCSR2GR, MASK_MOVFCSR2GR)
DECLARE_INSN(movfr2cf, OP_MOVFR2CF, MASK_MOVFR2CF)
DECLARE_INSN(movcf2fr, OP_MOVCF2FR, MASK_MOVCF2FR)
DECLARE_INSN(movgr2cf, OP_MOVGR2CF, MASK_MOVGR2CF)
DECLARE_INSN(movcf2gr, OP_MOVCF2GR, MASK_MOVCF2GR)
DECLARE_INSN(ftintrm_w_s, OP_FTINTRM_W_S, MASK_FTINTRM_W_S)
DECLARE_INSN(ftintrm_l_s, OP_FTINTRM_L_S, MASK_FTINTRM_L_S)
DECLARE_INSN(ftintrp_w_s, OP_FTINTRP_W_S, MASK_FTINTRP_W_S)
DECLARE_INSN(ftintrp_l_s, OP_FTINTRP_L_S, MASK_FTINTRP_L_S)
DECLARE_INSN(ftintrz_w_s, OP_FTINTRZ_W_S, MASK_FTINTRZ_W_S)
DECLARE_INSN(ftintrz_l_s, OP_FTINTRZ_L_S, MASK_FTINTRZ_L_S)
DECLARE_INSN(ftintrne_w_s, OP_FTINTRNE_W_S, MASK_FTINTRNE_W_S)
DECLARE_INSN(ftintrne_l_s, OP_FTINTRNE_L_S, MASK_FTINTRNE_L_S)
DECLARE_INSN(ftint_w_s, OP_FTINT_W_S, MASK_FTINT_W_S)
DECLARE_INSN(ftint_l_s, OP_FTINT_L_S, MASK_FTINT_L_S)
DECLARE_INSN(ffint_s_w, OP_FFINT_S_W, MASK_FFINT_S_W)
DECLARE_INSN(ffint_s_l, OP_FFINT_S_L, MASK_FFINT_S_L)
DECLARE_INSN(frint_s, OP_FRINT_S, MASK_FRINT_S)

/* loongarch double float instruction */
DECLARE_INSN(fadd_d, OP_FADD_D, MASK_FADD_D)
DECLARE_INSN(fsub_d, OP_FSUB_D, MASK_FSUB_D)
DECLARE_INSN(fmul_d, OP_FMUL_D, MASK_FMUL_D)
DECLARE_INSN(fdiv_d, OP_FDIV_D, MASK_FDIV_D)
DECLARE_INSN(fmax_d, OP_FMAX_D, MASK_FMAX_D)
DECLARE_INSN(fmin_d, OP_FMIN_D, MASK_FMIN_D)
DECLARE_INSN(fmaxa_d, OP_FMAXA_D, MASK_FMAXA_D)
DECLARE_INSN(fmina_d, OP_FMINA_D, MASK_FMINA_D)
DECLARE_INSN(fscaleb_d, OP_FSCALEB_D, MASK_FSCALEB_D)
DECLARE_INSN(fcopysign_d, OP_FCOPYSIGN_D, MASK_FCOPYSIGN_D)
DECLARE_INSN(fabs_d, OP_FABS_D, MASK_FABS_D)
DECLARE_INSN(fneg_d, OP_FNEG_D, MASK_FNEG_D)
DECLARE_INSN(flogb_d, OP_FLOGB_D, MASK_FLOGB_D)
DECLARE_INSN(fclass_d, OP_FCLASS_D, MASK_FCLASS_D)
DECLARE_INSN(fsqrt_d, OP_FSQRT_D, MASK_FSQRT_D)
DECLARE_INSN(frecip_d, OP_FRECIP_D, MASK_FRECIP_D)
DECLARE_INSN(frsqrt_d, OP_FRSQRT_D, MASK_FRSQRT_D)
DECLARE_INSN(frecipe_d, OP_FRECIPE_D, MASK_FRECIPE_D)
DECLARE_INSN(frsqrte_d, OP_FRSQRTE_D, MASK_FRSQRTE_D)
DECLARE_INSN(fmov_d, OP_FMOV_D, MASK_FMOV_D)
DECLARE_INSN(movgr2fr_d, OP_MOVGR2FR_D, MASK_MOVGR2FR_D)
DECLARE_INSN(movfr2gr_d, OP_MOVFR2GR_D, MASK_MOVFR2GR_D)
DECLARE_INSN(fcvt_s_d, OP_FCVT_S_D, MASK_FCVT_S_D)
DECLARE_INSN(fcvt_d_s, OP_FCVT_D_S, MASK_FCVT_D_S)
DECLARE_INSN(ftintrm_w_d, OP_FTINTRM_W_D, MASK_FTINTRM_W_D)
DECLARE_INSN(ftintrm_l_d, OP_FTINTRM_L_D, MASK_FTINTRM_L_D)
DECLARE_INSN(ftintrp_w_d, OP_FTINTRP_W_D, MASK_FTINTRP_W_D)
DECLARE_INSN(ftintrp_l_d, OP_FTINTRP_L_D, MASK_FTINTRP_L_D)
DECLARE_INSN(ftintrz_w_d, OP_FTINTRZ_W_D, MASK_FTINTRZ_W_D)
DECLARE_INSN(ftintrz_l_d, OP_FTINTRZ_L_D, MASK_FTINTRZ_L_D)
DECLARE_INSN(ftintrne_w_d, OP_FTINTRNE_W_D, MASK_FTINTRNE_W_D)
DECLARE_INSN(ftintrne_l_d, OP_FTINTRNE_L_D, MASK_FTINTRNE_L_D)
DECLARE_INSN(ftint_w_d, OP_FTINT_W_D, MASK_FTINT_W_D)
DECLARE_INSN(ftint_l_d, OP_FTINT_L_D, MASK_FTINT_L_D)
DECLARE_INSN(ffint_d_w, OP_FFINT_D_W, MASK_FFINT_D_W)
DECLARE_INSN(ffint_d_l, OP_FFINT_D_L, MASK_FFINT_D_L)
DECLARE_INSN(frint_d, OP_FRINT_D, MASK_FRINT_D)

/* loongarch imm instruction */
DECLARE_INSN(slti, OP_SLTI, MASK_SLTI)
DECLARE_INSN(sltui, OP_SLTUI, MASK_SLTUI)
DECLARE_INSN(addi_w, OP_ADDI_W, MASK_ADDI_W)
DECLARE_INSN(addi_d, OP_ADDI_D, MASK_ADDI_D)
DECLARE_INSN(lu52i_d, OP_LU52I_D, MASK_LU52I_D)
DECLARE_INSN(andi, OP_ANDI, MASK_ANDI)
DECLARE_INSN(ori, OP_ORI, MASK_ORI)
DECLARE_INSN(xori, OP_XORI, MASK_XORI)
DECLARE_INSN(addu16i_d, OP_ADDU16I_D, MASK_ADDU16I_D)
DECLARE_INSN(lu12i_w, OP_LU12I_W, MASK_LU12I_W)
DECLARE_INSN(lu32i_d, OP_LU32I_D, MASK_LU32I_D)
DECLARE_INSN(pcaddi, OP_PCADDI, MASK_PCADDI)
DECLARE_INSN(pcalau12i, OP_PCALAU12I, MASK_PCALAU12I)
DECLARE_INSN(pcaddu12i, OP_PCADDU12I, MASK_PCADDU12I)
DECLARE_INSN(pcaddu18i, OP_PCADDU18I, MASK_PCADDU18I)

/* loongarch privilege instruction */
DECLARE_INSN(csrrd, OP_CSRRD, MASK_CSRRD)
DECLARE_INSN(csrwr, OP_CSRWR, MASK_CSRWR)
DECLARE_INSN(csrxchg, OP_CSRXCHG, MASK_CSRXCHG)
DECLARE_INSN(cacop, OP_CACOP, MASK_CACOP)
DECLARE_INSN(lddir, OP_LDDIR, MASK_LDDIR)
DECLARE_INSN(ldpte, OP_LDPTE, MASK_LDPTE)
DECLARE_INSN(iocsrrd_b, OP_IOCSRRD_B, MASK_IOCSRRD_B)
DECLARE_INSN(iocsrrd_h, OP_IOCSRRD_H, MASK_IOCSRRD_H)
DECLARE_INSN(iocsrrd_w, OP_IOCSRRD_W, MASK_IOCSRRD_W)
DECLARE_INSN(iocsrrd_d, OP_IOCSRRD_D, MASK_IOCSRRD_D)
DECLARE_INSN(iocsrwr_b, OP_IOCSRWR_B, MASK_IOCSRWR_B)
DECLARE_INSN(iocsrwr_h, OP_IOCSRWR_H, MASK_IOCSRWR_H)
DECLARE_INSN(iocsrwr_w, OP_IOCSRWR_W, MASK_IOCSRWR_W)
DECLARE_INSN(iocsrwr_d, OP_IOCSRWR_D, MASK_IOCSRWR_D)
DECLARE_INSN(tlbclr, OP_TLBCLR, MASK_TLBCLR)
DECLARE_INSN(tlbflush, OP_TLBFLUSH, MASK_TLBFLUSH)
DECLARE_INSN(tlbsrch, OP_TLBSRCH, MASK_TLBSRCH)
DECLARE_INSN(tlbrd, OP_TLBRD, MASK_TLBRD)
DECLARE_INSN(tlbwr, OP_TLBWR, MASK_TLBWR)
DECLARE_INSN(tlbfill, OP_TLBFILL, MASK_TLBFILL)
DECLARE_INSN(ertn, OP_ERTN, MASK_ERTN)
DECLARE_INSN(idle, OP_IDLE, MASK_IDLE)
DECLARE_INSN(invtlb, OP_INVTLB, MASK_INVTLB)

/* loongarch 4opt single float instruction */
DECLARE_INSN(fmadd_s, OP_FMADD_S, MASK_FMADD_S)
DECLARE_INSN(fmsub_s, OP_FMSUB_S, MASK_FMSUB_S)
DECLARE_INSN(fnmadd_s, OP_FNMADD_S, MASK_FNMADD_S)
DECLARE_INSN(fnmsub_s, OP_FNMSUB_S, MASK_FNMSUB_S)
DECLARE_INSN(fcmp_caf_s, OP_FCMP_CAF_S, MASK_FCMP_CAF_S)
DECLARE_INSN(fcmp_saf_s, OP_FCMP_SAF_S, MASK_FCMP_SAF_S)
DECLARE_INSN(fcmp_clt_s, OP_FCMP_CLT_S, MASK_FCMP_CLT_S)
DECLARE_INSN(fcmp_slt_s, OP_FCMP_SLT_S, MASK_FCMP_SLT_S)
DECLARE_INSN(fcmp_sgt_s, OP_FCMP_SGT_S, MASK_FCMP_SGT_S)
DECLARE_INSN(fcmp_ceq_s, OP_FCMP_CEQ_S, MASK_FCMP_CEQ_S)
DECLARE_INSN(fcmp_seq_s, OP_FCMP_SEQ_S, MASK_FCMP_SEQ_S)
DECLARE_INSN(fcmp_cle_s, OP_FCMP_CLE_S, MASK_FCMP_CLE_S)
DECLARE_INSN(fcmp_sle_s, OP_FCMP_SLE_S, MASK_FCMP_SLE_S)
DECLARE_INSN(fcmp_sge_s, OP_FCMP_SGE_S, MASK_FCMP_SGE_S)
DECLARE_INSN(fcmp_cun_s, OP_FCMP_CUN_S, MASK_FCMP_CUN_S)
DECLARE_INSN(fcmp_sun_s, OP_FCMP_SUN_S, MASK_FCMP_SUN_S)
DECLARE_INSN(fcmp_cult_s, OP_FCMP_CULT_S, MASK_FCMP_CULT_S)
DECLARE_INSN(fcmp_cugt_s, OP_FCMP_CUGT_S, MASK_FCMP_CUGT_S)
DECLARE_INSN(fcmp_sult_s, OP_FCMP_SULT_S, MASK_FCMP_SULT_S)
DECLARE_INSN(fcmp_cueq_s, OP_FCMP_CUEQ_S, MASK_FCMP_CUEQ_S)
DECLARE_INSN(fcmp_sueq_s, OP_FCMP_SUEQ_S, MASK_FCMP_SUEQ_S)
DECLARE_INSN(fcmp_cule_s, OP_FCMP_CULE_S, MASK_FCMP_CULE_S)
DECLARE_INSN(fcmp_cuge_s, OP_FCMP_CUGE_S, MASK_FCMP_CUGE_S)
DECLARE_INSN(fcmp_sule_s, OP_FCMP_SULE_S, MASK_FCMP_SULE_S)
DECLARE_INSN(fcmp_cne_s, OP_FCMP_CNE_S, MASK_FCMP_CNE_S)
DECLARE_INSN(fcmp_sne_s, OP_FCMP_SNE_S, MASK_FCMP_SNE_S)
DECLARE_INSN(fcmp_cor_s, OP_FCMP_COR_S, MASK_FCMP_COR_S)
DECLARE_INSN(fcmp_sor_s, OP_FCMP_SOR_S, MASK_FCMP_SOR_S)
DECLARE_INSN(fcmp_cune_s, OP_FCMP_CUNE_S, MASK_FCMP_CUNE_S)
DECLARE_INSN(fcmp_sune_s, OP_FCMP_SUNE_S, MASK_FCMP_SUNE_S)
DECLARE_INSN(fsel, OP_FSEL, MASK_FSEL)

/* loongarch 4opt double float instruction */
DECLARE_INSN(fmadd_d, OP_FMADD_D, MASK_FMADD_D)
DECLARE_INSN(fmsub_d, OP_FMSUB_D, MASK_FMSUB_D)
DECLARE_INSN(fnmadd_d, OP_FNMADD_D, MASK_FNMADD_D)
DECLARE_INSN(fnmsub_d, OP_FNMSUB_D, MASK_FNMSUB_D)
DECLARE_INSN(fcmp_caf_d, OP_FCMP_CAF_D, MASK_FCMP_CAF_D)
DECLARE_INSN(fcmp_saf_d, OP_FCMP_SAF_D, MASK_FCMP_SAF_D)
DECLARE_INSN(fcmp_clt_d, OP_FCMP_CLT_D, MASK_FCMP_CLT_D)
DECLARE_INSN(fcmp_slt_d, OP_FCMP_SLT_D, MASK_FCMP_SLT_D)
DECLARE_INSN(fcmp_sgt_d, OP_FCMP_SGT_D, MASK_FCMP_SGT_D)
DECLARE_INSN(fcmp_ceq_d, OP_FCMP_CEQ_D, MASK_FCMP_CEQ_D)
DECLARE_INSN(fcmp_seq_d, OP_FCMP_SEQ_D, MASK_FCMP_SEQ_D)
DECLARE_INSN(fcmp_cle_d, OP_FCMP_CLE_D, MASK_FCMP_CLE_D)
DECLARE_INSN(fcmp_sle_d, OP_FCMP_SLE_D, MASK_FCMP_SLE_D)
DECLARE_INSN(fcmp_sge_d, OP_FCMP_SGE_D, MASK_FCMP_SGE_D)
DECLARE_INSN(fcmp_cun_d, OP_FCMP_CUN_D, MASK_FCMP_CUN_D)
DECLARE_INSN(fcmp_sun_d, OP_FCMP_SUN_D, MASK_FCMP_SUN_D)
DECLARE_INSN(fcmp_cult_d, OP_FCMP_CULT_D, MASK_FCMP_CULT_D)
DECLARE_INSN(fcmp_cugt_d, OP_FCMP_CUGT_D, MASK_FCMP_CUGT_D)
DECLARE_INSN(fcmp_sult_d, OP_FCMP_SULT_D, MASK_FCMP_SULT_D)
DECLARE_INSN(fcmp_cueq_d, OP_FCMP_CUEQ_D, MASK_FCMP_CUEQ_D)
DECLARE_INSN(fcmp_sueq_d, OP_FCMP_SUEQ_D, MASK_FCMP_SUEQ_D)
DECLARE_INSN(fcmp_cule_d, OP_FCMP_CULE_D, MASK_FCMP_CULE_D)
DECLARE_INSN(fcmp_cuge_d, OP_FCMP_CUGE_D, MASK_FCMP_CUGE_D)
DECLARE_INSN(fcmp_sule_d, OP_FCMP_SULE_D, MASK_FCMP_SULE_D)
DECLARE_INSN(fcmp_cne_d, OP_FCMP_CNE_D, MASK_FCMP_CNE_D)
DECLARE_INSN(fcmp_sne_d, OP_FCMP_SNE_D, MASK_FCMP_SNE_D)
DECLARE_INSN(fcmp_cor_d, OP_FCMP_COR_D, MASK_FCMP_COR_D)
DECLARE_INSN(fcmp_sor_d, OP_FCMP_SOR_D, MASK_FCMP_SOR_D)
DECLARE_INSN(fcmp_cune_d, OP_FCMP_CUNE_D, MASK_FCMP_CUNE_D)
DECLARE_INSN(fcmp_sune_d, OP_FCMP_SUNE_D, MASK_FCMP_SUNE_D)

/* loongarch load store instruction */
DECLARE_INSN(ll_w, OP_LL_W, MASK_LL_W)
DECLARE_INSN(sc_w, OP_SC_W, MASK_SC_W)
DECLARE_INSN(ll_d, OP_LL_D, MASK_LL_D)
DECLARE_INSN(sc_d, OP_SC_D, MASK_SC_D)
DECLARE_INSN(ldptr_w, OP_LDPTR_W, MASK_LDPTR_W)
DECLARE_INSN(stptr_w, OP_STPTR_W, MASK_STPTR_W)
DECLARE_INSN(ldptr_d, OP_LDPTR_D, MASK_LDPTR_D)
DECLARE_INSN(stptr_d, OP_STPTR_D, MASK_STPTR_D)
DECLARE_INSN(ld_b, OP_LD_B, MASK_LD_B)
DECLARE_INSN(ld_h, OP_LD_H, MASK_LD_H)
DECLARE_INSN(ld_w, OP_LD_W, MASK_LD_W)
DECLARE_INSN(ld_d, OP_LD_D, MASK_LD_D)
DECLARE_INSN(st_b, OP_ST_B, MASK_ST_B)
DECLARE_INSN(st_h, OP_ST_H, MASK_ST_H)
DECLARE_INSN(st_w, OP_ST_W, MASK_ST_W)
DECLARE_INSN(st_d, OP_ST_D, MASK_ST_D)
DECLARE_INSN(ld_bu, OP_LD_BU, MASK_LD_BU)
DECLARE_INSN(ld_hu, OP_LD_HU, MASK_LD_HU)
DECLARE_INSN(ld_wu, OP_LD_WU, MASK_LD_WU)
DECLARE_INSN(preld, OP_PRELD, MASK_PRELD)
DECLARE_INSN(ldx_b, OP_LDX_B, MASK_LDX_B)
DECLARE_INSN(ldx_h, OP_LDX_H, MASK_LDX_H)
DECLARE_INSN(ldx_w, OP_LDX_W, MASK_LDX_W)
DECLARE_INSN(ldx_d, OP_LDX_D, MASK_LDX_D)
DECLARE_INSN(stx_b, OP_STX_B, MASK_STX_B)
DECLARE_INSN(stx_h, OP_STX_H, MASK_STX_H)
DECLARE_INSN(stx_w, OP_STX_W, MASK_STX_W)
DECLARE_INSN(stx_d, OP_STX_D, MASK_STX_D)
DECLARE_INSN(ldx_bu, OP_LDX_BU, MASK_LDX_BU)
DECLARE_INSN(ldx_hu, OP_LDX_HU, MASK_LDX_HU)
DECLARE_INSN(ldx_wu, OP_LDX_WU, MASK_LDX_WU)
DECLARE_INSN(preldx, OP_PRELDX, MASK_PRELDX)
DECLARE_INSN(sc_q, OP_SC_Q, MASK_SC_Q)
DECLARE_INSN(llacq_w, OP_LLACQ_W, MASK_LLACQ_W)
DECLARE_INSN(screl_w, OP_SCREL_W, MASK_SCREL_W)
DECLARE_INSN(llacq_d, OP_LLACQ_D, MASK_LLACQ_D)
DECLARE_INSN(screl_d, OP_SCREL_D, MASK_LLACQ_D)
DECLARE_INSN(amcas_b, OP_AMCAS_B, MASK_AMCAS_B)
DECLARE_INSN(amcas_h, OP_AMCAS_H, MASK_AMCAS_H)
DECLARE_INSN(amcas_w, OP_AMCAS_W, MASK_AMCAS_W)
DECLARE_INSN(amcas_d, OP_AMCAS_D, MASK_AMCAS_D)
DECLARE_INSN(amcas_db_b, OP_AMCAS_DB_B, MASK_AMCAS_DB_B)
DECLARE_INSN(amcas_db_h, OP_AMCAS_DB_H, MASK_AMCAS_DB_H)
DECLARE_INSN(amcas_db_w, OP_AMCAS_DB_W, MASK_AMCAS_DB_W)
DECLARE_INSN(amcas_db_d, OP_AMCAS_DB_D, MASK_AMCAS_DB_D)
DECLARE_INSN(amswap_b, OP_AMSWAP_B, MASK_AMSWAP_B)
DECLARE_INSN(amswap_h, OP_AMSWAP_H, MASK_AMSWAP_H)
DECLARE_INSN(amadd_b, OP_AMADD_B, MASK_AMADD_B)
DECLARE_INSN(amadd_h, OP_AMADD_H, MASK_AMADD_H)
DECLARE_INSN(amswap_db_b, OP_AMSWAP_DB_B, MASK_AMSWAP_DB_B)
DECLARE_INSN(amswap_db_h, OP_AMSWAP_DB_H, MASK_AMSWAP_DB_H)
DECLARE_INSN(amadd_db_b, OP_AMADD_DB_B, MASK_AMADD_DB_B)
DECLARE_INSN(amadd_db_h, OP_AMADD_DB_H, MASK_AMADD_DB_H)
DECLARE_INSN(amswap_w, OP_AMSWAP_W, MASK_AMSWAP_W)
DECLARE_INSN(amswap_d, OP_AMSWAP_D, MASK_AMSWAP_D)
DECLARE_INSN(amadd_w, OP_AMADD_W, MASK_AMADD_W)
DECLARE_INSN(amadd_d, OP_AMADD_D, MASK_AMADD_D)
DECLARE_INSN(amand_w, OP_AMAND_W, MASK_AMAND_W)
DECLARE_INSN(amand_d, OP_AMAND_D, MASK_AMAND_D)
DECLARE_INSN(amor_w, OP_AMOR_W, MASK_AMOR_W)
DECLARE_INSN(amor_d, OP_AMOR_D, MASK_AMOR_D)
DECLARE_INSN(amxor_w, OP_AMXOR_W, MASK_AMXOR_W)
DECLARE_INSN(amxor_d, OP_AMXOR_D, MASK_AMXOR_D)
DECLARE_INSN(ammax_w, OP_AMMAX_W, MASK_AMMAX_W)
DECLARE_INSN(ammax_d, OP_AMMAX_D, MASK_AMMAX_D)
DECLARE_INSN(ammin_w, OP_AMMIN_W, MASK_AMMIN_W)
DECLARE_INSN(ammin_d, OP_AMMIN_D, MASK_AMMIN_D)
DECLARE_INSN(ammax_wu, OP_AMMAX_WU, MASK_AMMAX_WU)
DECLARE_INSN(ammax_du, OP_AMMAX_DU, MASK_AMMAX_DU)
DECLARE_INSN(ammin_wu, OP_AMMIN_WU, MASK_AMMIN_WU)
DECLARE_INSN(ammin_du, OP_AMMIN_DU, MASK_AMMIN_DU)
DECLARE_INSN(amswap_db_w, OP_AMSWAP_DB_W, MASK_AMSWAP_DB_W)
DECLARE_INSN(amswap_db_d, OP_AMSWAP_DB_D, MASK_AMSWAP_DB_D)
DECLARE_INSN(amadd_db_w, OP_AMADD_DB_W, MASK_AMADD_DB_W)
DECLARE_INSN(amadd_db_d, OP_AMADD_DB_D, MASK_AMADD_DB_D)
DECLARE_INSN(amand_db_w, OP_AMAND_DB_W, MASK_AMAND_DB_W)
DECLARE_INSN(amand_db_d, OP_AMAND_DB_D, MASK_AMAND_DB_D)
DECLARE_INSN(amor_db_w, OP_AMOR_DB_W, MASK_AMOR_DB_W)
DECLARE_INSN(amor_db_d, OP_AMOR_DB_D, MASK_AMOR_DB_D)
DECLARE_INSN(amxor_db_w, OP_AMXOR_DB_W, MASK_AMXOR_DB_W)
DECLARE_INSN(amxor_db_d, OP_AMXOR_DB_D, MASK_AMXOR_DB_D)
DECLARE_INSN(ammax_db_w, OP_AMMAX_DB_W, MASK_AMMAX_DB_W)
DECLARE_INSN(ammax_db_d, OP_AMMAX_DB_D, MASK_AMMAX_DB_D)
DECLARE_INSN(ammin_db_w, OP_AMMIN_DB_W, MASK_AMMIN_DB_W)
DECLARE_INSN(ammin_db_d, OP_AMMIN_DB_D, MASK_AMMIN_DB_D)
DECLARE_INSN(ammax_db_wu, OP_AMMAX_DB_WU, MASK_AMMAX_DB_WU)
DECLARE_INSN(ammax_db_du, OP_AMMAX_DB_DU, MASK_AMMAX_DB_DU)
DECLARE_INSN(ammin_db_wu, OP_AMMIN_DB_WU, MASK_AMMIN_DB_WU)
DECLARE_INSN(ammin_db_du, OP_AMMIN_DB_DU, MASK_AMMIN_DB_DU)
DECLARE_INSN(dbar, OP_DBAR, MASK_DBAR)
DECLARE_INSN(ibar, OP_IBAR, MASK_IBAR)
DECLARE_INSN(ldgt_b, OP_LDGT_B, MASK_LDGT_B)
DECLARE_INSN(ldgt_h, OP_LDGT_H, MASK_LDGT_H)
DECLARE_INSN(ldgt_w, OP_LDGT_W, MASK_LDGT_W)
DECLARE_INSN(ldgt_d, OP_LDGT_D, MASK_LDGT_D)
DECLARE_INSN(ldle_b, OP_LDLE_B, MASK_LDLE_B)
DECLARE_INSN(ldle_h, OP_LDLE_H, MASK_LDLE_H)
DECLARE_INSN(ldle_w, OP_LDLE_W, MASK_LDLE_W)
DECLARE_INSN(ldle_d, OP_LDLE_D, MASK_LDLE_D)
DECLARE_INSN(stgt_b, OP_STGT_B, MASK_STGT_B)
DECLARE_INSN(stgt_h, OP_STGT_H, MASK_STGT_H)
DECLARE_INSN(stgt_w, OP_STGT_W, MASK_STGT_W)
DECLARE_INSN(stgt_d, OP_STGT_D, MASK_STGT_D)
DECLARE_INSN(stle_b, OP_STLE_B, MASK_STLE_B)
DECLARE_INSN(stle_h, OP_STLE_H, MASK_STLE_H)
DECLARE_INSN(stle_w, OP_STLE_W, MASK_STLE_W)
DECLARE_INSN(stle_d, OP_STLE_D, MASK_STLE_D)
DECLARE_INSN(vld, OP_VLD, MASK_VLD)
DECLARE_INSN(vst, OP_VST, MASK_VST)
DECLARE_INSN(xvld, OP_XVLD, MASK_XVLD)
DECLARE_INSN(xvst, OP_XVST, MASK_XVST)

/* loongarch single float load store instruction */
DECLARE_INSN(fld_s, OP_FLD_S, MASK_FLD_S)
DECLARE_INSN(fst_s, OP_FST_S, MASK_FST_S)
DECLARE_INSN(fldx_s, OP_FLDX_S, MASK_FLDX_S)
DECLARE_INSN(fstx_s, OP_FSTX_S, MASK_FSTX_S)
DECLARE_INSN(fldgt_s, OP_FLDGT_S, MASK_FLDGT_S)
DECLARE_INSN(fldle_s, OP_FLDLE_S, MASK_FLDLE_S)
DECLARE_INSN(fstgt_s, OP_FSTGT_S, MASK_FSTGT_S)
DECLARE_INSN(fstle_s, OP_FSTLE_S, MASK_FSTLE_S)

/* loongarch double float load store instruction */
DECLARE_INSN(fld_d, OP_FLD_D, MASK_FLD_D)
DECLARE_INSN(fst_d, OP_FST_D, MASK_FST_D)
DECLARE_INSN(fldx_d, OP_FLDX_D, MASK_FLDX_D)
DECLARE_INSN(fstx_d, OP_FSTX_D, MASK_FSTX_D)
DECLARE_INSN(fldgt_d, OP_FLDGT_D, MASK_FLDGT_D)
DECLARE_INSN(fldle_d, OP_FLDLE_D, MASK_FLDLE_D)
DECLARE_INSN(fstgt_d, OP_FSTGT_D, MASK_FSTGT_D)
DECLARE_INSN(fstle_d, OP_FSTLE_D, MASK_FSTLE_D)

/* loongarch float jmp instruction */
DECLARE_INSN(bceqz, OP_BCEQZ, MASK_BCEQZ)
DECLARE_INSN(bcnez, OP_BCNEZ, MASK_BCNEZ)

/* loongarch jmp instruction */
DECLARE_INSN(beqz, OP_BEQZ, MASK_BEQZ)
DECLARE_INSN(bnez, OP_BNEZ, MASK_BNEZ)
DECLARE_INSN(jirl, OP_JIRL, MASK_JIRL)
DECLARE_INSN(b, OP_B, MASK_B)
DECLARE_INSN(bl, OP_BL, MASK_BL)
DECLARE_INSN(beq, OP_BEQ, MASK_BEQ)
DECLARE_INSN(bne, OP_BNE, MASK_BNE)
DECLARE_INSN(blt, OP_BLT, MASK_BLT)
DECLARE_INSN(bge, OP_BGE, MASK_BGE)
DECLARE_INSN(bltu, OP_BLTU, MASK_BLTU)
DECLARE_INSN(bgeu, OP_BGEU, MASK_BGEU)

#undef DECLARE_INSN

static inline bool
is_arithmetic_operation_insn (uint32_t insn)
{
  if (is_add_w_insn (insn)
      || is_add_d_insn (insn)
      || is_sub_w_insn (insn)
      || is_sub_d_insn (insn)
      || is_addi_w_insn (insn)
      || is_addi_d_insn (insn)
      || is_addu16i_d_insn (insn)
      || is_alsl_w_insn (insn)
      || is_alsl_wu_insn (insn)
      || is_alsl_d_insn (insn)
      || is_lu12i_w_insn (insn)
      || is_lu32i_d_insn (insn)
      || is_lu52i_d_insn (insn)
      || is_slt_insn (insn)
      || is_sltu_insn (insn)
      || is_slti_insn (insn)
      || is_sltui_insn (insn)
      || is_pcaddi_insn (insn)
      || is_pcaddu12i_insn (insn)
      || is_pcaddu18i_insn (insn)
      || is_pcalau12i_insn (insn)
      || is_and_insn (insn)
      || is_or_insn (insn)
      || is_nor_insn (insn)
      || is_xor_insn (insn)
      || is_andn_insn (insn)
      || is_orn_insn (insn)
      || is_andi_insn (insn)
      || is_ori_insn (insn)
      || is_xori_insn (insn)
      || is_mul_w_insn (insn)
      || is_mul_d_insn (insn)
      || is_mulh_w_insn (insn)
      || is_mulh_wu_insn (insn)
      || is_mulh_d_insn (insn)
      || is_mulh_du_insn (insn)
      || is_mulw_d_w_insn (insn)
      || is_mulw_d_wu_insn (insn)
      || is_div_w_insn (insn)
      || is_div_wu_insn (insn)
      || is_div_d_insn (insn)
      || is_div_du_insn (insn)
      || is_mod_w_insn (insn)
      || is_mod_wu_insn (insn)
      || is_mod_d_insn (insn)
      || is_mod_du_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_bit_shift_insn (uint32_t insn)
{
  if (is_sll_w_insn (insn)
      || is_srl_w_insn (insn)
      || is_sra_w_insn (insn)
      || is_rotr_w_insn (insn)
      || is_slli_w_insn (insn)
      || is_srli_w_insn (insn)
      || is_srai_w_insn (insn)
      || is_rotri_w_insn (insn)
      || is_slli_d_insn (insn)
      || is_srli_d_insn (insn)
      || is_srai_d_insn (insn)
      || is_rotri_d_insn (insn)
      || is_sll_d_insn (insn)
      || is_srl_d_insn (insn)
      || is_sra_d_insn (insn)
      || is_rotr_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_bit_manipulation_insn (uint32_t insn)
{
  if (is_clo_w_insn (insn)
      || is_clz_w_insn (insn)
      || is_cto_w_insn (insn)
      || is_ctz_w_insn (insn)
      || is_clo_d_insn (insn)
      || is_clz_d_insn (insn)
      || is_cto_d_insn (insn)
      || is_ctz_d_insn (insn)
      || is_ext_w_h_insn (insn)
      || is_ext_w_b_insn (insn)
      || is_bytepick_w_insn (insn)
      || is_bytepick_d_insn (insn)
      || is_revb_2h_insn (insn)
      || is_revb_4h_insn (insn)
      || is_revb_2w_insn (insn)
      || is_revb_d_insn (insn)
      || is_revh_2w_insn (insn)
      || is_revh_d_insn (insn)
      || is_bitrev_4b_insn (insn)
      || is_bitrev_8b_insn (insn)
      || is_bitrev_w_insn (insn)
      || is_bitrev_d_insn (insn)
      || is_bstrins_w_insn (insn)
      || is_bstrpick_w_insn (insn)
      || is_bstrins_d_insn (insn)
      || is_bstrpick_d_insn (insn)
      || is_maskeqz_insn (insn)
      || is_masknez_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_load_insn (uint32_t insn)
{
  if (is_ld_b_insn (insn)
      || is_ld_h_insn (insn)
      || is_ld_w_insn (insn)
      || is_ld_d_insn (insn)
      || is_ld_bu_insn (insn)
      || is_ld_hu_insn (insn)
      || is_ld_wu_insn (insn)
      || is_ldx_b_insn (insn)
      || is_ldx_h_insn (insn)
      || is_ldx_w_insn (insn)
      || is_ldx_d_insn (insn)
      || is_ldx_bu_insn (insn)
      || is_ldx_hu_insn (insn)
      || is_ldx_wu_insn (insn)
      || is_ldptr_w_insn (insn)
      || is_ldptr_d_insn (insn)
      || is_ll_w_insn (insn)
      || is_ll_d_insn (insn)
      || is_llacq_w_insn (insn)
      || is_llacq_d_insn (insn)
      || is_vld_insn (insn)
      || is_xvld_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_crc_check_insn (uint32_t insn)
{
  if (is_crc_w_b_w_insn (insn)
      || is_crc_w_h_w_insn (insn)
      || is_crc_w_w_w_insn (insn)
      || is_crc_w_d_w_insn (insn)
      || is_crcc_w_b_w_insn (insn)
      || is_crcc_w_h_w_insn (insn)
      || is_crcc_w_w_w_insn (insn)
      || is_crcc_w_d_w_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_fr_to_gr_insn (uint32_t insn)
{
  if (is_movfr2gr_s_insn (insn)
      || is_movfr2gr_d_insn (insn)
      || is_movfrh2gr_s_insn (insn)
      || is_movfcsr2gr_insn (insn)
      || is_movcf2gr_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_data_process_insn (uint32_t insn)
{
  if (is_arithmetic_operation_insn (insn)
      || is_bit_shift_insn (insn)
      || is_bit_manipulation_insn (insn)
      || is_load_insn (insn)
      || is_crc_check_insn (insn)
      || is_fr_to_gr_insn (insn)
      || is_cpucfg_insn (insn)
      || is_lddir_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_read_time_insn (uint32_t insn)
{
  if (is_rdtimel_w_insn (insn)
      || is_rdtimeh_w_insn (insn)
      || is_rdtime_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_arithmetic_operation_insn (uint32_t insn)
{
  if (is_fadd_s_insn (insn)
      || is_fsub_s_insn (insn)
      || is_fmul_s_insn (insn)
      || is_fdiv_s_insn (insn)
      || is_fadd_d_insn (insn)
      || is_fsub_d_insn (insn)
      || is_fmul_d_insn (insn)
      || is_fdiv_d_insn (insn)
      || is_fmadd_s_insn (insn)
      || is_fmsub_s_insn (insn)
      || is_fnmadd_s_insn (insn)
      || is_fnmsub_s_insn (insn)
      || is_fmadd_d_insn (insn)
      || is_fmsub_d_insn (insn)
      || is_fnmadd_d_insn (insn)
      || is_fnmsub_d_insn (insn)
      || is_fmax_s_insn (insn)
      || is_fmin_s_insn (insn)
      || is_fmax_d_insn (insn)
      || is_fmin_d_insn (insn)
      || is_fmaxa_s_insn (insn)
      || is_fmina_s_insn (insn)
      || is_fmaxa_d_insn (insn)
      || is_fmina_d_insn (insn)
      || is_fabs_s_insn (insn)
      || is_fneg_s_insn (insn)
      || is_fabs_d_insn (insn)
      || is_fneg_d_insn (insn)
      || is_fsqrt_s_insn (insn)
      || is_frecip_s_insn (insn)
      || is_frsqrt_s_insn (insn)
      || is_fsqrt_d_insn (insn)
      || is_frecip_d_insn (insn)
      || is_frsqrt_d_insn (insn)
      || is_fscaleb_s_insn (insn)
      || is_flogb_s_insn (insn)
      || is_fcopysign_s_insn (insn)
      || is_fscaleb_d_insn (insn)
      || is_flogb_d_insn (insn)
      || is_fcopysign_d_insn (insn)
      || is_fclass_s_insn (insn)
      || is_fclass_d_insn (insn)
      || is_frecipe_s_insn (insn)
      || is_frsqrte_s_insn (insn)
      || is_frecipe_d_insn (insn)
      || is_frsqrte_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_comparison_insn (uint32_t insn)
{
  if (is_fcmp_caf_s_insn (insn)
      || is_fcmp_cun_s_insn (insn)
      || is_fcmp_ceq_s_insn (insn)
      || is_fcmp_cueq_s_insn (insn)
      || is_fcmp_clt_s_insn (insn)
      || is_fcmp_cult_s_insn (insn)
      || is_fcmp_cle_s_insn (insn)
      || is_fcmp_cule_s_insn (insn)
      || is_fcmp_cne_s_insn (insn)
      || is_fcmp_cor_s_insn (insn)
      || is_fcmp_cune_s_insn (insn)
      || is_fcmp_saf_s_insn (insn)
      || is_fcmp_sun_s_insn (insn)
      || is_fcmp_seq_s_insn (insn)
      || is_fcmp_sueq_s_insn (insn)
      || is_fcmp_slt_s_insn (insn)
      || is_fcmp_sult_s_insn (insn)
      || is_fcmp_sle_s_insn (insn)
      || is_fcmp_sule_s_insn (insn)
      || is_fcmp_sne_s_insn (insn)
      || is_fcmp_sor_s_insn (insn)
      || is_fcmp_sune_s_insn (insn)
      || is_fcmp_sgt_s_insn (insn)
      || is_fcmp_sge_s_insn (insn)
      || is_fcmp_cugt_s_insn (insn)
      || is_fcmp_cuge_s_insn (insn)
      || is_fcmp_caf_d_insn (insn)
      || is_fcmp_cun_d_insn (insn)
      || is_fcmp_ceq_d_insn (insn)
      || is_fcmp_cueq_d_insn (insn)
      || is_fcmp_clt_d_insn (insn)
      || is_fcmp_cult_d_insn (insn)
      || is_fcmp_cle_d_insn (insn)
      || is_fcmp_cule_d_insn (insn)
      || is_fcmp_cne_d_insn (insn)
      || is_fcmp_cor_d_insn (insn)
      || is_fcmp_cune_d_insn (insn)
      || is_fcmp_saf_d_insn (insn)
      || is_fcmp_sun_d_insn (insn)
      || is_fcmp_seq_d_insn (insn)
      || is_fcmp_sueq_d_insn (insn)
      || is_fcmp_slt_d_insn (insn)
      || is_fcmp_sult_d_insn (insn)
      || is_fcmp_sle_d_insn (insn)
      || is_fcmp_sule_d_insn (insn)
      || is_fcmp_sne_d_insn (insn)
      || is_fcmp_sor_d_insn (insn)
      || is_fcmp_sune_d_insn (insn)
      || is_fcmp_sgt_d_insn (insn)
      || is_fcmp_sge_d_insn (insn)
      || is_fcmp_cugt_d_insn (insn)
      || is_fcmp_cuge_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_conversion_insn (uint32_t insn)
{
  if (is_fcvt_s_d_insn (insn)
      || is_fcvt_d_s_insn (insn)
      || is_ffint_s_w_insn (insn)
      || is_ffint_s_l_insn (insn)
      || is_ffint_d_w_insn (insn)
      || is_ffint_d_l_insn (insn)
      || is_ftint_w_s_insn (insn)
      || is_ftint_l_s_insn (insn)
      || is_ftint_w_d_insn (insn)
      || is_ftint_l_d_insn (insn)
      || is_ftintrm_w_s_insn (insn)
      || is_ftintrm_l_s_insn (insn)
      || is_ftintrp_w_s_insn (insn)
      || is_ftintrp_l_s_insn (insn)
      || is_ftintrz_w_s_insn (insn)
      || is_ftintrz_l_s_insn (insn)
      || is_ftintrne_w_s_insn (insn)
      || is_ftintrne_l_s_insn (insn)
      || is_ftintrm_w_d_insn (insn)
      || is_ftintrm_l_d_insn (insn)
      || is_ftintrp_w_d_insn (insn)
      || is_ftintrp_l_d_insn (insn)
      || is_ftintrz_w_d_insn (insn)
      || is_ftintrz_l_d_insn (insn)
      || is_ftintrne_w_d_insn (insn)
      || is_ftintrne_l_d_insn (insn)
      || is_frint_s_insn (insn)
      || is_frint_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_mov_insn (uint32_t insn)
{
  if (is_fmov_s_insn (insn)
      || is_fmov_d_insn (insn)
      || is_fsel_insn (insn)
      || is_movgr2fr_w_insn (insn)
      || is_movgr2fr_d_insn (insn)
      || is_movgr2frh_w_insn (insn)
      || is_movgr2fcsr_insn (insn)
      || is_movfr2cf_insn (insn)
      || is_movcf2fr_insn (insn)
      || is_movgr2cf_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_ld_insn (uint32_t insn)
{
  if (is_fld_s_insn (insn)
      || is_fld_d_insn (insn)
      || is_fldx_s_insn (insn)
      || is_fldx_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_mov2cf_insn (uint32_t insn)
{
  if (is_movfr2cf_insn (insn)
      || is_movgr2cf_insn (insn)
      || is_float_comparison_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_float_insn (uint32_t insn)
{
  if (is_float_arithmetic_operation_insn (insn)
      || is_float_conversion_insn (insn)
      || is_float_mov_insn (insn)
      || is_float_ld_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_store_insn (uint32_t insn)
{
  if (is_st_b_insn (insn)
      || is_st_h_insn (insn)
      || is_st_w_insn (insn)
      || is_st_d_insn (insn)
      || is_stx_b_insn (insn)
      || is_stx_h_insn (insn)
      || is_stx_w_insn (insn)
      || is_stx_d_insn (insn)
      || is_stptr_w_insn (insn)
      || is_stptr_d_insn (insn)
      || is_sc_w_insn (insn)
      || is_sc_d_insn (insn)
      || is_sc_q_insn (insn)
      || is_screl_w_insn (insn)
      || is_screl_d_insn (insn)
      || is_fst_s_insn (insn)
      || is_fst_d_insn (insn)
      || is_fstx_s_insn (insn)
      || is_fstx_d_insn (insn)
      || is_vst_insn (insn)
      || is_xvst_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_atomic_access_insn (uint32_t insn)
{
  if (is_amswap_w_insn (insn)
      || is_amswap_d_insn (insn)
      || is_amswap_db_w_insn (insn)
      || is_amswap_db_d_insn (insn)
      || is_amadd_w_insn (insn)
      || is_amadd_d_insn (insn)
      || is_amadd_db_w_insn (insn)
      || is_amadd_db_d_insn (insn)
      || is_amand_w_insn (insn)
      || is_amand_d_insn (insn)
      || is_amand_db_w_insn (insn)
      || is_amand_db_d_insn (insn)
      || is_amor_w_insn (insn)
      || is_amor_d_insn (insn)
      || is_amor_db_w_insn (insn)
      || is_amor_db_d_insn (insn)
      || is_amxor_w_insn (insn)
      || is_amxor_d_insn (insn)
      || is_amxor_db_w_insn (insn)
      || is_amxor_db_d_insn (insn)
      || is_ammax_w_insn (insn)
      || is_ammax_d_insn (insn)
      || is_ammax_db_w_insn (insn)
      || is_ammax_db_d_insn (insn)
      || is_ammin_w_insn (insn)
      || is_ammin_d_insn (insn)
      || is_ammin_db_w_insn (insn)
      || is_ammin_db_d_insn (insn)
      || is_ammax_wu_insn (insn)
      || is_ammax_du_insn (insn)
      || is_ammax_db_du_insn (insn)
      || is_ammin_db_wu_insn (insn)
      || is_ammin_wu_insn (insn)
      || is_ammin_du_insn (insn)
      || is_ammin_db_wu_insn (insn)
      || is_ammin_db_du_insn (insn)
      || is_amswap_b_insn (insn)
      || is_amswap_h_insn (insn)
      || is_amswap_db_b_insn (insn)
      || is_amswap_db_h_insn (insn)
      || is_amadd_b_insn (insn)
      || is_amadd_h_insn (insn)
      || is_amadd_db_b_insn (insn)
      || is_amadd_db_h_insn (insn)
      || is_amcas_b_insn (insn)
      || is_amcas_h_insn (insn)
      || is_amcas_w_insn (insn)
      || is_amcas_d_insn (insn)
      || is_amcas_db_b_insn (insn)
      || is_amcas_db_h_insn (insn)
      || is_amcas_db_w_insn (insn)
      || is_amcas_db_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_basic_am_w_d_insn (uint32_t insn)
{
  if (is_amswap_w_insn (insn)
      || is_amswap_d_insn (insn)
      || is_amswap_db_w_insn (insn)
      || is_amswap_db_d_insn (insn)
      || is_amadd_w_insn (insn)
      || is_amadd_d_insn (insn)
      || is_amadd_db_w_insn (insn)
      || is_amadd_db_d_insn (insn)
      || is_amand_w_insn (insn)
      || is_amand_d_insn (insn)
      || is_amand_db_w_insn (insn)
      || is_amand_db_d_insn (insn)
      || is_amor_w_insn (insn)
      || is_amor_d_insn (insn)
      || is_amor_db_w_insn (insn)
      || is_amor_db_d_insn (insn)
      || is_amxor_w_insn (insn)
      || is_amxor_d_insn (insn)
      || is_amxor_db_w_insn (insn)
      || is_amxor_db_d_insn (insn)
      || is_ammax_w_insn (insn)
      || is_ammax_d_insn (insn)
      || is_ammax_db_w_insn (insn)
      || is_ammax_db_d_insn (insn)
      || is_ammin_w_insn (insn)
      || is_ammin_d_insn (insn)
      || is_ammin_db_w_insn (insn)
      || is_ammin_db_d_insn (insn)
      || is_ammax_wu_insn (insn)
      || is_ammax_du_insn (insn)
      || is_ammax_db_du_insn (insn)
      || is_ammin_db_wu_insn (insn)
      || is_ammin_wu_insn (insn)
      || is_ammin_du_insn (insn)
      || is_ammin_db_wu_insn (insn)
      || is_ammin_db_du_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_am_b_h_insn (uint32_t insn)
{
  if (is_amswap_b_insn (insn)
      || is_amswap_h_insn (insn)
      || is_amswap_db_b_insn (insn)
      || is_amswap_db_h_insn (insn)
      || is_amadd_b_insn (insn)
      || is_amadd_h_insn (insn)
      || is_amadd_db_b_insn (insn)
      || is_amadd_db_h_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_amswap_insn (uint32_t insn)
{
  if (is_amswap_w_insn (insn)
      || is_amswap_d_insn (insn)
      || is_amswap_db_w_insn (insn)
      || is_amswap_db_d_insn (insn)
      || is_amswap_b_insn (insn)
      || is_amswap_h_insn (insn)
      || is_amswap_db_b_insn (insn)
      || is_amswap_db_h_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_amcas_insn (uint32_t insn)
{
  if (is_amcas_b_insn (insn)
      || is_amcas_h_insn (insn)
      || is_amcas_w_insn (insn)
      || is_amcas_d_insn (insn)
      || is_amcas_db_b_insn (insn)
      || is_amcas_db_h_insn (insn)
      || is_amcas_db_w_insn (insn)
      || is_amcas_db_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_bound_check_load_insn (uint32_t insn)
{
  if (is_ldgt_b_insn (insn)
      || is_ldgt_h_insn (insn)
      || is_ldgt_w_insn (insn)
      || is_ldgt_d_insn (insn)
      || is_ldle_b_insn (insn)
      || is_ldle_h_insn (insn)
      || is_ldle_w_insn (insn)
      || is_ldle_d_insn (insn)
      || is_fldgt_s_insn (insn)
      || is_fldle_s_insn (insn)
      || is_fldgt_d_insn (insn)
      || is_fldle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_bound_check_store_insn (uint32_t insn)
{
  if (is_stgt_b_insn (insn)
      || is_stgt_h_insn (insn)
      || is_stgt_w_insn (insn)
      || is_stgt_d_insn (insn)
      || is_stle_b_insn (insn)
      || is_stle_h_insn (insn)
      || is_stle_w_insn (insn)
      || is_stle_d_insn (insn)
      || is_fstgt_s_insn (insn)
      || is_fstle_s_insn (insn)
      || is_fstgt_d_insn (insn)
      || is_fstle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_ldgt_insn (uint32_t insn)
{
  if (is_ldgt_b_insn (insn)
      || is_ldgt_h_insn (insn)
      || is_ldgt_w_insn (insn)
      || is_ldgt_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_ldle_insn (uint32_t insn)
{
  if (is_ldle_b_insn (insn)
      || is_ldle_h_insn (insn)
      || is_ldle_w_insn (insn)
      || is_ldle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_fldgt_insn (uint32_t insn)
{
  if (is_fldgt_s_insn (insn)
      || is_fldgt_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_fldle_insn (uint32_t insn)
{
  if (is_fldle_s_insn (insn)
      || is_fldle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_stgt_insn (uint32_t insn)
{
  if (is_stgt_b_insn (insn)
      || is_stgt_h_insn (insn)
      || is_stgt_w_insn (insn)
      || is_stgt_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_stle_insn (uint32_t insn)
{
  if (is_stle_b_insn (insn)
      || is_stle_h_insn (insn)
      || is_stle_w_insn (insn)
      || is_stle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_fstgt_insn (uint32_t insn)
{
  if (is_fstgt_s_insn (insn)
      || is_fstgt_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_fstle_insn (uint32_t insn)
{
  if (is_fstle_s_insn (insn)
      || is_fstle_d_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_branch_insn (uint32_t insn)
{
  if (is_beqz_insn (insn)
      || is_bnez_insn (insn)
      || is_jirl_insn (insn)
      || is_b_insn (insn)
      || is_bl_insn (insn)
      || is_beq_insn (insn)
      || is_bne_insn (insn)
      || is_blt_insn (insn)
      || is_bge_insn (insn)
      || is_bltu_insn (insn)
      || is_bgeu_insn (insn)
      || is_bceqz_insn (insn)
      || is_bcnez_insn (insn))
    return true;
  else
    return false;
}

static inline bool
is_special_insn (uint32_t insn)
{
  if (is_cacop_insn (insn)
      || is_tlbsrch_insn (insn)
      || is_tlbrd_insn (insn)
      || is_tlbwr_insn (insn)
      || is_tlbfill_insn (insn)
      || is_tlbclr_insn (insn)
      || is_tlbflush_insn (insn)
      || is_invtlb_insn (insn)
      || is_ldpte_insn (insn)
      || is_ertn_insn (insn)
      || is_idle_insn (insn)
      || is_dbcl_insn (insn)
      || is_preld_insn (insn)
      || is_preldx_insn (insn)
      || is_dbar_insn (insn)
      || is_ibar_insn (insn))
    return true;
  else
    return false;
}

#endif /* GDB_ARCH_LOONGARCH_INSN_H */
