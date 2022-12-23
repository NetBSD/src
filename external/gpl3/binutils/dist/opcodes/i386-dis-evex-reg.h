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
  /* REG_EVEX_0F38C6_M_0_L_2 */
  {
    { Bad_Opcode },
    { "vgatherpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { "vgatherpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
    { "vscatterpf1dp%XW",  { MVexVSIBDWpX }, PREFIX_DATA },
  },
  /* REG_EVEX_0F38C7_M_0_L_2_W_0 */
  {
    { Bad_Opcode },
    { "vgatherpf0qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { "vgatherpf1qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
    { "vscatterpf1qp%XW",  { MVexVSIBQWpX }, PREFIX_DATA },
  },
