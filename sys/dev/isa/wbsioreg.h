/* $NetBSD: wbsioreg.h,v 1.7 2017/12/13 00:27:53 knakahara Exp $ */

/* $OpenBSD: wbsioreg.h,v 1.4 2015/01/02 23:02:54 chris Exp $ */
/*
 * Copyright (c) 2008 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Winbond LPC Super I/O driver registers
 */

/* ISA bus registers */
#define WBSIO_INDEX		0x00	/* Configuration Index Register */
#define WBSIO_DATA		0x01	/* Configuration Data Register */

#define WBSIO_IOSIZE		0x02	/* ISA I/O space size */

#define WBSIO_CONF_EN_MAGIC	0x87	/* enable configuration mode */
#define WBSIO_CONF_DS_MAGIC	0xaa	/* disable configuration mode */

/* Configuration Space Registers */
#define WBSIO_LDN		0x07	/* Logical Device Number */
#define WBSIO_MFS0		0x1A	/* Multi Function Selection 0 */
#define WBSIO_MFS1		0x1B	/* Multi Function Selection 1 */
#define WBSIO_MFS2		0x1C	/* Multi Function Selection 2 */
#define WBSIO_MFS3		0x1D	/* Multi Function Selection 3 */
#define WBSIO_ID		0x20	/* Device ID */
#define WBSIO_REV		0x21	/* Device Revision */
#define WBSIO_GOPT0		0x24	/* Global Option 0 */
#define WBSIO_GOPT1		0x26	/* Global Option 1 */
#define WBSIO_GOPT2		0x27	/* Global Option 2 */
#define WBSIO_GOPT3		0x28	/* Global Option 3 */
#define WBSIO_MFS4		0x2A	/* Multi Function Selection 4 */
#define WBSIO_MFS5		0x2B	/* Multi Function Selection 5 */
#define WBSIO_MFS6		0x2C	/* Multi Function Selection 6 */
#define WBSIO_SFR		0x2F	/* Strapping Function Result */


#define WBSIO_ID_W83627HF	0x52
#define WBSIO_ID_W83697HF	0x60
#define WBSIO_ID_W83637HF	0x70
#define WBSIO_ID_W83627THF	0x82
#define WBSIO_ID_W83687THF	0x85
#define WBSIO_ID_W83627SF	0x595
#define WBSIO_ID_W83697UG	0x681
#define WBSIO_ID_W83627EHF_A	0x885
#define WBSIO_ID_W83627EHF	0x886
#define WBSIO_ID_W83627DHG	0xa02
#define WBSIO_ID_W83627UHG	0xa23
#define WBSIO_ID_W83667HG	0xa51
#define WBSIO_ID_W83627DHGP	0xb07
#define WBSIO_ID_W83667HGB	0xb35
#define WBSIO_ID_NCT6775F	0xb47
#define WBSIO_ID_NCT6776F	0xc33
#define WBSIO_ID_NCT5104D	0xc45	/* 610[246]D */
#define WBSIO_ID_NCT6779D	0xc56
#define WBSIO_ID_NCT6791D	0xc80
#define WBSIO_ID_NCT6792D	0xc91
#define WBSIO_ID_NCT6793D	0xd12
#define WBSIO_ID_NCT6795D	0xd35

/* Strapping Function Result */
#define WBSIO_SFR_24M48M	0x01
#define WBSIO_SFR_LPT		0x02
#define WBSIO_SFR_TEST		0x04
#define WBSIO_SFR_DSW		0x08
#define WBSIO_SFR_AMDPWR	0x20
#define WBSIO_SFR_UARTP80	0x40

/* Logical Device Number (LDN) Assignments */
#define WBSIO_LDN_HM		0x0b
#define WBSIO_LDN_GPIO0		0x08	/* WDT, GPIO 0 */
#define WBSIO_LDN_GPIO1		0x09	/* GPIO 1 to GPIO 8 */

/* Hardware Monitor Control Registers (LDN B) */
#define WBSIO_HM_ADDR_MSB	0x60	/* Address [15:8] */
#define WBSIO_HM_ADDR_LSB	0x61	/* Address [7:0] */
#define WBSIO_HM_CONF		0xE4	/* Configuration Register */
#define WBSIO_HM_CONF_RSTOUT4	0x02	/* RSTOUT4# bit */
#define WBSIO_HM_CONF_RSTOUT3	0x04	/* RSTOUT3# bit */
#define WBSIO_HM_CONF_PWROK	0x08	/* Power OK Bit */

/* GPIO Registers */
#define WBSIO_GPIO_ADDR_MSB	0x60	/* Address [15:8] */
#define WBSIO_GPIO_ADDR_LSB	0x61	/* Address [7:0] */
#define WBSIO_GPIO_CONF		0x30	/* GPIO0, WDT1 config */
#define WBSIO_WDT_MODE		0xF5	/* WDT1 Control Mode */
#define WBSIO_WDT_CNTR		0xF6	/* WDT1 Counter */
#define WBSIO_WDT_STAT		0xF7	/* WDT1 Control & Status */
#define WBSIO_GPIO4_MFS		0xEE	/* GPIO4 Multi-Function Select */

#define WBSIO_GPIO0_WDT1	__BIT(0)
#define WBSIO_GPIO0_ENABLE	__BIT(1)

						/* Reserved */
