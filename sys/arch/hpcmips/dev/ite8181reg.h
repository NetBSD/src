/*	$NetBSD: ite8181reg.h,v 1.5 2022/07/03 11:30:48 andvar Exp $	*/

/*-
 * Copyright (c) 2000 SATO Kazumi
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/* ITE8181 configuration registers */
#define ITE8181_CONF_OFFSET	((8192 -1) * 1024)	/* offset of config reg */
#define ITE8181_ID       0x00	/* Device ID, Vender ID */
#define		ITE8181_DATA_ID 0x81811283
#define		ITE8181_PRODUCT_ID 0x8181
#define		ITE8181_VENDER_ID 0x1283
#define ITE8181_SCMD     0x04	/* Status, Command Reg. */
#define ITE8181_CLASS    0x08	/* Class, Sub-class, PRG, revision */
#define		ITE8181_DATA_CLASS      0x03800000
#define		ITE8181_CLASS_MASK      0xffff0000
#define		ITE8181_REV_MASK	0x000000ff
#define ITE8181_MBA      0x10	/* Memory Base Address(4MB boundary) */
#define ITE8181_GBA      0x14	/* GUI Base Address(32KB boundary) */
#define ITE8181_SBA      0x18	/* Graphic Base Address (64KB boundary) */
#define ITE8181_TEST     0x40	/* Test Reg. */
#define ITE8181_STANDBY  0x44	/* Standby Reg. */
#define 	ITE8181_DATA_PLL2_TEST		0xc0000	/* PLL2 is test mode */
#define 	ITE8181_DATA_PLL1_TEST		0x30000	/* PLL1 is test mode */
#define 	ITE8181_DATA_PLL2_RESET		0x8000	/* PLL2 reset */
#define 	ITE8181_DATA_PLL1_RESET		0x4000	/* PLL1 reset */
#define 	ITE8181_DATA_PLL2_PWDOWN	0x2000	/* PLL2 powerdown */
#define 	ITE8181_DATA_PLL1_PWDOWN	0x1000	/* PLL1 powerdown */
#define		ITE8181_DATA_PALETTESTBY	0x0200	/* Palette RAM standby */
#define		ITE8181_DATA_CURSORSTBY		0x0100	/* Cursor standby */
#define		ITE8181_DATA_BITBLTSTBY		0x0080	/* BitBlt engine standby */
#define		ITE8181_DATA_LINESTBY		0x0040	/* Line Draw standby */
#define		ITE8181_DATA_DACCLKSTOP		0x0020	/* DAC Clock stop */
#define		ITE8181_DATA_DACPOWERON		0x0010	/* DAC Power ON */
#define		ITE8181_DATA_GATEPLL2IN		0x0008	/* Gate PLL2 input clock */
#define		ITE8181_DATA_GATEPLL1IN		0x0004	/* Gate PLL1 input clock */
#define		ITE8181_DATA_CLOCKSTOP		0x0001	/* 14.318MHZ CLock Stop */
#define ITE8181_PLL1     0x48	/* PLL1 Reg. */
#define ITE8181_PLL2     0x4c	/* PLL2 Reg. */

/* ITE8181 GUI 32bit registers */
#define ITE8181_GUI_BSLE	0x00	/* BitBlt src/Line Draw End */
#define ITE8181_GUI_BDLS	0x04	/* BitBlt dst/Line Draw Start */
#define ITE8181_GUI_BPOA	0x08	/* BitBlt Pattern Offset Address */
#define ITE8181_GUI_BWH		0x0c	/* BitBlt Width, Hight */
#define ITE8181_GUI_BSO		0x10	/* BitBlt Screen Offset */
#define ITE8181_GUI_FCR		0x14	/* ForeGround Color Reg. */
#define ITE8181_GUI_BCR		0x18	/* BackGround Color Reg. */
#define ITE8181_GUI_BC		0x1c	/* BitBlt Control */
#define ITE8181_GUI_BS		0x20	/* BitBlt Status */
#define ITE8181_GUI_ASDS	0x24	/* Line Draw Axial Step, Diagonal Step */
#define ITE8181_GUI_LET		0x28	/* Line Draw Error Term/ Pixel Count */
#define ITE8181_GUI_LST		0x2c	/* Scissor Top */
#define ITE8181_GUI_LSB		0x30	/* Scissor Bottom */
#define ITE8181_GUI_LSR		0x34	/* Line Style Register */
#define ITE8181_GUI_SSVS	0x38	/* Short Stroke Vector Spec */
#define ITE8181_GUI_MR		0x4c	/* Misc Reg. */
#define ITE8181_GUI_PIO		0x40000	/* Pixel I/port for System Data */

