/* Definitions for decoding the ft32 opcode table.
   Copyright (C) 2013-2016 Free Software Foundation, Inc.
   Contributed by FTDI (support@ftdichip.com)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA
02110-1301, USA.  */

typedef struct ft32_opc_info_t
{
  const char *name;
  int dw;
  unsigned int mask;
  unsigned int bits;
  int fields;
} ft32_opc_info_t;

#define FT32_PAT_ALUOP    0x08
#define FT32_PAT_LDA      0x18
#define FT32_PAT_TOCI     0x01
#define FT32_PAT_CMPOP    0x0b
#define FT32_PAT_STA      0x17
#define FT32_PAT_EXA      0x07
#define FT32_PAT_LDK      0x0c
#define FT32_PAT_FFUOP    0x1e
#define FT32_PAT_LDI      0x15
#define FT32_PAT_STI      0x16
#define FT32_PAT_EXI      0x1d
#define FT32_PAT_POP      0x11
#define FT32_PAT_LPM      0x0d
#define FT32_PAT_LINK     0x12
#define FT32_PAT_TOC      0x00
#define FT32_PAT_PUSH     0x10
#define FT32_PAT_RETURN   0x14
#define FT32_PAT_UNLINK   0x13
#define FT32_PAT_LPMI     0x19

#define FT32_FLD_CBCRCV (1 << 0)
#define FT32_FLD_INT (1 << 1)
#define FT32_FLD_INT_BIT 26
#define FT32_FLD_INT_SIZ 1
#define FT32_FLD_DW (1 << 2)
#define FT32_FLD_DW_BIT 25
#define FT32_FLD_DW_SIZ 2
#define FT32_FLD_CB (1 << 3)
#define FT32_FLD_CB_BIT 22
#define FT32_FLD_CB_SIZ 5
#define FT32_FLD_R_D (1 << 4)
#define FT32_FLD_R_D_BIT 20
#define FT32_FLD_R_D_SIZ 5
#define FT32_FLD_CR (1 << 5)
#define FT32_FLD_CR_BIT 20
#define FT32_FLD_CR_SIZ 2
#define FT32_FLD_CV (1 << 6)
#define FT32_FLD_CV_BIT 19
#define FT32_FLD_CV_SIZ 1
#define FT32_FLD_BT (1 << 7)
#define FT32_FLD_BT_BIT 18
#define FT32_FLD_BT_SIZ 1
#define FT32_FLD_R_1 (1 << 8)
#define FT32_FLD_R_1_BIT 15
#define FT32_FLD_R_1_SIZ 5
#define FT32_FLD_RIMM (1 << 9)
#define FT32_FLD_RIMM_BIT 4
#define FT32_FLD_RIMM_SIZ 11
#define FT32_FLD_R_2 (1 << 10)
#define FT32_FLD_R_2_BIT 4
#define FT32_FLD_R_2_SIZ 5
#define FT32_FLD_K20 (1 << 11)
#define FT32_FLD_K20_BIT 0
#define FT32_FLD_K20_SIZ 20
#define FT32_FLD_PA (1 << 12)
#define FT32_FLD_PA_BIT 0
#define FT32_FLD_PA_SIZ 18
#define FT32_FLD_AA (1 << 13)
#define FT32_FLD_AA_BIT 0
#define FT32_FLD_AA_SIZ 17
#define FT32_FLD_K16 (1 << 14)
#define FT32_FLD_K16_BIT 0
#define FT32_FLD_K16_SIZ 16
#define FT32_FLD_K8 (1 << 15)
#define FT32_FLD_K8_BIT 0
#define FT32_FLD_K8_SIZ 8
#define FT32_FLD_AL (1 << 16)
#define FT32_FLD_AL_BIT 0
#define FT32_FLD_AL_SIZ 4

#define FT32_IS_CALL(inst)   (((inst) & 0xfffc0000) == 0x00340000)
#define FT32_IS_PUSH(inst)   (((inst) & 0xfff00000) == 0x84000000)
#define FT32_PUSH_REG(inst)  (((inst) >> 15) & 0x1f)
#define FT32_IS_LINK(inst)   (((inst) & 0xffff0000) == 0x95d00000)
#define FT32_LINK_SIZE(inst) ((inst) & 0xffff)

#define FT32_FLD_R_D_POST (1 << 17)
#define FT32_FLD_R_1_POST (1 << 18)
