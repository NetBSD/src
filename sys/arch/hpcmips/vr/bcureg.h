/*	$NetBSD: bcureg.h,v 1.7 2001/08/21 10:31:26 sato Exp $	*/

/*-
 * Copyright (c) 1999-2001 SATO Kazumi. All rights reserved.
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
 *		start 0xB000000 (vr4101,4102,4111,4121)
 *		start 0xA000000 (vr4181)
 *		start 0xF000000 (vr4122)
 */

#define	BCUCNT1_REG_W		0x000	/* BCU Control Register 1 */

#define		BCUCNT1_ROMMASK		(1<<15)	/* ROM SIZE (<= 4121,>= 4102) */
#define		BCUCNT1_ROM64M		(1<<15)	/* ROM SIZE 64Mbit*/
#define		BCUCNT1_ROM32M		(0<<15)	/* ROM SIZE 32Mbit*/

#define		BCUCNT1_DRAMMASK	(1<<14)	/* DRAM SIZE (<= 4121,>= 4102) */
#define		BCUCNT1_DRAM64M		(1<<14)	/* DRAM SIZE 64Mbit*/
#define		BCUCNT1_DRAM32M		(0<<14)	/* DRAM SIZE 32Mbit*/

#define		BCUCNT1_ROMSMASK	(0x3<<14) /* ROM SIZE (=4181) */
#define		BCUCNT1_ROMS64M		(0x2<<14) /* ROM SIZE 64Mbit */
#define		BCUCNT1_ROMS32M		(0x1<<14) /* ROM SIZE 32Mbit */

#define		BCUCNT1_ISAMLCD		(1<<13)	/* ISAM/LCD 0x0a000000 to 0xaffffff(>= 4102) */
#define		BCUCNT1_ISA		(1<<13)	/* ISA memory space */
#define		BCUCNT1_LCD		(0<<13)	/* LCD  space*/

#define		BCUCNT1_PAGEMASK	(1<<12)	/* Maximum burst access size for Page Rom (<= 4121,>= 4102) */
#define		BCUCNT1_PAGE128		(1<<12)	/* 128bit */
#define		BCUCNT1_PAGE64		(0<<12)	/* 64bit */

#define		BCUCNT1_PAGESIZEMASK	(3<<12)	/* PageROM PAGESIZE (= 4122) */
#define		BCUCNT1_PASESIZE32	(2<<12)	/* 32 byte */
#define		BCUCNT1_PASESIZE16	(1<<12)	/* 16 byte */
#define		BCUCNT1_PASESIZE8	(0<<12)	/* 8 byte */

#define		BCUCNT1_PAGE2MASK	(1<<10)	/* (<= 4122,>= 4102) */
#define		BCUCNT1_PAGE2PAGE	(1<<10)	/* Page ROM */
#define		BCUCNT1_PAGE2ORD	(0<<10)	/* Prginary ROM */

#define		BCUCNT1_PAGE0MASK	(1<<8)	/* (<= 4122,>= 4102) */
#define		BCUCNT1_PAGE0PAGE	(1<<8)	/* Page ROM */
#define		BCUCNT1_PAGE0ORD	(0<<8)	/* Prginary ROM */

#define		BCUCNT1_REFMASK		(1<<7)	/* DRAM refresh interval (= 4101) */
#define		BCUCNT1_REF1024		(1<<7)	/* 1024 cycles/128ms */
#define		BCUCNT1_REF4096		(0<<7)	/* 4096 cycles/128ms */

#define		BCUCNT1_ROMWEN2		(1<<6)	/* Enable Flash memory write ROM 2 (<= 4122,>= 4102) */
#define		BCUCNT1_ROMWEN2EN	(1<<6)	/* Enable */
#define		BCUCNT1_ROMWEN2DS	(0<<6)	/* Prohibit */

#define		BCUCNT1_PAGEROM		(1<<6)	/* Enable page ROM access (= 4101) */
#define		BCUCNT1_PAGEROMEN	(1<<6)	/* Page ROM */
#define		BCUCNT1_PAGEROMDIS	(0<<6)	/* not Page ROM */

#define		BCUCNT1_ROMWEN		(1<<5)	/* Enable Flash memory write ROM 0 (= 4101) */
#define		BCUCNT1_ROMWENEN	(1<<5)	/* Enable */
#define		BCUCNT1_ROMWENDS	(0<<5)	/* Prohibit */

