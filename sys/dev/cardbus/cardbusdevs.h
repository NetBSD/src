/*	$NetBSD: cardbusdevs.h,v 1.15 2001/05/15 04:25:12 kanaoka Exp $	*/

/*
 * THIS FILE AUTOMATICALLY GENERATED.  DO NOT EDIT.
 *
 * generated from:
 *	NetBSD: cardbusdevs,v 1.17 2001/03/14 01:41:09 christos Exp 
 */

/*
 * Copyright (C) 1999  Hayakawa Koichi.
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
 *      This product includes software developed by the author
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code is stolen from sys/dev/pci/pcidevs.
 */


/*
 * List of known CardBus vendors
 */

#define	CARDBUS_VENDOR_DEC	0x1011		/* Digital Equipment */
#define	CARDBUS_VENDOR_OPTI	0x1045		/* Opti */
#define	CARDBUS_VENDOR_HITACHI	0x1054		/* Hitachi */
#define	CARDBUS_VENDOR_3COM	0x10b7		/* 3Com */
#define	CARDBUS_VENDOR_ADVSYS	0x10cd		/* Advanced System Products */
#define	CARDBUS_VENDOR_REALTEK	0x10ec		/* Realtek Semiconductor */
#define	CARDBUS_VENDOR_IODATA	0x10fc		/* IO Data */
#define	CARDBUS_VENDOR_ACCTON	0x1113		/* Accton Technology */
#define	CARDBUS_VENDOR_XIRCOM	0x115d		/* Xircom */
#define	CARDBUS_VENDOR_COREGA	0x1259		/* Corega */
#define	CARDBUS_VENDOR_ALLIEDTELESYN	0x1259		/* Allied Telesyn International */
#define	CARDBUS_VENDOR_ABOCOM	0x13d1		/* AboCom Systems */
#define	CARDBUS_VENDOR_INTEL	0x8086		/* Intel */
#define	CARDBUS_VENDOR_ADP	0x9004		/* Adaptec */
#define	CARDBUS_VENDOR_ADP2	0x9005		/* Adaptec (2nd PCI Vendor ID) */
#define	CARDBUS_VENDOR_INVALID	0xffff		/* INVALID VENDOR ID */

/*
 * List of known products.  Grouped by vendor.
 */

/* DEC products */
#define	CARDBUS_PRODUCT_DEC_21142	0x0019		/* DECchip 21142/3 */

/* Opti products */
#define	CARDBUS_PRODUCT_OPTI_82C861	0xc861		/* 82C861 USB Host Controller (OHCI) */

/* Hitachi Products */
#define	CARDBUS_PRODUCT_HITACHI_SWC	0x0101		/* MSVCC01/02/03/04 Video Capture Cards */

/* 3COM Products */
#define	CARDBUS_PRODUCT_3COM_3C575TX	0x5057		/* 3c575-TX */
#define	CARDBUS_PRODUCT_3COM_3C575BTX	0x5157		/* 3CCFE575BT */
#define	CARDBUS_PRODUCT_3COM_3C575CTX	0x5257		/* 3CCFE575CT */
#define	CARDBUS_PRODUCT_3COM_3C656_E	0x6560		/* 3CCFEM656 10/100 Ethernet */
#define	CARDBUS_PRODUCT_3COM_3C656B_E	0x6562		/* 3CCFEM656B 10/100 Ethernet */
#define	CARDBUS_PRODUCT_3COM_3C656C_E	0x6564		/* 3CXFEM656C 10/100 Ethernet */

/* Advanced System Products products */
#define	CARDBUS_PRODUCT_ADVSYS_ULTRA	0x1300		/* Ultra SCSI */

/* Realtek (Creative Labs?) products */
#define	CARDBUS_PRODUCT_REALTEK_RT8138	0x8138		/* 8138 Ethernet */

/* IO Data products */
#define	CARDBUS_PRODUCT_IODATA_CBIDE2	0x0003		/* CBIDE2 IDE controller */

/* Accton products */
#define	CARDBUS_PRODUCT_ACCTON_MPX5030	0x1211		/* MPX 5030/5038 Ethernet */
#define	CARDBUS_PRODUCT_ACCTON_EN2242	0x1216		/* EN2242 10/100 Ethernet */

/* Xircom products */
/* is the `-3' here just indicating revision 3, or is it really part
   of the device name? */
#define	CARDBUS_PRODUCT_XIRCOM_X3201_3	0x0002		/* X3201-3 Fast Ethernet Controller */
/* this is the device id `indicating 21143 driver compatibility' */
#define	CARDBUS_PRODUCT_XIRCOM_X3201_3_21143	0x0003		/* X3201-3 Fast Ethernet Controller (21143) */
#define	CARDBUS_PRODUCT_XIRCOM_MODEM56	0x0103		/* 56k Modem */

/* Corega products */
#define	CARDBUS_PRODUCT_COREGA_CB_TXD	0xa117		/* FEther CB-TXD 10/100 Ethernet */

/* AboCom products */
#define	CARDBUS_PRODUCT_ABOCOM_FE2500	0xab02		/* FE2500 10/100 Ethernet */
#define	CARDBUS_PRODUCT_ABOCOM_PCM200	0xab03		/* FE2500 10/100 Ethernet */

/* Intel products */
#define	CARDBUS_PRODUCT_INTEL_82557	0x1229		/* 82557 Fast Ethernet LAN Controller */
/* XXX product name? */
#define	CARDBUS_PRODUCT_INTEL_MODEM56	0x1002		/* 56k Modem */

/* Adaptec products */
#define	CARDBUS_PRODUCT_ADP_1480	0x6075		/* APA-1480 */
