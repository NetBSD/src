/* $NetBSD: kn230.h,v 1.1.4.2 1999/11/12 11:07:19 nisimura Exp $ */

/*
 * Copyright (c) 1997,1998 Jonathan Stone.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jonathan Stone for
 *	the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*
 * Physical addresses of baseboard devices and registers on the
 * the DECsystem 5100 motherboard (also known as the kn230). 
 *
 * The only options on the kn230 are two slots for daughterboards.
 * Each slot can contain an extra four-port dc708x DZ11 clone chip.
 * Most baseborad chips are the same hardware and at the same
 * address as the DECstation 3100 (kn01), except:
 *
 *	The kn230 has more devices than the 3100, so instead
 *	of hardwiring each device to a CPU interrupt line,
 *	the kn230 provides an interrupt-request register (ICSR).
 *	Devices are mapped onto CPU interrupt lines as below:
 *
 *	    hardint 5:	fpu
 *	    hardint 4:	reset switch
 *	    hardint 3:	memory error (write to nonexistent memory)
 *	    hardint 2:	clock
 *	    hardint 1:	sii, lance
 *	    hardint 0:	baseboard DZ, I/O option cards
 *
 *	the ICSR only indicates which devices are requesting interrupts.
 *	there is no interrupt mask register.
 *
 *	There is no framebuffer, pcc (cursor), colormap or vdac on a 5100.
 */

#define KN230_SYS_LANCE		0x18000000	/* LANCE chip */
#define KN230_SYS_LANCE_B_START 0x19000000	/* 64 KB LANCE buffer */
#define KN230_SYS_LANCE_B_END	0x19010000
#define KN230_SYS_SII		0x1a000000	/* SII SCSI chip */
#define KN230_SYS_SII_B_START	0x1b000000	/* 128 KB SCSI buffer */
#define KN230_SYS_SII_B_END	0x1b020000
#define KN230_SYS_DZ0		0x1c000000	/* baseboard DZ serial chip */
#define KN230_SYS_CLOCK		0x1d000000	/* mc146818 rtc chip */
#define KN230_SYS_ICSR		0x1e000000	/* system control register */

/*
 * Interrupt-request bitmasks in the low-order 16 bits of the CSR.
 * If a bit is set, the corresponding device has an interrupt condition.
 * There is no equivalent per-device interrupt masking register.
 * (note: these were LED control bits in the kn01).
 */
#define KN230_CSR_INTR_RESET	0x00004000	/* reset button */
#define KN230_CSR_INTR_WMERR	0x00002000	/* badaddr() or write error */
#define KN230_CSR_INTR_LANCE	0x00001000	/* LANCE interrupt */
#define KN230_CSR_INTR_SII	0x00000800	/* SCSI interrupt */
#define KN230_CSR_INTR_OPT1	0x00000400	/* second option DZ */
#define KN230_CSR_INTR_OPT0	0x00000200	/* first option DZ */
#define KN230_CSR_INTR_DZ0	0x00000100	/* baseboard DZ */

/*
 * kn230 LED control register. 
 * Writing a 1 bit to any bit turns off the corresponding LED.
 * low bits are or'ed to control registers which have side effects.
 *
 * This is sort of like the 3100 CSR only at a different address, and
 * with the same external-logic hack as the 5000/200 to enable 38.4
 * baud mode on the baseboard DZ chip.
 */
#define KN230_SYS_CTL_LED	0x14000000	/* LED register */
#define KN230_LED7		0x00008000	/* no side effect */
#define KN230_LED6		0x00004000	/* no side effect */
#define KN230_LED5		0x00002000	/* no side effect */
#define KN230_LED4		0x00001000	/* no side effect */

#define KN230_LED3		0x00000800	/* Turns off led 3.
						 * Also disables main memory.
						 * enables writes to EEPROM.
						 */

#define KN230_LED2		0x00000400	/* Turns off led 2.
						 * 0:  forces DZ to run at
						 * 38.4	 when 19.2 is selected 
						 * 1:  is 19.2 means 19.2.
						 */

#define KN230_LED1		0x00000200	/* read-only
						 *  0:	8M memory in bank 1
						 *  1: 32M in bank 1
						 */
#define KN230_LED0		0x00000100

/*
 * Option slot addresses.
 * dc7084 registers appear here, if present.
 */
#define KN230_SYS_DZ1		0x15000000	/* DZ1 Control and status */
#define KN230_SYS_DZ2		0x15200000	/* DZ2 Control and status */

/*
 * Write error address.
 * Same address as the KN01_SYS_MERR, but with a presence bit for a
 * Prestoserve option and a secure console-mode the low-order bits.
 */
#define KN230_SYS_WEAR		0x17000000	/* Write-error address reg */
#define KN230_WEAR_OPTIONMASK	0x00000001	/* 1 if no card present */
#define KN230_WEAR_OPTION_FALSE KN230_WEAR_OPTIONMASK 
#define KN230_WEAR_OPTION_TRUE	0

#define KN230_WEAR_SECUREMASK	0x00000002	/* "1": system is insecure */
#define KN230_WEAR_INSECURE	KN230_WEAR_SECUREMASK
#define KN230_WEAR_SECURE	0

/*
 * various leftover stuff....
 */

/*
 *  Option ID register. Indicates whether options are present.
 */
#define KN230_SYS_OID		0x1f00020c	/* option slot ID register */
#define KN230_OID_MASK		0x000000ff	/* ID number mask */
#define KN230_ERRCNT_MASK	0x0000ff00	/* hardware failure count */


#define KN230_SYS_PASSWD	0x1f000244	/* password location */

/*
 * NVRAM state defintions.  
 * Used under Ultrix for PrestoServe.
 */
#define KN230_SYS_NVRAM_DIAG	0x1f000300	/* NVRAM diagnostic register */
#define KN230_NVRAM_PRESENT	0x00000001
#define KN230_NVRAM_TESTFAIL_RO 0x00000002
#define KN230_NVRAM_TESTFAIL_RW 0x00000004
#define KN230_NVRAM_FAILURE	0x00000008	/* ran out of power anyway? */
#define KN230_NVRAM_SIZEMASK	0x000000f0

#define KN230_SYS_NVRAM_ADDR	0x1f000304	 /* holds addr of NVRAM bank */
/*
 * NVRAM has separete control and status registers for each of
 * the two motherboard SIMM banks (even and odd),
 * located at offsets from the value at value at SYS_NVRAM_ADDR.
 */
# define KN230_SYS_NVRAM_EVENBNK_STATUS_OFFSET	(0x200000)
# define KN230_SYS_NVRAM_ODDBNK_STATUS_OFFSET	(0x200000 +4)
# define KN230_SYS_NVRAM_EVENBNK_CONTROL_OFFSET (-0x200000)
# define KN230_SYS_NVRAM_ODDNK_STATUS_OFFSET	(-0x200000+4)

/* flags in nvram per-bank status register */
#define KN230_NVRAM_BATFAIL	0x00000001	 /* battery failure */
#define KN230_NVRAM_BATKILL	0x00000002	 /* battery kill */
/*
 * To enable the battery,  write 0x00  to each nvram control reg.
 * To disable the battery, write the sequence
 *    0x01, 0x01,  0x00, 0x00,	0x01 
 * to both per-bank control registers (do banks in parallel, not in sequence).
 */
