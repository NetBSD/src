/* Bit Manipulation Instructions.  */

/* movl<.cl> */
{ "movh", 0x48080000, 0xf81f0000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_R_SRC1, NPS_UIMM16 }, { 0 }},
{ "movh", 0x48180000, 0xf81f0000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_UIMM16 }, { C_NPS_CL }},

/* movl<.cl> */
{ "movl", 0x48090000, 0xf81f0000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_R_SRC1, NPS_UIMM16 }, { 0 }},
{ "movl", 0x48190000, 0xf81f0000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_UIMM16 }, { C_NPS_CL }},

/* movb<.f><.cl> */
{ "movb", 0x48010000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_BITOP_DST_POS, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},
{ "movb", 0x48018000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_BITOP_DST_POS, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F, C_NPS_CL }},

/* movbi<.f><.cl> */
{ "movbi", 0x480f0000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_R_SRC1, NPS_BITOP_UIMM8, NPS_BITOP_DST_POS, NPS_BITOP_SIZE_2B  }, { C_NPS_F }},
{ "movbi", 0x480f8000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST, NPS_BITOP_UIMM8, NPS_BITOP_DST_POS, NPS_BITOP_SIZE_2B  }, { C_NPS_F, C_NPS_CL }},

/* decode1<.f> */
{ "decode1", 0x48038040, 0xf80f83e0, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},

/* decode1.cl<.f> */
{ "decode1", 0x48038060, 0xf80803e0, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_BITOP_DST_POS_SZ }, { C_NPS_CL, C_NPS_F }},

/* fbset<.f> */
{ "fbset", 0x48038000, 0xf80f83e0, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},

/* fbclr<.f> */
{ "fbclr", 0x48030000, 0xf80f83e0, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},

/* encode0<.f> */
{ "encode0", 0x48040000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},

/* encode1<.f> */
{ "encode1", 0x48048000, 0xf80f8000, ARC_OPCODE_ARC700, BITOP, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_BITOP_SRC_POS, NPS_BITOP_SIZE }, { C_NPS_F }},

/* mrgb - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mrgb.cl - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov2b - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov2b.cl - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* ext4 - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* ext4.cl - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* ins4 - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* ins4.cl - 48 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov3b - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov4b - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov3bcl - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov4bcl - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov3b.cl - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */
/* mov4b.cl - 64 bit instruction, see arc_long_opcodes in arc-opc.c.  */

/* rflt a,b,c   00111bbb00101110FBBBCCCCCCAAAAAA */
{ "rflt", 0x382e0000, 0xf8ff8000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, RC }, { 0 }},

/* rflt a,limm,c   0011111000101110F111CCCCCCAAAAAA */
{ "rflt", 0x3e2e7000, 0xfffff000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, RC }, { 0 }},

/* rflt a,b,u6   00111bbb01101110FBBBuuuuuuAAAAAA */
{ "rflt", 0x386e0000, 0xf8ff8000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, NPS_RFLT_UIMM6 }, { 0 }},

/* rflt 0,b,c   00111bbb00101110FBBBCCCCCC111110 */
{ "rflt", 0x382e003e, 0xf8ff803f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, RC }, { 0 }},

/* rflt 0,limm,c   0011111000101110F111CCCCCC111110 */
{ "rflt", 0x3e2e703e, 0xfffff03f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, RC }, { 0 }},

/* rflt 0,b,u6   00111bbb01101110FBBBuuuuuu111110 */
{ "rflt", 0x386e003e, 0xf8ff803f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, NPS_RFLT_UIMM6 }, { 0 }},

/* rflt 0,b,limm   00111bbb00101110FBBB111110111110 */
{ "rflt", 0x382e0fbe, 0xf8ff8fff, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, LIMM }, { 0 }},

/* rflt a,b,limm   00111bbb00101110FBBB111110AAAAAA */
{ "rflt", 0x382e0f80, 0xf8ff8fc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, LIMM }, { 0 }},

/* rflt a,limm,limm    0011111000101110F111111110AAAAAA */
{ "rflt", 0x3e2e7f80, 0xffffffc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, LIMMdup }, { 0 }},

/* rflt a,limm,u6   0011111001101110F111uuuuuuAAAAAA */
{ "rflt", 0x3e6e7000, 0xfffff000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, NPS_RFLT_UIMM6 }, { 0 }},

/* rflt 0,limm,u6   0011111001101110F111uuuuuu111110 */
{ "rflt", 0x3e6e703e, 0xfffff03f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, NPS_RFLT_UIMM6 }, { 0 }},

/* crc16<.r> a,b,c  00111bbb00110011RBBBCCCCCCAAAAAA */
{ "crc16", 0x38330000, 0xf8ff0000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, RC }, { C_NPS_R }},

/* crc16<.r> a,limm,c  0011111000110011R111CCCCCCAAAAAA */
{ "crc16", 0x3e337000, 0xffff7000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, RC }, { C_NPS_R }},

/* crc16<.r> a,b,u6  00111bbb01110011RBBBuuuuuuAAAAAA */
{ "crc16", 0x38730000, 0xf8ff0000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, UIMM6_20 }, { C_NPS_R }},

/* crc16<.r> 0,b,c  00111bbb00110011RBBBCCCCCC111110 */
{ "crc16", 0x3833003e, 0xf8ff003f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, RC }, { C_NPS_R }},

