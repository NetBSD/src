/*	$NetBSD: pciide_cmd_reg.h,v 1.2 1998/10/12 16:09:21 bouyer Exp $	*/

/*
 * Copyright (c) 1998 Manuel Bouyer.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * Registers definitions for CMD Technologies's PCI 064x IDE controllers.
 * Available from http://www.cmd.com/
 */

/* Configuration and drive 0 setup */
#define CMD_CONF_CTRL0 0x50
#define CMD_CONF_REV_MASK	0x00000003
#define CMD_CONF_DRV0_INTR	0x00000004
#define CMD_CONF_DEVID		0x00000018
#define CMD_CONF_VESAPRT	0x00000020
#define CMD_CONF_DSA1		0x00000040
#define CMD_CONF_DSA0		0x00000080

#define CMD_CONF_HR_FIFO	0x00000100
#define CMD_CONF_HW_FIFO	0x00000200
#define CMD_CONF_DEVSEL		0x00000400
#define CMD_CONF_2PORT		0x00000800
#define CMD_CONF_PAR		0x00001000
#define CMD_CONF_HW_HLD		0x00002000
#define CMD_CONF_DRV0_RAHEAD	0x00004000
#define CMD_CONF_DRV1_RAHEAD	0x00008000

#define CMD_CONF_CB_TIM_OFF	16
#define CMD_CONF_CB_REC_OFF	20

#define CMD_DRV0_ADDRSET_OFF	30