/* ITE8181 GUI 8bit registers */	
#define	ITE8181_GUI_C1C		0x100	/* Cursor1 Control Reg. */
#define	ITE8181_GUI_C1O		0x101	/* Cursor1 Offset Reg. */
#define	ITE8181_GUI_C1F		0x102	/* Cursor1 Feature Reg. */
#define	ITE8181_GUI_C1SAH	0x103	/* Icon Map Address MSB */
#define	ITE8181_GUI_C1SAL	0x108	/* Icon Map Address LSB */
#define	ITE8181_GUI_C1LPX	0x109	/* Cursor Clipping X Coord Reg. */
#define	ITE8181_GUI_C1LPY	0x10a	/* Cursor Clipping Y Coord Reg. */
#define	ITE8181_GUI_CC0R0	0x110	/* Cursor Color 0 Reg. */
#define	ITE8181_GUI_CC0R1	0x111
#define	ITE8181_GUI_CC0R2	0x112
#define	ITE8181_GUI_CC0R3	0x113
#define	ITE8181_GUI_CC1R0	0x114	/* Cursor Color 1 Reg. */
#define	ITE8181_GUI_CC1R1	0x115
#define	ITE8181_GUI_CC1R2	0x116
#define	ITE8181_GUI_CC1R3	0x117
#define	ITE8181_GUI_CC2R0	0x118	/* Cursor Color 2 Reg. */
#define	ITE8181_GUI_CC2R1	0x119
#define	ITE8181_GUI_CC2R2	0x11a
#define	ITE8181_GUI_CC2R3	0x11b
#define ITE8181_GUI_C1XC0	0x120	/* cursor 1 X coord bits[7:0] */
#define ITE8181_GUI_C1XC1	0x121	/* cursor 1 X coord bits[11:8] */
#define ITE8181_GUI_C1YC0	0x122	/* cursor 1 Y coord bits[7:0] */
#define ITE8181_GUI_C1YC1	0x123	/* cursor 1 Y coord bits[11:8] */

/* Extension Mode A registers */
#define ITE8181_EMA_EXAX	0x03d6	/* Extension Controller Index Reg. */
#define ITE8181_EMA_EXADATA	0x03d7	/* Extension Controller Data. */

#define ITE8181_EMA_ENABLEEMA	0x0b	/* Extension Index Enable Reg. */
#define 	ITE8181_EMA_ENABLEPASS	0xec	/* EMA enable passwd(w) */
#define 	ITE8181_EMA_DISABLEPASS	0xce	/* EMA disable passwd(w) */
#define 	ITE8181_EMA_ENABLED	0x01	/* EMA enabled (r) */

/* ITE8181 LCD Controller Timming Reg. */
#define ITE8181_EMA_HSIZE	0x80	/* LCD Controller H size Reg. */
#define ITE8181_EMA_HALIGN	0x81	/* LCD H Align Adjust Reg. */
#define ITE8181_EMA_HRETRACE	0x82	/* LCD H Retrace Adjust Reg. */
#define ITE8181_EMA_HADJUST	0x83	/* LCD H Adjust Reg. */
#define ITE8181_EMA_HSYNCDELAY	0x84	/* LCD HSYNC Delay Reg. */
#define ITE8181_EMA_VSIZE	0x85	/* LCD V size Reg. */
#define ITE8181_EMA_VSYNC_DELAY	0x86	/* LCD VSYNC Delay Reg. */
#define ITE8181_EMA_OVERFLOW	0x87	/* LCD Overflow Reg. */
#define ITE8181_EMA_MODULATION	0x88	/* LCD Modulation Reg. */
#define ITE8181_EMA_EXTMODE	0x89	/* LCD Ext Mode Tuning Reg. */
#define ITE8181_EMA_VALIGNA	0x8a	/* LCD V Align Adjust Reg A(350) */
#define ITE8181_EMA_VALIGNB	0x8b	/* LCD V Align Adjust Reg B(400) */
#define ITE8181_EMA_VALIGNC	0x8c	/* LCD V Align Adjust Reg C(>=480) */
#define ITE8181_EMA_VRETRACE	0x8d	/* LCD V Retrace adjusr Reg. */
#define ITE8181_EMA_VOVERFLOW	0x8e	/* LCD V Adjust Overflow Reg. */