#define		BCUCNT1_ROMWEN0		(1<<4)	/* Enable Flash memory write ROM 0 (<= 4122,>= 4102, =4181) */
#define		BCUCNT1_ROMWEN0EN	(1<<4)	/* Enable */
#define		BCUCNT1_ROMWEN0DS	(0<<4)	/* Prohibit */

#define		BCUCNT1_SRFSTAT		(1<<4)	/* DRAM refresh mode (= 4101) */
#define		BCUCNT1_SRFSTATSRF	(1<<4)	/* self refresh */
#define		BCUCNT1_SRFSTATCBR	(0<<4)	/* CBR refresh */

#define		BCUCNT1_BCPUR		(1<<3)	/* CPU bus cycle control (= 4101) */
#define		BCUCNT1_BCPUREN		(1<<3)	/* CPU bus cycle control enable */
#define		BCUCNT1_BCPURDIS	(0<<3)	/* CPU bus cycle control disable */

#define		BCUCNT1_HLD		(1<<2)	/* Bus hold enable (= 4122) */
#define		BCUCNT1_HLDEN		(1<<2)	/* enable */
#define		BCUCNT1_HLDDIS		(1<<2)	/* disable */

#define		BCUCNT1_BUSHERR		(1<<1)	/* Bus Timeout detection enable  (<= 4121,>= 4102) */

#define		BCUCNT1_BUSHERREN	(1<<1)	/* Enable */
#define		BCUCNT1_BUSHERRDS	(0<<1)	/* Prohibit */

#define		BCUCNT1_RTYPE		(0x3<<1) /* ROM type (=4181) */
#define		BCUCNT1_RTOROM		(0<<1)	/* Odinary ROM */
#define		BCUCNT1_RTFLASH		(1<<1)	/* flash ROM */
#define		BCUCNT1_RTPAGEROM	(2<<1)	/* Page ROM */

#define		BCUCNT1_RSTOUT		(1)	/* RSTOUT control bit */
#define		BCUCNT1_RSTOUTH		(1)	/* RSTOUT high level*/
#define		BCUCNT1_RSTOUTL		(0)	/* RSTOUT low level*/


#define	BCUCNT2_REG_W		0x002	/* BCU Control Register 2 (<= 4121,>= 4102, =4181) */

#define		BCUCNT2_GMODE		(1)	/* LCD access control */
#define		BCUCNT2_GMODENOM	(1)	/* not invert LCD */
#define		BCUCNT2_GMODEINV	(0)	/* invert LCD */

#define BCUBR_REG_W		0x002	/* BCU Bus Restrain Register (= 4101) */

#define BCUROMSIZE_REG_W	0x004	/* ROM size setting register (= 4122) */
#define		BCUROMSIZE_SIZE3	(7<<12)	/* Bank3 size */
#define		BCUROMSIZE_SIZE3_64	(5<<12)	/* 64MB */
#define		BCUROMSIZE_SIZE3_32	(4<<12)	/* 32MB */
#define		BCUROMSIZE_SIZE3_16	(3<<12)	/* 16MB */
#define		BCUROMSIZE_SIZE3_8	(2<<12)	/* 8MB */
#define		BCUROMSIZE_SIZE3_4	(1<<12)	/* 4MB */

#define		BCUROMSIZE_SIZE2	(7<<8)	/* Bank2 size */
#define		BCUROMSIZE_SIZE2_64	(5<<8)	/* 64MB */
#define		BCUROMSIZE_SIZE2_32	(4<<8)	/* 32MB */
#define		BCUROMSIZE_SIZE2_16	(3<<8)	/* 16MB */
#define		BCUROMSIZE_SIZE2_8	(2<<8)	/* 8MB */
#define		BCUROMSIZE_SIZE2_4	(1<<8)	/* 4MB */

#define		BCUROMSIZE_SIZE1	(7<<4)	/* Bank1 size */
#define		BCUROMSIZE_SIZE1_64	(5<<4)	/* 64MB */
#define		BCUROMSIZE_SIZE1_32	(4<<4)	/* 32MB */
#define		BCUROMSIZE_SIZE1_16	(3<<4)	/* 16MB */
#define		BCUROMSIZE_SIZE1_8	(2<<4)	/* 8MB */
#define		BCUROMSIZE_SIZE1_4	(1<<4)	/* 4MB */

#define		BCUROMSIZE_SIZE0	(7)	/* Bank0 size */
#define		BCUROMSIZE_SIZE0_64	(5)	/* 64MB */
#define		BCUROMSIZE_SIZE0_32	(4)	/* 32MB */
#define		BCUROMSIZE_SIZE0_16	(3)	/* 16MB */
#define		BCUROMSIZE_SIZE0_8	(2)	/* 8MB */
#define		BCUROMSIZE_SIZE0_4	(1)	/* 4MB */