/* crc16<.r> 0,limm,c  0011111000110011R111CCCCCC111110 */
{ "crc16", 0x3e33703e, 0xffff703f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, RC }, { C_NPS_R }},

/* crc16<.r> 0,b,u6  00111bbb01110011RBBBuuuuuu111110 */
{ "crc16", 0x3873003e, 0xf8ff003f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, UIMM6_20 }, { C_NPS_R }},

/* crc16<.r> 0,b,limm  00111bbb00110011RBBB111110111110 */
{ "crc16", 0x38330fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, LIMM }, { C_NPS_R }},

/* crc16<.r> a,b,limm  00111bbb00110011RBBB111110AAAAAA */
{ "crc16", 0x38330f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, LIMM }, { C_NPS_R }},

/* crc16<.r> a,limm,limm  0011111000110011R111111110AAAAAA */
{ "crc16", 0x3e337f80, 0xffff7fc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, LIMMdup }, { C_NPS_R }},

/* crc16<.r> a,limm,u6  0011111001110011R111uuuuuuAAAAAA */
{ "crc16", 0x3e737000, 0xffff7000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, UIMM6_20 }, { C_NPS_R }},

/* crc16<.r> 0,limm,u6  0011111001110011R111uuuuuu111110 */
{ "crc16", 0x3e73703e, 0xffff703f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, UIMM6_20 }, { C_NPS_R }},

/* crc32<.r> a,b,c		00111 bbb 00 110100 R BBB CCCCCC AAAAAA */
{ "crc32", 0x38340000, 0xf8ff0000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, RC }, { C_NPS_R }},

/* crc32<.r> a,limm,c		00111 110 00 110100 R 111 CCCCCC AAAAAA */
{ "crc32", 0x3e347000, 0xffff7000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, RC }, { C_NPS_R }},

/* crc32<.r> a,b,u6		00111 bbb 01 110100 R BBB uuuuuu AAAAAA */
{ "crc32", 0x38740000, 0xf8ff0000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, UIMM6_20 }, { C_NPS_R }},

/* crc32<.r> 0,b,c		00111 bbb 00 110100 R BBB CCCCCC 111110 */
{ "crc32", 0x3834003e, 0xf8ff003f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, RC }, { C_NPS_R }},

/* crc32<.r> 0,limm,c		00111 110 00 110100 R 111 CCCCCC 111110 */
{ "crc32", 0x3e34703e, 0xffff703f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, RC }, { C_NPS_R }},

/* crc32<.r> 0,b,u6		00111 bbb 01 110100 R BBB uuuuuu 111110 */
{ "crc32", 0x3874003e, 0xf8ff003f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, UIMM6_20 }, { C_NPS_R }},

/* crc32<.r> 0,b,limm		00111 bbb 00 110100 R BBB 111110 111110 */
{ "crc32", 0x38340fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, RB, LIMM }, { C_NPS_R }},

/* crc32<.r> a,b,limm		00111 bbb 00 110100 R BBB 111110 AAAAAA */
{ "crc32", 0x38340f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, RB, LIMM }, { C_NPS_R }},

/* crc32<.r> a,limm,limm	00111 110 00 110100 R 111 111110 AAAAAA */
{ "crc32", 0x3e347f80, 0xffff7fc0, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, LIMMdup }, { C_NPS_R }},

/* crc32<.r> a,limm,u6		00111 110 01 110100 R 111 uuuuuu AAAAAA */
{ "crc32", 0x3e747000, 0xffff7000, ARC_OPCODE_ARC700, BITOP, NPS400, { RA, LIMM, UIMM6_20 }, { C_NPS_R }},

/* crc32<.r> 0,limm,u6		00111 110 01 110100 R 111 uuuuuu 111110 */
{ "crc32", 0x3e74703e, 0xffff703f, ARC_OPCODE_ARC700, BITOP, NPS400, { ZA, LIMM, UIMM6_20 }, { C_NPS_R }},

/* Arithmetic & Logic Instructions.  */

#define ADDB_LIKE(NAME,SUBOP2)                                          \
  { NAME, (0x48000000 | SUBOP2), 0xf80f001f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC1_POS, NPS_SRC2_POS, NPS_ADDB_SIZE }, { C_NPS_F, C_NPS_SX }},

ADDB_LIKE ("addb", 0)
ADDB_LIKE ("subb", 4)
ADDB_LIKE ("adcb", 5)
ADDB_LIKE ("sbcb", 6)

#define ANDB_LIKE(NAME,SUBOP2,SIZE_OPERAND)                             \
  { NAME, (0x48000000 | SUBOP2), 0xf80f001f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC1_POS, NPS_SRC2_POS, SIZE_OPERAND }, { C_NPS_F }},

ANDB_LIKE ("andb", 1, NPS_ANDB_SIZE)
ANDB_LIKE ("xorb", 2, NPS_ANDB_SIZE)
ANDB_LIKE ("orb", 3, NPS_ANDB_SIZE)
ANDB_LIKE ("fxorb", 7, NPS_FXORB_SIZE)
ANDB_LIKE ("wxorb", 8, NPS_WXORB_SIZE)
ANDB_LIKE ("shlb", 0xb, NPS_ANDB_SIZE)
ANDB_LIKE ("shrb", 0xc, NPS_ANDB_SIZE)

#define NOTB_LIKE(NAME,SUBOP2)                                          \
  { NAME, (0x48000000 | SUBOP2), 0xf80f001f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_SRC2_POS, NPS_ANDB_SIZE }, { C_NPS_F }},