/* ITE8181 LCD Controller Reg. */
#define ITE8181_EMA_TYPE	0x90	/* LCD Type Select Reg. */
#define ITE8181_EMA_CONTROL	0x91	/* LCD Controller Reg. */
#define ITE8181_EMA_PINSEL	0x92	/* LCD Controller Pin Select Reg. */
#define ITE8181_EMA_MISCCTL	0x93	/* LCD Misc Control Reg. */

/* ITE8181 LCD Controller Power Management Register */
#define ITE8181_EMA_LCDPOWER	0x98
#define 	ITE8181_LCDSTANDBY	0x20	/* LCD S/W Standby */
#define ITE8181_EMA_LCDPOWERSEQ	0x9a
#define		ITE8181_PUP2		0x80	/* Panel Power UP phase 2 */
#define		ITE8181_PUP1		0x40	/* Panel Power UP phase 1 */
#define		ITE8181_PUP0		0x20	/* Panel Power UP phase 0 */
#define		ITE8181_PDP2		0x10	/* Panel Power DOWN phase 2 */
#define		ITE8181_PDP1		0x08	/* Panel Power DOWN phase 1 */
#define		ITE8181_PDP0		0x04	/* Panel Power DOWN phase 0 */
#define ITE8181_EMA_LCDPOWERSTAT 0x9b	/* data sheet seem to be not correct */
#define		ITE8181_PPTOBEMASK	0x01	/* Panel Power to be...*/
#define		ITE8181_PPTOBEON	0x01	/* Panel Power to be ON */
#define		ITE8181_PPTOBEOFF	0x00	/* Panel Power to be OFF */
#define		ITE8181_LCDPON		0x08	/* LCD ON? (XX no info) */
#define		ITE8181_LCDPSTANDBY	0x20	/* LCD STANDBY? (XX no info) */
#define		ITE8181_LCDPDOWN	0x40	/* LCD POWER DOWN PROGRESS(XX) */
#define		ITE8181_LCDPUP		0x80	/* LCD POWER UP PROGRESS (XX) */

/* ITE8181 LCD Controller Data Manipulation Registers */
#define ITE8181_EMA_DITHERCTRL1	0xa0	/* dither control 1 */
#define 	ITE8181_DITHER_CMASK	0xa0	/* dither enable mask */
#define		ITE8181_DITHER_DISABLE	0x00	/* disable */
#define		ITE8181_DITHER_SOMEMODE	0x40	/* dither 256/32k/64k/16M color mode */
#define		ITE8181_DITHER_ENABLE	0x80	/* dither in all mode */

#define		ITE8181_DITHER_BCMASK	0x38	/* Base Color select */
#define		ITE8181_DITHER_BC1BIT	0x00	
#define		ITE8181_DITHER_BC2BIT	0x08	
#define		ITE8181_DITHER_BC3BIT	0x10	
#define		ITE8181_DITHER_BC4BIT	0x18	
#define		ITE8181_DITHER_BC5BIT	0x20	
#define		ITE8181_DITHER_BC6BIT	0x28	
#define		ITE8181_DITHER_BC7BIT	0x30	
#define		ITE8181_DITHER_BC8BIT	0x38	

#define		ITE8181_DITHER_BSMASK	0x03	/* dither bit select */
#define		ITE8181_DITHER_BS6BIT	0x00
#define		ITE8181_DITHER_BS5BIT	0x01
#define		ITE8181_DITHER_BS4BIT	0x02
#define		ITE8181_DITHER_BS3BIT	0x03
#define		ITE8181_DITHER_BS2BIT	0x04
#define		ITE8181_DITHER_BS1BIT	0x05
#define		ITE8181_DITHER_BS0BIT	0x06

#define ITE8181_EMA_DITHERCTRL2	0xa1	/* dither control 2 */
#define		ITE8181_FMS_MASK	0x40	/* frame rate modulation select */
#define		ITE8181_FMS_2		0x00
#define		ITE8181_FMS_1		0x40

#define		ITE8181_GRC_MASK	0x20	/* graphics reverse control */
#define		ITE8181_GRC_NOGREVERSE	0x00
#define		ITE8181_GRC_GREVERSE	0x20

#define		ITE8181_TRC_MASK	0x10	/* text reverse control */
#define		ITE8181_TRC_NOTREVERSE	0x00	
#define		ITE8181_TRC_TREVERSE	0x10	

