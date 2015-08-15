#ifdef NEED_OPCODE_TABLE

static const struct dis386 evex_table[][256] = {
  /* EVEX_0F */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { PREFIX_TABLE (PREFIX_EVEX_0F10) },
    { PREFIX_TABLE (PREFIX_EVEX_0F11) },
    { PREFIX_TABLE (PREFIX_EVEX_0F12) },
    { PREFIX_TABLE (PREFIX_EVEX_0F13) },
    { PREFIX_TABLE (PREFIX_EVEX_0F14) },
    { PREFIX_TABLE (PREFIX_EVEX_0F15) },
    { PREFIX_TABLE (PREFIX_EVEX_0F16) },
    { PREFIX_TABLE (PREFIX_EVEX_0F17) },
    /* 18 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 20 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 28 */
    { PREFIX_TABLE (PREFIX_EVEX_0F28) },
    { PREFIX_TABLE (PREFIX_EVEX_0F29) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2B) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F2F) },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F51) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { PREFIX_TABLE (PREFIX_EVEX_0F58) },
    { PREFIX_TABLE (PREFIX_EVEX_0F59) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5B) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F5F) },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F62) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F66) },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F6A) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F6C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F6D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F6E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F6F) },
    /* 70 */
    { PREFIX_TABLE (PREFIX_EVEX_0F70) },
    { Bad_Opcode },
    { REG_TABLE (REG_EVEX_0F72) },
    { REG_TABLE (REG_EVEX_0F73) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F76) },
    { Bad_Opcode },
    /* 78 */
    { PREFIX_TABLE (PREFIX_EVEX_0F78) },
    { PREFIX_TABLE (PREFIX_EVEX_0F79) },
    { PREFIX_TABLE (PREFIX_EVEX_0F7A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F7B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F7E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F7F) },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* A0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* A8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* B0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* B8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* C0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FC2) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FC6) },
    { Bad_Opcode },
    /* C8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* D0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FD2) },
    { PREFIX_TABLE (PREFIX_EVEX_0FD3) },
    { PREFIX_TABLE (PREFIX_EVEX_0FD4) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FD6) },
    { Bad_Opcode },
    /* D8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FDB) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FDF) },
    /* E0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FE2) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FE6) },
    { PREFIX_TABLE (PREFIX_EVEX_0FE7) },
    /* E8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FEB) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FEF) },
    /* F0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FF2) },
    { PREFIX_TABLE (PREFIX_EVEX_0FF3) },
    { PREFIX_TABLE (PREFIX_EVEX_0FF4) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* F8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FFA) },
    { PREFIX_TABLE (PREFIX_EVEX_0FFB) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0FFE) },
    { Bad_Opcode },
  },
  /* EVEX_0F38 */
  {
    /* 00 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F380C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F380D) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3811) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3812) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3813) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3814) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3815) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3816) },
    { Bad_Opcode },
    /* 18 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3818) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3819) },
    { PREFIX_TABLE (PREFIX_EVEX_0F381A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F381B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F381E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F381F) },
    /* 20 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3821) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3822) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3823) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3824) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3825) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3827) },
    /* 28 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3828) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3829) },
    { PREFIX_TABLE (PREFIX_EVEX_0F382A) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F382C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F382D) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3831) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3832) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3833) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3834) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3835) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3836) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3837) },
    /* 38 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3839) },
    { PREFIX_TABLE (PREFIX_EVEX_0F383A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F383B) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F383D) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F383F) },
    /* 40 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3840) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3842) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3843) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3844) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3845) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3846) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3847) },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F384C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F384D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F384E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F384F) },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3858) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3859) },
    { PREFIX_TABLE (PREFIX_EVEX_0F385A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F385B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3864) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3865) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3876) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3877) },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F387C) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F387E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F387F) },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3888) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3889) },
    { PREFIX_TABLE (PREFIX_EVEX_0F388A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F388B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3890) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3891) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3892) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3893) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3896) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3897) },
    /* 98 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3898) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3899) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389B) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389C) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F389F) },
    /* A0 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38A0) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A1) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A2) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A3) },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A6) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A7) },
    /* A8 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38A8) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38A9) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AA) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AB) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AC) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AD) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AE) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38AF) },
    /* B0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F38B6) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38B7) },
    /* B8 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38B8) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38B9) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BA) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BB) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BC) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BD) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BE) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38BF) },
    /* C0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F38C4) },
    { Bad_Opcode },
    { REG_TABLE (REG_EVEX_0F38C6) },
    { REG_TABLE (REG_EVEX_0F38C7) },
    /* C8 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C8) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F38CA) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38CB) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38CC) },
    { PREFIX_TABLE (PREFIX_EVEX_0F38CD) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* D0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* D8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* E0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* E8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* F0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* F8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
  /* EVEX_0F3A */
  {
    /* 00 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3A00) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A01) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A03) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A04) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A05) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 08 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3A08) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A09) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A0A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A0B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 10 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A17) },
    /* 18 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3A18) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A19) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A1A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A1B) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A1D) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A1E) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A1F) },
    /* 20 */
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A21) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A23) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A25) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A26) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A27) },
    /* 28 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 30 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 38 */
    { PREFIX_TABLE (PREFIX_EVEX_0F3A38) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A39) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A3A) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A3B) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 40 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A43) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 48 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 50 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A54) },
    { PREFIX_TABLE (PREFIX_EVEX_0F3A55) },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 58 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 60 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 68 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 70 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 78 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 80 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 88 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 90 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* 98 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* A0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* A8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* B0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* B8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* C0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* C8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* D0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* D8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* E0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* E8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* F0 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    /* F8 */
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
  },
};
#endif /* NEED_OPCODE_TABLE */

#ifdef NEED_REG_TABLE
  /* REG_EVEX_0F72 */
  {
    { PREFIX_TABLE (PREFIX_EVEX_0F72_REG_0) },
    { PREFIX_TABLE (PREFIX_EVEX_0F72_REG_1) },
    { PREFIX_TABLE (PREFIX_EVEX_0F72_REG_2) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F72_REG_4) },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F72_REG_6) },
  },
  /* REG_EVEX_0F73 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F73_REG_2) },
    { Bad_Opcode },
    { Bad_Opcode },
    { Bad_Opcode },
    { PREFIX_TABLE (PREFIX_EVEX_0F73_REG_6) },
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
#endif /* NEED_REG_TABLE */