NOTB_LIKE ("notb", 0x9)
NOTB_LIKE ("cntbb", 0xa)

#define DIV_LIKE(NAME,DIV_MODE)                                          \
  { NAME, (0x4800000d | DIV_MODE << 14), 0xf80fc3ff, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC1_POS, NPS_SRC2_POS, }, { C_NPS_F }}, \
  { NAME, (0x4800020d | DIV_MODE << 14), 0xf8efc21f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_DIV_UIMM4, NPS_SRC1_POS }, { C_NPS_F }},

DIV_LIKE ("div", 0x1)
DIV_LIKE ("mod", 0x2)
DIV_LIKE ("divm", 0x0)

{ "qcmp", 0x4810000e, 0xf81f001e, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS, NPS_QCMP_SIZE, NPS_QCMP_M1, NPS_QCMP_M2, NPS_QCMP_M3 }, { C_NPS_AR_AL }},
{ "qcmp", 0x481001ee, 0xf81f01fe, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS, NPS_QCMP_SIZE, NPS_QCMP_M1, NPS_QCMP_M2 }, { C_NPS_AR_AL }},
{ "qcmp", 0x481001ee, 0xf81f81fe, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS, NPS_QCMP_SIZE, NPS_QCMP_M1  }, { C_NPS_AR_AL }},
{ "qcmp", 0x481001ee, 0xf81fc1fe, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS, NPS_QCMP_SIZE  }, { C_NPS_AR_AL }},

{ "calcsd", 0x48000010, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_CALC_ENTRY_SIZE }, { C_NPS_F }},
{ "calcxd", 0x48004010, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_CALC_ENTRY_SIZE }, { C_NPS_F }},

{ "calcbsd", 0x48000030, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B }, { C_NPS_F }},
{ "calcbxd", 0x48004030, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B }, { C_NPS_F }},

{ "calckey", 0x48000050, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B }, { C_NPS_F }},
{ "calcxkey", 0x48004050, 0xf80f407f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B }, { C_NPS_F }},

{ "mxb", 0x580b0000, 0xf81f8007, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_FIELD_START_POS, NPS_FIELD_SIZE, NPS_SHIFT_FACTOR }, { 0 }},
{ "mxb", 0x580b8000, 0xf81f8007, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_FIELD_START_POS, NPS_FIELD_SIZE, NPS_SHIFT_FACTOR, NPS_BITS_TO_SCRAMBLE }, { C_NPS_S }},
{ "imxb", 0x580b0001, 0xf81f8007, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_FIELD_START_POS, NPS_FIELD_SIZE, NPS_SHIFT_FACTOR }, { 0 }},
{ "imxb", 0x580b8001, 0xf81f8007, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_FIELD_START_POS, NPS_FIELD_SIZE, NPS_SHIFT_FACTOR, NPS_BITS_TO_SCRAMBLE }, { C_NPS_S }},

#define ADDL_LIKE(NAME,SUBOP2,SHIM)                                     \
  { NAME, (0x48000000 | (SUBOP2 << 16)), 0xf80f0000, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST, NPS_R_SRC1, SHIM }, { C_NPS_F }},

ADDL_LIKE ("addl", 0xA, NPS_SIMM16)
ADDL_LIKE ("subl", 0xB, NPS_SIMM16)
ADDL_LIKE ("orl", 0xC, NPS_UIMM16)
ADDL_LIKE ("andl", 0xD, NPS_UIMM16)
ADDL_LIKE ("xorl", 0xE, NPS_UIMM16)

{ "andab", 0x48000011, 0xf80f801f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_SRC2_POS_5B, NPS_BITOP_SIZE }, { C_NPS_F } },
{ "andab", 0x48008011, 0xf80f801f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS_5B, NPS_BITOP_SIZE }, { C_NPS_F } },
{ "orab", 0x48000012, 0xf80f801f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_SRC2_POS_5B, NPS_BITOP_SIZE }, { C_NPS_F } },
{ "orab", 0x48008012, 0xf80f801f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_SRC2_POS_5B, NPS_BITOP_SIZE }, { C_NPS_F } },

{ "lbdsize", 0x382f0005, 0xf8ff003f, ARC_OPCODE_ARC700, ARITH, NPS400, { RB, RC }, { C_F }},

{ "bdlen", 0x48000013, 0xf80fc01f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B, NPS_BDLEN_MAX_LEN }, { C_NPS_F }},
{ "bdlen", 0x48004013, 0xf80fc01f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC2_3B }, { C_NPS_F }},
{ "bdlen", 0x48008013, 0xf80fc01f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B, NPS_BDLEN_MAX_LEN }, { C_NPS_F }},
{ "bdlen", 0x4800c013, 0xf80fc01f, ARC_OPCODE_ARC700, ARITH, NPS400, { NPS_R_DST_3B, NPS_R_SRC1_3B, NPS_R_SRC2_3B }, { C_NPS_F }},

/* csma a,b,c   00111bbb00100001FBBBCCCCCCAAAAAA */
{ "csma", 0x382a0000, 0xf8ff8000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, RC }, { 0 }},

/* csma a,limm,c   0011111000100001F111CCCCCCAAAAAA */
{ "csma", 0x3e2a7000, 0xfffff000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, RC }, { 0 }},

/* csma a,b,u6   00111bbb01100001FBBBuuuuuuAAAAAA */
{ "csma", 0x386a0000, 0xf8ff8000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, UIMM6_20 }, { 0 }},

