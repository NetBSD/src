  /* REG_EVEX_0F71 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsrlw",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { "vpsraw",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { "vpsllw",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* REG_EVEX_0F72 */
  {
    { "vpror%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { "vprol%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { VEX_W_TABLE (EVEX_W_0F72_R_2) },
    { Bad_Opcode },
    { "vpsra%DQ",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F72_R_6) },
  },
  /* REG_EVEX_0F73 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_2) },
    { "vpsrldq",	{ Vex, EXx, Ib }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_6) },
    { "vpslldq",	{ Vex, EXx, Ib }, PREFIX_DATA },
  },
  /* REG_EVEX_0F38C6 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F38C6_REG_1) },
    { MOD_TABLE (MOD_EVEX_0F38C6_REG_2) },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F38C6_REG_5) },
    { MOD_TABLE (MOD_EVEX_0F38C6_REG_6) },
  },
  /* REG_EVEX_0F38C7 */
  {
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F38C7_REG_1) },
    { MOD_TABLE (MOD_EVEX_0F38C7_REG_2) },
    { Bad_Opcode },
    { Bad_Opcode },
    { MOD_TABLE (MOD_EVEX_0F38C7_REG_5) },
    { MOD_TABLE (MOD_EVEX_0F38C7_REG_6) },
  },