#define	BCUBRCNT_REG_W		0x004	/* BCU Bus Restrain Count Register (= 4101) */

#define BCUROMSPEED_REG_W	0x006	/* BCU ROM Speed Register (=4122) */
#define		BCUROMSPEED_PATIME	(0x3<<12)	/* Page Access time */
#define		BCUROMSPEED_PATIME_5VT	(0x3<<12)	/* 5VTClock */
#define		BCUROMSPEED_PATIME_4VT	(0x2<<12)	/* 4VTClock */
#define		BCUROMSPEED_PATIME_3VT	(0x1<<12)	/* 3VTClock */
#define		BCUROMSPEED_PATIME_2VT	(0x0<<12)	/* 2VTClock */

#define		BCUROMSPEED_ATIME	(0xf)	/* Access time */
#define		BCUROMSPEED_ATIME_18VT	(0xf)	/* 18VTClock */
#define		BCUROMSPEED_ATIME_17VT	(0xe)	/* 17VTClock */
#define		BCUROMSPEED_ATIME_16VT	(0xd)	/* 16VTClock */
#define		BCUROMSPEED_ATIME_15VT	(0xc)	/* 15VTClock */
#define		BCUROMSPEED_ATIME_14VT	(0xb)	/* 14VTClock */
#define		BCUROMSPEED_ATIME_13VT	(0xa)	/* 13VTClock */
#define		BCUROMSPEED_ATIME_12VT	(0x9)	/* 12VTClock */
#define		BCUROMSPEED_ATIME_11VT	(0x8)	/* 11VTClock */
#define		BCUROMSPEED_ATIME_10VT	(0x7)	/* 10VTClock */
#define		BCUROMSPEED_ATIME_9VT	(0x6)	/* 9VTClock */
#define		BCUROMSPEED_ATIME_8VT	(0x5)	/* 8VTClock */
#define		BCUROMSPEED_ATIME_7VT	(0x4)	/* 7VTClock */
#define		BCUROMSPEED_ATIME_6VT	(0x3)	/* 6VTClock */
#define		BCUROMSPEED_ATIME_5VT	(0x2)	/* 5VTClock */
#define		BCUROMSPEED_ATIME_4VT	(0x1)	/* 4VTClock */
#define		BCUROMSPEED_ATIME_3VT	(0x0)	/* 3VTClock */

#define BCUBCL_REG_W		0x006	/* BCU CPU Restrain Disable Register (= 4101) */

#define BCUIO0SPEED_REG_W	0x008	/* BCU IO0 Speed Register (=4122) */
#define		BCUIO0SPEED_RWCS	(0x3<<12)	/* R/W - CS time */
#define		BCUIO0SPEED_RWCS_5VT	(0x3<<12)	/* 5VTClock */
#define		BCUIO0SPEED_RWCS_4VT	(0x2<<12)	/* 4VTClock */
#define		BCUIO0SPEED_RWCS_3VT	(0x1<<12)	/* 3VTClock */
#define		BCUIO0SPEED_RWCS_2VT	(0x0<<12)	/* 2VTClock */

#define		BCUIO0SPEED_RDYRW	(0xf<<8)	/* IORDY-R/W time */
#define		BCUIO0SPEED_RDYRW_18VT	(0xf)	/* 18VTClock */
#define		BCUIO0SPEED_RDYRW_17VT	(0xe)	/* 17VTClock */
#define		BCUIO0SPEED_RDYRW_16VT	(0xd)	/* 16VTClock */
#define		BCUIO0SPEED_RDYRW_15VT	(0xc)	/* 15VTClock */
#define		BCUIO0SPEED_RDYRW_14VT	(0xb)	/* 14VTClock */
#define		BCUIO0SPEED_RDYRW_13VT	(0xa)	/* 13VTClock */
#define		BCUIO0SPEED_RDYRW_12VT	(0x9)	/* 12VTClock */
#define		BCUIO0SPEED_RDYRW_11VT	(0x8)	/* 11VTClock */
#define		BCUIO0SPEED_RDYRW_10VT	(0x7)	/* 10VTClock */
#define		BCUIO0SPEED_RDYRW_9VT	(0x6)	/* 9VTClock */
#define		BCUIO0SPEED_RDYRW_8VT	(0x5)	/* 8VTClock */
#define		BCUIO0SPEED_RDYRW_7VT	(0x4)	/* 7VTClock */
#define		BCUIO0SPEED_RDYRW_6VT	(0x3)	/* 6VTClock */
#define		BCUIO0SPEED_RDYRW_5VT	(0x2)	/* 5VTClock */
#define		BCUIO0SPEED_RDYRW_4VT	(0x1)	/* 4VTClock */
#define		BCUIO0SPEED_RDYRW_3VT	(0x0)	/* 3VTClock */