/* csma 0,b,c   00111bbb00100001FBBBCCCCCC111110 */
{ "csma", 0x382a003e, 0xf8ff803f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, RC }, { 0 }},

/* csma 0,limm,c   0011111000100001F111CCCCCC111110 */
{ "csma", 0x3e2a703e, 0xfffff03f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, RC }, { 0 }},

/* csma 0,b,u6   00111bbb01100001FBBBuuuuuu111110 */
{ "csma", 0x386a003e, 0xf8ff803f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, UIMM6_20 }, { 0 }},

/* csma 0,b,limm   00111bbb00100001FBBB111110111110 */
{ "csma", 0x382a0fbe, 0xf8ff8fff, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, LIMM }, { 0 }},

/* csma a,b,limm   00111bbb00100001FBBB111110AAAAAA */
{ "csma", 0x382a0f80, 0xf8ff8fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, LIMM }, { 0 }},

/* csma a,limm,limm    0011111000100001F111111110AAAAAA */
{ "csma", 0x3e2a7f80, 0xffffffc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, LIMMdup }, { 0 }},

/* csma a,limm,u6   0011111001100001F111uuuuuuAAAAAA */
{ "csma", 0x3e6a7000, 0xfffff000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, UIMM6_20 }, { 0 }},

/* csma 0,limm,u6   0011111001100001F111uuuuuu111110 */
{ "csma", 0x3e6a703e, 0xfffff03f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, UIMM6_20 }, { 0 }},

/* csms a,b,c   00111bbb00101100FBBBCCCCCCAAAAAA */
{ "csms", 0x382c0000, 0xf8ff8000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, RC }, { 0 }},

/* csma a,limm,c   0011111000101100F111CCCCCCAAAAAA */
{ "csms", 0x3e2c7000, 0xfffff000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, RC }, { 0 }},

/* csms a,b,u6   00111bbb01101100FBBBuuuuuuAAAAAA */
{ "csms", 0x386c0000, 0xf8ff8000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, UIMM6_20 }, { 0 }},

/* csms 0,b,c   00111bbb00101100FBBBCCCCCC111110 */
{ "csms", 0x382c003e, 0xf8ff803f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, RC }, { 0 }},

/* csms 0,limm,c   0011111000101100F111CCCCCC111110 */
{ "csms", 0x3e2c703e, 0xfffff03f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, RC }, { 0 }},

/* csms 0,b,u6   00111bbb01101100FBBBuuuuuu111110 */
{ "csms", 0x386c003e, 0xf8ff803f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, UIMM6_20 }, { 0 }},

/* csms 0,b,limm   00111bbb00101100FBBB111110111110 */
{ "csms", 0x382c0fbe, 0xf8ff8fff, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, LIMM }, { 0 }},

/* csms a,b,limm   00111bbb00101100FBBB111110AAAAAA */
{ "csms", 0x382c0f80, 0xf8ff8fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, LIMM }, { 0 }},

/* csms a,limm,limm   0011111000101100F111111110AAAAAA */
{ "csms", 0x3e2c7f80, 0xffffffc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, LIMMdup }, { 0 }},

/* csms a,limm,u6   0011111001101100F111uuuuuuAAAAAA */
{ "csms", 0x3e6c7000, 0xfffff000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, UIMM6_20 }, { 0 }},

/* csms 0,limm,u6   0011111001101100F111uuuuuu111110 */
{ "csms", 0x3e6c703e, 0xfffff03f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, UIMM6_20 }, { 0 }},

/* cbba a,b,c   00111bbb00101101FBBBCCCCCCAAAAAA */
{ "cbba", 0x382d0000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, RC }, { C_F }},

/* cbba a,limm,c    0011111000101101F111CCCCCCAAAAAA */
{ "cbba", 0x3e2d7000, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, RC }, { C_F }},

/* cbba a,b,u6   00111bbb01101101FBBBuuuuuuAAAAAA */
{ "cbba", 0x386d0000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, UIMM6_20 }, { C_F }},

/* cbba 0,b,c   00111bbb00101101FBBBCCCCCC111110 */
{ "cbba", 0x382d003e, 0xf8ff003f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, RC }, { C_F }},

/* cbba 0,limm,c   0011111000101101F111CCCCCC111110 */
{ "cbba", 0x3e2d703e, 0xffff703f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, RC }, { C_F }},

/* cbba 0,b,u6   00111bbb01101101FBBBuuuuuu111110 */
{ "cbba", 0x386d003e, 0xf8ff003f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, UIMM6_20 }, { C_F }},

/* cbba 0,b,limm   00111bbb00101101FBBB111110111110 */
{ "cbba", 0x382d0fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, LIMM }, { C_F }},

/* cbba a,b,limm   00111bbb00101101FBBB111110AAAAAA */
{ "cbba", 0x382d0f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, LIMM }, { C_F }},

/* cbba a,limm,limm   0011111000101101F111111110AAAAAA */
{ "cbba", 0x3e2d7f80, 0xffff7fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, LIMMdup }, { C_F }},

/* cbba a,limm,u6   0011111001101101F111uuuuuuAAAAAA */
{ "cbba", 0x3e6d7000, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, UIMM6_20 }, { C_F }},

/* cbba 0,limm,u6   0011111001101101F111uuuuuu111110 */
{ "cbba", 0x3e6d703e, 0xffff703f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, UIMM6_20 }, { C_F }},

