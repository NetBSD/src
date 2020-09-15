  /* PREFIX_EVEX_0F10 */
  {
    { "vmovupX",	{ XM, EXEvexXNoBcst }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F10_P_1) },
    { "vmovupX",	{ XM, EXEvexXNoBcst }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F10_P_3) },
  },
  /* PREFIX_EVEX_0F11 */
  {
    { "vmovupX",	{ EXxS, XM }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F11_P_1) },
    { "vmovupX",	{ EXxS, XM }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F11_P_3) },
  },
  /* PREFIX_EVEX_0F12 */
  {
    { MOD_TABLE (MOD_EVEX_0F12_PREFIX_0) },
    { VEX_W_TABLE (EVEX_W_0F12_P_1) },
    { MOD_TABLE (MOD_EVEX_0F12_PREFIX_2) },
    { VEX_W_TABLE (EVEX_W_0F12_P_3) },
  },
  /* PREFIX_EVEX_0F16 */
  {
    { MOD_TABLE (MOD_EVEX_0F16_PREFIX_0) },
    { VEX_W_TABLE (EVEX_W_0F16_P_1) },
    { MOD_TABLE (MOD_EVEX_0F16_PREFIX_2) },
  },
  /* PREFIX_EVEX_0F2A */
  {
    { Bad_Opcode },
    { "vcvtsi2ss{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR, Edq }, 0 },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2A_P_3) },
  },
  /* PREFIX_EVEX_0F51 */
  {
    { "vsqrtpX",	{ XM, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F51_P_1) },
    { "vsqrtpX",	{ XM, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F51_P_3) },
  },
  /* PREFIX_EVEX_0F58 */
  {
    { "vaddpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F58_P_1) },
    { "vaddpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F58_P_3) },
  },
  /* PREFIX_EVEX_0F59 */
  {
    { "vmulpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F59_P_1) },
    { "vmulpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F59_P_3) },
  },
  /* PREFIX_EVEX_0F5A */
  {
    { VEX_W_TABLE (EVEX_W_0F5A_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5A_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5A_P_2) },
    { VEX_W_TABLE (EVEX_W_0F5A_P_3) },
  },
  /* PREFIX_EVEX_0F5B */
  {
    { VEX_W_TABLE (EVEX_W_0F5B_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5B_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5B_P_2) },
  },
  /* PREFIX_EVEX_0F5C */
  {
    { "vsubpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5C_P_1) },
    { "vsubpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5C_P_3) },
  },
  /* PREFIX_EVEX_0F5D */
  {
    { "vminpX",	{ XM, Vex, EXx, EXxEVexS }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5D_P_1) },
    { "vminpX",	{ XM, Vex, EXx, EXxEVexS }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5D_P_3) },
  },
  /* PREFIX_EVEX_0F5E */
  {
    { "vdivpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5E_P_1) },
    { "vdivpX",	{ XM, Vex, EXx, EXxEVexR }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5E_P_3) },
  },
  /* PREFIX_EVEX_0F5F */
  {
    { "vmaxpX",	{ XM, Vex, EXx, EXxEVexS }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5F_P_1) },
    { "vmaxpX",	{ XM, Vex, EXx, EXxEVexS }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F5F_P_3) },
  },
  /* PREFIX_EVEX_0F6F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6F_P_1) },
    { VEX_W_TABLE (EVEX_W_0F6F_P_2) },
    { VEX_W_TABLE (EVEX_W_0F6F_P_3) },
  },
  /* PREFIX_EVEX_0F70 */
  {
    { Bad_Opcode },
    { "vpshufhw",	{ XM, EXx, Ib }, 0 },
    { VEX_W_TABLE (EVEX_W_0F70_P_2) },
    { "vpshuflw",	{ XM, EXx, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F78 */
  {
    { VEX_W_TABLE (EVEX_W_0F78_P_0) },
    { "vcvttss2usi",	{ Gdq, EXxmm_md, EXxEVexS }, 0 },
    { VEX_W_TABLE (EVEX_W_0F78_P_2) },
    { "vcvttsd2usi",	{ Gdq, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F79 */
  {
    { VEX_W_TABLE (EVEX_W_0F79_P_0) },
    { "vcvtss2usi",	{ Gdq, EXxmm_md, EXxEVexR }, 0 },
    { VEX_W_TABLE (EVEX_W_0F79_P_2) },
    { "vcvtsd2usi",	{ Gdq, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_0F7A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7A_P_1) },
    { VEX_W_TABLE (EVEX_W_0F7A_P_2) },
    { VEX_W_TABLE (EVEX_W_0F7A_P_3) },
  },
  /* PREFIX_EVEX_0F7B */
  {
    { Bad_Opcode },
    { "vcvtusi2ss{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR, Edq }, 0 },
    { VEX_W_TABLE (EVEX_W_0F7B_P_2) },
    { VEX_W_TABLE (EVEX_W_0F7B_P_3) },
  },
  /* PREFIX_EVEX_0F7E */
  {
    { Bad_Opcode },
    { EVEX_LEN_TABLE (EVEX_LEN_0F7E_P_1) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F7E_P_2) },
  },
  /* PREFIX_EVEX_0F7F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7F_P_1) },
    { VEX_W_TABLE (EVEX_W_0F7F_P_2) },
    { VEX_W_TABLE (EVEX_W_0F7F_P_3) },
  },
  /* PREFIX_EVEX_0FC2 */
  {
    { "vcmppX",	{ XMask, Vex, EXx, EXxEVexS, CMP }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0FC2_P_1) },
    { "vcmppX",	{ XMask, Vex, EXx, EXxEVexS, CMP }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0FC2_P_3) },
  },
  /* PREFIX_EVEX_0FE6 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FE6_P_1) },
    { VEX_W_TABLE (EVEX_W_0FE6_P_2) },
    { VEX_W_TABLE (EVEX_W_0FE6_P_3) },
  },
  /* PREFIX_EVEX_0F3810 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3810_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3810_P_2) },
  },
  /* PREFIX_EVEX_0F3811 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3811_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3811_P_2) },
  },
  /* PREFIX_EVEX_0F3812 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3812_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3812_P_2) },
  },
  /* PREFIX_EVEX_0F3813 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3813_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3813_P_2) },
  },
  /* PREFIX_EVEX_0F3814 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3814_P_1) },
    { "vprorv%DQ",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3815 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3815_P_1) },
    { "vprolv%DQ",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3820 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3820_P_1) },
    { "vpmovsxbw",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3821 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3821_P_1) },
    { "vpmovsxbd",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3822 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3822_P_1) },
    { "vpmovsxbq",	{ XM, EXxmmdw }, 0 },
  },
  /* PREFIX_EVEX_0F3823 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3823_P_1) },
    { "vpmovsxwd",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3824 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3824_P_1) },
    { "vpmovsxwq",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3825 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3825_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3825_P_2) },
  },
  /* PREFIX_EVEX_0F3826 */
  {
    { Bad_Opcode },
    { "vptestnm%BW",	{ XMask, Vex, EXx }, 0 },
    { "vptestm%BW",	{ XMask, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3827 */
  {
    { Bad_Opcode },
    { "vptestnm%DQ",	{ XMask, Vex, EXx }, 0 },
    { "vptestm%DQ",	{ XMask, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3828 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F3828_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3828_P_2) },
  },
  /* PREFIX_EVEX_0F3829 */
  {
    { Bad_Opcode },
    { "vpmov%BW2m",	{ XMask, EXx }, 0 },
    { VEX_W_TABLE (EVEX_W_0F3829_P_2) },
  },
  /* PREFIX_EVEX_0F382A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F382A_P_1) },
    { VEX_W_TABLE (EVEX_W_0F382A_P_2) },
  },
  /* PREFIX_EVEX_0F3830 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3830_P_1) },
    { "vpmovzxbw",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3831 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3831_P_1) },
    { "vpmovzxbd",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3832 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3832_P_1) },
    { "vpmovzxbq",	{ XM, EXxmmdw }, 0 },
  },
  /* PREFIX_EVEX_0F3833 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3833_P_1) },
    { "vpmovzxwd",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3834 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3834_P_1) },
    { "vpmovzxwq",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3835 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3835_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3835_P_2) },
  },
  /* PREFIX_EVEX_0F3838 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F3838_P_1) },
    { "vpminsb",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3839 */
  {
    { Bad_Opcode },
    { "vpmov%DQ2m",	{ XMask, EXx }, 0 },
    { "vpmins%DQ",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F383A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F383A_P_1) },
    { "vpminuw",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3852 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3852_P_1) },
    { "vpdpwssd",	{ XM, Vex, EXx }, 0 },
    { "vp4dpwssd",	{ XM, Vex, EXxmm }, 0 },
  },
  /* PREFIX_EVEX_0F3853 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpdpwssds",	{ XM, Vex, EXx }, 0 },
    { "vp4dpwssds",	{ XM, Vex, EXxmm }, 0 },
  },
  /* PREFIX_EVEX_0F3868 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vp2intersect%DQ", { XMask, Vex, EXx, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F3872 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3872_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3872_P_2) },
    { VEX_W_TABLE (EVEX_W_0F3872_P_3) },
  },
  /* PREFIX_EVEX_0F389A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132p%XW",	{ XM, Vex, EXx, EXxEVexR }, 0 },
    { "v4fmaddps",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F389B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132s%XW",	{ XMScalar, VexScalar, EXVexWdqScalar, EXxEVexR }, 0 },
    { "v4fmaddss",	{ XMScalar, VexScalar, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F38AA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213p%XW",	{ XM, Vex, EXx, EXxEVexR }, 0 },
    { "v4fnmaddps",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F38AB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213s%XW",	{ XMScalar, VexScalar, EXVexWdqScalar, EXxEVexR }, 0 },
    { "v4fnmaddss",	{ XMScalar, VexScalar, Mxmm }, 0 },
  },
