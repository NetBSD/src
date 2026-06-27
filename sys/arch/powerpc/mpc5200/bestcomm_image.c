/******************************************************************************
*
* Copyright (c) 2004 Freescale Semiconductor, Inc.
* 
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
******************************************************************************/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bestcomm_image.c,v 1.1 2026/06/27 13:28:34 rkujawa Exp $");

#include <sys/param.h>

#include <powerpc/mpc5200/bestcomm_image.h>

const uint32_t bestcomm_image_bytes = 0x00001500;  /* Number of bytes in image */

const uint32_t bestcomm_image_ntasks = 0x00000010;  /* Number of tasks in image */

const uint32_t bestcomm_image_entry = 0x00000000;  /* Offset to Entry section */

const uint32_t bestcomm_image[] = {

/* SmartDMA image contains 5376 bytes (578 b unused) */

/* Task0(TASK_PCI_TX): Start of Entry -> 0xf0008000 */
0x00000200,  /* Task 0 Descriptor Table */
0x00000238,
0x00000700,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001000,  /* Task 0 context save space */
0x00000000, 
/* Task1(TASK_PCI_RX): Start of Entry -> 0xf0008020 */
0x0000023c,  /* Task 0 Descriptor Table */
0x00000268,
0x00000780,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x00001050,  /* Task 0 context save space */
0x00000000, 
/* Task2(TASK_FEC_TX): Start of Entry -> 0xf0008040 */
0x0000026c,  /* Task 0 Descriptor Table */
0x000002f8,
0x00000800,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000010a0,  /* Task 0 context save space */
0x00000000, 
/* Task3(TASK_FEC_RX): Start of Entry -> 0xf0008060 */
0x000002fc,  /* Task 0 Descriptor Table */
0x00000358,
0x00000880,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000010f0,  /* Task 0 context save space */
0x00000000, 
/* Task4(TASK_LPC): Start of Entry -> 0xf0008080 */
0x0000035c,  /* Task 0 Descriptor Table */
0x0000038c,
0x00000900,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x00001140,  /* Task 0 context save space */
0x00000000, 
/* Task5(TASK_ATA): Start of Entry -> 0xf00080a0 */
0x00000390,  /* Task 0 Descriptor Table */
0x000003c4,
0x00000980,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001190,  /* Task 0 context save space */
0x00000000, 
/* Task6(TASK_CRC16_DP_0): Start of Entry -> 0xf00080c0 */
0x000003c8,  /* Task 0 Descriptor Table */
0x0000040c,
0x00000a00,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000011e0,  /* Task 0 context save space */
0x00000000, 
/* Task7(TASK_CRC16_DP_1): Start of Entry -> 0xf00080e0 */
0x00000410,  /* Task 0 Descriptor Table */
0x00000454,
0x00000a80,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001230,  /* Task 0 context save space */
0x00000000, 
/* Task8(TASK_GEN_DP_0): Start of Entry -> 0xf0008100 */
0x00000458,  /* Task 0 Descriptor Table */
0x00000488,
0x00000b00,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x00001280,  /* Task 0 context save space */
0x00000000, 
/* Task9(TASK_GEN_DP_1): Start of Entry -> 0xf0008120 */
0x0000048c,  /* Task 0 Descriptor Table */
0x000004bc,
0x00000b80,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x000012d0,  /* Task 0 context save space */
0x00000000, 
/* Task10(TASK_GEN_DP_2): Start of Entry -> 0xf0008140 */
0x000004c0,  /* Task 0 Descriptor Table */
0x000004f0,
0x00000c00,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x00001320,  /* Task 0 context save space */
0x00000000, 
/* Task11(TASK_GEN_DP_3): Start of Entry -> 0xf0008160 */
0x000004f4,  /* Task 0 Descriptor Table */
0x00000524,
0x00000c80,  /* Task 0 Variable Table */
0x00000027, /* No FDT */
0x00000000, 
0x00000000, 
0x00001370,  /* Task 0 context save space */
0x00000000, 
/* Task12(TASK_GEN_TX_BD): Start of Entry -> 0xf0008180 */
0x00000528,  /* Task 0 Descriptor Table */
0x00000560,
0x00000d00,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000013c0,  /* Task 0 context save space */
0x00000000, 
/* Task13(TASK_GEN_RX_BD): Start of Entry -> 0xf00081a0 */
0x00000564,  /* Task 0 Descriptor Table */
0x00000594,
0x00000d80,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001410,  /* Task 0 context save space */
0x00000000, 
/* Task14(TASK_GEN_DP_BD_0): Start of Entry -> 0xf00081c0 */
0x00000598,  /* Task 0 Descriptor Table */
0x000005cc,
0x00000e00,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x00001460,  /* Task 0 context save space */
0x00000000, 
/* Task15(TASK_GEN_DP_BD_1): Start of Entry -> 0xf00081e0 */
0x000005d0,  /* Task 0 Descriptor Table */
0x00000604,
0x00000e80,  /* Task 0 Variable Table */
0x00000f27,	/* Task 0 Function Descriptor Table & Flags */
0x00000000, 
0x00000000, 
0x000014b0,  /* Task 0 context save space */
0x00000000, 

/* Task0(TASK_PCI_TX): Start of TDT -> 0xf0008200 */
0xc080601b, /* 0000(../LIB_incl/hdplx.sc:167):  LCDEXT: idx0 = var1, idx1 = var0; ; idx0 += inc3, idx1 += inc3 */
0x82190292, /* 0004(../LIB_incl/hdplx.sc:177):  LCD: idx2 = var4; idx2 >= var10; idx2 += inc2 */
0x1004c018, /* 0008(../LIB_incl/hdplx.sc:179):    DRD1A: *idx0 = var3; FN=0 MORE init=0 WS=2 RS=0 */
0x8381a288, /* 000C(../LIB_incl/hdplx.sc:183):    LCD: idx3 = var7, idx4 = var3; idx4 > var10; idx3 += inc1, idx4 += inc0 */
0x011ec798, /* 0010(../LIB_incl/hdplx.sc:200):      DRD1A: *idx1 = *idx3; FN=0 init=8 WS=3 RS=3 */
0x999a001b, /* 0014(../LIB_incl/hdplx.sc:249):    LCD: idx3 = idx3, idx4 = idx4; idx3 once var0; idx3 += inc3, idx4 += inc3 */
0x00001f18, /* 0018(../LIB_incl/hdplx.sc:258):      DRD1A: var7 = idx3; FN=0 init=0 WS=0 RS=0 */
0x850102e3, /* 001C(../LIB_incl/hdplx.sc:281):    LCD: idx3 = var10, idx4 = var2; idx3 != var11; idx3 += inc4, idx4 += inc3 */
0x60080002, /* 0020(../LIB_incl/hdplx.sc:281):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=0 RS=1 */
0x08ccfd0b, /* 0024(../LIB_incl/hdplx.sc:281):      DRD2B1: idx3 = EU3(); EU3(*idx4,var11)  */
0x9a19801b, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx3 = idx4; idx3 once var0; idx3 += inc3 */
0x0002cc58, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: *idx3 = var11; FN=0 init=0 WS=1 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
0x8018001b, /* 0034(../LIB_incl/hdplx.sc:285):  LCD: idx0 = var0; idx0 once var0; idx0 += inc3 */
0x040001f8, /* 0038(../LIB_incl/hdplx.sc:285):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task1(TASK_PCI_RX): Start of TDT -> 0xf000823c */
0xc000e01b, /* 0000(../LIB_incl/hdplx.sc:167):  LCDEXT: idx0 = var0, idx1 = var1; ; idx0 += inc3, idx1 += inc3 */
0x81990212, /* 0004(../LIB_incl/hdplx.sc:177):  LCD: idx2 = var3; idx2 >= var8; idx2 += inc2 */
0x1004c010, /* 0008(../LIB_incl/hdplx.sc:179):    DRD1A: *idx0 = var2; FN=0 MORE init=0 WS=2 RS=0 */
0x83012208, /* 000C(../LIB_incl/hdplx.sc:186):    LCD: idx3 = var6, idx4 = var2; idx4 > var8; idx3 += inc1, idx4 += inc0 */
0x00fecf88, /* 0010(../LIB_incl/hdplx.sc:200):      DRD1A: *idx3 = *idx1; FN=0 init=7 WS=3 RS=3 */
0x999a001b, /* 0014(../LIB_incl/hdplx.sc:252):    LCD: idx3 = idx3, idx4 = idx4; idx3 once var0; idx3 += inc3, idx4 += inc3 */
0x00001b18, /* 0018(../LIB_incl/hdplx.sc:261):      DRD1A: var6 = idx3; FN=0 init=0 WS=0 RS=0 */
0x8019801b, /* 001C(../LIB_incl/hdplx.sc:281):    LCD: idx3 = var0; idx3 once var0; idx3 += inc3 */
0x040001f8, /* 0020(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0024(:0):    NOP */
0x8018001b, /* 0028(../LIB_incl/hdplx.sc:285):  LCD: idx0 = var0; idx0 once var0; idx0 += inc3 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:285):    DRD1A: FN=0 INT init=0 WS=0 RS=0 */
/* Task2(TASK_FEC_TX): Start of TDT -> 0xf000826c */
0x8018001b, /* 0000(../LIB_incl/bd_hdplx.sc:303):  LCD: idx0 = var0; idx0 <= var0; idx0 += inc3 */
0x60000005, /* 0004(../LIB_incl/bd_hdplx.sc:307):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x01ccfc0d, /* 0008(../LIB_incl/bd_hdplx.sc:307):    DRD2B1: var7 = EU3(); EU3(*idx0,var13)  */
0x8082a123, /* 000C(../LIB_incl/bd_hdplx.sc:316):  LCD: idx0 = var1, idx1 = var5; idx1 <= var4; idx0 += inc4, idx1 += inc3 */
0x10801418, /* 0010(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var5 = var3; FN=0 MORE init=4 WS=0 RS=0 */
0xf88103a4, /* 0014(../LIB_incl/bd_hdplx.sc:347):    LCDEXT: idx2 = *idx1, idx3 = var2; idx2 < var14; idx2 += inc4, idx3 += inc4 */
0x801a6024, /* 0018(../LIB_incl/bd_hdplx.sc:351):    LCD: idx4 = var0; ; idx4 += inc4 */
0x10001708, /* 001C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var5 = idx1; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0020(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0cccfccf, /* 0024(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx3 = EU3(); EU3(*idx3,var15)  */
0x991a002c, /* 0028(../LIB_incl/bd_hdplx.sc:373):    LCD: idx2 = idx2, idx3 = idx4; idx2 once var0; idx2 += inc5, idx3 += inc4 */
0x70000002, /* 002C(../LIB_incl/bd_hdplx.sc:377):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT MORE init=0 WS=0 RS=0 */
0x024cfc4d, /* 0030(../LIB_incl/bd_hdplx.sc:377):      DRD2B1: var9 = EU3(); EU3(*idx1,var13)  */
0x60000003, /* 0034(../LIB_incl/bd_hdplx.sc:378):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=3 EXT init=0 WS=0 RS=0 */
0x0cccf247, /* 0038(../LIB_incl/bd_hdplx.sc:378):      DRD2B1: *idx3 = EU3(); EU3(var9,var7)  */
0x80004000, /* 003C(../LIB_incl/bd_hdplx.sc:390):    LCDEXT: idx2 = 0x00000000; ; */
0xb8c80029, /* 0040(../LIB_incl/bd_hdplx.sc:390):    LCD: idx3 = *(idx1 + var0000001a); idx3 once var0; idx3 += inc5 */
0x70000002, /* 0044(../LIB_incl/bd_hdplx.sc:401):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT MORE init=0 WS=0 RS=0 */
0x088cf8d1, /* 0048(../LIB_incl/bd_hdplx.sc:401):      DRD2B1: idx2 = EU3(); EU3(idx3,var17)  */
0x00002f10, /* 004C(../LIB_incl/bd_hdplx.sc:404):      DRD1A: var11 = idx2; FN=0 init=0 WS=0 RS=0 */
0x99198432, /* 0050(../LIB_incl/bd_hdplx.sc:411):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var16; idx2 += inc6, idx3 += inc2 */
0x008ac398, /* 0054(../LIB_incl/bd_hdplx.sc:428):      DRD1A: *idx0 = *idx3; FN=0 init=4 WS=1 RS=1 */
0x80004000, /* 0058(../LIB_incl/bd_hdplx.sc:434):    LCDEXT: idx2 = 0x00000000; ; */
0x9999802d, /* 005C(../LIB_incl/bd_hdplx.sc:439):    LCD: idx3 = idx3; idx3 once var0; idx3 += inc5 */
0x70000002, /* 0060(../LIB_incl/bd_hdplx.sc:446):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT MORE init=0 WS=0 RS=0 */
0x048cfc53, /* 0064(../LIB_incl/bd_hdplx.sc:446):      DRD2B1: var18 = EU3(); EU3(*idx1,var19)  */
0x60000008, /* 0068(../LIB_incl/bd_hdplx.sc:450):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=8 EXT init=0 WS=0 RS=0 */
0x088cf48b, /* 006C(../LIB_incl/bd_hdplx.sc:450):      DRD2B1: idx2 = EU3(); EU3(var18,var11)  */
0x99198481, /* 0070(../LIB_incl/bd_hdplx.sc:461):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var18; idx2 += inc0, idx3 += inc1 */
0x009ec398, /* 0074(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx0 = *idx3; FN=0 init=4 WS=3 RS=3 */
0x991983b2, /* 0078(../LIB_incl/bd_hdplx.sc:513):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var14; idx2 += inc6, idx3 += inc2 */
0x088ac398, /* 007C(../LIB_incl/bd_hdplx.sc:530):      DRD1A: *idx0 = *idx3; FN=0 TFD init=4 WS=1 RS=1 */
0x9919002d, /* 0080(../LIB_incl/bd_hdplx.sc:554):    LCD: idx2 = idx2; idx2 once var0; idx2 += inc5 */
0x60000005, /* 0084(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf88e, /* 0088(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx1 = EU3(); EU3(idx2,var14)  */
0x000001f8, /* 008C(:0):    NOP */
/* Task3(TASK_FEC_RX): Start of TDT -> 0xf00082fc */
0x808220e3, /* 0000(../LIB_incl/bd_hdplx.sc:313):  LCD: idx0 = var1, idx1 = var4; idx1 <= var3; idx0 += inc4, idx1 += inc3 */
0x10601010, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var4 = var2; FN=0 MORE init=3 WS=0 RS=0 */
0xb8800264, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx2 = *idx1, idx3 = var0; idx2 < var9; idx2 += inc4, idx3 += inc4 */
0x10001308, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var4 = idx1; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0cccfcca, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx3 = EU3(); EU3(*idx3,var10)  */
0x80004000, /* 0018(../LIB_incl/bd_hdplx.sc:393):    LCDEXT: idx2 = 0x00000000; ; */
0xb8c58029, /* 001C(../LIB_incl/bd_hdplx.sc:393):    LCD: idx3 = *(idx1 + var00000015); idx3 once var0; idx3 += inc5 */
0x60000002, /* 0020(../LIB_incl/bd_hdplx.sc:398):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=0 RS=0 */
0x088cf8cc, /* 0024(../LIB_incl/bd_hdplx.sc:398):      DRD2B1: idx2 = EU3(); EU3(idx3,var12)  */
0x991982f2, /* 0028(../LIB_incl/bd_hdplx.sc:414):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var11; idx2 += inc6, idx3 += inc2 */
0x006acf80, /* 002C(../LIB_incl/bd_hdplx.sc:428):      DRD1A: *idx3 = *idx0; FN=0 init=3 WS=1 RS=1 */
0x80004000, /* 0030(../LIB_incl/bd_hdplx.sc:437):    LCDEXT: idx2 = 0x00000000; ; */
0x9999802d, /* 0034(../LIB_incl/bd_hdplx.sc:439):    LCD: idx3 = idx3; idx3 once var0; idx3 += inc5 */
0x70000002, /* 0038(../LIB_incl/bd_hdplx.sc:446):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT MORE init=0 WS=0 RS=0 */
0x034cfc4e, /* 003C(../LIB_incl/bd_hdplx.sc:446):      DRD2B1: var13 = EU3(); EU3(*idx1,var14)  */
0x00008868, /* 0040(../LIB_incl/bd_hdplx.sc:448):      DRD1A: idx2 = var13; FN=0 init=0 WS=0 RS=0 */
0x99198341, /* 0044(../LIB_incl/bd_hdplx.sc:464):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var13; idx2 += inc0, idx3 += inc1 */
0x007ecf80, /* 0048(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx3 = *idx0; FN=0 init=3 WS=3 RS=3 */
0x99198272, /* 004C(../LIB_incl/bd_hdplx.sc:516):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var9; idx2 += inc6, idx3 += inc2 */
0x046acf80, /* 0050(../LIB_incl/bd_hdplx.sc:530):      DRD1A: *idx3 = *idx0; FN=0 INT init=3 WS=1 RS=1 */
0x9819002d, /* 0054(../LIB_incl/bd_hdplx.sc:550):    LCD: idx2 = idx0; idx2 once var0; idx2 += inc5 */
0x0060c790, /* 0058(../LIB_incl/bd_hdplx.sc:551):      DRD1A: *idx1 = *idx2; FN=0 init=3 WS=0 RS=0 */
0x000001f8, /* 005C(:0):    NOP */
/* Task4(TASK_LPC): Start of TDT -> 0xf000835c */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x8018803f, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx1 = var0; idx1 once var0; idx1 += inc7 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
/* Task5(TASK_ATA): Start of TDT -> 0xf0008390 */
0x8198009b, /* 0000(../LIB_incl/bd_hdplx.sc:321):  LCD: idx0 = var3; idx0 <= var2; idx0 += inc3 */
0x13e00c08, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var3 = var1; FN=0 MORE init=31 WS=0 RS=0 */
0xb8000264, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx1 = *idx0, idx2 = var0; idx1 < var9; idx1 += inc4, idx2 += inc4 */
0x10000f00, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var3 = idx0; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0c8cfc8a, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx2 = EU3(); EU3(*idx2,var10)  */
0xd8988240, /* 0018(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx1 = idx1; idx1 > var9; idx1 += inc0 */
0xf845e011, /* 001C(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx2 = *(idx0 + var00000015); ; idx2 += inc2 */
0xb845e00a, /* 0020(../LIB_incl/bd_hdplx.sc:472):    LCD: idx3 = *(idx0 + var00000019); ; idx3 += inc1 */
0x0bfecf90, /* 0024(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x9898802d, /* 0028(../LIB_incl/bd_hdplx.sc:554):    LCD: idx1 = idx1; idx1 once var0; idx1 += inc5 */
0x64000005, /* 002C(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf849, /* 0030(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx0 = EU3(); EU3(idx1,var9)  */
0x000001f8, /* 0034(:0):    NOP */
/* Task6(TASK_CRC16_DP_0): Start of TDT -> 0xf00083c8 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0x70000000, /* 0004(../LIB_incl/hdplx.sc:179):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=0 EXT MORE init=0 WS=0 RS=0 */
0x2c87c7df, /* 0008(../LIB_incl/hdplx.sc:179):    DRD2B2: EU3(var8)  */
0xc2826019, /* 000C(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198240, /* 0010(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var9; idx3 += inc0 */
0x63fe000c, /* 0014(../LIB_incl/hdplx.sc:200):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0018(../LIB_incl/hdplx.sc:200):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8990019, /* 001C(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0020(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x63fe000c, /* 0024(../LIB_incl/hdplx.sc:218):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0028(../LIB_incl/hdplx.sc:218):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8996022, /* 002C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0030(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x6bea000c, /* 0034(../LIB_incl/hdplx.sc:241):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 TFD EXT init=31 WS=1 RS=1 */
0x0c8cfc5f, /* 0038(../LIB_incl/hdplx.sc:241):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0x9918803f, /* 003C(../LIB_incl/hdplx.sc:281):    LCD: idx1 = idx2; idx1 once var0; idx1 += inc7 */
0x0404c599, /* 0040(../LIB_incl/hdplx.sc:281):      DRD1A: *idx1 = EU3(); FN=1 INT init=0 WS=2 RS=0 */
0x000001f8, /* 0044(:0):    NOP */
/* Task7(TASK_CRC16_DP_1): Start of TDT -> 0xf0008410 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0x70000000, /* 0004(../LIB_incl/hdplx.sc:179):    DRD2A: EU0=0 EU1=0 EU2=0 EU3=0 EXT MORE init=0 WS=0 RS=0 */
0x2c87c7df, /* 0008(../LIB_incl/hdplx.sc:179):    DRD2B2: EU3(var8)  */
0xc2826019, /* 000C(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198240, /* 0010(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var9; idx3 += inc0 */
0x63fe000c, /* 0014(../LIB_incl/hdplx.sc:200):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0018(../LIB_incl/hdplx.sc:200):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8990019, /* 001C(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0020(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x63fe000c, /* 0024(../LIB_incl/hdplx.sc:218):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 EXT init=31 WS=3 RS=3 */
0x0c8cfc5f, /* 0028(../LIB_incl/hdplx.sc:218):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0xd8996022, /* 002C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0030(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x6bea000c, /* 0034(../LIB_incl/hdplx.sc:241):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=12 TFD EXT init=31 WS=1 RS=1 */
0x0c8cfc5f, /* 0038(../LIB_incl/hdplx.sc:241):      DRD2B1: *idx2 = EU3(); EU3(*idx1)  */
0x9918803f, /* 003C(../LIB_incl/hdplx.sc:281):    LCD: idx1 = idx2; idx1 once var0; idx1 += inc7 */
0x0404c599, /* 0040(../LIB_incl/hdplx.sc:281):      DRD1A: *idx1 = EU3(); FN=1 INT init=0 WS=2 RS=0 */
0x000001f8, /* 0044(:0):    NOP */
/* Task8(TASK_GEN_DP_0): Start of TDT -> 0xf0008458 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x8018803f, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx1 = var0; idx1 once var0; idx1 += inc7 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
/* Task9(TASK_GEN_DP_1): Start of TDT -> 0xf000848c */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x8018803f, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx1 = var0; idx1 once var0; idx1 += inc7 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
/* Task10(TASK_GEN_DP_2): Start of TDT -> 0xf00084c0 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x8018803f, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx1 = var0; idx1 once var0; idx1 += inc7 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
/* Task11(TASK_GEN_DP_3): Start of TDT -> 0xf00084f4 */
0x809801ed, /* 0000(../LIB_incl/hdplx.sc:177):  LCD: idx0 = var1; idx0 >= var7; idx0 += inc5 */
0xc2826019, /* 0004(../LIB_incl/hdplx.sc:183):    LCDEXT: idx1 = var5, idx2 = var4; ; idx1 += inc3, idx2 += inc1 */
0x80198200, /* 0008(../LIB_incl/hdplx.sc:188):    LCD: idx3 = var0; idx3 > var8; idx3 += inc0 */
0x03fecb88, /* 000C(../LIB_incl/hdplx.sc:200):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8990019, /* 0010(../LIB_incl/hdplx.sc:208):    LCDEXT: idx1 = idx1, idx2 = idx2; idx1 once var0; idx1 += inc3, idx2 += inc1 */
0x9999e000, /* 0014(../LIB_incl/hdplx.sc:211):    LCD: idx3 = idx3; ; idx3 += inc0 */
0x03fecb88, /* 0018(../LIB_incl/hdplx.sc:218):      DRD1A: *idx2 = *idx1; FN=0 init=31 WS=3 RS=3 */
0xd8996022, /* 001C(../LIB_incl/hdplx.sc:224):    LCDEXT: idx1 = idx1, idx2 = idx2; ; idx1 += inc4, idx2 += inc2 */
0x999981f6, /* 0020(../LIB_incl/hdplx.sc:229):    LCD: idx3 = idx3; idx3 > var7; idx3 += inc6 */
0x0beacb88, /* 0024(../LIB_incl/hdplx.sc:241):      DRD1A: *idx2 = *idx1; FN=0 TFD init=31 WS=1 RS=1 */
0x8018803f, /* 0028(../LIB_incl/hdplx.sc:281):    LCD: idx1 = var0; idx1 once var0; idx1 += inc7 */
0x040001f8, /* 002C(../LIB_incl/hdplx.sc:281):      DRD1A: FN=0 INT init=0 WS=0 RS=0 */
0x000001f8, /* 0030(:0):    NOP */
/* Task12(TASK_GEN_TX_BD): Start of TDT -> 0xf0008528 */
0x800220e3, /* 0000(../LIB_incl/bd_hdplx.sc:316):  LCD: idx0 = var0, idx1 = var4; idx1 <= var3; idx0 += inc4, idx1 += inc3 */
0x13e01010, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var4 = var2; FN=0 MORE init=31 WS=0 RS=0 */
0xb8808264, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx2 = *idx1, idx3 = var1; idx2 < var9; idx2 += inc4, idx3 += inc4 */
0x10001308, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var4 = idx1; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0cccfcca, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx3 = EU3(); EU3(*idx3,var10)  */
0xd9190300, /* 0018(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx2 = idx2; idx2 > var12; idx2 += inc0 */
0xb8c5e009, /* 001C(../LIB_incl/bd_hdplx.sc:469):    LCD: idx3 = *(idx1 + var00000015); ; idx3 += inc1 */
0x03fec398, /* 0020(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx0 = *idx3; FN=0 init=31 WS=3 RS=3 */
0x9919826a, /* 0024(../LIB_incl/bd_hdplx.sc:513):    LCD: idx2 = idx2, idx3 = idx3; idx2 > var9; idx2 += inc5, idx3 += inc2 */
0x0feac398, /* 0028(../LIB_incl/bd_hdplx.sc:530):      DRD1A: *idx0 = *idx3; FN=0 TFD INT init=31 WS=1 RS=1 */
0x99190036, /* 002C(../LIB_incl/bd_hdplx.sc:554):    LCD: idx2 = idx2; idx2 once var0; idx2 += inc6 */
0x60000005, /* 0030(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf889, /* 0034(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx1 = EU3(); EU3(idx2,var9)  */
0x000001f8, /* 0038(:0):    NOP */
/* Task13(TASK_GEN_RX_BD): Start of TDT -> 0xf0008564 */
0x808220da, /* 0000(../LIB_incl/bd_hdplx.sc:313):  LCD: idx0 = var1, idx1 = var4; idx1 <= var3; idx0 += inc3, idx1 += inc2 */
0x13e01010, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var4 = var2; FN=0 MORE init=31 WS=0 RS=0 */
0xb880025b, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx2 = *idx1, idx3 = var0; idx2 < var9; idx2 += inc3, idx3 += inc3 */
0x10001308, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var4 = idx1; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0cccfcca, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx3 = EU3(); EU3(*idx3,var10)  */
0xd9190240, /* 0018(../LIB_incl/bd_hdplx.sc:472):    LCDEXT: idx2 = idx2; idx2 > var9; idx2 += inc0 */
0xb8c5e009, /* 001C(../LIB_incl/bd_hdplx.sc:472):    LCD: idx3 = *(idx1 + var00000015); ; idx3 += inc1 */
0x07fecf80, /* 0020(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx3 = *idx0; FN=0 INT init=31 WS=3 RS=3 */
0x99190024, /* 0024(../LIB_incl/bd_hdplx.sc:554):    LCD: idx2 = idx2; idx2 once var0; idx2 += inc4 */
0x60000005, /* 0028(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 EXT init=0 WS=0 RS=0 */
0x0c4cf889, /* 002C(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx1 = EU3(); EU3(idx2,var9)  */
0x000001f8, /* 0030(:0):    NOP */
/* Task14(TASK_GEN_DP_BD_0): Start of TDT -> 0xf0008598 */
0x8198009b, /* 0000(../LIB_incl/bd_hdplx.sc:321):  LCD: idx0 = var3; idx0 <= var2; idx0 += inc3 */
0x13e00c08, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var3 = var1; FN=0 MORE init=31 WS=0 RS=0 */
0xb8000264, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx1 = *idx0, idx2 = var0; idx1 < var9; idx1 += inc4, idx2 += inc4 */
0x10000f00, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var3 = idx0; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0c8cfc8a, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx2 = EU3(); EU3(*idx2,var10)  */
0xd8988240, /* 0018(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx1 = idx1; idx1 > var9; idx1 += inc0 */
0xf845e011, /* 001C(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx2 = *(idx0 + var00000015); ; idx2 += inc2 */
0xb845e00a, /* 0020(../LIB_incl/bd_hdplx.sc:472):    LCD: idx3 = *(idx0 + var00000019); ; idx3 += inc1 */
0x0bfecf90, /* 0024(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x9898802d, /* 0028(../LIB_incl/bd_hdplx.sc:554):    LCD: idx1 = idx1; idx1 once var0; idx1 += inc5 */
0x64000005, /* 002C(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf849, /* 0030(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx0 = EU3(); EU3(idx1,var9)  */
0x000001f8, /* 0034(:0):    NOP */
/* Task15(TASK_GEN_DP_BD_1): Start of TDT -> 0xf00085d0 */
0x8198009b, /* 0000(../LIB_incl/bd_hdplx.sc:321):  LCD: idx0 = var3; idx0 <= var2; idx0 += inc3 */
0x13e00c08, /* 0004(../LIB_incl/bd_hdplx.sc:343):    DRD1A: var3 = var1; FN=0 MORE init=31 WS=0 RS=0 */
0xb8000264, /* 0008(../LIB_incl/bd_hdplx.sc:347):    LCD: idx1 = *idx0, idx2 = var0; idx1 < var9; idx1 += inc4, idx2 += inc4 */
0x10000f00, /* 000C(../LIB_incl/bd_hdplx.sc:353):      DRD1A: var3 = idx0; FN=0 MORE init=0 WS=0 RS=0 */
0x60140002, /* 0010(../LIB_incl/bd_hdplx.sc:356):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=2 EXT init=0 WS=2 RS=2 */
0x0c8cfc8a, /* 0014(../LIB_incl/bd_hdplx.sc:356):      DRD2B1: *idx2 = EU3(); EU3(*idx2,var10)  */
0xd8988240, /* 0018(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx1 = idx1; idx1 > var9; idx1 += inc0 */
0xf845e011, /* 001C(../LIB_incl/bd_hdplx.sc:469):    LCDEXT: idx2 = *(idx0 + var00000015); ; idx2 += inc2 */
0xb845e00a, /* 0020(../LIB_incl/bd_hdplx.sc:472):    LCD: idx3 = *(idx0 + var00000019); ; idx3 += inc1 */
0x0bfecf90, /* 0024(../LIB_incl/bd_hdplx.sc:486):      DRD1A: *idx3 = *idx2; FN=0 TFD init=31 WS=3 RS=3 */
0x9898802d, /* 0028(../LIB_incl/bd_hdplx.sc:554):    LCD: idx1 = idx1; idx1 once var0; idx1 += inc5 */
0x64000005, /* 002C(../LIB_incl/bd_hdplx.sc:556):      DRD2A: EU0=0 EU1=0 EU2=0 EU3=5 INT EXT init=0 WS=0 RS=0 */
0x0c0cf849, /* 0030(../LIB_incl/bd_hdplx.sc:556):      DRD2B1: *idx0 = EU3(); EU3(idx1,var9)  */
0x000001f8, /* 0034(:0):    NOP */

0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
0x00000000, /* alignment */
/* Task0(TASK_PCI_TX): Start of VarTab -> 0xf0008700 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000001, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xc000ffff, /* inc[2] */
0x00000000, /* inc[3] */
0x60000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task1(TASK_PCI_RX): Start of VarTab -> 0xf0008780 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xc000ffff, /* inc[2] */
0x00000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task2(TASK_FEC_TX): Start of VarTab -> 0xf0008800 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x0c000000, /* var[13] */
0x40000000, /* var[14] */
0x7fff7fff, /* var[15] */
0x00000000, /* var[16] */
0x00000003, /* var[17] */
0x40000004, /* var[18] */
0x43ffffff, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa0000008, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task3(TASK_FEC_RX): Start of VarTab -> 0xf0008880 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x00000003, /* var[12] */
0x40000008, /* var[13] */
0x43ffffff, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa0000008, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task4(TASK_LPC): Start of VarTab -> 0xf0008900 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task5(TASK_ATA): Start of VarTab -> 0xf0008980 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task6(TASK_CRC16_DP_0): Start of VarTab -> 0xf0008a00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000001, /* var[8] */
0x00000008, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task7(TASK_CRC16_DP_1): Start of VarTab -> 0xf0008a80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000001, /* var[8] */
0x00000008, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task8(TASK_GEN_DP_0): Start of VarTab -> 0xf0008b00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task9(TASK_GEN_DP_1): Start of VarTab -> 0xf0008b80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task10(TASK_GEN_DP_2): Start of VarTab -> 0xf0008c00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task11(TASK_GEN_DP_3): Start of VarTab -> 0xf0008c80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000008, /* var[8] */
0x00000000, /* var[9] */
0x00000000, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0x00000000, /* inc[3] */
0xe0000000, /* inc[4] */
0xc000ffff, /* inc[5] */
0x4000ffff, /* inc[6] */
0x00000000, /* inc[7] */
/* Task12(TASK_GEN_TX_BD): Start of VarTab -> 0xf0008d00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x40000004, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa0000008, /* inc[3] */
0x20000000, /* inc[4] */
0x4000ffff, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task13(TASK_GEN_RX_BD): Start of VarTab -> 0xf0008d80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xa0000008, /* inc[2] */
0x20000000, /* inc[3] */
0x00000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task14(TASK_GEN_DP_BD_0): Start of VarTab -> 0xf0008e00 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */
/* Task15(TASK_GEN_DP_BD_1): Start of VarTab -> 0xf0008e80 */
0x00000000, /* var[0] */
0x00000000, /* var[1] */
0x00000000, /* var[2] */
0x00000000, /* var[3] */
0x00000000, /* var[4] */
0x00000000, /* var[5] */
0x00000000, /* var[6] */
0x00000000, /* var[7] */
0x00000000, /* var[8] */
0x40000000, /* var[9] */
0x7fff7fff, /* var[10] */
0x00000000, /* var[11] */
0x00000000, /* var[12] */
0x00000000, /* var[13] */
0x00000000, /* var[14] */
0x00000000, /* var[15] */
0x00000000, /* var[16] */
0x00000000, /* var[17] */
0x00000000, /* var[18] */
0x00000000, /* var[19] */
0x00000000, /* var[20] */
0x00000000, /* var[21] */
0x00000000, /* var[22] */
0x00000000, /* var[23] */
0x40000000, /* inc[0] */
0xe0000000, /* inc[1] */
0xe0000000, /* inc[2] */
0xa000000c, /* inc[3] */
0x20000000, /* inc[4] */
0x00000000, /* inc[5] */
0x00000000, /* inc[6] */
0x00000000, /* inc[7] */

/* Task0(TASK_PCI_TX): Start of FDT -> 0xf0008f00 */
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0x00000000, 
0xa0045670, /* load_acc(), EU# 3 */
0x80045670, /* unload_acc(), EU# 3 */
0x21800000, /* and(), EU# 3 */
0x21e00000, /* or(), EU# 3 */
0x21500000, /* xor(), EU# 3 */
0x21400000, /* andn(), EU# 3 */
0x21500000, /* not(), EU# 3 */
0x20400000, /* add(), EU# 3 */
0x20500000, /* sub(), EU# 3 */
0x20800000, /* lsh(), EU# 3 */
0x20a00000, /* rsh(), EU# 3 */
0xc0170000, /* crc8(), EU# 3 */
0xc0145670, /* crc16(), EU# 3 */
0xc0345670, /* crc32(), EU# 3 */
0xa0076540, /* endian32(), EU# 3 */
0xa0000760, /* endian16(), EU# 3 */

/* Task0(TASK_PCI_TX): Start of CSave -> 0xf0009000 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task1(TASK_PCI_RX): Start of CSave -> 0xf0009050 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task2(TASK_FEC_TX): Start of CSave -> 0xf00090a0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task3(TASK_FEC_RX): Start of CSave -> 0xf00090f0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task4(TASK_LPC): Start of CSave -> 0xf0009140 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task5(TASK_ATA): Start of CSave -> 0xf0009190 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task6(TASK_CRC16_DP_0): Start of CSave -> 0xf00091e0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task7(TASK_CRC16_DP_1): Start of CSave -> 0xf0009230 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task8(TASK_GEN_DP_0): Start of CSave -> 0xf0009280 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task9(TASK_GEN_DP_1): Start of CSave -> 0xf00092d0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task10(TASK_GEN_DP_2): Start of CSave -> 0xf0009320 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task11(TASK_GEN_DP_3): Start of CSave -> 0xf0009370 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task12(TASK_GEN_TX_BD): Start of CSave -> 0xf00093c0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task13(TASK_GEN_RX_BD): Start of CSave -> 0xf0009410 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task14(TASK_GEN_DP_BD_0): Start of CSave -> 0xf0009460 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Task15(TASK_GEN_DP_BD_1): Start of CSave -> 0xf00094b0 */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
0x00000000, /* reserve */
/* Start of free code space -> f0009500 */

};