/* zncv<.rd|.wr> a,b,c   00111bbb001101010BBBCCCCCCAAAAAA */
{ "zncv", 0x38350000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, RC }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> a,b,u6   00111bbb011101010BBBuuuuuuAAAAAA */
{ "zncv", 0x38750000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, UIMM6_20}, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> b,b,s12   00111bbb101101010BBBssssssSSSSSS */
{ "zncv", 0x38b50000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RB, RBdup, SIMM12_20 }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> a,b,limm   00111bbb001101010BBB111110AAAAAA */
{ "zncv", 0x38350f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, LIMM }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> a,limm,c   00111110001101010111CCCCCCAAAAAA */
{ "zncv", 0x3e357000, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, RC }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> a,limm,u6   00111110011101010111uuuuuuAAAAAA */
{ "zncv", 0x3e757000, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, UIMM6_20 }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> a,limm,limm   00111110001101010111111110AAAAAA */
{ "zncv", 0x3e357f80, 0xffff7fc0, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, LIMM, LIMMdup }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,b,c 00111bbb001101010BBBCCCCCC111110 */
{ "zncv", 0x3835003e, 0xf8ff003f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, RC }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,b,u6   00111bbb011101010BBBuuuuuu111110 */
{ "zncv", 0x3875003e, 0xf8ff003f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, UIMM6_20 }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,b,limm   00111bbb001101010BBB111110111110 */
{ "zncv", 0x38350fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, RB, LIMM }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,limm,c   00111110001101010111CCCCCC111110 */
{ "zncv", 0x3e35703e, 0xffff703f, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, RC }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,limm,u6   00111110011101010111uuuuuu111110 */
{ "zncv", 0x3e75703e, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, UIMM6_20 }, { C_NPS_ZNCV }},

/* zncv<.rd|.wr> 0,limm,s12   00111110101101010111ssssssSSSSSS */
{ "zncv", 0x3eb57000, 0xffff7000, ARC_OPCODE_ARC700, ARITH, NPS400, { ZA, LIMM, SIMM12_20 }, { C_NPS_ZNCV }},

/* hofs a,b,c */
{ "hofs", 0x38360000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, RC }, { C_F }},

/* hofs a,b,min_hofs,psbc */
{ "hofs", 0x38760000, 0xf8ff0000, ARC_OPCODE_ARC700, ARITH, NPS400, { RA, RB, NPS_MIN_HOFS, NPS_PSBC }, { C_F }},

/* Protocol Decoder Instructions.  */

/* dctcp b,c  00111bbb001011110bbbcccccc000000 */
{ "dctcp", 0x382f0000, 0xf8ff803f, ARC_OPCODE_ARC700, NET, NPS400, { RB, RC }, { 0 }},

/* dcip a,b,c  00111bbb001011110bbbccccccaaaaaa */
{ "dcip", 0x38290000, 0xf8ff8000, ARC_OPCODE_ARC700, NET, NPS400, { RA, RB, RC }, { 0 }},

/* dcet b,c  00111bbb001011110bbbcccccc000010 */
{ "dcet", 0x382f0002, 0xf8ff803f, ARC_OPCODE_ARC700, NET, NPS400, { RB, RC }, { 0 }},

/* dcet a,b,c  00111bbb001000000bbbccccccaaaaaa */
{ "dcet", 0x38200000, 0xf8ff8000, ARC_OPCODE_ARC700, NET, NPS400, { RA, RB, RC }, { 0 }},

/* ACL Instructions.  */

/* dcacl<.f> a,b,c  00111bbb001001010bbbccccccaaaaaa */
{ "dcacl", 0x38250000, 0xf8ff0000, ARC_OPCODE_ARC700, ACL, NPS400, { RA, RB, RC }, { C_F }},

/* DPI Instructions.  */

/* hash dst,src1,src2,width,perm,nonlinear,basemat */
{ "hash", 0x58180000, 0xf81f0000, ARC_OPCODE_ARC700, DPI, NPS400, { NPS_DPI_DST, NPS_DPI_SRC1_3B, NPS_R_SRC2_3B, NPS_HASH_WIDTH, NPS_HASH_PERM, NPS_HASH_NONLINEAR, NPS_HASH_BASEMAT }, { 0 }},

/* hash.pN dst,src1,src2,width,len,ofs,basemat */

#define HASH_P(FUNC, SUBOP2)                                            \
  { "hash", (0x58100000 | (SUBOP2 << 16)), 0xf81f0000, ARC_OPCODE_ARC700, DPI, NPS400, { NPS_DPI_DST, NPS_DPI_SRC1_3B, NPS_R_SRC2_3B, NPS_HASH_WIDTH, NPS_HASH_LEN, NPS_HASH_OFS, NPS_HASH_BASEMAT2 }, { C_NPS_P##FUNC }},

HASH_P(0, 0x9)
HASH_P(1, 0xA)
HASH_P(2, 0xB)
HASH_P(3, 0xC)

/* tr<.f> a,b,c   00111bbb00100001FBBBCCCCCCAAAAAA */
{ "tr", 0x38210000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, RC }, { C_F }},

/* tr<.f> a,limm,c   0011111000100001F111CCCCCCAAAAAA */
{ "tr", 0x3e217000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, RC }, { C_F }},

/* tr<.f> a,b,u6   00111bbb01100001FBBBuuuuuuAAAAAA */
{ "tr", 0x38610000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, UIMM6_20 }, { C_F }},

