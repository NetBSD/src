/*	$NetBSD: bcureg.h,v 1.1.1.1.8.1 1999/12/27 18:32:14 wrstuden Exp $	*/

/*-
 * Copyright (c) 1999 SATO Kazumi. All rights reserved.
 * Copyright (c) 1999 PocketBSD Project. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 *	BCU (Bus Control Unit) Registers definitions.
 *		start 0xB000000
 */

#define	BCUCNT1_REG_W		0x000	/* BCU Control Register 1 */

#define		BCUCNT1_ROMMASK		(1<<15)	/* ROM SIZE */
#define		BCUCNT1_ROM64M		(1<<15)	/* ROM SIZE 64Mbit*/
#define		BCUCNT1_ROM32M		(0<<15)	/* ROM SIZE 32Mbit*/

#define		BCUCNT1_DRAMMASK	(1<<14)	/* DRAM SIZE */
#define		BCUCNT1_DRAM64M		(1<<14)	/* DRAM SIZE 64Mbit*/
#define		BCUCNT1_DRAM32M		(0<<14)	/* DRAM SIZE 32Mbit*/

#define		BCUCNT1_ISAMLCD		(1<<13)	/* ISAM/LCD 0x0a000000 to 0xaffffff*/
#define		BCUCNT1_ISA		(1<<13)	/* ISA memory space */
#define		BCUCNT1_LCD		(0<<13)	/* LCD  space*/

#define		BCUCNT1_PAGEMASK	(1<<12)	/* Maximum burst access size for Page Rom */
#define		BCUCNT1_PAGE128		(1<<12)	/* 128bit */
#define		BCUCNT1_PAGE64		(0<<12)	/* 64bit */

#define		BCUCNT1_PAGE2MASK	(1<<10)	/* */
#define		BCUCNT1_PAGE2PAGE	(1<<10)	/* Page ROM */
#define		BCUCNT1_PAGE2ORD	(0<<10)	/* Prginary ROM */

#define		BCUCNT1_PAGE0MASK	(1<<8)	/* */
#define		BCUCNT1_PAGE0PAGE	(1<<8)	/* Page ROM */
#define		BCUCNT1_PAGE0ORD	(0<<8)	/* Prginary ROM */

#define		BCUCNT1_ROMWEN2		(1<<6)	/* Enable Flash memory write ROM 2*/
#define		BCUCNT1_ROMWEN2EN	(1<<6)	/* Enable */
#define		BCUCNT1_ROMWEN2DS	(0<<6)	/* Prohibit */

#define		BCUCNT1_ROMWEN0		(1<<4)	/* Enable Flash memory write ROM 0*/
#define		BCUCNT1_ROMWEN0EN	(1<<4)	/* Enable */
#define		BCUCNT1_ROMWEN0DS	(0<<4)	/* Prohibit */

#define		BCUCNT1_BUSHERR		(1<<1)	/* Bus Timeout detection enable */

#define		BCUCNT1_BUSHERREN	(1<<1)	/* Enable */
#define		BCUCNT1_BUSHERRDS	(0<<1)	/* Prohibit */

#define		BCUCNT1_RSTOUT		(1)	/* RSTOUT control bit */
#define		BCUCNT1_RSTOUTH		(1)	/* RSTOUT high level*/
#define		BCUCNT1_RSTOUTL		(0)	/* RSTOUT low level*/


#define	BCUCNT2_REG_W		0x002	/* BCU Control Register 2 */

#define		BCUCNT2_GMODE		(1)	/* LCD access control */
#define		BCUCNT2_GMODENOM	(1)	/* not invert LCD */
#define		BCUCNT2_GMODEINV	(0)	/* invert LCD */


#define	BCUSPEED_REG_W		0x00A	/* BCU Access Cycle Change Register */

#define		BCUSPD_WPROM		(0x3<<12)	/* Page ROM access speed */
#define		BCUSPD_WPROMRFU		(0x3<<12)	/* RFU */
#define		BCUSPD_WPROM1T		(0x2<<12)	/* 1TClock */
#define		BCUSPD_WPROM2T		(0x1<<12)	/* 2TClock */
#define		BCUSPD_WPROM3T		(0x0<<12)	/* 3TClock */

#define		BCUSPD_WLCDM		(0x7<<18)	/* access speed 0x0a000000-0affffff */

		/* BCUCNT1_ISAMLCD == BCUCNT1_LCD */
#define		BCUSPD_WLCDRFU		(0x7<<12)	/* LCD RFU */
#define		BCUSPD_WLCDRFU1		(0x6<<12)	/* LCD RFU */
#define		BCUSPD_WLCDRFU2		(0x5<<12)	/* LCD RFU */
#define		BCUSPD_WLCDRFU3		(0x4<<12)	/* LCD RFU */
#define		BCUSPD_WLCD2T		(0x3<<12)	/* LCD 2TClock */
#define		BCUSPD_WLCD4T		(0x2<<12)	/* LCD 4TClock */
#define		BCUSPD_WLCD6T		(0x1<<12)	/* LCD 6TClock */
#define		BCUSPD_WLCD8T		(0x0<<12)	/* LCD 8TClock */
		/* BCUCNT1_ISAMLCD == BCUCNT1_ISAM */
#define		BCUSPD_ISAM1T		(0x7<<12)	/* ISAM 1TClock */
#define		BCUSPD_ISAM2T		(0x6<<12)	/* ISAM 2TClock */
#define		BCUSPD_ISAM3T		(0x5<<12)	/* ISAM 3TClock */
#define		BCUSPD_ISAM4T		(0x4<<12)	/* ISAM 4TClock */
#define		BCUSPD_ISAM5T		(0x3<<12)	/* ISAM 5TClock */
#define		BCUSPD_ISAM6T		(0x2<<12)	/* ISAM 6TClock */
#define		BCUSPD_ISAM7T		(0x1<<12)	/* ISAM 7TClock */
#define		BCUSPD_ISAM8T		(0x0<<12)	/* ISAM 8TClock */

