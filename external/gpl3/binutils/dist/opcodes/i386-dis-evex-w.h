  /* EVEX_W_0F10_P_0 */
  {
    { "vmovups",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F10_P_1 */
  {
    { "vmovss",	{ XMVexScalar, VexScalar, EXdScalar }, 0 },
  },
  /* EVEX_W_0F10_P_2 */
  {
    { Bad_Opcode },
    { "vmovupd",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F10_P_3 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ XMVexScalar, VexScalar, EXqScalar }, 0 },
  },
  /* EVEX_W_0F11_P_0 */
  {
    { "vmovups",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F11_P_1 */
  {
    { "vmovss",	{ EXdVexScalarS, VexScalar, XMScalar }, 0 },
  },
  /* EVEX_W_0F11_P_2 */
  {
    { Bad_Opcode },
    { "vmovupd",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F11_P_3 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ EXqVexScalarS, VexScalar, XMScalar }, 0 },
  },
  /* EVEX_W_0F12_P_0_M_0 */
  {
    { "vmovlps",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F12_P_0_M_1 */
  {
    { "vmovhlps",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F12_P_1 */
  {
    { "vmovsldup",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F12_P_2 */
  {
    { Bad_Opcode },
    { "vmovlpd",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F12_P_3 */
  {
    { Bad_Opcode },
    { "vmovddup",	{ XM, EXymmq }, 0 },
  },
  /* EVEX_W_0F13_P_0 */
  {
    { "vmovlps",	{ EXxmm_mq, XMM }, 0 },
  },
  /* EVEX_W_0F13_P_2 */
  {
    { Bad_Opcode },
    { "vmovlpd",	{ EXxmm_mq, XMM }, 0 },
  },
  /* EVEX_W_0F14_P_0 */
  {
    { "vunpcklps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F14_P_2 */
  {
    { Bad_Opcode },
    { "vunpcklpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F15_P_0 */
  {
    { "vunpckhps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F15_P_2 */
  {
    { Bad_Opcode },
    { "vunpckhpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F16_P_0_M_0 */
  {
    { "vmovhps",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F16_P_0_M_1 */
  {
    { "vmovlhps",	{ XMM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F16_P_1 */
  {
    { "vmovshdup",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F16_P_2 */
  {
    { Bad_Opcode },
    { "vmovhpd",	{ XMM, Vex, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F17_P_0 */
  {
    { "vmovhps",	{ EXxmm_mq, XMM }, 0 },
  },
  /* EVEX_W_0F17_P_2 */
  {
    { Bad_Opcode },
    { "vmovhpd",	{ EXxmm_mq, XMM }, 0 },
  },
  /* EVEX_W_0F28_P_0 */
  {
    { "vmovaps",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F28_P_2 */
  {
    { Bad_Opcode },
    { "vmovapd",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F29_P_0 */
  {
    { "vmovaps",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F29_P_2 */
  {
    { Bad_Opcode },
    { "vmovapd",	{ EXxS, XM }, 0 },
  },
  /* EVEX_W_0F2A_P_3 */
  {
    { "vcvtsi2sd%LQ",	{ XMScalar, VexScalar, Ed }, 0 },
    { "vcvtsi2sd%LQ",	{ XMScalar, VexScalar, EXxEVexR64, Edq }, 0 },
  },
  /* EVEX_W_0F2B_P_0 */
  {
    { "vmovntps",	{ EXx, XM }, 0 },
  },
  /* EVEX_W_0F2B_P_2 */
  {
    { Bad_Opcode },
    { "vmovntpd",	{ EXx, XM }, 0 },
  },
  /* EVEX_W_0F2E_P_0 */
  {
    { "vucomiss",	{ XMScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F2E_P_2 */
  {
    { Bad_Opcode },
    { "vucomisd",	{ XMScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F2F_P_0 */
  {
    { "vcomiss",	{ XMScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F2F_P_2 */
  {
    { Bad_Opcode },
    { "vcomisd",	{ XMScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F51_P_0 */
  {
    { "vsqrtps",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F51_P_1 */
  {
    { "vsqrtss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F51_P_2 */
  {
    { Bad_Opcode },
    { "vsqrtpd",	{ XM, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F51_P_3 */
  {
    { Bad_Opcode },
    { "vsqrtsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F54_P_0 */
  {
    { "vandps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F54_P_2 */
  {
    { Bad_Opcode },
    { "vandpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F55_P_0 */
  {
    { "vandnps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F55_P_2 */
  {
    { Bad_Opcode },
    { "vandnpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F56_P_0 */
  {
    { "vorps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F56_P_2 */
  {
    { Bad_Opcode },
    { "vorpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F57_P_0 */
  {
    { "vxorps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F57_P_2 */
  {
    { Bad_Opcode },
    { "vxorpd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F58_P_0 */
  {
    { "vaddps",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F58_P_1 */
  {
    { "vaddss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F58_P_2 */
  {
    { Bad_Opcode },
    { "vaddpd",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F58_P_3 */
  {
    { Bad_Opcode },
    { "vaddsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F59_P_0 */
  {
    { "vmulps",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F59_P_1 */
  {
    { "vmulss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F59_P_2 */
  {
    { Bad_Opcode },
    { "vmulpd",	{ XM, Vex, EXx, EXxEVexR }, 0 },
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
  /* EVEX_W_0F5C_P_0 */
  {
    { "vsubps",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5C_P_1 */
  {
    { "vsubss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5C_P_2 */
  {
    { Bad_Opcode },
    { "vsubpd",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5C_P_3 */
  {
    { Bad_Opcode },
    { "vsubsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5D_P_0 */
  {
    { "vminps",	{ XM, Vex, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5D_P_1 */
  {
    { "vminss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5D_P_2 */
  {
    { Bad_Opcode },
    { "vminpd",	{ XM, Vex, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5D_P_3 */
  {
    { Bad_Opcode },
    { "vminsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5E_P_0 */
  {
    { "vdivps",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5E_P_1 */
  {
    { "vdivss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5E_P_2 */
  {
    { Bad_Opcode },
    { "vdivpd",	{ XM, Vex, EXx, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5E_P_3 */
  {
    { Bad_Opcode },
    { "vdivsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR }, 0 },
  },
  /* EVEX_W_0F5F_P_0 */
  {
    { "vmaxps",	{ XM, Vex, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5F_P_1 */
  {
    { "vmaxss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5F_P_2 */
  {
    { Bad_Opcode },
    { "vmaxpd",	{ XM, Vex, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F5F_P_3 */
  {
    { Bad_Opcode },
    { "vmaxsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F62_P_2 */
  {
    { "vpunpckldq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F66_P_2 */
  {
    { "vpcmpgtd",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F6A_P_2 */
  {
    { "vpunpckhdq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F6B_P_2 */
  {
    { "vpackssdw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F6C_P_2 */
  {
    { Bad_Opcode },
    { "vpunpcklqdq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F6D_P_2 */
  {
    { Bad_Opcode },
    { "vpunpckhqdq",	{ XM, Vex, EXx }, 0 },
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
  /* EVEX_W_0F72_R_2_P_2 */
  {
    { "vpsrld",	{ Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F72_R_6_P_2 */
  {
    { "vpslld",	{ Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F73_R_2_P_2 */
  {
    { Bad_Opcode },
    { "vpsrlq",	{ Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F73_R_6_P_2 */
  {
    { Bad_Opcode },
    { "vpsllq",	{ Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F76_P_2 */
  {
    { "vpcmpeqd",	{ XMask, Vex, EXx }, 0 },
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
    { "vcvtusi2sd%LQ",	{ XMScalar, VexScalar, Ed }, 0 },
    { "vcvtusi2sd%LQ",	{ XMScalar, VexScalar, EXxEVexR64, Edq }, 0 },
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
  /* EVEX_W_0FC2_P_0 */
  {
    { "vcmpps",	{ XMask, Vex, EXx, EXxEVexS, VCMP }, 0 },
  },
  /* EVEX_W_0FC2_P_1 */
  {
    { "vcmpss",	{ XMask, VexScalar, EXxmm_md, EXxEVexS, VCMP }, 0 },
  },
  /* EVEX_W_0FC2_P_2 */
  {
    { Bad_Opcode },
    { "vcmppd",	{ XMask, Vex, EXx, EXxEVexS, VCMP }, 0 },
  },
  /* EVEX_W_0FC2_P_3 */
  {
    { Bad_Opcode },
    { "vcmpsd",	{ XMask, VexScalar, EXxmm_mq, EXxEVexS, VCMP }, 0 },
  },
  /* EVEX_W_0FC6_P_0 */
  {
    { "vshufps",	{ XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0FC6_P_2 */
  {
    { Bad_Opcode },
    { "vshufpd",	{ XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0FD2_P_2 */
  {
    { "vpsrld",	{ XM, Vex, EXxmm }, 0 },
  },
  /* EVEX_W_0FD3_P_2 */
  {
    { Bad_Opcode },
    { "vpsrlq",	{ XM, Vex, EXxmm }, 0 },
  },
  /* EVEX_W_0FD4_P_2 */
  {
    { Bad_Opcode },
    { "vpaddq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0FD6_P_2 */
  {
    { Bad_Opcode },
    { "vmovq",	{ EXxmm_mq, XMScalar }, 0 },
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
  /* EVEX_W_0FE7_P_2 */
  {
    { "vmovntdq",	{ EXEvexXNoBcst, XM }, 0 },
  },
  /* EVEX_W_0FF2_P_2 */
  {
    { "vpslld",	{ XM, Vex, EXxmm }, 0 },
  },
  /* EVEX_W_0FF3_P_2 */
  {
    { Bad_Opcode },
    { "vpsllq",	{ XM, Vex, EXxmm }, 0 },
  },
  /* EVEX_W_0FF4_P_2 */
  {
    { Bad_Opcode },
    { "vpmuludq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0FFA_P_2 */
  {
    { "vpsubd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0FFB_P_2 */
  {
    { Bad_Opcode },
    { "vpsubq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0FFE_P_2 */
  {
    { "vpaddd",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F380C_P_2 */
  {
    { "vpermilps",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F380D_P_2 */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, Vex, EXx }, 0 },
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
  /* EVEX_W_0F3818_P_2 */
  {
    { "vbroadcastss",	{ XM, EXxmm_md }, 0 },
  },
  /* EVEX_W_0F3819_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3819_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3819_P_2_W_1) },
  },
  /* EVEX_W_0F381A_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381A_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F381A_P_2_W_1) },
  },
  /* EVEX_W_0F381B_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381B_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F381B_P_2_W_1) },
  },
  /* EVEX_W_0F381E_P_2 */
  {
    { "vpabsd",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F381F_P_2 */
  {
    { Bad_Opcode },
    { "vpabsq",	{ XM, EXx }, 0 },
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
  /* EVEX_W_0F3826_P_1 */
  {
    { "vptestnmb",	{ XMask, Vex, EXx }, 0 },
    { "vptestnmw",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3826_P_2 */
  {
    { "vptestmb",	{ XMask, Vex, EXx }, 0 },
    { "vptestmw",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3828_P_1 */
  {
    { "vpmovm2b",	{ XM, MaskR }, 0 },
    { "vpmovm2w",	{ XM, MaskR }, 0 },
  },
  /* EVEX_W_0F3828_P_2 */
  {
    { Bad_Opcode },
    { "vpmuldq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3829_P_1 */
  {
    { "vpmovb2m",	{ XMask, EXx }, 0 },
    { "vpmovw2m",	{ XMask, EXx }, 0 },
  },
  /* EVEX_W_0F3829_P_2 */
  {
    { Bad_Opcode },
    { "vpcmpeqq",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F382A_P_1 */
  {
    { Bad_Opcode },
    { "vpbroadcastmb2q",	{ XM, MaskR }, 0 },
  },
  /* EVEX_W_0F382A_P_2 */
  {
    { "vmovntdqa",	{ XM, EXEvexXNoBcst }, 0 },
  },
  /* EVEX_W_0F382B_P_2 */
  {
    { "vpackusdw",	{ XM, Vex, EXx }, 0 },
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
  /* EVEX_W_0F3837_P_2 */
  {
    { Bad_Opcode },
    { "vpcmpgtq",	{ XMask, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3838_P_1 */
  {
    { "vpmovm2d",	{ XM, MaskR }, 0 },
    { "vpmovm2q",	{ XM, MaskR }, 0 },
  },
  /* EVEX_W_0F3839_P_1 */
  {
    { "vpmovd2m",	{ XMask, EXx }, 0 },
    { "vpmovq2m",	{ XMask, EXx }, 0 },
  },
  /* EVEX_W_0F383A_P_1 */
  {
    { "vpbroadcastmw2d",	{ XM, MaskR }, 0 },
  },
  /* EVEX_W_0F3840_P_2 */
  {
    { "vpmulld",	{ XM, Vex, EXx }, 0 },
    { "vpmullq",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3852_P_1 */
  {
    { "vdpbf16ps",	{ XM, Vex, EXx }, 0 },
    { Bad_Opcode },
  },
  /* EVEX_W_0F3854_P_2 */
  {
    { "vpopcntb",	{ XM, EXx }, 0 },
    { "vpopcntw",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F3855_P_2 */
  {
    { "vpopcntd",	{ XM, EXx }, 0 },
    { "vpopcntq",	{ XM, EXx }, 0 },
  },
  /* EVEX_W_0F3858_P_2 */
  {
    { "vpbroadcastd",	{ XM, EXxmm_md }, 0 },
  },
  /* EVEX_W_0F3859_P_2 */
  {
    { "vbroadcasti32x2",	{ XM, EXxmm_mq }, 0 },
    { "vpbroadcastq",	{ XM, EXxmm_mq }, 0 },
  },
  /* EVEX_W_0F385A_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385A_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F385A_P_2_W_1) },
  },
  /* EVEX_W_0F385B_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385B_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F385B_P_2_W_1) },
  },
  /* EVEX_W_0F3862_P_2 */
  {
    { "vpexpandb", { XM, EXbScalar }, 0 },
    { "vpexpandw", { XM, EXwScalar }, 0 },
  },
  /* EVEX_W_0F3863_P_2 */
  {
    { "vpcompressb",   { EXbScalar, XM }, 0 },
    { "vpcompressw",   { EXwScalar, XM }, 0 },
  },
  /* EVEX_W_0F3866_P_2 */
  {
    { "vpblendmb",	{ XM, Vex, EXx }, 0 },
    { "vpblendmw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3868_P_3 */
  {
    { "vp2intersectd", { XMask, Vex, EXx, EXxEVexS }, 0 },
    { "vp2intersectq", { XMask, Vex, EXx, EXxEVexS }, 0 },
  },
  /* EVEX_W_0F3870_P_2 */
  {
    { Bad_Opcode },
    { "vpshldvw",  { XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3871_P_2 */
  {
    { "vpshldvd",  { XM, Vex, EXx }, 0 },
    { "vpshldvq",  { XM, Vex, EXx }, 0 },
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
  /* EVEX_W_0F3873_P_2 */
  {
    { "vpshrdvd",  { XM, Vex, EXx }, 0 },
    { "vpshrdvq",  { XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3875_P_2 */
  {
    { "vpermi2b",	{ XM, Vex, EXx }, 0 },
    { "vpermi2w",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3878_P_2 */
  {
    { "vpbroadcastb",	{ XM, EXxmm_mb }, 0 },
  },
  /* EVEX_W_0F3879_P_2 */
  {
    { "vpbroadcastw",	{ XM, EXxmm_mw }, 0 },
  },
  /* EVEX_W_0F387A_P_2 */
  {
    { "vpbroadcastb",	{ XM, Rd }, 0 },
  },
  /* EVEX_W_0F387B_P_2 */
  {
    { "vpbroadcastw",	{ XM, Rd }, 0 },
  },
  /* EVEX_W_0F387D_P_2 */
  {
    { "vpermt2b",	{ XM, Vex, EXx }, 0 },
    { "vpermt2w",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3883_P_2 */
  {
    { Bad_Opcode },
    { "vpmultishiftqb",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F388D_P_2 */
  {
    { "vpermb",	{ XM, Vex, EXx }, 0 },
    { "vpermw",	{ XM, Vex, EXx }, 0 },
  },
  /* EVEX_W_0F3891_P_2 */
  {
    { "vpgatherqd",	{ XMxmmq, MVexVSIBQDWpX }, 0 },
    { "vpgatherqq",	{ XM, MVexVSIBQWpX }, 0 },
  },
  /* EVEX_W_0F3893_P_2 */
  {
    { "vgatherqps",	{ XMxmmq, MVexVSIBQDWpX }, 0 },
    { "vgatherqpd",	{ XM, MVexVSIBQWpX }, 0 },
  },
  /* EVEX_W_0F38A1_P_2 */
  {
    { "vpscatterqd",	{ MVexVSIBQDWpX, XMxmmq }, 0 },
    { "vpscatterqq",	{ MVexVSIBQWpX, XM }, 0 },
  },
  /* EVEX_W_0F38A3_P_2 */
  {
    { "vscatterqps",	{ MVexVSIBQDWpX, XMxmmq }, 0 },
    { "vscatterqpd",	{ MVexVSIBQWpX, XM }, 0 },
  },
  /* EVEX_W_0F38C7_R_1_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_1_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_1_P_2_W_1) },
  },
  /* EVEX_W_0F38C7_R_2_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_2_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_2_P_2_W_1) },
  },
  /* EVEX_W_0F38C7_R_5_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_5_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_5_P_2_W_1) },
  },
  /* EVEX_W_0F38C7_R_6_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_6_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C7_R_6_P_2_W_1) },
  },
  /* EVEX_W_0F3A00_P_2 */
  {
    { Bad_Opcode },
    { "vpermq",	{ XM, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A01_P_2 */
  {
    { Bad_Opcode },
    { "vpermpd",	{ XM, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A04_P_2 */
  {
    { "vpermilps",	{ XM, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A05_P_2 */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A08_P_2 */
  {
    { "vrndscaleps",	{ XM, EXx, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A09_P_2 */
  {
    { Bad_Opcode },
    { "vrndscalepd",	{ XM, EXx, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A0A_P_2 */
  {
    { "vrndscaless",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A0B_P_2 */
  {
    { Bad_Opcode },
    { "vrndscalesd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A18_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A18_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A18_P_2_W_1) },
  },
  /* EVEX_W_0F3A19_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A19_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A19_P_2_W_1) },
  },
  /* EVEX_W_0F3A1A_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1A_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1A_P_2_W_1) },
  },
  /* EVEX_W_0F3A1B_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1B_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A1B_P_2_W_1) },
  },
  /* EVEX_W_0F3A1D_P_2 */
  {
    { "vcvtps2ph",	{ EXxmmq, XM, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A21_P_2 */
  {
    { "vinsertps",	{ XMM, Vex, EXxmm_md, Ib }, 0 },
  },
  /* EVEX_W_0F3A23_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A23_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A23_P_2_W_1) },
  },
  /* EVEX_W_0F3A38_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A38_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A38_P_2_W_1) },
  },
  /* EVEX_W_0F3A39_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A39_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A39_P_2_W_1) },
  },
  /* EVEX_W_0F3A3A_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3A_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3A_P_2_W_1) },
  },
  /* EVEX_W_0F3A3B_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3B_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A3B_P_2_W_1) },
  },
  /* EVEX_W_0F3A3E_P_2 */
  {
    { "vpcmpub",	{ XMask, Vex, EXx, VPCMP }, 0 },
    { "vpcmpuw",	{ XMask, Vex, EXx, VPCMP }, 0 },
  },
  /* EVEX_W_0F3A3F_P_2 */
  {
    { "vpcmpb",	{ XMask, Vex, EXx, VPCMP }, 0 },
    { "vpcmpw",	{ XMask, Vex, EXx, VPCMP }, 0 },
  },
  /* EVEX_W_0F3A42_P_2 */
  {
    { "vdbpsadbw",	{ XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A43_P_2 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A43_P_2_W_0) },
    { EVEX_LEN_TABLE (EVEX_LEN_0F3A43_P_2_W_1) },
  },
  /* EVEX_W_0F3A50_P_2 */
  {
    { "vrangeps",	{ XM, Vex, EXx, EXxEVexS, Ib }, 0 },
    { "vrangepd",	{ XM, Vex, EXx, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A51_P_2 */
  {
    { "vrangess",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS, Ib }, 0 },
    { "vrangesd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A56_P_2 */
  {
    { "vreduceps",	{ XM, EXx, EXxEVexS, Ib }, 0 },
    { "vreducepd",	{ XM, EXx, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A57_P_2 */
  {
    { "vreducess",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS, Ib }, 0 },
    { "vreducesd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS, Ib }, 0 },
  },
  /* EVEX_W_0F3A66_P_2 */
  {
    { "vfpclassps%XZ",	{ XMask, EXx, Ib }, 0 },
    { "vfpclasspd%XZ",	{ XMask, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A67_P_2 */
  {
    { "vfpclassss",	{ XMask, EXxmm_md, Ib }, 0 },
    { "vfpclasssd",	{ XMask, EXxmm_mq, Ib }, 0 },
  },
  /* EVEX_W_0F3A70_P_2 */
  {
    { Bad_Opcode },
    { "vpshldw",   { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A71_P_2 */
  {
    { "vpshldd",   { XM, Vex, EXx, Ib }, 0 },
    { "vpshldq",   { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A72_P_2 */
  {
    { Bad_Opcode },
    { "vpshrdw",   { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3A73_P_2 */
  {
    { "vpshrdd",   { XM, Vex, EXx, Ib }, 0 },
    { "vpshrdq",   { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3ACE_P_2 */
  {
    { Bad_Opcode },
    { "vgf2p8affineqb",    { XM, Vex, EXx, Ib }, 0 },
  },
  /* EVEX_W_0F3ACF_P_2 */
  {
    { Bad_Opcode },
    { "vgf2p8affineinvqb", { XM, Vex, EXx, Ib }, 0 },
  },