#define		BCUIO0SPEED_RWRDY	(0xf<<4)	/* R/W-IORDY time */
#define		BCUIO0SPEED_RWRDY_14VT	(0xf)	/* 14VTClock */
#define		BCUIO0SPEED_RWRDY_13VT	(0xe)	/* 13VTClock */
#define		BCUIO0SPEED_RWRDY_12VT	(0xd)	/* 12VTClock */
#define		BCUIO0SPEED_RWRDY_11VT	(0xc)	/* 11VTClock */
#define		BCUIO0SPEED_RWRDY_10VT	(0xb)	/* 10VTClock */
#define		BCUIO0SPEED_RWRDY_9VT	(0xa)	/* 9VTClock */
#define		BCUIO0SPEED_RWRDY_8VT	(0x9)	/* 8VTClock */
#define		BCUIO0SPEED_RWRDY_7VT	(0x8)	/* 7VTClock */
#define		BCUIO0SPEED_RWRDY_6VT	(0x7)	/* 6VTClock */
#define		BCUIO0SPEED_RWRDY_5VT	(0x6)	/* 5VTClock */
#define		BCUIO0SPEED_RWRDY_4VT	(0x5)	/* 4VTClock */
#define		BCUIO0SPEED_RWRDY_3VT	(0x4)	/* 3VTClock */
#define		BCUIO0SPEED_RWRDY_2VT	(0x3)	/* 2VTClock */
#define		BCUIO0SPEED_RWRDY_1VT	(0x2)	/* 1VTClock */
#define		BCUIO0SPEED_RWRDY_0VT	(0x1)	/* 0VTClock */
#define		BCUIO0SPEED_RWRDY_M1VT	(0x0)	/* -1VTClock */

#define		BCUIO0SPEED_CSRW	(0xf<<0)	/* IORDY-R/W time */
#define		BCUIO0SPEED_CSRW_16VT	(0xf)	/* 16VTClock */
#define		BCUIO0SPEED_CSRW_15VT	(0xe)	/* 15VTClock */
#define		BCUIO0SPEED_CSRW_14VT	(0xd)	/* 14VTClock */
#define		BCUIO0SPEED_CSRW_13VT	(0xc)	/* 13VTClock */
#define		BCUIO0SPEED_CSRW_12VT	(0xb)	/* 12VTClock */
#define		BCUIO0SPEED_CSRW_11VT	(0xa)	/* 11VTClock */
#define		BCUIO0SPEED_CSRW_10VT	(0x9)	/* 10VTClock */
#define		BCUIO0SPEED_CSRW_9VT	(0x8)	/* 9VTClock */
#define		BCUIO0SPEED_CSRW_8VT	(0x7)	/* 8VTClock */
#define		BCUIO0SPEED_CSRW_7VT	(0x6)	/* 7VTClock */
#define		BCUIO0SPEED_CSRW_6VT	(0x5)	/* 6VTClock */
#define		BCUIO0SPEED_CSRW_5VT	(0x4)	/* 5VTClock */
#define		BCUIO0SPEED_CSRW_4VT	(0x3)	/* 4VTClock */
#define		BCUIO0SPEED_CSRW_3VT	(0x2)	/* 3VTClock */
#define		BCUIO0SPEED_CSRW_2VT	(0x1)	/* 2VTClock */
#define		BCUIO0SPEED_CSRW_1VT	(0x0)	/* 1VTClock */

#define BCUBCLCNT_REG_W		0x008	/* BCU CPU Restrain Disable Count Register (= 4101) */

#define BCUIO1SPEED_REG_W	0x00A	/* BCU IO1 Speed Register (=4122) */
#define		BCUIO1SPEED_RWCS	(0x3<<12)	/* R/W - CS time */
#define		BCUIO1SPEED_RWCS_5VT	(0x3<<12)	/* 5VTClock */
#define		BCUIO1SPEED_RWCS_4VT	(0x2<<12)	/* 4VTClock */
#define		BCUIO1SPEED_RWCS_3VT	(0x1<<12)	/* 3VTClock */
#define		BCUIO1SPEED_RWCS_2VT	(0x0<<12)	/* 2VTClock */