#define WBSIO_GPIO_BASEADDR	__BIT(3)	/* Base address mode */
#define WBSIO_GPIO1_ENABLE	__BIT(1)
#define WBSIO_GPIO2_ENABLE	__BIT(2)
#define WBSIO_GPIO3_ENABLE	__BIT(3)
#define WBSIO_GPIO4_ENABLE	__BIT(4)
#define WBSIO_GPIO5_ENABLE	__BIT(5)
#define WBSIO_GPIO6_ENABLE	__BIT(6)
#define WBSIO_GPIO7_ENABLE	__BIT(7)
#define WBSIO_GPIO8_ENABLE	__BIT(0)

#define WBSIO_GPIO_NPINS	64
#define WBSIO_GPIO_IOSIZE	0x06	/* GPIO register table size */

#define WBSIO_GPIO_GSR		0x00	/* GPIO Select Register */
#define WBSIO_GPIO_IOR		0x01	/* I/O direction */
#define WBSIO_GPIO_DAT		0x02	/* Data */
#define WBSIO_GPIO_INV		0x03	/* Inversion */
#define WBSIO_GPIO_DST		0x04	/* Event Status */
					/* WDT1 Control Mode */
					/* WDT1 control */
#define WBSIO_WDT_MODE_LEVEL	__BIT(0)
					/* enable/disable KBRST */
#define WBSIO_WDT_MODE_KBCRST	__BIT(2)
#define WBSIO_WDT_MODE_MINUTES	__BIT(3)
#define WBSIO_WDT_MODE_FASTER	__BIT(4)

#define WBSIO_WDT_CNTR_STOP	0
#define WBSIO_WDT_CNTR_MAX	255

#define WBSIO_WDT_STAT_TIMEOUT	__BIT(4)
#define WBSIO_WDT_STAT_EVENT	__BIT(5)


/* NCT6779D */
#define WBSIO_NCT6779D_MFS2_GP00	__BIT(0)
#define WBSIO_NCT6779D_MFS2_GP01	__BIT(1)
#define WBSIO_NCT6779D_MFS2_GP02	__BIT(2)
#define WBSIO_NCT6779D_MFS2_GP03_MASK	(__BIT(3)|__BIT(4))
#define WBSIO_NCT6779D_MFS2_GP04	__BIT(5)
#define WBSIO_NCT6779D_MFS2_GP05	__BIT(6)
#define WBSIO_NCT6779D_MFS2_GP06	__BIT(7)
#define WBSIO_NCT6779D_MFS3_GP07_MASK	__BIT(0)
#define WBSIO_NCT6779D_MFS4_GP10_GP17	__BIT(6)
#define WBSIO_NCT6779D_MFS4_GP20_GP21	__BIT(0)
#define WBSIO_NCT6779D_MFS4_GP22_GP23	__BIT(1)
#define WBSIO_NCT6779D_MFS1_GP24_MASK	__BIT(4)
#define WBSIO_NCT6779D_GOPT2_GP24_MASK	__BIT(3)
#define WBSIO_NCT6779D_MFS4_GP25_MASK	__BIT(3)
#define WBSIO_NCT6779D_GOPT2_GP25_MASK	__BIT(3)
#define WBSIO_NCT6779D_MFS6_GP26	__BIT(0)
#define WBSIO_NCT6779D_MFS6_GP27_MASK	(__BIT(3)|__BIT(4))
#define WBSIO_NCT6779D_MFS0_GP30	__BIT(6)
#define WBSIO_NCT6779D_MFS0_GP30_MASK	__BIT(7)
#define WBSIO_NCT6779D_MFS0_GP31	__BIT(5)
#define WBSIO_NCT6779D_MFS1_GP31_MASK	__BIT(0)
#define WBSIO_NCT6779D_MFS0_GP32	__BIT(4)
#define WBSIO_NCT6779D_MFS1_GP32_MASK	__BIT(0)
#define WBSIO_NCT6779D_MFS6_GP33	__BIT(7)
#define WBSIO_NCT6779D_MFS6_GP33_MASK	__BIT(6)
#define WBSIO_NCT6779D_MFS1_GP40	__BIT(3)
#define WBSIO_NCT6779D_MFS0_GP41	__BIT(3)
#define WBSIO_NCT6779D_MFS0_GP41_MASK	__BIT(2)
#define WBSIO_NCT6779D_MFS1_GP42	(__BIT(1)|__BIT(2))
#define WBSIO_NCT6779D_GOPT2_GP43	__BIT(4)
#define WBSIO_NCT6779D_MFS1_GP44_GP45_MASK	__BIT(6)
#define WBSIO_NCT6779D_GOPT2_GP46_MASK	__BIT(5)
#define WBSIO_NCT6779D_MFS1_GP47	__BIT(7)
#define WBSIO_NCT6779D_HM_GP50_MASK	__BIT(2)
#define WBSIO_NCT6779D_HM_GP52_MASK	__BIT(1)
#define WBSIO_NCT6779D_HM_GP55_MASK	__BIT(3)
#define WBSIO_NCT6779D_MFS5_GP74	__BIT(5)
#define WBSIO_NCT6779D_MFS5_GP75	__BIT(6)
#define WBSIO_NCT6779D_MFS5_GP76	__BIT(7)
#define WBSIO_NCT6779D_GPIO4_WDTO	__BIT(4)
