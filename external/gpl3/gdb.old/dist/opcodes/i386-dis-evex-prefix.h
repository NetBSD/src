  /* PREFIX_EVEX_0F2E */
  {
    { "%XEvucomis%XS",	{ XMScalar, EXd, EXxEVexS }, 0 },
    { "vucomxs%XS",	{ XMScalar, EXd, EXxEVexS }, 0 },
    { "%XEvucomis%XD",	{ XMScalar, EXq, EXxEVexS }, 0 },
    { "vucomxs%XD",	{ XMScalar, EXq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F2F */
  {
    { "%XEvcomis%XS",	{ XMScalar, EXd, EXxEVexS }, 0 },
    { "vcomxs%XS",	{ XMScalar, EXd, EXxEVexS }, 0 },
    { "%XEvcomis%XD",	{ XMScalar, EXq, EXxEVexS }, 0 },
    { "vcomxs%XD",	{ XMScalar, EXq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F5B */
  {
    { VEX_W_TABLE (EVEX_W_0F5B_P_0) },
    { "%XEvcvttp%XS2dq", { XM, EXx, EXxEVexS }, 0 },
    { "%XEvcvtp%XS2dq", { XM, EXx, EXxEVexR }, 0 },
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
    { "%XEvpshufhw",	{ XM, EXx, Ib }, 0 },
    { VEX_W_TABLE (EVEX_W_0F70_P_2) },
    { "%XEvpshuflw",	{ XM, EXx, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F78 */
  {
    { VEX_W_TABLE (EVEX_W_0F78_P_0) },
    { "vcvttss2usi",	{ Gdq, EXd, EXxEVexS }, 0 },
    { VEX_W_TABLE (EVEX_W_0F78_P_2) },
    { "vcvttsd2usi",	{ Gdq, EXq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F79 */
  {
    { VEX_W_TABLE (EVEX_W_0F79_P_0) },
    { "vcvtss2usi",	{ Gdq, EXd, EXxEVexR }, 0 },
    { VEX_W_TABLE (EVEX_W_0F79_P_2) },
    { "vcvtsd2usi",	{ Gdq, EXq, EXxEVexR }, 0 },
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
    { "vcvtusi2ssY{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR, Edq }, 0 },
    { VEX_W_TABLE (EVEX_W_0F7B_P_2) },
    { "vcvtusi2sdY{%LQ|}",	{ XMScalar, VexScalar, EXxEVexR64, Edq }, 0 },
  },
  /* PREFIX_EVEX_0F7E */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7E_P_1) },
    { VEX_LEN_TABLE (VEX_LEN_0F7E_P_2) },
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
    { "vcmppX",	{ MaskG, Vex, EXx, EXxEVexS, CMP }, PREFIX_OPCODE },
    { "vcmps%XS",	{ MaskG, VexScalar, EXd, EXxEVexS, CMP }, 0 },
    { "vcmppX",	{ MaskG, Vex, EXx, EXxEVexS, CMP }, PREFIX_OPCODE },
    { "vcmps%XD",	{ MaskG, VexScalar, EXq, EXxEVexS, CMP }, 0 },
  },
  /* PREFIX_EVEX_0FE6 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FE6_P_1) },
    { "%XEvcvttp%XD2dq%XY", { XMxmmq, EXx, EXxEVexS }, 0 },
    { "%XEvcvtp%XD2dq%XY", { XMxmmq, EXx, EXxEVexR }, 0 },
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
    { "%XEvcvtph2p%XS", { XM, EXxmmq, EXxEVexS }, 0 },
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
    { "%XEvpmovsxbw",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3821 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3821_P_1) },
    { "%XEvpmovsxbd",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3822 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3822_P_1) },
    { "%XEvpmovsxbq",	{ XM, EXxmmdw }, 0 },
  },
  /* PREFIX_EVEX_0F3823 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3823_P_1) },
    { "%XEvpmovsxwd",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3824 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3824_P_1) },
    { "%XEvpmovsxwq",	{ XM, EXxmmqd }, 0 },
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
    { "vptestnm%BW",	{ MaskG, Vex, EXx }, 0 },
    { "vptestm%BW",	{ MaskG, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3827 */
  {
    { Bad_Opcode },
    { "vptestnm%DQ",	{ MaskG, Vex, EXx }, 0 },
    { "vptestm%DQ",	{ MaskG, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3828 */
  {
    { Bad_Opcode },
    { "vpmovm2Y%BW",	{ XM, MaskR }, 0 },
    { VEX_W_TABLE (EVEX_W_0F3828_P_2) },
  },
  /* PREFIX_EVEX_0F3829 */
  {
    { Bad_Opcode },
    { "vpmov%BW2mY",	{ MaskG, Ux }, 0 },
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
    { "%XEvpmovzxbw",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3831 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3831_P_1) },
    { "%XEvpmovzxbd",	{ XM, EXxmmqd }, 0 },
  },
  /* PREFIX_EVEX_0F3832 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3832_P_1) },
    { "%XEvpmovzxbq",	{ XM, EXxmmdw }, 0 },
  },
  /* PREFIX_EVEX_0F3833 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3833_P_1) },
    { "%XEvpmovzxwd",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_0F3834 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3834_P_1) },
    { "%XEvpmovzxwq",	{ XM, EXxmmqd }, 0 },
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
    { "vpmovm2Y%DQ",	{ XM, MaskR }, 0 },
    { "%XEvpminsb",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3839 */
  {
    { Bad_Opcode },
    { "vpmov%DQ2mY",	{ MaskG, Ux }, 0 },
    { "%XEvpmins%DQ",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F383A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F383A_P_1) },
    { "%XEvpminuw",	{ XM, Vex, EXx }, 0 },
  },
  /* PREFIX_EVEX_0F3852 */
  {
    { "vdpphp%XS",	{ XM, Vex, EXx }, 0 },
    { "vdpbf16p%XS",	{ XM, Vex, EXx }, 0 },
    { VEX_W_TABLE (VEX_W_0F3852) },
    { "vp4dpws%XSd",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F3853 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (VEX_W_0F3853) },
    { "vp4dpws%XSds",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F3868 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vp2intersectY%DQ", { MaskG, Vex, EXx, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_0F3872 */
  {
    { Bad_Opcode },
    { "vcvtnep%XS2bf16%XY", { XMxmmq, EXx }, 0 },
    { VEX_W_TABLE (EVEX_W_0F3872_P_2) },
    { "vcvtne2p%XS2bf16", { XM, Vex, EXx}, 0 },
  },
  /* PREFIX_EVEX_0F3874 */
  {
    { "vcvtbiasp%XH2bf8",	{ XMxmmq, Vex, EXxh }, 0 },
    { "vcvtnep%XH2bf8%XY",	{ XMxmmq, EXxh }, 0 },
    { Bad_Opcode },
    { "vcvtne2p%XH2bf8",	{ XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_0F389A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "%XEvfmsub132p%XW",	{ XM, Vex, EXx, EXxEVexR }, 0 },
    { "v4fmaddp%XS",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F389B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "%XEvfmsub132s%XW",	{ XMScalar, VexScalar, EXdq, EXxEVexR }, 0 },
    { "v4fmadds%XS",	{ XMScalar, VexScalar, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F38AA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "%XEvfmsub213p%XW",	{ XM, Vex, EXx, EXxEVexR }, 0 },
    { "v4fnmaddp%XS",	{ XM, Vex, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F38AB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "%XEvfmsub213s%XW",	{ XMScalar, VexScalar, EXdq, EXxEVexR }, 0 },
    { "v4fnmadds%XS",	{ XMScalar, VexScalar, Mxmm }, 0 },
  },
  /* PREFIX_EVEX_0F3A08 */
  {
    { "vrndscalep%XH",  { XM, EXxh, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vrndscalep%XS",  { XM, EXx, EXxEVexS, Ib }, 0 },
    { "vrndscalenep%XB",	{ XM, EXxh, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A0A */
  {
    { "vrndscales%XH",  { XMScalar, VexScalar, EXw, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vrndscales%XS",  { XMScalar, VexScalar, EXd, EXxEVexS, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A26 */
  {
    { "vgetmantp%XH",     { XM, EXxh, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vgetmantp%XW",	{ XM, EXx, EXxEVexS, Ib }, 0 },
    { "vgetmantp%XB",	{ XM, EXxh, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A27 */
  {
    { "vgetmants%XH",     { XMScalar, VexScalar, EXw, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vgetmants%XW",	{ XMScalar, VexScalar, EXdq, EXxEVexS, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A42_W_0 */
  {
    { Bad_Opcode },
    { "%XEvmpsadbw",	{ XM, Vex, EXx, Ib }, 0 },
    { "vdbpsadbw",	{ XM, Vex, EXx, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A52 */
  {
    { "vminmaxp%XH",	{ XM, Vex, EXxh, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vminmaxp%XW",	{ XM, Vex, EXx, EXxEVexS, Ib }, 0 },
    { "vminmaxp%XB",	{ XM, Vex, EXxh, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A53 */
  {
    { "vminmaxs%XH",	{ XMScalar, VexScalar, EXw, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vminmaxs%XW",	{ XMScalar, VexScalar, EXdq, EXxEVexS, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A56 */
  {
    { "vreducep%XH",      { XM, EXxh, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vreducep%XW",	{ XM, EXx, EXxEVexS, Ib }, 0 },
    { "vreducenep%XB",	{ XM, EXxh, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A57 */
  {
    { "vreduces%XH",      { XMScalar, VexScalar, EXw, EXxEVexS, Ib }, 0 },
    { Bad_Opcode },
    { "vreduces%XW",	{ XMScalar, VexScalar, EXdq, EXxEVexS, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A66 */
  {
    { "vfpclassp%XH%XZ",  { MaskG, EXxh, Ib }, 0 },
    { Bad_Opcode },
    { "vfpclassp%XW%XZ",    { MaskG, EXx, Ib }, 0 },
    { "vfpclassp%XB%XZ",	{ MaskG, EXxh, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3A67 */
  {
    { "vfpclasss%XH",     { MaskG, EXw, Ib }, 0 },
    { Bad_Opcode },
    { "vfpclasss%XW",	{ MaskG, EXdq, Ib }, 0 },
  },
  /* PREFIX_EVEX_0F3AC2 */
  {
    { "vcmpp%XH", { MaskG, Vex, EXxh, EXxEVexS, CMP }, 0 },
    { "vcmps%XH", { MaskG, VexScalar, EXw, EXxEVexS, CMP }, 0 },
    { Bad_Opcode },
    { "vcmpp%XB",	{ MaskG, Vex, EXxh, CMP }, 0 },
  },
  /* PREFIX_EVEX_MAP4_4x */
  {
    { "%CFcmov%CCS",	{ VexGv, { CFCMOV_Fixup, 0 }, { CFCMOV_Fixup, 1 } }, 0 },
    { Bad_Opcode },
    { "%CFcmov%CCS",	{ VexGv, { CFCMOV_Fixup, 0 }, { CFCMOV_Fixup, 1 } }, 0 },
    { "set%ZU%CC",	{ Eb }, 0 },
  },
  /* PREFIX_EVEX_MAP4_F0 */
  {
    { "crc32A", { Gdq, Eb }, 0 },
    { "invept", { Gm, Mo }, 0 },
  },
  /* PREFIX_EVEX_MAP4_F1 */
  {
    { "crc32Q", { Gdq, Ev }, 0 },
    { "invvpid", { Gm, Mo }, 0 },
    { "crc32Q", { Gdq, Ev }, 0 },
  },
  /* PREFIX_EVEX_MAP4_F2 */
  {
    { Bad_Opcode },
    { "invpcid", { Gm, M }, 0 },
  },
  /* PREFIX_EVEX_MAP4_F8 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_MAP4_F8_P_1) },
    { "movdir64b", { Gva, M }, 0 },
    { MOD_TABLE (MOD_EVEX_MAP4_F8_P_3) },
  },
  /* PREFIX_EVEX_MAP5_10 */
  {
    { Bad_Opcode },
    { "vmovs%XH", { XMScalar, VexScalarR, EXw }, 0 },
  },
  /* PREFIX_EVEX_MAP5_11 */
  {
    { Bad_Opcode },
    { "vmovs%XH", { EXwS, VexScalarR, XMScalar }, 0 },
  },
  /* PREFIX_EVEX_MAP5_18 */
  {
    { "vcvtbiasp%XH2hf8",	{ XMxmmq, Vex, EXxh }, 0 },
    { "vcvtnep%XH2hf8%XY",	{ XMxmmq, EXxh }, 0 },
    { Bad_Opcode },
    { "vcvtne2p%XH2hf8",	{ XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_1B */
  {
    { "vcvtbiasp%XH2hf8s",	{ XMxmmq, Vex, EXxh }, 0 },
    { "vcvtnep%XH2hf8s%XY",	{ XMxmmq, EXxh }, 0 },
    { Bad_Opcode },
    { "vcvtne2p%XH2hf8s",	{ XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_1D */
  {
    { "vcvtss2s%XH",      { XMScalar, VexScalar, EXd, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vcvtps2p%XHx%XY",  { XMxmmq, EXx, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_1E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vcvthf82p%XH",	{ XM, EXxmmq }, 0 },
  },
  /* PREFIX_EVEX_MAP5_2A */
  {
    { Bad_Opcode },
    { "vcvtsi2shY{%LQ|}",        { XMScalar, VexScalar, EXxEVexR, Edq }, 0 },
  },
  /* PREFIX_EVEX_MAP5_2C */
  {
    { Bad_Opcode },
    { "vcvttsh2si",     { Gdq, EXw, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_2D */
  {
    { Bad_Opcode },
    { "vcvtsh2si",      { Gdq, EXw, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_2E */
  {
    { "vucomisY%XH",	{ XMScalar, EXw, EXxEVexS }, 0 },
    { "vucomxs%XH",	{ XMScalar, EXw, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_2F */
  {
    { "vcomisY%XH",	{ XMScalar, EXw, EXxEVexS }, 0 },
    { "vcomxs%XH",	{ XMScalar, EXw, EXxEVexS }, 0 },
    { "vcoms%XB",	{ XMScalar, EXw, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_51 */
  {
    { "vsqrtp%XH",        { XM, EXxh, EXxEVexR }, 0 },
    { "vsqrts%XH",        { XMScalar, VexScalar, EXw, EXxEVexR }, 0 },
    { "vsqrtnep%XB",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_58 */
  {
    { "vaddp%XH", { XM, Vex, EXxh, EXxEVexR }, 0 },
    { "vadds%XH", { XMScalar, VexScalar, EXw, EXxEVexR }, 0 },
    { "vaddnep%XB",	{ XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_59 */
  {
    { "vmulp%XH", { XM, Vex, EXxh, EXxEVexR }, 0 },
    { "vmuls%XH", { XMScalar, VexScalar, EXw, EXxEVexR }, 0 },
    { "vmulnep%XB", { XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5A */
  {
    { "vcvtp%XH2pd",    { XM, EXxmmqdh, EXxEVexS }, 0 },
    { "vcvts%XH2sd",    { XMScalar, VexScalar, EXw, EXxEVexS }, 0 },
    { "vcvtp%XD2ph%XZ", { XMM, EXx, EXxEVexR }, 0 },
    { "vcvts%XD2sh",    { XMScalar, VexScalar, EXq, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5B */
  {
    { VEX_W_TABLE (EVEX_W_MAP5_5B_P_0) },
    { "vcvttp%XH2dq",   { XM, EXxmmqh, EXxEVexS }, 0 },
    { "vcvtp%XH2dq",    { XM, EXxmmqh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5C */
  {
    { "vsubp%XH", { XM, Vex, EXxh, EXxEVexR }, 0 },
    { "vsubs%XH", { XMScalar, VexScalar, EXw, EXxEVexR }, 0 },
    { "vsubnep%XB", { XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5D */
  {
    { "vminp%XH", { XM, Vex, EXxh, EXxEVexS }, 0 },
    { "vmins%XH", { XMScalar, VexScalar, EXw, EXxEVexS }, 0 },
    { "vminp%XB", { XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5E */
  {
    { "vdivp%XH", { XM, Vex, EXxh, EXxEVexR }, 0 },
    { "vdivs%XH", { XMScalar, VexScalar, EXw, EXxEVexR }, 0 },
    { "vdivnep%XB", { XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_5F */
  {
    { "vmaxp%XH", { XM, Vex, EXxh, EXxEVexS }, 0 },
    { "vmaxs%XH", { XMScalar, VexScalar, EXw, EXxEVexS }, 0 },
    { "vmaxp%XB", { XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_68 */
  {
    { "vcvttp%XH2ibs",	{ XM, EXxh, EXxEVexS }, 0 },
    { Bad_Opcode },
    { "vcvttp%XS2ibs",	{ XM, EXx, EXxEVexS }, 0 },
    { "vcvtt%XB2ibs",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_69 */
  {
    { "vcvtp%XH2ibs",	{ XM, EXxh, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vcvtp%XS2ibs",	{ XM, EXx, EXxEVexR }, 0 },
    { "vcvtne%XB2ibs",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_6A */
  {
    { "vcvttp%XH2iubs",	{ XM, EXxh, EXxEVexS }, 0 },
    { Bad_Opcode },
    { "vcvttp%XS2iubs",	{ XM, EXx, EXxEVexS }, 0 },
    { "vcvtt%XB2iubs",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_6B */
  {
    { "vcvtp%XH2iubs",	{ XM, EXxh, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vcvtp%XS2iubs",	{ XM, EXx, EXxEVexR }, 0 },
    { "vcvtne%XB2iubs",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_6C */
  {
    { VEX_W_TABLE (EVEX_W_MAP5_6C_P_0) },
    { "vcvttss2usis",	{ Gdq, EXd, EXxEVexS }, 0 },
    { VEX_W_TABLE (EVEX_W_MAP5_6C_P_2) },
    { "vcvttsd2usis",	{ Gdq, EXq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_6D */
  {
    { VEX_W_TABLE (EVEX_W_MAP5_6D_P_0) },
    { "vcvttss2sis",	{ Gdq, EXd, EXxEVexS }, 0 },
    { VEX_W_TABLE (EVEX_W_MAP5_6D_P_2) },
    { "vcvttsd2sis",	{ Gdq, EXq, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_6E_L_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_MAP5_6E_P_1) },
    { "vmovwY",	{ XMScalar, Edw }, 0 },
  },
  /* PREFIX_EVEX_MAP5_74 */
  {
    { "vcvtbiasp%XH2bf8s",	{ XMxmmq, Vex, EXxh }, 0 },
    { "vcvtnep%XH2bf8s%XY",	{ XMxmmq, EXxh }, 0 },
    { Bad_Opcode },
    { "vcvtne2p%XH2bf8s",	{ XM, Vex, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP5_78 */
  {
    { "vcvttp%XH2udq",  { XM, EXxmmqh, EXxEVexS }, 0 },
    { "vcvttsh2usi",    { Gdq, EXw, EXxEVexS }, 0 },
    { "vcvttp%XH2uqq",  { XM, EXxmmqdh, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_79 */
  {
    { "vcvtp%XH2udq",   { XM, EXxmmqh, EXxEVexR }, 0 },
    { "vcvtsh2usi",     { Gdq, EXw, EXxEVexR }, 0 },
    { "vcvtp%XH2uqq",   { XM, EXxmmqdh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_7A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vcvttp%XH2qq",   { XM, EXxmmqdh, EXxEVexS }, 0 },
    { VEX_W_TABLE (EVEX_W_MAP5_7A_P_3) },
  },
  /* PREFIX_EVEX_MAP5_7B */
  {
    { Bad_Opcode },
    { "vcvtusi2shY{%LQ|}",       { XMScalar, VexScalar, EXxEVexR, Edq }, 0 },
    { "vcvtp%XH2qq",    { XM, EXxmmqdh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_7C */
  {
    { "vcvttp%XH2uw",   { XM, EXxh, EXxEVexS }, 0 },
    { Bad_Opcode },
    { "vcvttp%XH2w",    { XM, EXxh, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP5_7D */
  {
    { "vcvtp%XH2uw",    { XM, EXxh, EXxEVexR }, 0 },
    { "vcvtw2p%XH",     { XM, EXxh, EXxEVexR }, 0 },
    { "vcvtp%XH2w",     { XM, EXxh, EXxEVexR }, 0 },
    { "vcvtuw2p%XH",    { XM, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP5_7E_L_0 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_MAP5_7E_P_1) },
    { "vmovw",	{ Edw, XMScalar }, 0 },
  },
  /* PREFIX_EVEX_MAP6_13 */
  {
    { "vcvts%XH2ss",	{ XMScalar, VexScalar, EXw, EXxEVexS }, 0 },
    { Bad_Opcode },
    { "vcvtp%XH2psx",	{ XM, EXxmmqh, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP6_2C */
  {
    { "vscalefnep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vscalefp%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_42 */
  {
    { "vgetexpp%XB",	{ XM, EXxh }, 0 },
    { Bad_Opcode },
    { "vgetexpp%XH",	{ XM, EXxh, EXxEVexS }, 0 },
  },
  /* PREFIX_EVEX_MAP6_4C */
  {
    { "vrcpp%XB",	{ XM, EXxh }, 0 },
    { Bad_Opcode },
    { "vrcpp%XH",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP6_4E */
  {
    { "vrsqrtp%XB",	{ XM, EXxh }, 0 },
    { Bad_Opcode },
    { "vrsqrtp%XH",	{ XM, EXxh }, 0 },
  },
  /* PREFIX_EVEX_MAP6_56 */
  {
    { Bad_Opcode },
    { "vfmaddcp%XH",      { { DistinctDest_Fixup, 0 }, Vex, EXx, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vfcmaddcp%XH",     { { DistinctDest_Fixup, 0 }, Vex, EXx, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_57 */
  {
    { Bad_Opcode },
    { "vfmaddcs%XH",      { { DistinctDest_Fixup, scalar_mode }, VexScalar, EXd, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vfcmaddcs%XH",     { { DistinctDest_Fixup, scalar_mode }, VexScalar, EXd, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_98 */
  {
    { "vfmadd132nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmadd132p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_9A */
  {
    { "vfmsub132nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmsub132p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_9C */
  {
    { "vfnmadd132nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmadd132p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_9E */
  {
    { "vfnmsub132nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmsub132p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_A8 */
  {
    { "vfmadd213nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmadd213p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_AA */
  {
    { "vfmsub213nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmsub213p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_AC */
  {
    { "vfnmadd213nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmadd213p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_AE */
  {
    { "vfnmsub213nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmsub213p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_B8 */
  {
    { "vfmadd231nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmadd231p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_BA */
  {
    { "vfmsub231nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfmsub231p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_BC */
  {
    { "vfnmadd231nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmadd231p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_BE */
  {
    { "vfnmsub231nep%XB",	{ XM, Vex, EXxh }, 0 },
    { Bad_Opcode },
    { "vfnmsub231p%XH",	{ XM, Vex, EXxh, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_D6 */
  {
    { Bad_Opcode },
    { "vfmulcp%XH",     { { DistinctDest_Fixup, 0 }, Vex, EXx, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vfcmulcp%XH",    { { DistinctDest_Fixup, 0 }, Vex, EXx, EXxEVexR }, 0 },
  },
  /* PREFIX_EVEX_MAP6_D7 */
  {
    { Bad_Opcode },
    { "vfmulcs%XH",     { { DistinctDest_Fixup, scalar_mode }, VexScalar, EXd, EXxEVexR }, 0 },
    { Bad_Opcode },
    { "vfcmulcs%XH",    { { DistinctDest_Fixup, scalar_mode }, VexScalar, EXd, EXxEVexR }, 0 },
  },