#define		BCUIO1SPEED_RDYRW	(0xf<<8)	/* IORDY-R/W time */
#define		BCUIO1SPEED_RDYRW_18VT	(0xf)	/* 18VTClock */
#define		BCUIO1SPEED_RDYRW_17VT	(0xe)	/* 17VTClock */
#define		BCUIO1SPEED_RDYRW_16VT	(0xd)	/* 16VTClock */
#define		BCUIO1SPEED_RDYRW_15VT	(0xc)	/* 15VTClock */
#define		BCUIO1SPEED_RDYRW_14VT	(0xb)	/* 14VTClock */
#define		BCUIO1SPEED_RDYRW_13VT	(0xa)	/* 13VTClock */
#define		BCUIO1SPEED_RDYRW_12VT	(0x9)	/* 12VTClock */
#define		BCUIO1SPEED_RDYRW_11VT	(0x8)	/* 11VTClock */
#define		BCUIO1SPEED_RDYRW_10VT	(0x7)	/* 10VTClock */
#define		BCUIO1SPEED_RDYRW_9VT	(0x6)	/* 9VTClock */
#define		BCUIO1SPEED_RDYRW_8VT	(0x5)	/* 8VTClock */
#define		BCUIO1SPEED_RDYRW_7VT	(0x4)	/* 7VTClock */
#define		BCUIO1SPEED_RDYRW_6VT	(0x3)	/* 6VTClock */
#define		BCUIO1SPEED_RDYRW_5VT	(0x2)	/* 5VTClock */
#define		BCUIO1SPEED_RDYRW_4VT	(0x1)	/* 4VTClock */
#define		BCUIO1SPEED_RDYRW_3VT	(0x0)	/* 3VTClock */

#define		BCUIO1SPEED_RWRDY	(0xf<<4)	/* R/W-IORDY time */
#define		BCUIO1SPEED_RWRDY_14VT	(0xf)	/* 14VTClock */
#define		BCUIO1SPEED_RWRDY_13VT	(0xe)	/* 13VTClock */
#define		BCUIO1SPEED_RWRDY_12VT	(0xd)	/* 12VTClock */
#define		BCUIO1SPEED_RWRDY_11VT	(0xc)	/* 11VTClock */
#define		BCUIO1SPEED_RWRDY_10VT	(0xb)	/* 10VTClock */
#define		BCUIO1SPEED_RWRDY_9VT	(0xa)	/* 9VTClock */
#define		BCUIO1SPEED_RWRDY_8VT	(0x9)	/* 8VTClock */
#define		BCUIO1SPEED_RWRDY_7VT	(0x8)	/* 7VTClock */
#define		BCUIO1SPEED_RWRDY_6VT	(0x7)	/* 6VTClock */
#define		BCUIO1SPEED_RWRDY_5VT	(0x6)	/* 5VTClock */
#define		BCUIO1SPEED_RWRDY_4VT	(0x5)	/* 4VTClock */
#define		BCUIO1SPEED_RWRDY_3VT	(0x4)	/* 3VTClock */
#define		BCUIO1SPEED_RWRDY_2VT	(0x3)	/* 2VTClock */
#define		BCUIO1SPEED_RWRDY_1VT	(0x2)	/* 1VTClock */
#define		BCUIO1SPEED_RWRDY_0VT	(0x1)	/* 0VTClock */
#define		BCUIO1SPEED_RWRDY_M1VT	(0x0)	/* -1VTClock */

#define		BCUIO1SPEED_CSRW	(0xf<<0)	/* IORDY-R/W time */
#define		BCUIO1SPEED_CSRW_16VT	(0xf)	/* 16VTClock */
#define		BCUIO1SPEED_CSRW_15VT	(0xe)	/* 15VTClock */
#define		BCUIO1SPEED_CSRW_14VT	(0xd)	/* 14VTClock */
#define		BCUIO1SPEED_CSRW_13VT	(0xc)	/* 13VTClock */
#define		BCUIO1SPEED_CSRW_12VT	(0xb)	/* 12VTClock */
#define		BCUIO1SPEED_CSRW_11VT	(0xa)	/* 11VTClock */
#define		BCUIO1SPEED_CSRW_10VT	(0x9)	/* 10VTClock */
#define		BCUIO1SPEED_CSRW_9VT	(0x8)	/* 9VTClock */
#define		BCUIO1SPEED_CSRW_8VT	(0x7)	/* 8VTClock */
#define		BCUIO1SPEED_CSRW_7VT	(0x6)	/* 7VTClock */
#define		BCUIO1SPEED_CSRW_6VT	(0x5)	/* 6VTClock */
#define		BCUIO1SPEED_CSRW_5VT	(0x4)	/* 5VTClock */
#define		BCUIO1SPEED_CSRW_4VT	(0x3)	/* 4VTClock */
#define		BCUIO1SPEED_CSRW_3VT	(0x2)	/* 3VTClock */
#define		BCUIO1SPEED_CSRW_2VT	(0x1)	/* 2VTClock */
#define		BCUIO1SPEED_CSRW_1VT	(0x0)	/* 1VTClock */