#define		BCUSPD_WISAA		(0x7<<4)	/* System Bus Access Speed */
#define		BCUSPD_WISAA3T		(0x5<<4)	/* 3TClock */
#define		BCUSPD_WISAA4T		(0x4<<4)	/* 4TClock */
#define		BCUSPD_WISAA5T		(0x3<<4)	/* 5TClock */
#define		BCUSPD_WISAA6T		(0x2<<4)	/* 6TClock */
#define		BCUSPD_WISAA7T		(0x1<<4)	/* 7TClock */
#define		BCUSPD_WISAA8T		(0x0<<4)	/* 8TClock */

#define		BCUSPD_WROMA		(0x7<<0)	/* System Bus Access Speed */
#define		BCUSPD_WROMA2T		(0x7<<0)	/* 2TClock */
#define		BCUSPD_WROMA3T		(0x6<<0)	/* 3TClock */
#define		BCUSPD_WROMA4T		(0x5<<0)	/* 4TClock */
#define		BCUSPD_WROMA5T		(0x4<<0)	/* 5TClock */
#define		BCUSPD_WROMA6T		(0x3<<0)	/* 6TClock */
#define		BCUSPD_WROMA7T		(0x2<<0)	/* 7TClock */
#define		BCUSPD_WROMA8T		(0x1<<0)	/* 8TClock */
#define		BCUSPD_WROMA9T		(0x0<<0)	/* 9TClock */


#define	BCUERRST_REG_W		0x00C	/* BCU BUS ERROR Status Register */

#define		BCUERRST_BUSERRMASK	(1)		/* Bus error, clear to 0 when 1 is written */
#define		BCUERRST_BUSERR		(1)		/* Bus error */
#define		BCUERRST_BUSNORM	(0)		/* Normal */


#define	BCURFCNT_REG_W		0x00E	/* BCU Refresh Control Register */

#define		BCURFCNT_MASK		0x3fff		/* refresh interval MASK */


#define	BCUREVID_REG_W		0x010	/* BCU Revision ID Register */

#define		BCUREVID_RIDMASK	(0x7<<12)	/* Revision ID */
#define		BCUREVID_RIDSHFT	(12)		/* Revision ID */
#define		BCUREVID_RID_4121	(0x3)		/* VR4121 */
#define		BCUREVID_RID_4111	(0x2)		/* VR4111 */
#define		BCUREVID_RID_4102	(0x1)		/* VR4102 */
#define		BCUREVID_RID_4101	(0x0)		/* VR4101 */

#define		BCUREVID_MJREVMASK	(0x7<<8)	/* Major Revision */
#define		BCUREVID_MJREVSHFT	(8)		/* Major Revision */

#define		BCUREVID_MNREVMASK	(0x7)		/* Minor Revision */
#define		BCUREVID_MNREVSHFT	(0)		/* Minor Revision */


#define	BCUREFCOUNT_REG_W	0x012	/* BCU Refresh Count Register */

#define		BCUREFCOUNT_MASK	0x3fff	/* refresh 	count MASK */


#define	BCUCLKSPEED_REG_W	0x014	/* Clock Speed Register */

#define		BCUCLKSPEED_DIV2B	(1<<15)
#define		BCUCLKSPEED_DIV3B	(1<<14)
#define		BCUCLKSPEED_DIV4B	(1<<13)

#define		BCUCLKSPEED_CLKSPMASK	(0xf)		/* calculate for Clock */
#define		BCUCLKSPEED_CLKSPSHFT	(0)		

#define	BCUCNT3_REG_W		0x016	/* BCU Control Register 3 (>= vr4111) */

#define		BCUCNT3_EXTROMMASK	(1<<15)		/* ROM SIZE */
#define		BCUCNT3_EXTROM64M	(1<<15)		/* 64Mbit DRAM */
#define		BCUCNT3_EXTROM32M	(0<<15)		/* 32Mbit DRAM */

#define		BCUCNT3_EXTDRAMMASK	(1<<14)		/* DRAM SIZE */
#define		BCUCNT3_EXTDRAM64M	(1<<14)		/* 64Mbit DRAM */
#define		BCUCNT3_EXTDRAM16M	(0<<14)		/* 16Mbit DRAM */

#define		BCUCNT3_EXTROMCS	(0x3<<12)	/* Bank3,2 */
#define		BCUCNT3_ROMROM		(0x3<<12)	/* Bank3 ROM ,2 ROM  */
#define		BCUCNT3_ROMRAM		(0x2<<12)	/* Bank3 ROM ,2 RAM  */
#define		BCUCNT3_RAMRAM		(0x0<<12)	/* Bank3 RAM ,2 RAM  */

#define		BCUCNT3_EXTMEM		(1<<11)		/* EXT MEN enable */
#define		BCUCNT3_EXTMEMEN	(1<<11)		/* EXT MEN enable */
#define		BCUCNT3_EXTMEMDS	(0<<11)		/* EXT MEN disable */

#define		BCUCNT3_LCDSIZE		(1<<7)		/* LCD bus size */
#define		BCUCNT3_LCD32		(1<<7)		/* LCD bus 32bit */
#define		BCUCNT3_LCD16		(0<<7)		/* LCD bus 16bit */

/* END bcureg.h */