/* tr<.f> 0,b,c   00111bbb00100001FBBBCCCCCC111110 */
{ "tr", 0x3821003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, RC }, { C_F }},

/* tr<.f> 0,limm,c   0011111000100001F111CCCCCC111110 */
{ "tr", 0x3e21703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, RC }, { C_F }},

/* tr<.f> 0,b,u6   00111bbb01100001FBBBuuuuuu111110 */
{ "tr", 0x3861003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, UIMM6_20 }, { C_F }},

/* tr<.f> 0,b,limm   00111bbb00100001FBBB111110111110 */
{ "tr", 0x38210fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, LIMM }, { C_F }},

/* tr<.f> a,b,limm   00111bbb00100001FBBB111110AAAAAA */
{ "tr", 0x38210f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, LIMM }, { C_F }},

/* tr<.f> a,limm,limm   0011111000100001F111111110AAAAAA */
{ "tr", 0x3e217f80, 0xffff7fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, LIMMdup }, { C_F }},

/* tr<.f> a,limm,u6   0011111001100001F111uuuuuuAAAAAA */
{ "tr", 0x3e617000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, UIMM6_20 }, { C_F }},

/* tr<.f> 0,limm,u6   0011111001100001F111uuuuuu111110 */
{ "tr", 0x3e61703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, UIMM6_20 }, { C_F }},

/* utf8 a,b,c       00111bbb00100011FBBBCCCCCCAAAAAA */
{ "utf8", 0x38220000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, RC }, { C_F }},

/* utf8 a,limm,c    0011111000100011F111CCCCCCAAAAAA */
{ "utf8", 0x3e227000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, RC }, { C_F }},

/* utf8 a,b,u6      00111bbb01100011FBBBuuuuuuAAAAAA */
{ "utf8", 0x38620000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, UIMM6_20 }, { C_F }},

/* utf8 0,b,c       00111bbb00100011FBBBCCCCCC111110 */
{ "utf8", 0x3822003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, RC }, { C_F }},

/* utf8 0,limm,c    0011111000100011F111CCCCCC111110 */
{ "utf8", 0x3e22703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, RC }, { C_F }},

/* utf8 0,b,u6      00111bbb01100011FBBBuuuuuu111110 */
{ "utf8", 0x3862003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, UIMM6_20 }, { C_F }},

/* utf8 0,b,limm    00111bbb00100011FBBB111110111110 */
{ "utf8", 0x38220fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, LIMM }, { C_F }},

/* utf8 a,b,limm    00111bbb00100011FBBB111110AAAAAA */
{ "utf8", 0x38220f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, LIMM }, { C_F }},

/* utf8 a,limm,limm 0011111000100011F111111110AAAAAA */
{ "utf8", 0x3e227f80, 0xffff7fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, LIMMdup }, { C_F }},

/* utf8 a,limm,u6   0011111001100011F111uuuuuuAAAAAA */
{ "utf8", 0x3e627000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, UIMM6_20 }, { C_F }},

/* utf8 0,limm,u6   0011111001100011F111uuuuuu111110 */
{ "utf8", 0x3e62703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, UIMM6_20 }, { C_F }},

/* e4by dst,src1,src2,index0,index1,index2,index3 */
{ "e4by", 0x581d0000, 0xf81f0000, ARC_OPCODE_ARC700, DPI, NPS400, { NPS_DPI_DST, NPS_DPI_SRC1_3B, NPS_R_SRC2_3B, NPS_E4BY_INDEX0, NPS_E4BY_INDEX1, NPS_E4BY_INDEX2, NPS_E4BY_INDEX3 }, { 0 }},

/* addf<.f> a,b,c       00111bbb00100011FBBBCCCCCCAAAAAA */
{ "addf", 0x38230000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, RC }, { C_F }},

/* addf<.f> a,limm,c    0011111000100011F111CCCCCCAAAAAA */
{ "addf", 0x3e237000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, RC }, { C_F }},

/* addf<.f> a,b,u6      00111bbb01100011FBBBuuuuuuAAAAAA */
{ "addf", 0x38630000, 0xf8ff0000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, UIMM6_20 }, { C_F }},

/* addf<.f> 0,b,c       00111bbb00100011FBBBCCCCCC111110 */
{ "addf", 0x3823003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, RC }, { C_F }},

/* addf<.f> 0,limm,c    0011111000100011F111CCCCCC111110 */
{ "addf", 0x3e23703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, RC }, { C_F }},

/* addf<.f> 0,b,u6      00111bbb01100011FBBBuuuuuu111110 */
{ "addf", 0x3863003e, 0xf8ff003f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, UIMM6_20 }, { C_F }},

/* addf<.f> 0,b,limm    00111bbb00100011FBBB111110111110 */
{ "addf", 0x38230fbe, 0xf8ff0fff, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, RB, LIMM }, { C_F }},

/* addf<.f> a,b,limm    00111bbb00100011FBBB111110AAAAAA */
{ "addf", 0x38230f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, RB, LIMM }, { C_F }},

/* addf<.f> a,limm,limm 0011111000100011F111111110AAAAAA */
{ "addf", 0x3e237f80, 0xffff7fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, LIMMdup }, { C_F }},

/* addf<.f> a,limm,u6   0011111001100011F111uuuuuuAAAAAA */
{ "addf", 0x3e637000, 0xffff7000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, LIMM, UIMM6_20 }, { C_F }},

