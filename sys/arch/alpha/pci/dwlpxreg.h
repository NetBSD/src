/* $NetBSD: dwlpxreg.h,v 1.4 1997/04/16 22:20:52 mjacob Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Taken from combinations of:
 *
 *	``DWLPA and DWLPB PCI Adapter Technical Manual,
 *	  Order Number: EK-DWLPX-TM.A01''
 *
 *  and
 *
 *	``AlphaServer 8200/8400 System Technical Manual,
 *	  Order Number EK-T8030-TM. A01''
 */

#define	REGVAL(r)	(*(int32_t *)ALPHA_PHYS_TO_K0SEG(r))

/*
 * There are (potentially) 4 I/O hoses, and there are three
 * (electrically distinct) PCI busses per DWLPX (which appear
 * as one logical PCI bus).
 *
 * A CPU to PCI Address Mapping looks (roughly) like this:
 *
 *  39 38........36 35.34 33.....32 31....................5 4.........3 2...0
 *  --------------------------------------------------------------------------
 *  |1| I/O NodeID |Hose#|PCI Space|Byte Aligned I/O <26:0>|Byte Length|0 0 0|
 *  --------------------------------------------------------------------------
 *
 * I/O Node is the TLSB Node ID minus 4. Don't ask.
 */

#define	NHPC	3

/*
 * Address Space Cookies
 *
 * (lacking I/O Node ID and Hose Numbers)
 */

#define	DWLPX_PCI_DENSE		0x000000000LL
#define	DWLPX_PCI_SPARSE	0x100000000LL
#define	DWLPX_PCI_IOSPACE	0x200000000LL
#define	DWLPX_PCI_CONF		0x300000000LL

/*
 * PCIA Interface Adapter Register Addresses (Offsets from Node Address)
 *
 *
 * Addresses are for Hose #0, PCI bus #0. Macros below will offset
 * per bus. I/O Hose and TLSB Node I/D offsets must be added separately.
 */

#define	_PCIA_CTL	0x380000000LL	/* PCI 0 Bus Control */
#define	_PCIA_MRETRY	0x380000080LL	/* PCI 0 Master Retry Limit */
#define	_PCIA_GPR	0x380000100LL	/* PCI 0 General Purpose */
#define	_PCIA_ERR	0x380000180LL	/* PCI 0 Error Summary */
#define	_PCIA_FADR	0x380000200LL	/* PCI 0 Failing Address */
#define	_PCIA_IMASK	0x380000280LL	/* PCI 0 Interrupt Mask */
#define	_PCIA_DIAG	0x380000300LL	/* PCI 0 Diagnostic  */
#define	_PCIA_IPEND	0x380000380LL	/* PCI 0 Interrupt Pending */
#define	_PCIA_IPROG	0x380000400LL	/* PCI 0 Interrupt in Progress */
#define	_PCIA_WMASK_A	0x380000480LL	/* PCI 0 Window Mask A */
#define	_PCIA_WBASE_A	0x380000500LL	/* PCI 0 Window Base A */
#define	_PCIA_TBASE_A	0x380000580LL	/* PCI 0 Window Translated Base A */
#define	_PCIA_WMASK_B	0x380000600LL	/* PCI 0 Window Mask B */
#define	_PCIA_WBASE_B	0x380000680LL	/* PCI 0 Window Base B */
#define	_PCIA_TBASE_B	0x380000700LL	/* PCI 0 Window Translated Base B */
#define	_PCIA_WMASK_C	0x380000780LL	/* PCI 0 Window Mask C */
#define	_PCIA_WBASE_C	0x380000800LL	/* PCI 0 Window Base C */
#define	_PCIA_TBASE_C	0x380000880LL	/* PCI 0 Window Translated Base C */
#define	_PCIA_ERRVEC	0x380000900LL	/* PCI 0 Error Interrupt Vector */
#define	_PCIA_DEVVEC	0x380001000LL	/* PCI 0 Device Interrupt Vector */


#define	PCIA_CTL(hpc)		(_PCIA_CTL	+ (0x200000 * (hpc)))
#define	PCIA_MRETRY(hpc)	(_PCIA_MRETRY	+ (0x200000 * (hpc)))
#define	PCIA_GPR(hpc)		(_PCIA_GPR	+ (0x200000 * (hpc)))
#define	PCIA_ERR(hpc)		(_PCIA_ERR	+ (0x200000 * (hpc)))
#define	PCIA_FADR(hpc)		(_PCIA_FADR	+ (0x200000 * (hpc)))
#define	PCIA_IMASK(hpc)		(_PCIA_IMASK	+ (0x200000 * (hpc)))
#define	PCIA_DIAG(hpc)		(_PCIA_DIAG	+ (0x200000 * (hpc)))
#define	PCIA_IPEND(hpc)		(_PCIA_IPEND	+ (0x200000 * (hpc)))
#define	PCIA_IPROG(hpc)		(_PCIA_IPROG	+ (0x200000 * (hpc)))
#define	PCIA_WMASK_A(hpc)	(_PCIA_WMASK_A	+ (0x200000 * (hpc)))
#define	PCIA_WBASE_A(hpc)	(_PCIA_WBASE_A	+ (0x200000 * (hpc)))
#define	PCIA_TBASE_A(hpc)	(_PCIA_TBASE_A	+ (0x200000 * (hpc)))
#define	PCIA_WMASK_B(hpc)	(_PCIA_WMASK_B	+ (0x200000 * (hpc)))
#define	PCIA_WBASE_B(hpc)	(_PCIA_WBASE_B	+ (0x200000 * (hpc)))
#define	PCIA_TBASE_B(hpc)	(_PCIA_TBASE_B	+ (0x200000 * (hpc)))
#define	PCIA_WMASK_C(hpc)	(_PCIA_WMASK_C	+ (0x200000 * (hpc)))
#define	PCIA_WBASE_C(hpc)	(_PCIA_WBASE_C	+ (0x200000 * (hpc)))
#define	PCIA_TBASE_C(hpc)	(_PCIA_TBASE_C	+ (0x200000 * (hpc)))
#define	PCIA_ERRVEC(hpc)	(_PCIA_ERRVEC	+ (0x200000 * (hpc)))

#define	PCIA_DEVVEC(hpc, subslot, ipin)	\
 (_PCIA_DEVVEC + (0x200000 * (hpc)) + ((subslot) * 0x200) + ((ipin-1) * 0x80))

#define	PCIA_SCYCLE	0x380002000LL	/* PCI Special Cycle */
#define	PCIA_IACK	0x380002080LL	/* PCI Interrupt Acknowledge */

#define	PCIA_PRESENT	0x380800000LL	/* PCI Slot Present */
#define	PCIA_TBIT	0x380A00000LL	/* PCI TBIT */
#define	PCIA_MCTL	0x380C00000LL	/* PCI Module Control */
#define	PCIA_IBR	0x380E00000LL	/* PCI Information Base Repair */
