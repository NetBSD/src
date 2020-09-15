  {
    /* MOD_EVEX_0F12_PREFIX_0 */
    { "vmovlpX",	{ XMM, Vex, EXxmm_mq }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F12_P_0_M_1) },
  },
  {
    /* MOD_EVEX_0F12_PREFIX_2 */
    { "vmovlpX",	{ XMM, Vex, EXxmm_mq }, PREFIX_OPCODE },
  },
  {
    /* MOD_EVEX_0F13 */
    { "vmovlpX",	{ EXxmm_mq, XMM }, PREFIX_OPCODE },
  },
  {
    /* MOD_EVEX_0F16_PREFIX_0 */
    { "vmovhpX",	{ XMM, Vex, EXxmm_mq }, PREFIX_OPCODE },
    { VEX_W_TABLE (EVEX_W_0F16_P_0_M_1) },
  },
  {
    /* MOD_EVEX_0F16_PREFIX_2 */
    { "vmovhpX",	{ XMM, Vex, EXxmm_mq }, PREFIX_OPCODE },
  },
  {
    /* MOD_EVEX_0F17 */
    { "vmovhpX",	{ EXxmm_mq, XMM }, PREFIX_OPCODE },
  },
  {
    /* MOD_EVEX_0F2B */
    { "vmovntpX",	{ EXx, XM }, PREFIX_OPCODE },
  },
  /* MOD_EVEX_0F381A_W_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381A_W_0_M_0) },
  },
  /* MOD_EVEX_0F381A_W_1 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381A_W_1_M_0) },
  },
  /* MOD_EVEX_0F381B_W_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381B_W_0_M_0) },
  },
  /* MOD_EVEX_0F381B_W_1 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F381B_W_1_M_0) },
  },
  /* MOD_EVEX_0F3828_P_1 */
  {
    { Bad_Opcode },
    { "vpmovm2%BW",	{ XM, MaskE }, 0 },
  },
  /* MOD_EVEX_0F382A_P_1_W_1 */
  {
    { Bad_Opcode },
    { "vpbroadcastmb2q",	{ XM, MaskE }, 0 },
  },
  /* MOD_EVEX_0F3838_P_1 */
  {
    { Bad_Opcode },
    { "vpmovm2%DQ",	{ XM, MaskE }, 0 },
  },
  /* MOD_EVEX_0F383A_P_1_W_0 */
  {
    { Bad_Opcode },
    { "vpbroadcastmw2d",	{ XM, MaskE }, 0 },
  },
  /* MOD_EVEX_0F385A_W_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385A_W_0_M_0) },
  },
  /* MOD_EVEX_0F385A_W_1 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385A_W_1_M_0) },
  },
  /* MOD_EVEX_0F385B_W_0 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385B_W_0_M_0) },
  },
  /* MOD_EVEX_0F385B_W_1 */
  {
    { EVEX_LEN_TABLE (EVEX_LEN_0F385B_W_1_M_0) },
  },
  /* MOD_EVEX_0F387A_W_0 */
  {
    { Bad_Opcode },
    { "vpbroadcastb",	{ XM, Ed }, PREFIX_DATA },
  },
  /* MOD_EVEX_0F387B_W_0 */
  {
    { Bad_Opcode },
    { "vpbroadcastw",	{ XM, Ed }, PREFIX_DATA },
  },
  /* MOD_EVEX_0F387C */
  {
    { Bad_Opcode },
    { "vpbroadcastK",	{ XM, Edq }, PREFIX_DATA },
  },
  {
    /* MOD_EVEX_0F38C6_REG_1 */
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C6_R_1_M_0) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_2 */
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C6_R_2_M_0) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_5 */
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C6_R_5_M_0) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_6 */
    { EVEX_LEN_TABLE (EVEX_LEN_0F38C6_R_6_M_0) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_1 */
    { VEX_W_TABLE (EVEX_W_0F38C7_R_1_M_0) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_2 */
    { VEX_W_TABLE (EVEX_W_0F38C7_R_2_M_0) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_5 */
    { VEX_W_TABLE (EVEX_W_0F38C7_R_5_M_0) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_6 */
    { VEX_W_TABLE (EVEX_W_0F38C7_R_6_M_0) },
  },
