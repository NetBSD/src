/* $NetBSD: bt459reg.h,v 1.1.2.1 1998/10/15 02:48:59 nisimura Exp $ */

/*
 * Register definitions for the Brooktree Bt459 135 MHz Monolithic
 * CMOS 256x64 Color Palette RAMDAC.
 */
				/* 0000-00ff Color Map entries */

#define	BT459_REG_CCOLOR_1	0x0181	/* Cursor color regs */
#define	BT459_REG_CCOLOR_2	0x0182
#define	BT459_REG_CCOLOR_3	0x0183
#define	BT459_REG_ID		0x0200	/* read-only, gives "4a" */
#define	BT459_REG_COMMAND_0	0x0201
#define	BT459_REG_COMMAND_1	0x0202
#define	BT459_REG_COMMAND_2	0x0203
#define	BT459_REG_PRM		0x0204
				/* 0205 reserved */
#define	BT459_REG_PBM		0x0206
				/* 0207 reserved */
#define	BT459_REG_ORM		0x0208
#define	BT459_REG_OBM		0x0209
#define	BT459_REG_ILV		0x020a
#define	BT459_REG_TEST		0x020b
#define	BT459_REG_RSIG		0x020c
#define	BT459_REG_GSIG		0x020d
#define	BT459_REG_BSIG		0x020e
				/* 020f-02ff reserved */
#define	BT459_REG_CCR		0x0300
#define	BT459_REG_CURSOR_X_LOW	0x0301
#define	BT459_REG_CURSOR_X_HIGH	0x0302
#define	BT459_REG_CURSOR_Y_LOW	0x0303
#define	BT459_REG_CURSOR_Y_HIGH	0x0304
#define	BT459_REG_WXLO		0x0305
#define	BT459_REG_WXHI		0x0306
#define	BT459_REG_WYLO		0x0307
#define	BT459_REG_WYHI		0x0308
#define	BT459_REG_WWLO		0x0309
#define	BT459_REG_WWHI		0x030a
#define	BT459_REG_WHLO		0x030b
#define	BT459_REG_WHHI		0x030c
				/* 030d-03ff reserved */
#define BT459_REG_CRAM_BASE	0x0400