#define	BCUSPEED_REG_W		0x00A	/* BCU Access Cycle Change Register (4121>=4102)*/

#define		BCUSPD_WPROM		(0x3<<12)	/* Page ROM access speed */
#define		BCUSPD_WPROMRFU		(0x3<<12)	/* RFU */
#define		BCUSPD_WPROM1T		(0x2<<12)	/* 1TClock */
#define		BCUSPD_WPROM2T		(0x1<<12)	/* 2TClock */
#define		BCUSPD_WPROM3T		(0x0<<12)	/* 3TClock */

#define		BCUSPD_WLCDM		(0x7<<8)	/* access speed 0x0a000000-0affffff */

		/* BCUCNT1_ISAMLCD == BCUCNT1_LCD */
#define		BCUSPD_WLCDRFU		(0x7<<8)	/* LCD RFU */
#define		BCUSPD_WLCDRFU1		(0x6<<8)	/* LCD RFU */
#define		BCUSPD_WLCDRFU2		(0x5<<8)	/* LCD RFU */
#define		BCUSPD_WLCDRFU3		(0x4<<8)	/* LCD RFU */
#define		BCUSPD_WLCD2T		(0x3<<8)	/* LCD 2TClock */
#define		BCUSPD_WLCD4T		(0x2<<8)	/* LCD 4TClock */
#define		BCUSPD_WLCD6T		(0x1<<8)	/* LCD 6TClock */
#define		BCUSPD_WLCD8T		(0x0<<8)	/* LCD 8TClock */
		/* BCUCNT1_ISAMLCD == BCUCNT1_ISAM */
#define		BCUSPD_ISAM1T		(0x7<<8)	/* ISAM 1TClock */
#define		BCUSPD_ISAM2T		(0x6<<8)	/* ISAM 2TClock */
#define		BCUSPD_ISAM3T		(0x5<<8)	/* ISAM 3TClock */
#define		BCUSPD_ISAM4T		(0x4<<8)	/* ISAM 4TClock */
#define		BCUSPD_ISAM5T		(0x3<<8)	/* ISAM 5TClock */
#define		BCUSPD_ISAM6T		(0x2<<8)	/* ISAM 6TClock */
#define		BCUSPD_ISAM7T		(0x1<<8)	/* ISAM 7TClock */
#define		BCUSPD_ISAM8T		(0x0<<8)	/* ISAM 8TClock */

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


#define	BCUERRST_REG_W		0x00C	/* BCU BUS ERROR Status Register (4121>=4102)*/

#define		BCUERRST_BUSERRMASK	(1)		/* Bus error, clear to 0 when 1 is written */
#define		BCUERRST_BUSERR		(1)		/* Bus error */
#define		BCUERRST_BUSNORM	(0)		/* Normal */

#define	BCU81SPEED_REG_W	0x00C	/* BCU Access Cycle Change Register (=4181)*/

#define		BCU81SPD_WPROM		(0x7<<12)	/* Page ROM access speed */
#define		BCU81SPD_WPROM8T	(0x7<<12)	/* 8TClock */
#define		BCU81SPD_WPROM7T	(0x6<<12)	/* 7TClock */
#define		BCU81SPD_WPROM6T	(0x5<<12)	/* 6TClock */
#define		BCU81SPD_WPROM5T	(0x4<<12)	/* 5TClock */
#define		BCU81SPD_WPROM4T	(0x3<<12)	/* 4TClock */
#define		BCU81SPD_WPROM3T	(0x2<<12)	/* 3TClock */
#define		BCU81SPD_WPROM2T	(0x1<<12)	/* 2TClock */
#define		BCU81SPD_WPROM1T	(0x0<<12)	/* 1TClock */

