static const struct dis386 evex_len_table[][3] = {
  /* EVEX_LEN_0F6E */
  {
    { "vmovK",	{ XMScalar, Edq }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F7E_P_1 */
  {
    { VEX_W_TABLE (EVEX_W_0F7E_P_1) },
  },

  /* EVEX_LEN_0F7E_P_2 */
  {
    { "vmovK",	{ Edq, XMScalar }, 0 },
  },

  /* EVEX_LEN_0FC4 */
  {
    { "vpinsrw",	{ XM, Vex, Edqw, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0FC5 */
  {
    { "vpextrw",	{ Gdq, XS, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0FD6 */
  {
    { VEX_W_TABLE (EVEX_W_0FD6_L_0) },
  },

  /* EVEX_LEN_0F3816 */
  {
    { Bad_Opcode },
    { "vpermp%XW",	{ XM, Vex, EXx }, PREFIX_DATA },
    { "vpermp%XW",	{ XM, Vex, EXx }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3819_W_0 */
  {
    { Bad_Opcode },
    { "vbroadcastf32x2",	{ XM, EXxmm_mq }, PREFIX_DATA },
    { "vbroadcastf32x2",	{ XM, EXxmm_mq }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3819_W_1 */
  {
    { Bad_Opcode },
    { "vbroadcastsd",	{ XM, EXxmm_mq }, PREFIX_DATA },
    { "vbroadcastsd",	{ XM, EXxmm_mq }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F381A_W_0_M_0 */
  {
    { Bad_Opcode },
    { "vbroadcastf32x4",	{ XM, EXxmm }, PREFIX_DATA },
    { "vbroadcastf32x4",	{ XM, EXxmm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F381A_W_1_M_0 */
  {
    { Bad_Opcode },
    { "vbroadcastf64x2",	{ XM, EXxmm }, PREFIX_DATA },
    { "vbroadcastf64x2",	{ XM, EXxmm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F381B_W_0_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vbroadcastf32x8",	{ XM, EXymm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F381B_P_2_W_1_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vbroadcastf64x4",	{ XM, EXymm }, 0 },
  },

  /* EVEX_LEN_0F3836 */
  {
    { Bad_Opcode },
    { "vperm%DQ",	{ XM, Vex, EXx }, PREFIX_DATA },
    { "vperm%DQ",	{ XM, Vex, EXx }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F385A_W_0_M_0 */
  {
    { Bad_Opcode },
    { "vbroadcasti32x4",	{ XM, EXxmm }, PREFIX_DATA },
    { "vbroadcasti32x4",	{ XM, EXxmm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F385A_W_1_M_0 */
  {
    { Bad_Opcode },
    { "vbroadcasti64x2",	{ XM, EXxmm }, PREFIX_DATA },
    { "vbroadcasti64x2",	{ XM, EXxmm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F385B_W_0_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vbroadcasti32x8",	{ XM, EXymm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F385B_W_1_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vbroadcasti64x4",	{ XM, EXymm }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C6_R_1_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C6_R_2_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C6_R_5_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C6_R_6_M_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_1_M_0_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf0qps",  { MVexVSIBDQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_1_M_0_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf0qpd",  { MVexVSIBQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_2_M_0_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf1qps",  { MVexVSIBDQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_2_M_0_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf1qpd",  { MVexVSIBQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_5_M_0_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0qps",  { MVexVSIBDQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_5_M_0_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0qpd",  { MVexVSIBQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_6_M_0_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf1qps",  { MVexVSIBDQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F38C7_R_6_M_0_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf1qpd",  { MVexVSIBQWpX }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A00_W_1 */
  {
    { Bad_Opcode },
    { "vpermq",	{ XM, EXx, Ib }, PREFIX_DATA },
    { "vpermq",	{ XM, EXx, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A01_W_1 */
  {
    { Bad_Opcode },
    { "vpermpd",	{ XM, EXx, Ib }, PREFIX_DATA },
    { "vpermpd",	{ XM, EXx, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A14 */
  {
    { "vpextrb",	{ Edqb, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A15 */
  {
    { "vpextrw",	{ Edqw, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A16 */
  {
    { "vpextrK",	{ Edq, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A17 */
  {
    { "vextractps",	{ Edqd, XMM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A18_W_0 */
  {
    { Bad_Opcode },
    { "vinsertf32x4",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
    { "vinsertf32x4",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A18_W_1 */
  {
    { Bad_Opcode },
    { "vinsertf64x2",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
    { "vinsertf64x2",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A19_W_0 */
  {
    { Bad_Opcode },
    { "vextractf32x4",	{ EXxmm, XM, Ib }, PREFIX_DATA },
    { "vextractf32x4",	{ EXxmm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A19_W_1 */
  {
    { Bad_Opcode },
    { "vextractf64x2",	{ EXxmm, XM, Ib }, PREFIX_DATA },
    { "vextractf64x2",	{ EXxmm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A1A_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vinsertf32x8",	{ XM, Vex, EXymm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A1A_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vinsertf64x4",	{ XM, Vex, EXymm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A1B_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vextractf32x8",	{ EXymm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A1B_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vextractf64x4",	{ EXymm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A20 */
  {
    { "vpinsrb",	{ XM, Vex, Edqb, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A21_W_0 */
  {
    { "vinsertps",	{ XMM, Vex, EXxmm_md, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A22 */
  {
    { "vpinsrK",	{ XM, Vex, Edq, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A23_W_0 */
  {
    { Bad_Opcode },
    { "vshuff32x4",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
    { "vshuff32x4",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A23_W_1 */
  {
    { Bad_Opcode },
    { "vshuff64x2",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
    { "vshuff64x2",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A38_W_0 */
  {
    { Bad_Opcode },
    { "vinserti32x4",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
    { "vinserti32x4",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A38_W_1 */
  {
    { Bad_Opcode },
    { "vinserti64x2",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
    { "vinserti64x2",	{ XM, Vex, EXxmm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A39_W_0 */
  {
    { Bad_Opcode },
    { "vextracti32x4",	{ EXxmm, XM, Ib }, PREFIX_DATA },
    { "vextracti32x4",	{ EXxmm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A39_W_1 */
  {
    { Bad_Opcode },
    { "vextracti64x2",	{ EXxmm, XM, Ib }, PREFIX_DATA },
    { "vextracti64x2",	{ EXxmm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A3A_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vinserti32x8",	{ XM, Vex, EXymm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A3A_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vinserti64x4",	{ XM, Vex, EXymm, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A3B_W_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vextracti32x8",	{ EXymm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A3B_W_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vextracti64x4",	{ EXymm, XM, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A43_W_0 */
  {
    { Bad_Opcode },
    { "vshufi32x4",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
    { "vshufi32x4",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
  },

  /* EVEX_LEN_0F3A43_W_1 */
  {
    { Bad_Opcode },
    { "vshufi64x2",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
    { "vshufi64x2",	{ XM, Vex, EXx, Ib }, PREFIX_DATA },
  },
};