#ifdef NEED_PREFIX_TABLE
  /* PREFIX_EVEX_0F10 */
  {
    { VEX_W_TABLE (EVEX_W_0F10_P_0) },
    { MOD_TABLE (MOD_EVEX_0F10_PREFIX_1) },
    { VEX_W_TABLE (EVEX_W_0F10_P_2) },
    { MOD_TABLE (MOD_EVEX_0F10_PREFIX_3) },
  },
  /* PREFIX_EVEX_0F11 */
  {
    { VEX_W_TABLE (EVEX_W_0F11_P_0) },
    { MOD_TABLE (MOD_EVEX_0F11_PREFIX_1) },
    { VEX_W_TABLE (EVEX_W_0F11_P_2) },
    { MOD_TABLE (MOD_EVEX_0F11_PREFIX_3) },
  },
  /* PREFIX_EVEX_0F12 */
  {
    { MOD_TABLE (MOD_EVEX_0F12_PREFIX_0) },
    { VEX_W_TABLE (EVEX_W_0F12_P_1) },
    { VEX_W_TABLE (EVEX_W_0F12_P_2) },
    { VEX_W_TABLE (EVEX_W_0F12_P_3) },
  },
  /* PREFIX_EVEX_0F13 */
  {
    { VEX_W_TABLE (EVEX_W_0F13_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F13_P_2) },
  },
  /* PREFIX_EVEX_0F14 */
  {
    { VEX_W_TABLE (EVEX_W_0F14_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F14_P_2) },
  },
  /* PREFIX_EVEX_0F15 */
  {
    { VEX_W_TABLE (EVEX_W_0F15_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F15_P_2) },
  },
  /* PREFIX_EVEX_0F16 */
  {
    { MOD_TABLE (MOD_EVEX_0F16_PREFIX_0) },
    { VEX_W_TABLE (EVEX_W_0F16_P_1) },
    { VEX_W_TABLE (EVEX_W_0F16_P_2) },
  },
  /* PREFIX_EVEX_0F17 */
  {
    { VEX_W_TABLE (EVEX_W_0F17_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F17_P_2) },
  },
  /* PREFIX_EVEX_0F28 */
  {
    { VEX_W_TABLE (EVEX_W_0F28_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F28_P_2) },
  },
  /* PREFIX_EVEX_0F29 */
  {
    { VEX_W_TABLE (EVEX_W_0F29_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F29_P_2) },
  },
  /* PREFIX_EVEX_0F2A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2A_P_1) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2A_P_3) },
  },
  /* PREFIX_EVEX_0F2B */
  {
    { VEX_W_TABLE (EVEX_W_0F2B_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2B_P_2) },
  },
  /* PREFIX_EVEX_0F2C */
  {
    { Bad_Opcode },
    { "vcvttss2si",	{ Gdq, EXxmm_md, EXxEVexS } },
    { Bad_Opcode },
    { "vcvttsd2si",	{ Gdq, EXxmm_mq, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F2D */
  {
    { Bad_Opcode },
    { "vcvtss2si",	{ Gdq, EXxmm_md, EXxEVexR } },
    { Bad_Opcode },
    { "vcvtsd2si",	{ Gdq, EXxmm_mq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F2E */
  {
    { VEX_W_TABLE (EVEX_W_0F2E_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2E_P_2) },
  },
  /* PREFIX_EVEX_0F2F */
  {
    { VEX_W_TABLE (EVEX_W_0F2F_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F2F_P_2) },
  },
  /* PREFIX_EVEX_0F51 */
  {
    { VEX_W_TABLE (EVEX_W_0F51_P_0) },
    { VEX_W_TABLE (EVEX_W_0F51_P_1) },
    { VEX_W_TABLE (EVEX_W_0F51_P_2) },
    { VEX_W_TABLE (EVEX_W_0F51_P_3) },
  },
  /* PREFIX_EVEX_0F58 */
  {
    { VEX_W_TABLE (EVEX_W_0F58_P_0) },
    { VEX_W_TABLE (EVEX_W_0F58_P_1) },
    { VEX_W_TABLE (EVEX_W_0F58_P_2) },
    { VEX_W_TABLE (EVEX_W_0F58_P_3) },
  },
  /* PREFIX_EVEX_0F59 */
  {
    { VEX_W_TABLE (EVEX_W_0F59_P_0) },
    { VEX_W_TABLE (EVEX_W_0F59_P_1) },
    { VEX_W_TABLE (EVEX_W_0F59_P_2) },
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
    { VEX_W_TABLE (EVEX_W_0F5C_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5C_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5C_P_2) },
    { VEX_W_TABLE (EVEX_W_0F5C_P_3) },
  },
  /* PREFIX_EVEX_0F5D */
  {
    { VEX_W_TABLE (EVEX_W_0F5D_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5D_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5D_P_2) },
    { VEX_W_TABLE (EVEX_W_0F5D_P_3) },
  },
  /* PREFIX_EVEX_0F5E */
  {
    { VEX_W_TABLE (EVEX_W_0F5E_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5E_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5E_P_2) },
    { VEX_W_TABLE (EVEX_W_0F5E_P_3) },
  },
  /* PREFIX_EVEX_0F5F */
  {
    { VEX_W_TABLE (EVEX_W_0F5F_P_0) },
    { VEX_W_TABLE (EVEX_W_0F5F_P_1) },
    { VEX_W_TABLE (EVEX_W_0F5F_P_2) },
    { VEX_W_TABLE (EVEX_W_0F5F_P_3) },
  },
  /* PREFIX_EVEX_0F62 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F62_P_2) },
  },
  /* PREFIX_EVEX_0F66 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F66_P_2) },
  },
  /* PREFIX_EVEX_0F6A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6A_P_2) },
  },
  /* PREFIX_EVEX_0F6C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6C_P_2) },
  },
  /* PREFIX_EVEX_0F6D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6D_P_2) },
  },
  /* PREFIX_EVEX_0F6E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6E_P_2) },
  },
  /* PREFIX_EVEX_0F6F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F6F_P_1) },
    { VEX_W_TABLE (EVEX_W_0F6F_P_2) },
  },
  /* PREFIX_EVEX_0F70 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F70_P_2) },
  },
  /* PREFIX_EVEX_0F72_REG_0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpror%LW",	{ Vex, EXx, Ib } },
  },
  /* PREFIX_EVEX_0F72_REG_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vprol%LW",	{ Vex, EXx, Ib } },
  },
  /* PREFIX_EVEX_0F72_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F72_R_2_P_2) },
  },
  /* PREFIX_EVEX_0F72_REG_4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsra%LW",	{ Vex, EXx, Ib } },
  },
  /* PREFIX_EVEX_0F72_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F72_R_6_P_2) },
  },
  /* PREFIX_EVEX_0F73_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_2_P_2) },
  },
  /* PREFIX_EVEX_0F73_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F73_R_6_P_2) },
  },
  /* PREFIX_EVEX_0F76 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F76_P_2) },
  },
  /* PREFIX_EVEX_0F78 */
  {
    { VEX_W_TABLE (EVEX_W_0F78_P_0) },
    { "vcvttss2usi",	{ Gdq, EXxmm_md, EXxEVexS } },
    { Bad_Opcode },
    { "vcvttsd2usi",	{ Gdq, EXxmm_mq, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F79 */
  {
    { VEX_W_TABLE (EVEX_W_0F79_P_0) },
    { "vcvtss2usi",	{ Gdq, EXxmm_md, EXxEVexR } },
    { Bad_Opcode },
    { "vcvtsd2usi",	{ Gdq, EXxmm_mq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F7A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7A_P_1) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7A_P_3) },
  },
  /* PREFIX_EVEX_0F7B */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7B_P_1) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7B_P_3) },
  },
  /* PREFIX_EVEX_0F7E */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7E_P_1) },
    { VEX_W_TABLE (EVEX_W_0F7E_P_2) },
  },
  /* PREFIX_EVEX_0F7F */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F7F_P_1) },
    { VEX_W_TABLE (EVEX_W_0F7F_P_2) },
  },

  /* PREFIX_EVEX_0FC2 */
  {
    { VEX_W_TABLE (EVEX_W_0FC2_P_0) },
    { VEX_W_TABLE (EVEX_W_0FC2_P_1) },
    { VEX_W_TABLE (EVEX_W_0FC2_P_2) },
    { VEX_W_TABLE (EVEX_W_0FC2_P_3) },
  },
  /* PREFIX_EVEX_0FC6 */
  {
    { VEX_W_TABLE (EVEX_W_0FC6_P_0) },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FC6_P_2) },
  },
  /* PREFIX_EVEX_0FD2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FD2_P_2) },
  },
  /* PREFIX_EVEX_0FD3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FD3_P_2) },
  },
  /* PREFIX_EVEX_0FD4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FD4_P_2) },
  },
  /* PREFIX_EVEX_0FD6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FD6_P_2) },
  },
  /* PREFIX_EVEX_0FDB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpand%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0FDF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpandn%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0FE2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsra%LW",	{ XM, Vex, EXxmm } },
  },
  /* PREFIX_EVEX_0FE6 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FE6_P_1) },
    { VEX_W_TABLE (EVEX_W_0FE6_P_2) },
    { VEX_W_TABLE (EVEX_W_0FE6_P_3) },
  },
  /* PREFIX_EVEX_0FE7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FE7_P_2) },
  },
  /* PREFIX_EVEX_0FEB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpor%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0FEF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpxor%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0FF2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FF2_P_2) },
  },
  /* PREFIX_EVEX_0FF3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FF3_P_2) },
  },
  /* PREFIX_EVEX_0FF4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FF4_P_2) },
  },
  /* PREFIX_EVEX_0FFA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FFA_P_2) },
  },
  /* PREFIX_EVEX_0FFB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FFB_P_2) },
  },
  /* PREFIX_EVEX_0FFE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0FFE_P_2) },
  },

  /* PREFIX_EVEX_0F380C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F380C_P_2) },
  },
  /* PREFIX_EVEX_0F380D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F380D_P_2) },
  },
  /* PREFIX_EVEX_0F3811 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3811_P_1) },
  },
  /* PREFIX_EVEX_0F3812 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3812_P_1) },
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
    { "vprorv%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3815 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3815_P_1) },
    { "vprolv%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3816 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpermp%XW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3818 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3818_P_2) },
  },
  /* PREFIX_EVEX_0F3819 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3819_P_2) },
  },
  /* PREFIX_EVEX_0F381A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F381A_P_2) },
  },
  /* PREFIX_EVEX_0F381B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F381B_P_2) },
  },
  /* PREFIX_EVEX_0F381E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F381E_P_2) },
  },
  /* PREFIX_EVEX_0F381F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F381F_P_2) },
  },
  /* PREFIX_EVEX_0F3821 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3821_P_1) },
    { "vpmovsxbd",	{ XM, EXxmmqd } },
  },
  /* PREFIX_EVEX_0F3822 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3822_P_1) },
    { "vpmovsxbq",	{ XM, EXxmmdw } },
  },
  /* PREFIX_EVEX_0F3823 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3823_P_1) },
    { "vpmovsxwd",	{ XM, EXxmmq } },
  },
  /* PREFIX_EVEX_0F3824 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3824_P_1) },
    { "vpmovsxwq",	{ XM, EXxmmqd } },
  },
  /* PREFIX_EVEX_0F3825 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3825_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3825_P_2) },
  },
  /* PREFIX_EVEX_0F3827 */
  {
    { Bad_Opcode },
    { "vptestnm%LW",	{ XMask, Vex, EXx } },
    { "vptestm%LW",	{ XMask, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3828 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3828_P_2) },
  },
  /* PREFIX_EVEX_0F3829 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3829_P_2) },
  },
  /* PREFIX_EVEX_0F382A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F382A_P_1) },
    { VEX_W_TABLE (EVEX_W_0F382A_P_2) },
  },
  /* PREFIX_EVEX_0F382C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscalefp%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F382D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscalefs%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F3831 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3831_P_1) },
    { "vpmovzxbd",	{ XM, EXxmmqd } },
  },
  /* PREFIX_EVEX_0F3832 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3832_P_1) },
    { "vpmovzxbq",	{ XM, EXxmmdw } },
  },
  /* PREFIX_EVEX_0F3833 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3833_P_1) },
    { "vpmovzxwd",	{ XM, EXxmmq } },
  },
  /* PREFIX_EVEX_0F3834 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3834_P_1) },
    { "vpmovzxwq",	{ XM, EXxmmqd } },
  },
  /* PREFIX_EVEX_0F3835 */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3835_P_1) },
    { VEX_W_TABLE (EVEX_W_0F3835_P_2) },
  },
  /* PREFIX_EVEX_0F3836 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vperm%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3837 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3837_P_2) },
  },
  /* PREFIX_EVEX_0F3839 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmins%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F383A */
  {
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F383A_P_1) },
  },
  /* PREFIX_EVEX_0F383B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpminu%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F383D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmaxs%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F383F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpmaxu%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3840 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3840_P_2) },
  },
  /* PREFIX_EVEX_0F3842 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgetexpp%XW",	{ XM, EXx, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F3843 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgetexps%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F3844 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vplzcnt%LW",	{ XM, EXx } },
  },
  /* PREFIX_EVEX_0F3845 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsrlv%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3846 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsrav%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3847 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpsllv%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F384C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrcp14p%XW",	{ XM, EXx } },
  },
  /* PREFIX_EVEX_0F384D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrcp14s%XW",	{ XMScalar, VexScalar, EXxmm_mdq } },
  },
  /* PREFIX_EVEX_0F384E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrsqrt14p%XW",	{ XM, EXx } },
  },
  /* PREFIX_EVEX_0F384F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrsqrt14s%XW",	{ XMScalar, VexScalar, EXxmm_mdq } },
  },
  /* PREFIX_EVEX_0F3858 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3858_P_2) },
  },
  /* PREFIX_EVEX_0F3859 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3859_P_2) },
  },
  /* PREFIX_EVEX_0F385A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F385A_P_2) },
  },
  /* PREFIX_EVEX_0F385B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F385B_P_2) },
  },
  /* PREFIX_EVEX_0F3864 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpblendm%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3865 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vblendmp%XW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3876 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpermi2%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3877 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpermi2p%XW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F387C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpbroadcast%LW",	{ XM, Rdq } },
  },
  /* PREFIX_EVEX_0F387E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpermt2%LW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F387F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpermt2p%XW",	{ XM, Vex, EXx } },
  },
  /* PREFIX_EVEX_0F3888 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vexpandp%XW",	{ XM, EXEvexXGscat } },
  },
  /* PREFIX_EVEX_0F3889 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpexpand%LW",	{ XM, EXEvexXGscat } },
  },
  /* PREFIX_EVEX_0F388A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vcompressp%XW",	{ EXEvexXGscat, XM } },
  },
  /* PREFIX_EVEX_0F388B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpcompress%LW",	{ EXEvexXGscat, XM } },
  },
  /* PREFIX_EVEX_0F3890 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpgatherd%LW",	{ XM, MVexVSIBDWpX } },
  },
  /* PREFIX_EVEX_0F3891 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3891_P_2) },
  },
  /* PREFIX_EVEX_0F3892 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherdp%XW",	{ XM, MVexVSIBDWpX} },
  },
  /* PREFIX_EVEX_0F3893 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3893_P_2) },
  },
  /* PREFIX_EVEX_0F3896 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F3897 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F3898 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F3899 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd132s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub132s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389C */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd132s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub132p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F389F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub132s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38A0 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpscatterd%LW",	{ MVexVSIBDWpX, XM } },
  },
  /* PREFIX_EVEX_0F38A1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38A1_P_2) },
  },
  /* PREFIX_EVEX_0F38A2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterdp%XW",	{ MVexVSIBDWpX, XM } },
  },
  /* PREFIX_EVEX_0F38A3 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38A3_P_2) },
  },
  /* PREFIX_EVEX_0F38A6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38A7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38A8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38A9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd213s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub213s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd213s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub213p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38AF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub213s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38B6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmaddsub231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38B7 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsubadd231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38B8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38B9 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmadd231s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfmsub231s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmadd231s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BE */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub231p%XW",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38BF */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfnmsub231s%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexR } },
  },
  /* PREFIX_EVEX_0F38C4 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpconflict%LW",	{ XM, EXx } },
  },
  /* PREFIX_EVEX_0F38C6_REG_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf0dp%XW",  { MVexVSIBDWpX } },
  },
  /* PREFIX_EVEX_0F38C6_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgatherpf1dp%XW",  { MVexVSIBDWpX } },
  },
  /* PREFIX_EVEX_0F38C6_REG_5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf0dp%XW",  { MVexVSIBDWpX } },
  },
  /* PREFIX_EVEX_0F38C6_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vscatterpf1dp%XW",  { MVexVSIBDWpX } },
  },
  /* PREFIX_EVEX_0F38C7_REG_1 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38C7_R_1_P_2) },
  },
  /* PREFIX_EVEX_0F38C7_REG_2 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38C7_R_2_P_2) },
  },
  /* PREFIX_EVEX_0F38C7_REG_5 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38C7_R_5_P_2) },
  },
  /* PREFIX_EVEX_0F38C7_REG_6 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F38C7_R_6_P_2) },
  },
  /* PREFIX_EVEX_0F38C8 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vexp2p%XW",        { XM, EXx, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F38CA */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrcp28p%XW",       { XM, EXx, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F38CB */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrcp28s%XW",       { XMScalar, VexScalar, EXxmm_mdq, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F38CC */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrsqrt28p%XW",     { XM, EXx, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F38CD */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vrsqrt28s%XW",     { XMScalar, VexScalar, EXxmm_mdq, EXxEVexS } },
  },
  /* PREFIX_EVEX_0F3A00 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A00_P_2) },
  },
  /* PREFIX_EVEX_0F3A01 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A01_P_2) },
  },
  /* PREFIX_EVEX_0F3A03 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "valign%LW",	{ XM, Vex, EXx, Ib } },
  },
  /* PREFIX_EVEX_0F3A04 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A04_P_2) },
  },
  /* PREFIX_EVEX_0F3A05 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A05_P_2) },
  },
  /* PREFIX_EVEX_0F3A08 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A08_P_2) },
  },
  /* PREFIX_EVEX_0F3A09 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A09_P_2) },
  },
  /* PREFIX_EVEX_0F3A0A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A0A_P_2) },
  },
  /* PREFIX_EVEX_0F3A0B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A0B_P_2) },
  },
  /* PREFIX_EVEX_0F3A17 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vextractps",	{ Edqd, XMM, Ib } },
  },
  /* PREFIX_EVEX_0F3A18 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A18_P_2) },
  },
  /* PREFIX_EVEX_0F3A19 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A19_P_2) },
  },
  /* PREFIX_EVEX_0F3A1A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A1A_P_2) },
  },
  /* PREFIX_EVEX_0F3A1B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A1B_P_2) },
  },
  /* PREFIX_EVEX_0F3A1D */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A1D_P_2) },
  },
  /* PREFIX_EVEX_0F3A1E */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpcmpu%LW",	{ XMask, Vex, EXx, VPCMP } },
  },
  /* PREFIX_EVEX_0F3A1F */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpcmp%LW",	{ XMask, Vex, EXx, VPCMP } },
  },
  /* PREFIX_EVEX_0F3A21 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A21_P_2) },
  },
  /* PREFIX_EVEX_0F3A23 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A23_P_2) },
  },
  /* PREFIX_EVEX_0F3A25 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vpternlog%LW",	{ XM, Vex, EXx, Ib } },
  },
  /* PREFIX_EVEX_0F3A26 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgetmantp%XW",	{ XM, EXx, EXxEVexS, Ib } },
  },
  /* PREFIX_EVEX_0F3A27 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vgetmants%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexS, Ib } },
  },
  /* PREFIX_EVEX_0F3A38 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A38_P_2) },
  },
  /* PREFIX_EVEX_0F3A39 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A39_P_2) },
  },
  /* PREFIX_EVEX_0F3A3A */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A3A_P_2) },
  },
  /* PREFIX_EVEX_0F3A3B */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A3B_P_2) },
  },
  /* PREFIX_EVEX_0F3A43 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { VEX_W_TABLE (EVEX_W_0F3A43_P_2) },
  },
  /* PREFIX_EVEX_0F3A54 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfixupimmp%XW",	{ XM, Vex, EXx, EXxEVexS, Ib } },
  },
  /* PREFIX_EVEX_0F3A55 */
  {
    { Bad_Opcode },
    { Bad_Opcode },
    { "vfixupimms%XW",	{ XMScalar, VexScalar, EXxmm_mdq, EXxEVexS, Ib } },
  },