#define		BCU81SPD_WROMA		(0xf<<0)	/* System Bus Access Speed */
#define		BCU81SPD_WROMA16T	(0xf<<0)	/* 16TClock */
#define		BCU81SPD_WROMA15T	(0xe<<0)	/* 15TClock */
#define		BCU81SPD_WROMA14T	(0xd<<0)	/* 14TClock */
#define		BCU81SPD_WROMA13T	(0xc<<0)	/* 13TClock */
#define		BCU81SPD_WROMA12T	(0xb<<0)	/* 12TClock */
#define		BCU81SPD_WROMA11T	(0xa<<0)	/* 11TClock */
#define		BCU81SPD_WROMA10T	(0x9<<0)	/* 10TClock */
#define		BCU81SPD_WROMA9T	(0x8<<0)	/* 9TClock */
#define		BCU81SPD_WROMA8T	(0x7<<0)	/* 8TClock */
#define		BCU81SPD_WROMA7T	(0x6<<0)	/* 7TClock */
#define		BCU81SPD_WROMA6T	(0x5<<0)	/* 6TClock */
#define		BCU81SPD_WROMA5T	(0x4<<0)	/* 5TClock */
#define		BCU81SPD_WROMA4T	(0x3<<0)	/* 4TClock */
#define		BCU81SPD_WROMA3T	(0x2<<0)	/* 3TClock */
#define		BCU81SPD_WROMA2T	(0x1<<0)	/* 2TClock */
#define		BCU81SPD_WROMA1T	(0x0<<0)	/* 1TClock */

#define	BCURFCNT_REG_W		0x00E	/* BCU Refresh Control Register(4121>=4102) */
#define	BCU81RFCNT_REG_W	0x010	/* BCU Refresh Control Register(=4181) */

#define		BCURFCNT_MASK		0x3fff		/* refresh interval MASK */

#define	BCUREVID_REG_W		0x010	/* BCU Revision ID Register (4122>=4101)*/
#define	BCU81REVID_REG_W	0x014	/* BCU Revision ID Register (=4181)*/

#define		BCUREVID_RIDMASK	(0xf<<12)	/* Revision ID */
#define		BCUREVID_RIDSHFT	(12)		/* Revision ID */
#define		BCUREVID_RID_4122	(0x4)		/* VR4122 */
#define		BCUREVID_RID_4121	(0x3)		/* VR4121 */
#define		BCUREVID_RID_4111	(0x2)		/* VR4111 */
#define		BCUREVID_RID_4102	(0x1)		/* VR4102 */
#define		BCUREVID_RID_4101	(0x0)		/* VR4101 */
#define		BCUREVID_RID_4181	(0x0)		/* VR4181 conflict VR4101 */
#define		BCUREVID_FIXRID_OFF	(0x10)		/* conflict offset */
#define		BCUREVID_FIXRID_4181	(0x10)		/* VR4181 for kernel */

#define		BCUREVID_MJREVMASK	(0xf<<8)	/* Major Revision */
#define		BCUREVID_MJREVSHFT	(8)		/* Major Revision */

#define		BCUREVID_MNREVMASK	(0xf)		/* Minor Revision */
#define		BCUREVID_MNREVSHFT	(0)		/* Minor Revision */


#define	BCUREFCOUNT_REG_W	0x012	/* BCU Refresh Count Register (>= 4102) */

#define		BCUREFCOUNT_MASK	0x3fff	/* refresh 	count MASK */


#define	BCUCLKSPEED_REG_W	0x014	/* Clock Speed Register (>= 4102) */
#define	BCU81CLKSPEED_REG_W	0x018	/* Clock Speed Register (= 4181) */

#define		BCUCLKSPEED_DIVT2B	(1<<15)		/* (= 4102, 4111) */
#define		BCUCLKSPEED_DIVT3B	(1<<14)		/* (= 4111) */
#define		BCUCLKSPEED_DIVT4B	(1<<13)		/* (= 4111) */

#define		BCUCLKSPEED_DIVTMASK	(0xf<<12)	/* (= 4121) */
#define			BCUCLKSPEED_DIVT3	0x3
#define			BCUCLKSPEED_DIVT4	0x4
#define			BCUCLKSPEED_DIVT5	0x5
#define			BCUCLKSPEED_DIVT6	0x6
#define		BCUCLKSPEED_DIVTSHFT	(12)		

#define		BCUCLKSPEED_TDIVMODE	(0x1<<12)	/* (= 4122) */
#define			BCUCLKSPEED_TDIV4	0x1
#define			BCUCLKSPEED_TDIV2	0x0
#define		BCUCLKSPEED_TDIVSHFT	(12)		