/* addf<.f> 0,limm,u6   0011111001100011F111uuuuuu111110 */
{ "addf", 0x3e63703e, 0xffff703f, ARC_OPCODE_ARC700, DPI, NPS400, { ZA, LIMM, UIMM6_20 }, { C_F }},

/* ldbit<.x2|.x4>.di<.cl> a,[b]       00010bbb00000000SBBB10011XAAAAAA */
{ "ldbit", 0x10000980, 0xf8ff8980, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, RB, BRAKETdup }, { C_NPS_LDBIT_X_1, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL1 }},

/* ldbit<.x2|.x4>.di<.cl> a,[b,s9]    00010bbbssssssssSBBB10011XAAAAAA */
{ "ldbit", 0x10000980, 0xf8000980, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, RB, SIMM9_8, BRAKETdup }, { C_NPS_LDBIT_X_1, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL1 }},

/* ldbit<.x2|.x4>.di<.cl> a,[limm]    0001011000000000011110011XAAAAAA */
{ "ldbit", 0x16007980, 0xfffff980, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, LIMM, BRAKETdup }, { C_NPS_LDBIT_X_1, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL1 }},

/* ldbit<.x2|.x4>.di<.cl> a,[limm,s9] 00010110ssssssssS11110011XAAAAAA */
{ "ldbit", 0x16007980, 0xff007980, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, LIMM, SIMM9_8, BRAKETdup }, { C_NPS_LDBIT_X_1, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL1 }},

/* ldbit<.x2|.x4>.di<.cl> a,[b,c]     00100bbb0011011X1BBBCCCCCCAAAAAA */
{ "ldbit", 0x20368000, 0xf83e8000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, RB, RC, BRAKETdup }, { C_NPS_LDBIT_X_2, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL2 }},

/* ldbit<.x2|.x4>.di<.cl> a,[b,limm]  00100bbb0011011X1BBB111110AAAAAA */
{ "ldbit", 0x20368f80, 0xf83e8fc0, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, RB, LIMM, BRAKETdup }, { C_NPS_LDBIT_X_2, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL2 }},

/* ldbit<.x2|.x4>.di<.cl> a,[limm,c]  001001100011011X1111CCCCCCAAAAAA */
{ "ldbit", 0x2636f000, 0xff3ef000, ARC_OPCODE_ARC700, DPI, NPS400, { RA, BRAKET, LIMM, RC, BRAKETdup }, { C_NPS_LDBIT_X_2, C_NPS_LDBIT_DI, C_NPS_LDBIT_CL2 }},

/* Pipeline Control Instructions.  */

/* schd<.rw|.rd> */
{ "schd", 0x3e6f7004, 0xffffff7f, ARC_OPCODE_ARC700, CONTROL, NPS400, { 0 }, { C_NPS_SCHD_RW }},

/* schd.wft.<.ie1|.ie2|.ie12> */
{ "schd", 0x3e6f7044, 0xfffffcff, ARC_OPCODE_ARC700, CONTROL, NPS400, { 0 }, { C_NPS_SCHD_TRIG, C_NPS_SCHD_IE }},

/* sync<.rd|.wr> */
{ "sync", 0x3e6f703f, 0xffffffbf, ARC_OPCODE_ARC700, CONTROL, NPS400, { 0 }, { C_NPS_SYNC }},

/* hwscd.off B */
{ "hwschd", 0x386f00bf, 0xf8ff8fff, ARC_OPCODE_ARC700, CONTROL, NPS400, { RB }, { C_NPS_HWS_OFF }},

/* hwscd.restore 0,C */
{ "hwschd", 0x3e6f7003, 0xfffff03f, ARC_OPCODE_ARC700, CONTROL, NPS400, { ZA, RC }, { C_NPS_HWS_RESTORE }},

/* Load / Store From (0x57f00000 + Offset) Instructions.  */

#define XLDST_LIKE(NAME,SUBOP2)                                          \
  { NAME, (0x58000000 | (SUBOP2 << 16)), 0xf81f0000, ARC_OPCODE_ARC700, MEMORY, NPS400, { NPS_R_DST, BRAKET, NPS_XLDST_UIMM16, BRAKETdup }, { 0 }},

XLDST_LIKE("xldb", 0x8)
XLDST_LIKE("xldw", 0x9)
XLDST_LIKE("xld", 0xa)
XLDST_LIKE("xstb", 0xc)
XLDST_LIKE("xstw", 0xd)
XLDST_LIKE("xst", 0xe)

/* BMU Instructions.  */

/* sbdalc dst, src1, type */
{ "sbdalc", 0x38500040, 0xf8ff09c0, ARC_OPCODE_ARC700, BMU, NPS400, { RA, RB, NPS_BD_TYPE }, { 0 }},

/* bdalc dst, [cm:src1], src1, src2 */
{ "bdalc", 0x38100000, 0xf8ff0000, ARC_OPCODE_ARC700, BMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup,  RC }, { 0 }},

/* bdalc dst, [cm:src1], src1, type, num_buff */
{ "bdalc", 0x38500800, 0xf8ff0800, ARC_OPCODE_ARC700, BMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_BD_TYPE, NPS_BMU_NUM }, { 0 }},

/* sbdfre 0, src1, src2 */
{ "sbdfre", 0x3817003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, RC }, { 0 }},

/* bdfre 0, [cm:src1], src1, src2 */
{ "bdfre", 0x3811003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, RC }, { 0 }},