#endif /* NEED_PREFIX_TABLE */

#ifdef NEED_VEX_W_TABLE
  /* EVEX_W_0F10_P_0 */
  {
    { "vmovups",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F10_P_1_M_0 */
  {
    { "vmovss",	{ XMScalar, EXdScalar } },
  },
  /* EVEX_W_0F10_P_1_M_1 */
  {
    { "vmovss",	{ XMScalar, VexScalar, EXx } },
  },
  /* EVEX_W_0F10_P_2 */
  {
    { Bad_Opcode },
    { "vmovupd",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F10_P_3_M_0 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ XMScalar, EXqScalar } },
  },
  /* EVEX_W_0F10_P_3_M_1 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ XMScalar, VexScalar, EXx } },
  },
  /* EVEX_W_0F11_P_0 */
  {
    { "vmovups",	{ EXxS, XM } },
  },
  /* EVEX_W_0F11_P_1_M_0 */
  {
    { "vmovss",	{ EXdScalarS, XMScalar } },
  },
  /* EVEX_W_0F11_P_1_M_1 */
  {
    { "vmovss",	{ EXxS, Vex, XMScalar } },
  },
  /* EVEX_W_0F11_P_2 */
  {
    { Bad_Opcode },
    { "vmovupd",	{ EXxS, XM } },
  },
  /* EVEX_W_0F11_P_3_M_0 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ EXqScalarS, XMScalar } },
  },
  /* EVEX_W_0F11_P_3_M_1 */
  {
    { Bad_Opcode },
    { "vmovsd",	{ EXxS, Vex, XMScalar } },
  },
  /* EVEX_W_0F12_P_0_M_0 */
  {
    { "vmovlps",	{ XMM, Vex, EXxmm_mq } },
  },
  /* EVEX_W_0F12_P_0_M_1 */
  {
    { "vmovhlps",	{ XMM, Vex, EXxmm_mq } },
  },
  /* EVEX_W_0F12_P_1 */
  {
    { "vmovsldup",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F12_P_2 */
  {
    { Bad_Opcode },
    { "vmovlpd",	{ XMM, Vex, EXxmm_mq } },
  },
  /* EVEX_W_0F12_P_3 */
  {
    { Bad_Opcode },
    { "vmovddup",	{ XM, EXymmq } },
  },
  /* EVEX_W_0F13_P_0 */
  {
    { "vmovlps",	{ EXxmm_mq, XMM } },
  },
  /* EVEX_W_0F13_P_2 */
  {
    { Bad_Opcode },
    { "vmovlpd",	{ EXxmm_mq, XMM } },
  },
  /* EVEX_W_0F14_P_0 */
  {
    { "vunpcklps",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F14_P_2 */
  {
    { Bad_Opcode },
    { "vunpcklpd",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F15_P_0 */
  {
    { "vunpckhps",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F15_P_2 */
  {
    { Bad_Opcode },
    { "vunpckhpd",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F16_P_0_M_0 */
  {
    { "vmovhps",	{ XMM, Vex, EXxmm_mq } },
  },
  /* EVEX_W_0F16_P_0_M_1 */
  {
    { "vmovlhps",	{ XMM, Vex, EXx } },
  },
  /* EVEX_W_0F16_P_1 */
  {
    { "vmovshdup",	{ XM, EXx } },
  },
  /* EVEX_W_0F16_P_2 */
  {
    { Bad_Opcode },
    { "vmovhpd",	{ XMM, Vex, EXxmm_mq } },
  },
  /* EVEX_W_0F17_P_0 */
  {
    { "vmovhps",	{ EXxmm_mq, XMM } },
  },
  /* EVEX_W_0F17_P_2 */
  {
    { Bad_Opcode },
    { "vmovhpd",	{ EXxmm_mq, XMM } },
  },
  /* EVEX_W_0F28_P_0 */
  {
    { "vmovaps",	{ XM, EXx } },
  },
  /* EVEX_W_0F28_P_2 */
  {
    { Bad_Opcode },
    { "vmovapd",	{ XM, EXx } },
  },
  /* EVEX_W_0F29_P_0 */
  {
    { "vmovaps",	{ EXxS, XM } },
  },
  /* EVEX_W_0F29_P_2 */
  {
    { Bad_Opcode },
    { "vmovapd",	{ EXxS, XM } },
  },
  /* EVEX_W_0F2A_P_1 */
  {
    { "vcvtsi2ss",	{ XMScalar, VexScalar, EXxEVexR, Ed } },
    { "vcvtsi2ss",	{ XMScalar, VexScalar, EXxEVexR, Eq } },
  },
  /* EVEX_W_0F2A_P_3 */
  {
    { "vcvtsi2sd",	{ XMScalar, VexScalar, Ed } },
    { "vcvtsi2sd",	{ XMScalar, VexScalar, EXxEVexR, Eq } },
  },
  /* EVEX_W_0F2B_P_0 */
  {
    { "vmovntps",	{ EXx, XM } },
  },
  /* EVEX_W_0F2B_P_2 */
  {
    { Bad_Opcode },
    { "vmovntpd",	{ EXx, XM } },
  },
  /* EVEX_W_0F2E_P_0 */
  {
    { "vucomiss",	{ XMScalar, EXxmm_md, EXxEVexS } },
  },
  /* EVEX_W_0F2E_P_2 */
  {
    { Bad_Opcode },
    { "vucomisd",	{ XMScalar, EXxmm_mq, EXxEVexS } },
  },
  /* EVEX_W_0F2F_P_0 */
  {
    { "vcomiss",	{ XMScalar, EXxmm_md, EXxEVexS } },
  },
  /* EVEX_W_0F2F_P_2 */
  {
    { Bad_Opcode },
    { "vcomisd",	{ XMScalar, EXxmm_mq, EXxEVexS } },
  },
  /* EVEX_W_0F51_P_0 */
  {
    { "vsqrtps",	{ XM, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F51_P_1 */
  {
    { "vsqrtss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR } },
  },
  /* EVEX_W_0F51_P_2 */
  {
    { Bad_Opcode },
    { "vsqrtpd",	{ XM, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F51_P_3 */
  {
    { Bad_Opcode },
    { "vsqrtsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F58_P_0 */
  {
    { "vaddps",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F58_P_1 */
  {
    { "vaddss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR } },
  },
  /* EVEX_W_0F58_P_2 */
  {
    { Bad_Opcode },
    { "vaddpd",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F58_P_3 */
  {
    { Bad_Opcode },
    { "vaddsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F59_P_0 */
  {
    { "vmulps",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F59_P_1 */
  {
    { "vmulss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR } },
  },
  /* EVEX_W_0F59_P_2 */
  {
    { Bad_Opcode },
    { "vmulpd",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F59_P_3 */
  {
    { Bad_Opcode },
    { "vmulsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F5A_P_0 */
  {
    { "vcvtps2pd",   { XM, EXEvexHalfBcstXmmq, EXxEVexS } },
  },
  /* EVEX_W_0F5A_P_1 */
  {
    { "vcvtss2sd",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS } },
  },
  /* EVEX_W_0F5A_P_2 */
  {
    { Bad_Opcode },
    { "vcvtpd2ps",	{ XMxmmq, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5A_P_3 */
  {
    { Bad_Opcode },
    { "vcvtsd2ss",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F5B_P_0 */
  {
    { "vcvtdq2ps",	{ XM, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5B_P_1 */
  {
    { "vcvttps2dq",	{ XM, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F5B_P_2 */
  {
    { "vcvtps2dq",	{ XM, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5C_P_0 */
  {
    { "vsubps",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5C_P_1 */
  {
    { "vsubss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR } },
  },
  /* EVEX_W_0F5C_P_2 */
  {
    { Bad_Opcode },
    { "vsubpd",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5C_P_3 */
  {
    { Bad_Opcode },
    { "vsubsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F5D_P_0 */
  {
    { "vminps",	{ XM, Vex, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F5D_P_1 */
  {
    { "vminss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS } },
  },
  /* EVEX_W_0F5D_P_2 */
  {
    { Bad_Opcode },
    { "vminpd",	{ XM, Vex, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F5D_P_3 */
  {
    { Bad_Opcode },
    { "vminsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS } },
  },
  /* EVEX_W_0F5E_P_0 */
  {
    { "vdivps",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5E_P_1 */
  {
    { "vdivss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexR } },
  },
  /* EVEX_W_0F5E_P_2 */
  {
    { Bad_Opcode },
    { "vdivpd",	{ XM, Vex, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F5E_P_3 */
  {
    { Bad_Opcode },
    { "vdivsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexR } },
  },
  /* EVEX_W_0F5F_P_0 */
  {
    { "vmaxps",	{ XM, Vex, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F5F_P_1 */
  {
    { "vmaxss",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS } },
  },
  /* EVEX_W_0F5F_P_2 */
  {
    { Bad_Opcode },
    { "vmaxpd",	{ XM, Vex, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F5F_P_3 */
  {
    { Bad_Opcode },
    { "vmaxsd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS } },
  },
  /* EVEX_W_0F62_P_2 */
  {
    { "vpunpckldq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F66_P_2 */
  {
    { "vpcmpgtd",	{ XMask, Vex, EXx } },
  },
  /* EVEX_W_0F6A_P_2 */
  {
    { "vpunpckhdq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F6C_P_2 */
  {
    { Bad_Opcode },
    { "vpunpcklqdq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F6D_P_2 */
  {
    { Bad_Opcode },
    { "vpunpckhqdq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F6E_P_2 */
  {
    { "vmovd",	{ XMScalar, Ed } },
    { "vmovq",	{ XMScalar, Eq } },
  },
  /* EVEX_W_0F6F_P_1 */
  {
    { "vmovdqu32",	{ XM, EXEvexXNoBcst } },
    { "vmovdqu64",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F6F_P_2 */
  {
    { "vmovdqa32",	{ XM, EXEvexXNoBcst } },
    { "vmovdqa64",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F70_P_2 */
  {
    { "vpshufd",	{ XM, EXx, Ib } },
  },
  /* EVEX_W_0F72_R_2_P_2 */
  {
    { "vpsrld",	{ Vex, EXx, Ib } },
  },
  /* EVEX_W_0F72_R_6_P_2 */
  {
    { "vpslld",	{ Vex, EXx, Ib } },
  },
  /* EVEX_W_0F73_R_2_P_2 */
  {
    { Bad_Opcode },
    { "vpsrlq",	{ Vex, EXx, Ib } },
  },
  /* EVEX_W_0F73_R_6_P_2 */
  {
    { Bad_Opcode },
    { "vpsllq",	{ Vex, EXx, Ib } },
  },
  /* EVEX_W_0F76_P_2 */
  {
    { "vpcmpeqd",	{ XMask, Vex, EXx } },
  },
  /* EVEX_W_0F78_P_0 */
  {
    { "vcvttps2udq",	{ XM, EXx, EXxEVexS } },
    { "vcvttpd2udq",	{ XMxmmq, EXx, EXxEVexS } },
  },
  /* EVEX_W_0F79_P_0 */
  {
    { "vcvtps2udq",	{ XM, EXx, EXxEVexR } },
    { "vcvtpd2udq",	{ XMxmmq, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F7A_P_1 */
  {
    { "vcvtudq2pd",	{ XM, EXEvexHalfBcstXmmq } },
  },
  /* EVEX_W_0F7A_P_3 */
  {
    { "vcvtudq2ps",	{ XM, EXx, EXxEVexR } },
  },
  /* EVEX_W_0F7B_P_1 */
  {
    { "vcvtusi2ss",	{ XMScalar, VexScalar, EXxEVexR, Ed } },
    { "vcvtusi2ss",	{ XMScalar, VexScalar, EXxEVexR, Eq } },
  },
  /* EVEX_W_0F7B_P_3 */
  {
    { "vcvtusi2sd",	{ XMScalar, VexScalar, Ed } },
    { "vcvtusi2sd",	{ XMScalar, VexScalar, EXxEVexR, Eq } },
  },
  /* EVEX_W_0F7E_P_1 */
  {
    { Bad_Opcode },
    { "vmovq",	{ XMScalar, EXxmm_mq } },
  },
  /* EVEX_W_0F7E_P_2 */
  {
    { "vmovd",	{ Ed, XMScalar } },
    { "vmovq",	{ Eq, XMScalar } },
  },
  /* EVEX_W_0F7F_P_1 */
  {
    { "vmovdqu32",	{ EXxS, XM } },
    { "vmovdqu64",	{ EXxS, XM } },
  },
  /* EVEX_W_0F7F_P_2 */
  {
    { "vmovdqa32",	{ EXxS, XM } },
    { "vmovdqa64",	{ EXxS, XM } },
  },
  /* EVEX_W_0FC2_P_0 */
  {
    { "vcmpps",	{ XMask, Vex, EXx, EXxEVexS, VCMP } },
  },
  /* EVEX_W_0FC2_P_1 */
  {
    { "vcmpss",	{ XMask, VexScalar, EXxmm_md, EXxEVexS, VCMP } },
  },
  /* EVEX_W_0FC2_P_2 */
  {
    { Bad_Opcode },
    { "vcmppd",	{ XMask, Vex, EXx, EXxEVexS, VCMP } },
  },
  /* EVEX_W_0FC2_P_3 */
  {
    { Bad_Opcode },
    { "vcmpsd",	{ XMask, VexScalar, EXxmm_mq, EXxEVexS, VCMP } },
  },
  /* EVEX_W_0FC6_P_0 */
  {
    { "vshufps",	{ XM, Vex, EXx, Ib } },
  },
  /* EVEX_W_0FC6_P_2 */
  {
    { Bad_Opcode },
    { "vshufpd",	{ XM, Vex, EXx, Ib } },
  },
  /* EVEX_W_0FD2_P_2 */
  {
    { "vpsrld",	{ XM, Vex, EXxmm } },
  },
  /* EVEX_W_0FD3_P_2 */
  {
    { Bad_Opcode },
    { "vpsrlq",	{ XM, Vex, EXxmm } },
  },
  /* EVEX_W_0FD4_P_2 */
  {
    { Bad_Opcode },
    { "vpaddq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0FD6_P_2 */
  {
    { Bad_Opcode },
    { "vmovq",	{ EXxmm_mq, XMScalar } },
  },
  /* EVEX_W_0FE6_P_1 */
  {
    { "vcvtdq2pd",	{ XM, EXEvexHalfBcstXmmq } },
  },
  /* EVEX_W_0FE6_P_2 */
  {
    { Bad_Opcode },
    { "vcvttpd2dq",	{ XMxmmq, EXx, EXxEVexS } },
  },
  /* EVEX_W_0FE6_P_3 */
  {
    { Bad_Opcode },
    { "vcvtpd2dq",	{ XMxmmq, EXx, EXxEVexR } },
  },
  /* EVEX_W_0FE7_P_2 */
  {
    { "vmovntdq",	{ EXEvexXNoBcst, XM } },
  },
  /* EVEX_W_0FF2_P_2 */
  {
    { "vpslld",	{ XM, Vex, EXxmm } },
  },
  /* EVEX_W_0FF3_P_2 */
  {
    { Bad_Opcode },
    { "vpsllq",	{ XM, Vex, EXxmm } },
  },
  /* EVEX_W_0FF4_P_2 */
  {
    { Bad_Opcode },
    { "vpmuludq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0FFA_P_2 */
  {
    { "vpsubd",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0FFB_P_2 */
  {
    { Bad_Opcode },
    { "vpsubq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0FFE_P_2 */
  {
    { "vpaddd",	{ XM, Vex, EXx } },
  },

  /* EVEX_W_0F380C_P_2 */
  {
    { "vpermilps",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F380D_P_2 */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F3811_P_1 */
  {
    { "vpmovusdb",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3812_P_1 */
  {
    { "vpmovusqb",	{ EXxmmdw, XM } },
  },
  /* EVEX_W_0F3813_P_1 */
  {
    { "vpmovusdw",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3813_P_2 */
  {
    { "vcvtph2ps",	{ XM, EXxmmq, EXxEVexS } },
  },
  /* EVEX_W_0F3814_P_1 */
  {
    { "vpmovusqw",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3815_P_1 */
  {
    { "vpmovusqd",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3818_P_2 */
  {
    { "vbroadcastss",	{ XM, EXxmm_md } },
  },
  /* EVEX_W_0F3819_P_2 */
  {
    { Bad_Opcode },
    { "vbroadcastsd",	{ XM, EXxmm_mq } },
  },
  /* EVEX_W_0F381A_P_2 */
  {
    { "vbroadcastf32x4",	{ XM, EXxmm } },
  },
  /* EVEX_W_0F381B_P_2 */
  {
    { Bad_Opcode },
    { "vbroadcastf64x4",	{ XM, EXymm } },
  },
  /* EVEX_W_0F381E_P_2 */
  {
    { "vpabsd",	{ XM, EXx } },
  },
  /* EVEX_W_0F381F_P_2 */
  {
    { Bad_Opcode },
    { "vpabsq",	{ XM, EXx } },
  },
  /* EVEX_W_0F3821_P_1 */
  {
    { "vpmovsdb",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3822_P_1 */
  {
    { "vpmovsqb",	{ EXxmmdw, XM } },
  },
  /* EVEX_W_0F3823_P_1 */
  {
    { "vpmovsdw",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3824_P_1 */
  {
    { "vpmovsqw",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3825_P_1 */
  {
    { "vpmovsqd",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3825_P_2 */
  {
    { "vpmovsxdq",	{ XM, EXxmmq } },
  },
  /* EVEX_W_0F3828_P_2 */
  {
    { Bad_Opcode },
    { "vpmuldq",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F3829_P_2 */
  {
    { Bad_Opcode },
    { "vpcmpeqq",	{ XMask, Vex, EXx } },
  },
  /* EVEX_W_0F382A_P_1 */
  {
    { Bad_Opcode },
    { "vpbroadcastmb2q",	{ XM, MaskR } },
  },
  /* EVEX_W_0F382A_P_2 */
  {
    { "vmovntdqa",	{ XM, EXEvexXNoBcst } },
  },
  /* EVEX_W_0F3831_P_1 */
  {
    { "vpmovdb",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3832_P_1 */
  {
    { "vpmovqb",	{ EXxmmdw, XM } },
  },
  /* EVEX_W_0F3833_P_1 */
  {
    { "vpmovdw",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3834_P_1 */
  {
    { "vpmovqw",	{ EXxmmqd, XM } },
  },
  /* EVEX_W_0F3835_P_1 */
  {
    { "vpmovqd",	{ EXxmmq, XM } },
  },
  /* EVEX_W_0F3835_P_2 */
  {
    { "vpmovzxdq",	{ XM, EXxmmq } },
  },
  /* EVEX_W_0F3837_P_2 */
  {
    { Bad_Opcode },
    { "vpcmpgtq",	{ XMask, Vex, EXx } },
  },
  /* EVEX_W_0F383A_P_1 */
  {
    { "vpbroadcastmw2d",	{ XM, MaskR } },
  },
  /* EVEX_W_0F3840_P_2 */
  {
    { "vpmulld",	{ XM, Vex, EXx } },
  },
  /* EVEX_W_0F3858_P_2 */
  {
    { "vpbroadcastd",	{ XM, EXxmm_md } },
  },
  /* EVEX_W_0F3859_P_2 */
  {
    { Bad_Opcode },
    { "vpbroadcastq",	{ XM, EXxmm_mq } },
  },
  /* EVEX_W_0F385A_P_2 */
  {
    { "vbroadcasti32x4",	{ XM, EXxmm } },
  },
  /* EVEX_W_0F385B_P_2 */
  {
    { Bad_Opcode },
    { "vbroadcasti64x4",	{ XM, EXymm } },
  },
  /* EVEX_W_0F3891_P_2 */
  {
    { "vpgatherqd",	{ XMxmmq, MVexVSIBQWpX } },
    { "vpgatherqq",	{ XM, MVexVSIBQWpX } },
  },
  /* EVEX_W_0F3893_P_2 */
  {
    { "vgatherqps",	{ XMxmmq, MVexVSIBQWpX } },
    { "vgatherqpd",	{ XM, MVexVSIBQWpX } },
  },
  /* EVEX_W_0F38A1_P_2 */
  {
    { "vpscatterqd",	{ MVexVSIBQWpX, XMxmmq } },
    { "vpscatterqq",	{ MVexVSIBQWpX, XM } },
  },
  /* EVEX_W_0F38A3_P_2 */
  {
    { "vscatterqps",	{ MVexVSIBQWpX, XMxmmq } },
    { "vscatterqpd",	{ MVexVSIBQWpX, XM } },
  },
  /* EVEX_W_0F38C7_R_1_P_2 */
  {
    { "vgatherpf0qps",  { MVexVSIBDWpX } },
    { "vgatherpf0qpd",  { MVexVSIBQWpX } },
  },
  /* EVEX_W_0F38C7_R_2_P_2 */
  {
    { "vgatherpf1qps",  { MVexVSIBDWpX } },
    { "vgatherpf1qpd",  { MVexVSIBQWpX } },
  },
  /* EVEX_W_0F38C7_R_5_P_2 */
  {
    { "vscatterpf0qps",  { MVexVSIBDWpX } },
    { "vscatterpf0qpd",  { MVexVSIBQWpX } },
  },
  /* EVEX_W_0F38C7_R_6_P_2 */
  {
    { "vscatterpf1qps",  { MVexVSIBDWpX } },
    { "vscatterpf1qpd",  { MVexVSIBQWpX } },
  },

  /* EVEX_W_0F3A00_P_2 */
  {
    { Bad_Opcode },
    { "vpermq",	{ XM, EXx, Ib } },
  },
  /* EVEX_W_0F3A01_P_2 */
  {
    { Bad_Opcode },
    { "vpermpd",	{ XM, EXx, Ib } },
  },
  /* EVEX_W_0F3A04_P_2 */
  {
    { "vpermilps",	{ XM, EXx, Ib } },
  },
  /* EVEX_W_0F3A05_P_2 */
  {
    { Bad_Opcode },
    { "vpermilpd",	{ XM, EXx, Ib } },
  },
  /* EVEX_W_0F3A08_P_2 */
  {
    { "vrndscaleps",	{ XM, EXx, EXxEVexS, Ib } },
  },
  /* EVEX_W_0F3A09_P_2 */
  {
    { Bad_Opcode },
    { "vrndscalepd",	{ XM, EXx, EXxEVexS, Ib } },
  },
  /* EVEX_W_0F3A0A_P_2 */
  {
    { "vrndscaless",	{ XMScalar, VexScalar, EXxmm_md, EXxEVexS, Ib } },
  },
  /* EVEX_W_0F3A0B_P_2 */
  {
    { Bad_Opcode },
    { "vrndscalesd",	{ XMScalar, VexScalar, EXxmm_mq, EXxEVexS, Ib } },
  },
  /* EVEX_W_0F3A18_P_2 */
  {
    { "vinsertf32x4",	{ XM, Vex, EXxmm, Ib } },
  },
  /* EVEX_W_0F3A19_P_2 */
  {
    { "vextractf32x4",	{ EXxmm, XM, Ib } },
  },
  /* EVEX_W_0F3A1A_P_2 */
  {
    { Bad_Opcode },
    { "vinsertf64x4",	{ XM, Vex, EXxmmq, Ib } },
  },
  /* EVEX_W_0F3A1B_P_2 */
  {
    { Bad_Opcode },
    { "vextractf64x4",	{ EXxmmq, XM, Ib } },
  },
  /* EVEX_W_0F3A1D_P_2 */
  {
    { "vcvtps2ph",	{ EXxmmq, XM, EXxEVexS, Ib } },
  },
  /* EVEX_W_0F3A21_P_2 */
  {
    { "vinsertps",	{ XMM, Vex, EXxmm_md, Ib } },
  },
  /* EVEX_W_0F3A23_P_2 */
  {
    { "vshuff32x4",	{ XM, Vex, EXx, Ib } },
    { "vshuff64x2",	{ XM, Vex, EXx, Ib } },
  },
  /* EVEX_W_0F3A38_P_2 */
  {
    { "vinserti32x4",	{ XM, Vex, EXxmm, Ib } },
  },
  /* EVEX_W_0F3A39_P_2 */
  {
    { "vextracti32x4",	{ EXxmm, XM, Ib } },
  },
  /* EVEX_W_0F3A3A_P_2 */
  {
    { Bad_Opcode },
    { "vinserti64x4",	{ XM, Vex, EXxmmq, Ib } },
  },
  /* EVEX_W_0F3A3B_P_2 */
  {
    { Bad_Opcode },
    { "vextracti64x4",	{ EXxmmq, XM, Ib } },
  },
  /* EVEX_W_0F3A43_P_2 */
  {
    { "vshufi32x4",	{ XM, Vex, EXx, Ib } },
    { "vshufi64x2",	{ XM, Vex, EXx, Ib } },
  },
#endif /* NEED_VEX_W_TABLE */
#ifdef NEED_MOD_TABLE
  {
    /* MOD_EVEX_0F10_PREFIX_1 */
    { VEX_W_TABLE (EVEX_W_0F10_P_1_M_0) },
    { VEX_W_TABLE (EVEX_W_0F10_P_1_M_1) },
  },
  {
    /* MOD_EVEX_0F10_PREFIX_3 */
    { VEX_W_TABLE (EVEX_W_0F10_P_3_M_0) },
    { VEX_W_TABLE (EVEX_W_0F10_P_3_M_1) },
  },
  {
    /* MOD_EVEX_0F11_PREFIX_1 */
    { VEX_W_TABLE (EVEX_W_0F11_P_1_M_0) },
    { VEX_W_TABLE (EVEX_W_0F11_P_1_M_1) },
  },
  {
    /* MOD_EVEX_0F11_PREFIX_3 */
    { VEX_W_TABLE (EVEX_W_0F11_P_3_M_0) },
    { VEX_W_TABLE (EVEX_W_0F11_P_3_M_1) },
  },
  {
    /* MOD_EVEX_0F12_PREFIX_0 */
    { VEX_W_TABLE (EVEX_W_0F12_P_0_M_0) },
    { VEX_W_TABLE (EVEX_W_0F12_P_0_M_1) },
  },
  {
    /* MOD_EVEX_0F16_PREFIX_0 */
    { VEX_W_TABLE (EVEX_W_0F16_P_0_M_0) },
    { VEX_W_TABLE (EVEX_W_0F16_P_0_M_1) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_1 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C6_REG_1) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_2 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C6_REG_2) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_5 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C6_REG_5) },
  },
  {
    /* MOD_EVEX_0F38C6_REG_6 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C6_REG_6) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_1 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C7_REG_1) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_2 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C7_REG_2) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_5 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C7_REG_5) },
  },
  {
    /* MOD_EVEX_0F38C7_REG_6 */
    { PREFIX_TABLE (PREFIX_EVEX_0F38C7_REG_6) },
  },
#endif /* NEED_MOD_TABLE */