#define		BCU81CLKSPEED_DIVTMASK	(0x7<<12)	/* (=4181) */
#define			BCU81CLKSPEED_DIVT1	0x7
#define			BCU81CLKSPEED_DIVT2	0x3
#define			BCU81CLKSPEED_DIVT3	0x5
#define			BCU81CLKSPEED_DIVT4	0x6
#define		BCU81CLKSPEED_DIVTSHFT	(12)		

#define		BCUCLKSPEED_DIVVTMASK	(0xf<<8)	/* (= 4121) */
#define			BCUCLKSPEED_DIVVT1	0x1
#define			BCUCLKSPEED_DIVVT2	0x2
#define			BCUCLKSPEED_DIVVT3	0x3
#define			BCUCLKSPEED_DIVVT4	0x4
#define			BCUCLKSPEED_DIVVT5	0x5
#define			BCUCLKSPEED_DIVVT6	0x6
#define			BCUCLKSPEED_DIVVT1_5	0x9
#define			BCUCLKSPEED_DIVVT2_5	0xa
#define		BCUCLKSPEED_DIVVTSHFT	(8)		

#define		BCUCLKSPEED_VTDIVMODE	(0x7<<8)	/* (= 4122) */
#define			BCUCLKSPEED_VTDIV6	0x6
#define			BCUCLKSPEED_VTDIVT5	0x5
#define			BCUCLKSPEED_VTDIVT4	0x4
#define			BCUCLKSPEED_VTDIVT3	0x3
#define			BCUCLKSPEED_VTDIVT2	0x2
#define			BCUCLKSPEED_VTDIVT1	0x1
#define		BCUCLKSPEED_VTDIVSHFT	(8)		

#define		BCUCLKSPEED_CLKSPMASK	(0x1f)		/* calculate for Clock */
#define		BCUCLKSPEED_CLKSPSHFT	(0)		

#define	BCUCNT3_REG_W		0x016	/* BCU Control Register 3 (>= 4111) */

#define		BCUCNT3_EXTROMMASK	(1<<15)		/* ROM SIZE (4111,4121)*/ 
#define		BCUCNT3_EXTROM64M	(1<<15)		/* 64Mbit DRAM */
#define		BCUCNT3_EXTROM32M	(0<<15)		/* 32Mbit DRAM */

#define		BCUCNT3_EXTDRAMMASK	(1<<14)		/* DRAM SIZE (4111,4121)*/
#define		BCUCNT3_EXTDRAM64M	(1<<14)		/* 64Mbit DRAM */
#define		BCUCNT3_EXTDRAM16M	(0<<14)		/* 16Mbit DRAM */

#define		BCUCNT3_EXTROMCS	(0x3<<12)	/* Bank3,2 */
#define		BCUCNT3_ROMROM		(0x3<<12)	/* Bank3 ROM ,2 ROM  */
#define		BCUCNT3_ROMRAM		(0x2<<12)	/* Bank3 ROM ,2 RAM  */
#define		BCUCNT3_RAMRAM		(0x0<<12)	/* Bank3 RAM ,2 RAM  */

#define		BCUCNT3_EXTMEM		(1<<11)		/* EXT MEN enable (4111,4121)*/
#define		BCUCNT3_EXTMEMEN	(1<<11)		/* EXT MEN enable */
#define		BCUCNT3_EXTMEMDS	(0<<11)		/* EXT MEN disable */

#define		BCUCNT3_LCDSIZE		(1<<7)		/* LCD bus size */
#define		BCUCNT3_LCD32		(1<<7)		/* LCD bus 32bit */
#define		BCUCNT3_LCD16		(0<<7)		/* LCD bus 16bit */

#define		BCUCNT3_SYSDIREN	(1<<3)		/* SYSDIR or GPIO6(=4122)*/
#define		BCUCNT3_SYSDIR		(1<<3)		/* SYSDIR */
#define		BCUCNT3_GPIO6		(0<<3)		/* GPIO6 */

#define		BCUCNT3_LCDSEL1		(1<<1)		/* 0xc00-0xdff area buffer (=4122)*/
#define		BCUCNT3_LCDSEL1_NOBUF	(1<<1)		/* nobuffer */
#define		BCUCNT3_LCDSEL1_BUF	(0<<1)		/* buffer */

#define		BCUCNT3_LCDSEL0		(1<<1)		/* 0xa00-0xbff area buffer (=4122)*/
#define		BCUCNT3_LCDSEL0_NOBUF	(1<<1)		/* nobuffer */
#define		BCUCNT3_LCDSEL0_BUF	(0<<1)		/* buffer */

/* END bcureg.h */
