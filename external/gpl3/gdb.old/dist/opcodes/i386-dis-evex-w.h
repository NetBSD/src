  /* EVEX_W_0F10_P_1 */
  {
    { "vmovss",	{ XMScalar, VexScalarR, EXxmm_md }, 0 },
  },
  /* EVEX_W_0F10_P_3 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ XMScalar, VexScalarR, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F11_P_1 */
  {
    { "vmovss",	{ EXdS, VexScalarR, XMScalar }, 0 },
  },
  /* EVEX_W_0F11_P_3 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ EXqS, VexScalarR, XMScalar }, 0 },
  },
  /* EVEX_W_0F12_P_0_M_1 */
  {
    { "vmovhlps",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F12_P_1 */
  {
    { "vmovsldup",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F12_P_3 */
  {
    { Bad_Opcode },
    { "vmovddup",	{ XM, EXymmq }, 0 },
  },
  /* EVEX_W_0F16_P_0_M_1 */
  {
    { "vmovlhps",	{ XMM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F16_P_1 */
  {
    { "vmovshdup",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F2A_P_3 */
  {
    { "vcvtsi2sd{%LQ|}",	{ XMScalar, VexScalar, Ed }, 0 },
    { "vcvtsi2sd{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR64, Edq }, 0 },
  },
  /* EVEX_W_0F51_P_1 */
  {
    { "vsqrtss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F51_P_3 */
  {
    { Bad_Opcode },
    { "vsqrtsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F58_P_1 */
  {
    { "vaddss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F58_P_3 */
  {
    { Bad_Opcode },
    { "vaddsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F59_P_1 */
  {
    { "vmulss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F59_P_3 */
  {
    { Bad_Opcode },
    { "vmulsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5A_P_0 */
  {
    { "vcvtps2pd",   { XM, EXEvexHalfBcstXmmq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5A_P_1 */
  {
    { "vcvtss2sd",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5A_P_2 */
  {
    { Bad_Opcode },
    { "vcvtpd2ps%XY",	{ XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5A_P_3 */
  {
    { Bad_Opcode },
    { "vcvtsd2ss",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5B_P_0 */
  {
    { "vcvtdq2ps",	{ XM, EXx, EXxEVexR }, 0 },
    { "vcvtqq2ps%XY",	{ XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5B_P_1 */
  {
    { "vcvttps2dq",	{ XM, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5B_P_2 */
  {
    { "vcvtps2dq",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5C_P_1 */
  {
    { "vsubss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5C_P_3 */
  {
    { Bad_Opcode },
    { "vsubsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5D_P_1 */
  {
    { "vminss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5D_P_3 */
  {
    { Bad_Opcode },
    { "vminsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5E_P_1 */
  {
    { "vdivss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5E_P_3 */
  {
    { Bad_Opcode },
    { "vdivsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5F_P_1 */
  {
    { "vmaxss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5F_P_3 */
  {
    { Bad_Opcode },
    { "vmaxsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F62 */
  {
    { "vpunpckldq",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F66 */
  {
    { "vpcmpgtd",	{ XMask, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F6A */
  {
    { "vpunpckhdq",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F6B */
  {
    { "vpackssdw",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F6C */
  {
    { Bad_Opcode },
    { "vpunpcklqdq",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F6D */
  {
    { Bad_Opcode },
    { "vpunpckhqdq",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F6F_P_1 */
  {
    { "vmovdqu32",	{ XM, EXEvexXNoBcst }, 0 },
    { "vmovdqu64",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F6F_P_2 */
  {
    { "vmovdqa32",	{ XM, EXEvexXNoBcst }, 0 },
    { "vmovdqa64",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F6F_P_3 */
  {
    { "vmovdqu8",	{ XM, EXx }, 0 },
    { "vmovdqu16",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F70_P_2 */
  {
    { "vpshufd",	{ XM, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F72_R_2 */
  {
    { "vpsrld",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F72_R_6 */
  {
    { "vpslld",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F73_R_2 */
  {
    { Bad_Opcode },
    { "vpsrlq",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F73_R_6 */
  {
    { Bad_Opcode },
    { "vpsllq",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F76 */
  {
    { "vpcmpeqd",	{ XMask, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F78_P_0 */
  {
    { "vcvttps2udq",	{ XM, EXx, EXxEVexS }, 0 },
    { "vcvttpd2udq%XY",	{ XMxmmq, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F78_P_2 */
  {
    { "vcvttps2uqq",	{ XM, EXEvexHalfBcstXmmq, EXxEVexS }, 0 },
    { "vcvttpd2uqq",	{ XM, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F79_P_0 */
  {
    { "vcvtps2udq",	{ XM, EXx, EXxEVexR }, 0 },
    { "vcvtpd2udq%XY",	{ XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F79_P_2 */
  {
    { "vcvtps2uqq",	{ XM, EXEvexHalfBcstXmmq, EXxEVexR }, 0 },
    { "vcvtpd2uqq",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F7A_P_1 */
  {
    { "vcvtudq2pd",	{ XM, EXEvexHalfBcstXmmq }, 0 },
    { "vcvtuqq2pd",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F7A_P_2 */
  {
    { "vcvttps2qq",	{ XM, EXEvexHalfBcstXmmq, EXxEVexS }, 0 },
    { "vcvttpd2qq",	{ XM, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F7A_P_3 */
  {
    { "vcvtudq2ps",	{ XM, EXx, EXxEVexR }, 0 },
    { "vcvtuqq2ps%XY",	{ XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F7B_P_2 */
  {
    { "vcvtps2qq",	{ XM, EXEvexHalfBcstXmmq, EXxEVexR }, 0 },
    { "vcvtpd2qq",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F7B_P_3 */
  {
    { "vcvtusi2sd{%LQ|}",	{ XMScalar, VexScalar, Ed }, 0 },
    { "vcvtusi2sd{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR64, Edq }, 0 },
  },
  /* EVEX_W_0F7E_P_1 */
  {
    { Bad_Opcode },
    { "vmovq",	{ XMScalar, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F7F_P_1 */
  {
    { "vmovdqu32",	{ EXxS, XM }, 0 },
    { "vmovdqu64",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F7F_P_2 */
  {
    { "vmovdqa32",	{ EXxS, XM }, 0 },
    { "vmovdqa64",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F7F_P_3 */
  {
    { "vmovdqu8",	{ EXxS, XM }, 0 },
    { "vmovdqu16",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0FC2_P_1 */
  {
    { "vcmpss",	{ XMask, VexScalar, EXxmm_md, EXxEVexS, CMP }, 0 },
  },
  /* EVEX_W_0FC2_P_3 */
  {
    { Bad_Opcode },
    { "vcmpsd",	{ XMask, VexScalar, EXxmm_mq, EXxEVexS, CMP }, 0 },
  },
  /* EVEX_W_0FD2 */
  {
    { "vpsrld",		{ XM, Vex, EXxmm }, PREFIX_DATA },
  },
  /* EVEX_W_0FD3 */
  {
    { Bad_Opcode },
    { "vpsrlq",		{ XM, Vex, EXxmm }, PREFIX_DATA },
  },
  /* EVEX_W_0FD4 */
  {
    { Bad_Opcode },
    { "vpaddq",		{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0FD6_L_0 */
  {
    { Bad_Opcode },
    { "vmovq",	{ EXqS, XMScalar }, PREFIX_DATA },
  },
  /* EVEX_W_0FE6_P_1 */
  {
    { "vcvtdq2pd",	{ XM, EXEvexHalfBcstXmmq }, 0 },
    { "vcvtqq2pd",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0FE6_P_2 */
  {
    { Bad_Opcode },
    { "vcvttpd2dq%XY",	{ XMxmmq, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0FE6_P_3 */
  {
    { Bad_Opcode },
    { "vcvtpd2dq%XY",	{ XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0FE7 */
  {
    { "vmovntdq",	{ EXEvexXNoBcst, XM }, PREFIX_DATA },
  },
  /* EVEX_W_0FF2 */
  {
    { "vpslld",		{ XM, Vex, EXxmm }, PREFIX_DATA },
  },
  /* EVEX_W_0FF3 */
  {
    { Bad_Opcode },
    { "vpsllq",		{ XM, Vex, EXxmm }, PREFIX_DATA },
  },
  /* EVEX_W_0FF4 */
  {
    { Bad_Opcode },
    { "vpmuludq",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0FFA */
  {
    { "vpsubd",		{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0FFB */
  {
    { Bad_Opcode },
    { "vpsubq",		{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0FFE */
  {
    { "vpaddd",		{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F380D */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F3810_P_1 */
  {
    { "vpmovuswb",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3810_P_2 */
  {
    { Bad_Opcode },
    { "vpsrlvw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3811_P_1 */
  {
    { "vpmovusdb",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3811_P_2 */
  {
    { Bad_Opcode },
    { "vpsravw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3812_P_1 */
  {
    { "vpmovusqb",	{ EXxmmdw, XM }, 0 },
  },
  /* EVEX_W_0F3812_P_2 */
  {
    { Bad_Opcode },
    { "vpsllvw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3813_P_1 */
  {
    { "vpmovusdw",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3813_P_2 */
  {
    { "vcvtph2ps",	{ XM, EXxmmq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F3814_P_1 */
  {
    { "vpmovusqw",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3815_P_1 */
  {
    { "vpmovusqd",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3819 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3819_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3819_W_1) },
  },
  /* EVEX_W_0F381A */
  {
    { MOD_TABLE (MOD_EVEX_0F381A_W_0) },
    { MOD_TABLE (MOD_EVEX_0F381A_W_1) },
  },
  /* EVEX_W_0F381B */
  {
    { MOD_TABLE (MOD_EVEX_0F381B_W_0) },
    { MOD_TABLE (MOD_EVEX_0F381B_W_1) },
  },
  /* EVEX_W_0F381E */
  {
    { "vpabsd",	{ XM, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F381F */
  {
    { Bad_Opcode },
    { "vpabsq",	{ XM, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F3820_P_1 */
  {
    { "vpmovswb",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3821_P_1 */
  {
    { "vpmovsdb",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3822_P_1 */
  {
    { "vpmovsqb",	{ EXxmmdw, XM }, 0 },
  },
  /* EVEX_W_0F3823_P_1 */
  {
    { "vpmovsdw",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3824_P_1 */
  {
    { "vpmovsqw",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3825_P_1 */
  {
    { "vpmovsqd",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3825_P_2 */
  {
    { "vpmovsxdq",	{ XM, EXxmmq }, 0 },
  },
  /* EVEX_W_0F3828_P_2 */
  {
    { Bad_Opcode },
    { "vpmuldq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3829_P_2 */
  {
    { Bad_Opcode },
    { "vpcmpeqq",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F382A_P_1 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F382A_P_1_W_1) },
  },
  /* EVEX_W_0F382A_P_2 */
  {
    { "vmovntdqa",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F382B */
  {
    { "vpackusdw",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F3830_P_1 */
  {
    { "vpmovwb",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3831_P_1 */
  {
    { "vpmovdb",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3832_P_1 */
  {
    { "vpmovqb",	{ EXxmmdw, XM }, 0 },
  },
  /* EVEX_W_0F3833_P_1 */
  {
    { "vpmovdw",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3834_P_1 */
  {
    { "vpmovqw",	{ EXxmmqd, XM }, 0 },
  },
  /* EVEX_W_0F3835_P_1 */
  {
    { "vpmovqd",	{ EXxmmq, XM }, 0 },
  },
  /* EVEX_W_0F3835_P_2 */
  {
    { "vpmovzxdq",	{ XM, EXxmmq }, 0 },
  },
  /* EVEX_W_0F3837 */
  {
    { Bad_Opcode },
    { "vpcmpgtq",	{ XMask, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F383A_P_1 */
  {
    { MOD_TABLE (MOD_EVEX_0F383A_P_1_W_0) },
  },
  /* EVEX_W_0F3852_P_1 */
  {
    { "vdpbf16ps",	{ XM, Vex, EXx }, 0 },
    { Bad_Opcode },
  },
  /* EVEX_W_0F3859 */
  {
    { "vbroadcasti32x2",	{ XM, EXxmm_mq }, PREFIX_DATA },
    { "vpbroadcastq",	{ XM, EXxmm_mq }, PREFIX_DATA },
  },
  /* EVEX_W_0F385A */
  {
    { MOD_TABLE (MOD_EVEX_0F385A_W_0) },
    { MOD_TABLE (MOD_EVEX_0F385A_W_1) },
  },
  /* EVEX_W_0F385B */
  {
    { MOD_TABLE (MOD_EVEX_0F385B_W_0) },
    { MOD_TABLE (MOD_EVEX_0F385B_W_1) },
  },
  /* EVEX_W_0F3870 */
  {
    { Bad_Opcode },
    { "vpshldvw",  { XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F3872_P_1 */
  {
    { "vcvtneps2bf16%XY", { XMxmmq, EXx }, 0 },
    { Bad_Opcode },
  },
  /* EVEX_W_0F3872_P_2 */
  {
    { Bad_Opcode },
    { "vpshrdvw",  { XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3872_P_3 */
  {
    { "vcvtne2ps2bf16", { XM, Vex, EXx}, 0 },
    { Bad_Opcode },
  },
  /* EVEX_W_0F387A */
  {
    { MOD_TABLE (MOD_EVEX_0F387A_W_0) },
  },
  /* EVEX_W_0F387B */
  {
    { MOD_TABLE (MOD_EVEX_0F387B_W_0) },
  },
  /* EVEX_W_0F3883 */
  {
    { Bad_Opcode },
    { "vpmultishiftqb",	{ XM, Vex, EXx }, PREFIX_DATA },
  },
  /* EVEX_W_0F3891 */
  {
    { "vpgatherqd",	{ XMxmmq, MVexVSIBQDWpX }, PREFIX_DATA },
    { "vpgatherqq",	{ XM, MVexVSIBQWpX }, 0 },
  },
  /* EVEX_W_0F3893 */
  {
    { "vgatherqps",	{ XMxmmq, MVexVSIBQDWpX }, PREFIX_DATA },
    { "vgatherqpd",	{ XM, MVexVSIBQWpX }, 0 },
  },
  /* EVEX_W_0F38A1 */
  {
    { "vpscatterqd",	{ MVexVSIBQDWpX, XMxmmq }, PREFIX_DATA },
    { "vpscatterqq",	{ MVexVSIBQWpX, XM }, 0 },
  },
  /* EVEX_W_0F38A3 */
  {
    { "vscatterqps",	{ MVexVSIBQDWpX, XMxmmq }, PREFIX_DATA },
    { "vscatterqpd",	{ MVexVSIBQWpX, XM }, 0 },
  },
  /* EVEX_W_0F38C7_R_1_M_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_1_M_0_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_1_M_0_W_1) },
  },
  /* EVEX_W_0F38C7_R_2_M_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_2_M_0_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_2_M_0_W_1) },
  },
  /* EVEX_W_0F38C7_R_5_M_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_5_M_0_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_5_M_0_W_1) },
  },
  /* EVEX_W_0F38C7_R_6_M_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_6_M_0_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_6_M_0_W_1) },
  },
  /* EVEX_W_0F3A00 */
  {
    { Bad_Opcode },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A00_W_1) },
  },
  /* EVEX_W_0F3A01 */
  {
    { Bad_Opcode },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A01_W_1) },
  },
  /* EVEX_W_0F3A05 */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, EXx, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F3A08 */
  {
    { "vrndscaleps",	{ XM, EXx, EXxEVexS, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F3A09 */
  {
    { Bad_Opcode },
    { "vrndscalepd",	{ XM, EXx, EXxEVexS, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F3A0A */
  {
    { "vrndscaless",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F3A0B */
  {
    { Bad_Opcode },
    { "vrndscalesd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS, Ib }, PREFIX_DATA },
  },
  /* EVEX_W_0F3A18 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A18_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A18_W_1) },
  },
  /* EVEX_W_0F3A19 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A19_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A19_W_1) },
  },
  /* EVEX_W_0F3A1A */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1A_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1A_W_1) },
  },
  /* EVEX_W_0F3A1B */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1B_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1B_W_1) },
  },
  /* EVEX_W_0F3A21 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A21_W_0) },
  },
  /* EVEX_W_0F3A23 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A23_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A23_W_1) },
  },
  /* EVEX_W_0F3A38 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A38_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A38_W_1) },
  },
  /* EVEX_W_0F3A39 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A39_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A39_W_1) },
  },
  /* EVEX_W_0F3A3A */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3A_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3A_W_1) },
  },
  /* EVEX_W_0F3A3B */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3B_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3B_W_1) },
  },
  /* EVEX_W_0F3A42 */
  {
    { "vdbpsadbw",	{ XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A43 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A43_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A43_W_1) },
  },
  /* EVEX_W_0F3A70 */
  {
    { Bad_Opcode },
    { "vpshldw",   { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A72 */
  {
    { Bad_Opcode },
    { "vpshrdw",   { XM, Vex, EXx, Ib }, 0 },
  },