#define		ITE8181_CM_MASK		0x01	/* color to mono map */
#define		ITE8181_CM_NTSC		0x00	/* NTSC weighting */
#define		ITE8181_CM_GREEN	0x01	/* green only */

#define ITE8181_EMA_FRCCOL	0xa2	/* FRC Color */
#define		ITE8181_FRCCOL_MASK	0x80	/* FRC color option */
#define		ITE8181_FRCCOL_8	0x80	/* option1, 8 color */
#define		ITE8181_FRCCOL_16	0x00	/* option2, 16 color */

#define ITE8181_EMA_FRCPAT	0xa3	/* select frame rate pattern */
#define		ITE8181_FRCPAT_PROGRAM	0x80
#define		ITE8181_FRCPAT_CONSTANT	0x00

#define ITE8181_EMA_FBADDR1	0xa8	/* FB addr1 [21:14] */
#define ITE8181_EMA_FBADDR2	0xa9	/* FB addr2 [21:14] */
#define ITE8181_EMA_FBADDR3	0xaa	/* FB addr3 [21:14] */

#define ITE8181_EMA_REDBCOLOR	0xaa	/* Red Border color */
#define ITE8181_EMA_GREENBCOLOR	0xab	/* Green Border color */
#define ITE8181_EMA_BLUEBCOLOR	0xac	/* Blue Border color */

#define ITE8181_EMA_DISPERSION1	0xb0	/* Dispersion-1 B0-B7 */
#define ITE8181_EMA_DISPERSION2	0xb8	/* Dispersion-2 B8-BF */

#define ITE8181_EMA_FRCPAT0	0xc0	/* Frame Rate Pattern0 C0(lsb)-C1(msb) */
#define ITE8181_EMA_FRCPAT1	0xc2	/* Frame Rate Pattern1 C2(lsb)-C3(msb) */
#define ITE8181_EMA_FRCPAT2	0xc4	/* Frame Rate Pattern2 C4(lsb)-C5(msb) */
#define ITE8181_EMA_FRCPAT3	0xc6	/* Frame Rate Pattern3 C6(lsb)-C7(msb) */
#define ITE8181_EMA_FRCPAT4	0xc8	/* Frame Rate Pattern4 C8(lsb)-C9(msb) */
#define ITE8181_EMA_FRCPAT5	0xca	/* Frame Rate Pattern5 CA(lsb)-CB(msb) */
#define ITE8181_EMA_FRCPAT6	0xcc	/* Frame Rate Pattern6 CC(lsb)-CD(msb) */
#define ITE8181_EMA_FRCPAT7	0xce	/* Frame Rate Pattern7 CE(lsb)-CF(msb) */
#define ITE8181_EMA_FRCPAT8	0xd0	/* Frame Rate Pattern8 D0(lsb)-D1(msb) */
#define ITE8181_EMA_FRCPAT9	0xd2	/* Frame Rate Pattern9 D2(lsb)-D3(msb) */
#define ITE8181_EMA_FRCPAT10	0xd4	/* Frame Rate Pattern10 D4(lsb)-D5(msb) */
#define ITE8181_EMA_FRCPAT11	0xd6	/* Frame Rate Pattern11 D6(lsb)-D7(msb) */
#define ITE8181_EMA_FRCPAT12	0xd8	/* Frame Rate Pattern12 D8(lsb)-D9(msb) */
#define ITE8181_EMA_FRCPAT13	0xda	/* Frame Rate Pattern13 DA(lsb)-DB(msb) */
#define ITE8181_EMA_FRCPAT14	0xdc	/* Frame Rate Pattern14 DC(lsb)-DD(msb) */
#define ITE8181_EMA_FRCPAT15	0xde	/* Frame Rate Pattern15 DE(lsb)-DF(msb) */


/* Extension Mode B registers */
#define ITE8181_EMB_EXBX	0x03ce	/* Extension Controller Index Reg. */
#define ITE8181_EMB_EXBDATA	0x03cf	/* Extension Controller Data. */

#define ITE8181_EMA_ENABLEEMA	0x0b	/* Extension Index Enable Reg. */
#define 	ITE8181_EMB_ENABLEPASS	0xca	/* EMB enable passwd(w) */
#define 	ITE8181_EMB_DISABLEPASS	0x35	/* EMB disable passwd(w) */
#define 	ITE8181_EMB_ENABLED	0x01	/* EMB enabled (read) */

/* XXX - not yet all - */

/* end */
