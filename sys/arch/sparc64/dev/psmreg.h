/*
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Ported from Tadpole Solaris sources by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*
 * Copyright (c) 2002 by Tadpole Technology
 */

#ifndef PSM_H
#define PSM_H

#define PSM_PRDL	0x00	/* Posted read data low byte */
#define PSM_PRDU	0x01	/* Posted read data high byte */
#define PSM_ISR		0x02	/* Interrupt status register */
#define PSM_STAT	0x03	/* Status register */
#define PSM_PSR0	0x04	/* Programmable status register #0 */
#define PSM_PSR1	0x05	/* Programmable status register #1 */
#define PSM_PSR2	0x06	/* Programmable status register #2 */
#define PSM_PSR3	0x07	/* Programmable status register #3 */

#define PSM_PWDL	0x00	/* Posted write data low byte */
#define PSM_PWDU	0x01	/* Posted write data high byte */
#define PSM_IAR		0x02	/* Indirect access register */
#define PSM_CMR		0x03	/* Command mode register */
#define PSM_RSV1	0x04	/* Reserved */
#define PSM_ICR		0x05	/* Interrupt clear register */
#define PSM_RSV2	0x06	/* Reserved */
#define PSM_MCR		0x07	/* Master command register */

/* Interrupt status register defenitions */

#define PSM_ISR_PO	0x01	/* Power switch activated */
#define PSM_ISR_DK	0x02	/* System has been docked */
#define PSM_ISR_UDK	0x04	/* System has been un-docked */
#define PSM_ISR_LIDO	0x08	/* Transition to clamshell closed */
#define PSM_ISR_LIDC	0x10	/* Transition to clamshell open */
#define PSM_ISR_TMP	0x20	/* Over temperature condition detected */
#define PSM_ISR_BCC	0x40	/* Battery configuration changed */
#define PSM_ISR_RPD	0x80	/* Request to power down */

/* Status registert defenitions */

#define PSM_STAT_AC	0x01	/* Operating under AC power */
#define PSM_STAT_OVT	0x02	/* Over temperature condition */
#define PSM_STAT_UN1	0x04	/* Unused */
#define PSM_STAT_UN2	0x08	/* Unused */
#define PSM_STAT_ERR	0x10	/* Hardware error occurred */
#define PSM_STAT_MCR	0x20	/* Master Command Register busy */
#define PSM_STAT_WBF	0x40	/* Write buffer full */
#define PSM_STAT_RDA	0x80	/* Read data available */

/* Command Mode Register defenitions */

#define PSM_CMR_DATA(m,l,d,ra)	(ra & 0x07) | \
				((d & 0x01) << 3) | \
				((l & 0x01) << 4) | \
				((m & 0x07) << 5)

#define PSM_MODE_SYSCFG	0x00	/* System configuration mode */
#define PSM_MODE_BQRW	0x01	/* Read write battery fuel gauge */
#define PSM_MODE_BCB	0x02	/* Battery status block control */
#define PSM_MODE_PMPS	0x03	/* Power management policies/status */
#define PSM_MODE_MISC	0x04	/* Misc. control / status registers */
#define PSM_MODE_I2C	0x05	/* Direct I2C control */
#define PSM_MODE_UN1	0x06	/* Unused */
#define PSM_MODE_UN2	0x07	/* Unused */

#define PSM_L_8		0x00
#define PSM_L_16	0x01

#define PSM_D_WR	0x00
#define PSM_D_RD	0x01

/* Master Command Register defenitions */

#define PSM_MCR_NA1	0x01	/* Not available */
#define PSM_MCR_NA2	0x02	/* Not available */
#define PSM_MCR_NA3	0x04	/* Not available */
#define PSM_MCR_AUTO	0x08	/* Enable active battery management */
#define PSM_MCR_SD	0x10	/* Shutdown permission granted */
#define PSM_MCR_MON	0x20	/* Monitor motherboard interrupts/dma */
#define PSM_MCR_OBP	0x40	/* OBP done notification */
#define PSM_MCR_RST	0x80	/* Reset PSMbus interface */

/* Mode dependent registers */

/* Mode 0 - System configuration */

#define PSM_SYSCFG_PSSR0	0x00
#define PSM_SYSCFG_PSCR0	0x01
#define PSM_SYSCFG_PSSR1 	0x02
#define PSM_SYSCFG_PSCR1	0x03
#define PSM_SYSCFG_PSSR2 	0x04
#define PSM_SYSCFG_PSCR2	0x05
#define PSM_SYSCFG_PSSR3 	0x06
#define PSM_SYSCFG_PSCR3	0x07