/* bdfre 0, [cm:src1], src1, type, num_buff */
{ "bdfre", 0x3851083e, 0xf8ff083f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_BD_TYPE, NPS_BMU_NUM }, { 0 }},

/* bdfre 0, [cm:src1], src1, num_buff */
{ "bdfre", 0x3851003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_BMU_NUM }, { 0 }},

/* bdbgt 0, src1, src2 */
{ "bdbgt", 0x3818003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, RC }, { 0 }},

/* sidxalc dst, src1 */
{ "sidxalc", 0x385c0040, 0xf8ff0040, ARC_OPCODE_ARC700, BMU, NPS400, { RA, RB }, { 0 }},

/* idxalc dst, [cm:src1], src1, src2 */
{ "idxalc", 0x381c0000, 0xf8ff0000, ARC_OPCODE_ARC700, BMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, RC }, { 0 }},

/* idxalc dst, [cm:src1], src1, num_idx */
{ "idxalc", 0x385c0800, 0xf8ff0800, ARC_OPCODE_ARC700, BMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_BMU_NUM }, { 0 }},

/* sidxfre 0, src1, src2 */
{ "sidxfre", 0x381d003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, RC }, { 0 }},

/* idxfre 0, [cm:src1], src1, src2 */
{ "idxfre", 0x381e003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, RC }, { 0 }},

/* idxfre 0, [cm:src1], src1, num_buff */
{ "idxfre", 0x385e003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_BMU_NUM }, { 0 }},

/* idxbgt 0, src1, src2 */
{ "idxbgt", 0x3819003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, RC }, { 0 }},

/* efabgt 0, limm, src2 */
{ "efabgt", 0x3e0d703e, 0xfffff03f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, LIMM, RC }, { 0 }},

/* efabgt 0, src1, limm */
{ "efabgt", 0x380d0fbe, 0xf8ff80ff, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, LIMM }, { 0 }},

/* efabgt 0, src1, src2 */
{ "efabgt", 0x380d003e, 0xf8ff003f, ARC_OPCODE_ARC700, BMU, NPS400, { ZA, RB, RC }, { 0 }},

/* efabgt dst, limm, src2 */
{ "efabgt", 0x3e0d7000, 0xfffff000, ARC_OPCODE_ARC700, BMU, NPS400, { RA, LIMM, RC }, { 0 }},

/* efabgt dst, src1, limm */
{ "efabgt", 0x380d0f80, 0xf8ff0fc0, ARC_OPCODE_ARC700, BMU, NPS400, { RA, RB, LIMM }, { 0 }},

/* efabgt dst, src1, src2 */
{ "efabgt", 0x380d0000, 0xf8ff8000, ARC_OPCODE_ARC700, BMU, NPS400, { RA, RB, RC }, { 0 }},

/* PMU Instructions. */

/* jobget<.cl> 0, [cjid:src1] */
{ "jobget", 0x3e2f7020, 0xfffff03f, ARC_OPCODE_ARC700, PMU, NPS400, { ZA, BRAKET, NPS_CJID, COLON, RC, BRAKET }, { 0 }},

{ "jobget", 0x3e2f7021, 0xfffff03f, ARC_OPCODE_ARC700, PMU, NPS400, { ZA, BRAKET, NPS_CJID, COLON, RC, BRAKET }, { C_NPS_CL }},

/* jobdn 0, [cjid:src1], src1, src2 */
{ "jobdn", 0x3812003e, 0xf8ff803f, ARC_OPCODE_ARC700, PMU, NPS400, { ZA, BRAKET, NPS_CJID, COLON, RB, BRAKETdup, RBdup, RC }, { 0 }},

/* jobdn 0, [cjid:src1], src1, nxt_dst */
{ "jobdn", 0x3852003e, 0xf8ff803f, ARC_OPCODE_ARC700, PMU, NPS400, { ZA, BRAKET, NPS_CJID, COLON, RB, BRAKETdup, RBdup, NPS_PMU_NXT_DST }, { 0 }},

/* sjobalc dst, src1 */
{ "sjobalc", 0x385f0040, 0xf8ff8fc0, ARC_OPCODE_ARC700, PMU, NPS400, { RA, RB }, { 0 }},

/* jobalc dst, [cm:src1], src1, num_job */
{ "jobalc", 0x385f0800, 0xf8ff8800, ARC_OPCODE_ARC700, PMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, NPS_PMU_NUM_JOB }, { 0 }},

/* jobalc dst, [cm:src1], src1, src2 */
{ "jobalc", 0x381f0000, 0xf8ff8000, ARC_OPCODE_ARC700, PMU, NPS400, { RA, BRAKET, NPS_CM, COLON, RB, BRAKETdup, RBdup, RC }, { 0 }},

/* jobbgt dst, src1, src2 */
{ "jobbgt", 0x381a0000, 0xf8ff0000, ARC_OPCODE_ARC700, PMU, NPS400, { RA, RB, RC }, { 0 }},

/* cnljob 0 */
{ "cnljob", 0x3e6f70ff, 0xffffffff, ARC_OPCODE_ARC700, PMU, NPS400, { ZA }, { 0 }},

/* qseq dst, [src1] */
{ "qseq", 0x386f0028, 0xf8ff803f, ARC_OPCODE_ARC700, PMU, NPS400, { RB, BRAKET, RC, BRAKETdup }, { 0 }},