#define PSM_SYSCFG_PSSR(batt,fgr)	(fgr & 0x1f ) | \
					((batt & 0x07) << 5)

#define PSM_SYSCFG_PSCR(e,lo,ti)	(ti & 0x0f) | \
					((lo & 0x01) << 6) | \
					((e & 0x01) << 7)

/* Mode 1 - Battery fuel gauge read / write */

#define PSM_BQRW_CACHED		0x80
#define PSM_BQRW_REGMASK	0x1f

/* Mode 2 - Battery control block read / write */

#define PSM_BCB_BATC0		0x00
#define PSM_BCB_BATC1		0x01
#define PSM_BCB_BATC2		0x02
#define PSM_BCB_BATC3		0x03
#define PSM_BCB_BATC4		0x04

#define PSM_BCB_CR		0x01	/* Calibration required */
#define PSM_BCB_BCF		0x02	/* Battery control block failure */
#define PSM_BCB_FGF		0x04	/* Fuel gauge failure */
#define PSM_BCB_FULL		0x08	/* Battery is full */
#define	PSM_BCB_CHG		0x10	/* Battery pack charging */
#define PSM_BCB_USE		0x20	/* Battery pack in use */
#define PSM_BCB_E		0x40	/* Battery pack enabled */
#define PSM_BCB_IN		0x80	/* Battery pack in use */

/* Mode 4 - Miscellaneous control/status registers */

#define PSM_MISC_HVER	0x00	/* Hardware version number */
#define	PSM_MISC_FVER	0x01	/* Firmware version number */
#define PSM_MISC_BLITE	0x10	/* Backlight intensity register */
#define PSM_MISC_IMR	0x20	/* Interrupt mask register */
#define PSM_MISC_UPS	0x21	/* UPS battery pack number */
#define PSM_MISC_FMTA	0x30	/* Battery format registers */
#define PSM_MISC_FMTB	0x31	/* Battery format registers */
#define PSM_MISC_FMTC	0x32	/* Battery format registers */
#define PSM_MISC_FMTD	0x33	/* Battery format registers */
#define PSM_MISC_FAN0	0x40	/* Fan control */
#define PSM_MISC_FAN1	0x41	/* Fan control */
#define PSM_MISC_FAN2	0x42	/* Fan control */
#define PSM_MISC_FAN3	0x43	/* Fan control */
#define PSM_MISC_FAN4	0x44	/* Fan control */
#define PSM_MISC_AD0	0x50	/* Processor internal thermal */
#define PSM_MISC_AD1	0x51	/* Processor vicinity thermal */
#define PSM_MISC_AD2	0x52	/* Processor case thermal */
#define PSM_MISC_AD3	0x53	/* Clamshell ambient  thermal */
#define PSM_MISC_AD4	0x54	/* Reserved */
#define PSM_MISC_AD5	0x55	/* Reserved */
#define PSM_MISC_AD6	0x56	/* Reserved */
#define PSM_MISC_AD7	0x57	/* Discharge bus voltage */
#define PSM_MISC_XMON	0x60	/* External monitor */
#define PSM_MISC_PCYCLE	0x70	/* Power cycle */
#define PSM_MISC_ERROR0	0x80
#define PSM_MISC_ERROR1	0x81
#define PSM_MISC_PEM	0x90
#define PSM_MISC_PEMAD0	0xa0
#define PSM_MISC_PEMAD1	0xa1
#define PSM_MISC_PEMAD2	0xa2
#define PSM_MISC_PEMAD3	0xa3

/* Masks */

#define PSM_FAN_MASK	0x1f	/* 0-31 */

/* Interrupt mask register defenitions */

#define PSM_IMR_MBCC	0x40	/* Battery config change interrupt */
#define PSM_IMR_MTMP	0x20	/* Over temp interrupt */
#define PSM_IMR_MLIDC	0x10	/* Lid close interrupt */
#define PSM_IMR_MLIDO	0x08	/* Lid close interrupt */
#define PSM_IMR_MD	0x04	/* Dock/undock interrupts */
#define PSM_IMR_MPS	0x01	/* Master power switch interrupt */

#define PSM_IMR_ALL	PSM_IMR_MBCC|PSM_IMR_MTMP|PSM_IMR_MLIDO|PSM_IMR_MLIDC \
			|PSM_IMR_MD|PSM_IMR_MPS

/* Battery information */

#define PSM_MAX_BATTERIES	1
#define PSM_VBATT		11100	/* 11.1v nominal battery voltage */

#endif /* PSMREG_H */
