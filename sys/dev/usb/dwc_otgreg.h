/* $FreeBSD: src/sys/dev/usb/controller/dwc_otgreg.h,v 1.6 2012/09/27 15:23:38 hselasky Exp $ */

/*-
 * Copyright (c) 2010,2011 Aleksandr Rybalko. All rights reserved.
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
 */

#ifndef _DWC_OTGREG_H_
#define	_DWC_OTGREG_H_

/* Core Global Registers *****************************************/

#define	DOTG_GOTGCTL		0x0000
#define	DOTG_GOTGINT		0x0004
#define	DOTG_GAHBCFG		0x0008
#define	DOTG_GUSBCFG		0x000C
#define	DOTG_GRSTCTL		0x0010
#define	DOTG_GINTSTS		0x0014
#define	DOTG_GINTMSK		0x0018
#define	DOTG_GRXSTSR		0x001C
#define	DOTG_GRXSTSP		0x0020
#define	DOTG_GRXFSIZ		0x0024
#define	DOTG_GNPTXFSIZ		0x0028
#define	DOTG_GNPTXSTS		0x002C
#define	DOTG_GI2CCTL		0x0030
#define	DOTG_GPVNDCTL		0x0034
#define	DOTG_GGPIO		0x0038
#define	DOTG_GUID		0x003C
#define	DOTG_GSNPSID		0x0040
#define	DOTG_GHWCFG1		0x0044
#define	DOTG_GHWCFG2		0x0048
#define	DOTG_GHWCFG3		0x004C
#define	DOTG_GHWCFG4		0x0050
#define	DOTG_GLPMCFG		0x0054

#define	DOTG_HPTXFSIZ		0x0100
/* start from 0x104, but fifo0 does not exist */
#define	DOTG_DPTXFSIZ(fifo)	(0x0100 + (4*(fifo)))
#define	DOTG_DIEPTXF(fifo)	(0x0100 + (4*(fifo)))

/* Device Global Registers ***************************************/

#define	DOTG_DCFG		0x0800
#define	DOTG_DCTL		0x0804
#define	DOTG_DSTS		0x0808
#define	DOTG_DIEPMSK		0x0810
#define	DOTG_DOEPMSK		0x0814
#define	DOTG_DAINT		0x0818
#define	DOTG_DAINTMSK		0x081C
#define	DOTG_DTKNQR1		0x0820
#define	DOTG_DTKNQR2		0x0824
#define	DOTG_DVBUSDIS		0x0828
#define	DOTG_DVBUSPULSE		0x082C
#define	DOTG_DTHRCTL		0x0830
#define	DOTG_DTKNQR4		0x0834
#define	DOTG_DIEPEMPMSK		0x0834
#define	DOTG_DEACHINT		0x0838
#define	DOTG_DEACHINTMSK	0x083C
#define	DOTG_DIEPEACHINTMSK(ch)	(0x0840 + (4*(ch)))
#define	DOTG_DOEPEACHINTMSK(ch)	(0x0880 + (4*(ch)))

/* Device Endpoint Specific Registers ****************************/

#define	DOTG_DIEPCTL(ep)	(0x0900 + (32*(ep)))
#define	DOTG_DIEPINT(ep)	(0x0908 + (32*(ep)))
#define	DOTG_DIEPTSIZ(ep)	(0x0910 + (32*(ep)))
#define	DOTG_DIEPDMA(ep)	(0x0914 + (32*(ep)))
#define	DOTG_DTXFSTS(ep)	(0x0918 + (32*(ep)))
#define	DOTG_DIEPDMAB(ep)	(0x091c + (32*(ep)))

#define	DOTG_DOEPCTL(ep)	(0x0B00 + (32*(ep)))
#define	DOTG_DOEPFN(ep)		(0x0B04 + (32*(ep)))
#define	DOTG_DOEPINT(ep)	(0x0B08 + (32*(ep)))
#define	DOTG_DOEPTSIZ(ep)	(0x0B10 + (32*(ep)))
#define	DOTG_DOEPDMA(ep)	(0x0B14 + (32*(ep)))
#define	DOTG_DOEPDMAB(ep)	(0x0B1c + (32*(ep)))

/* Host Global Registers *****************************************/

#define	DOTG_HCFG		0x0400
#define	DOTG_HFIR		0x0404
#define	DOTG_HFNUM		0x0408
#define	DOTG_HPTXSTS		0x0410
#define	DOTG_HAINT		0x0414
#define	DOTG_HAINTMSK		0x0418
#define	DOTG_HFLBADDR		0x041C
#define	DOTG_HPRT		0x0440

/* Host Port CSRs ************************************************/

/* Power and clock gating CSR */
#define	DOTG_PCGCCTL		0x0E00

/* Host Channel Specific Registers *******************************/

#define	DOTG_HCCHAR(ch)		(0x0500 + (32*(ch)))
#define	DOTG_HCSPLT(ch)		(0x0504 + (32*(ch)))
#define	DOTG_HCINT(ch)		(0x0508 + (32*(ch)))
#define	DOTG_HCINTMSK(ch)	(0x050C + (32*(ch)))
#define	DOTG_HCTSIZ(ch)		(0x0510 + (32*(ch)))
#define	DOTG_HCDMA(ch)		(0x0514 + (32*(ch)))
#define	DOTG_HCDMAB(ch)		(0x051C + (32*(ch)))

/* FIFO Access Registers (PIO-mode) ******************************/

#define	DOTG_DFIFO(n)		(0x1000 + (0x1000 * (n)))

/*****************************************************************/

/* Core Control and Status Register */
#define	GOTGCTL_CHIRP_ON		(1<<27)
#define	GOTGCTL_BSESVLD			(1<<19)
#define	GOTGCTL_ASESVLD			(1<<18)
#define	GOTGCTL_DBNCTIME		(1<<17)
#define	GOTGCTL_CONIDSTS		(1<<16)
#define	GOTGCTL_DEVHNPEN		(1<<11)
#define	GOTGCTL_HSTSETHNPEN		(1<<10)
#define	GOTGCTL_HNPREQ			(1<<9)
#define	GOTGCTL_HSTNEGSCS		(1<<8)
#define	GOTGCTL_SESREQ			(1<<1)
#define	GOTGCTL_SESREQSCS		(1<<0)

#define	GOTGCTL_DBNCEDONE		(1<<19)
#define	GOTGCTL_ADEVTOUTCHG		(1<<18)
#define	GOTGCTL_HSTNEGDET		(1<<17)
#define	GOTGCTL_HSTNEGSUCSTSCHG		(1<<9)
#define	GOTGCTL_SESREQSUCSTSCHG		(1<<8)
#define	GOTGCTL_SESENDDET		(1<<2)

/* Core AHB Configuration Register */
#define	GAHBCFG_PTXFEMPLVL		(1<<8)
#define	GAHBCFG_NPTXFEMPLVL		(1<<7)
#define	GAHBCFG_DMAEN			(1<<5)
#define	GAHBCFG_HBSTLEN_MASK		0x0000001e
#define	GAHBCFG_HBSTLEN_SHIFT		1
#define	GAHBCFG_GLBLINTRMSK		(1<<0)

/* Core USB Configuration Register */
#define	GUSBCFG_CORRUPTTXPACKET		(1<<31)
#define	GUSBCFG_FORCEDEVMODE		(1<<30)
#define	GUSBCFG_FORCEHOSTMODE		(1<<29)
#define	GUSBCFG_NO_PULLUP		(1<<27)
#define	GUSBCFG_IC_USB_CAP		(1<<26)
#define	GUSBCFG_TERMSELDLPULSE		(1<<22)
#define	GUSBCFG_ULPIEXTVBUSINDICATOR	(1<<21)
#define	GUSBCFG_ULPIEXTVBUSDRV		(1<<20)
#define	GUSBCFG_ULPICLKSUSM		(1<<19)
#define	GUSBCFG_ULPIAUTORES		(1<<18)
#define	GUSBCFG_ULPIFSLS		(1<<17)
#define	GUSBCFG_OTGI2CSEL		(1<<16)
#define	GUSBCFG_PHYLPWRCLKSEL		(1<<15)
#define	GUSBCFG_USBTRDTIM_MASK		0x00003c00
#define	GUSBCFG_USBTRDTIM_SHIFT		10
#define	GUSBCFG_TRD_TIM_SET(x)		(((x) & 15) << 10)
#define	GUSBCFG_HNPCAP			(1<<9)
#define	GUSBCFG_SRPCAP			(1<<8)
#define	GUSBCFG_DDRSEL			(1<<7)
#define	GUSBCFG_PHYSEL			(1<<6)
#define	GUSBCFG_FSINTF			(1<<5)
#define	GUSBCFG_ULPI_UTMI_SEL		(1<<4)
#define	GUSBCFG_PHYIF			(1<<3)
#define	GUSBCFG_TOUTCAL_MASK		0x00000007
#define	GUSBCFG_TOUTCAL_SHIFT		0

/* Core Reset Register */
#define	GRSTCTL_AHBIDLE			(1<<31)
#define	GRSTCTL_DMAREQ			(1<<30)
#define	GRSTCTL_TXFNUM_MASK		0x000007c0
#define	GRSTCTL_TXFNUM_SHIFT		6
#define	GRSTCTL_TXFIFO(n)		(((n) & 31) << 6)
#define	GRSTCTL_TXFFLSH			(1<<5)
#define	GRSTCTL_RXFFLSH			(1<<4)
#define	GRSTCTL_INTKNQFLSH		(1<<3)
#define	GRSTCTL_FRMCNTRRST		(1<<2)
#define	GRSTCTL_HSFTRST			(1<<1)
#define	GRSTCTL_CSFTRST			(1<<0)

/* Core Interrupt Register */
#define	GINTSTS_WKUPINT			(1<<31)
#define	GINTSTS_SESSREQINT		(1<<30)
#define	GINTSTS_DISCONNINT		(1<<29)
#define	GINTSTS_CONIDSTSCHNG		(1<<28)
#define	GINTSTS_LPM			(1<<27)
#define	GINTSTS_PTXFEMP			(1<<26)
#define	GINTSTS_HCHINT			(1<<25)
#define	GINTSTS_PRTINT			(1<<24)
#define	GINTSTS_RESETDET		(1<<23)
#define	GINTSTS_FETSUSP			(1<<22)
#define	GINTSTS_INCOMPLP		(1<<21)
#define	GINTSTS_INCOMPISOIN		(1<<20)
#define	GINTSTS_OEPINT			(1<<19)
#define	GINTSTS_IEPINT			(1<<18)
#define	GINTSTS_EPMIS			(1<<17)
#define	GINTSTS_RESTORE_DONE		(1<<16)
#define	GINTSTS_EOPF			(1<<15)
#define	GINTSTS_ISOOUTDROP		(1<<14)
#define	GINTSTS_ENUMDONE		(1<<13)
#define	GINTSTS_USBRST			(1<<12)
#define	GINTSTS_USBSUSP			(1<<11)
#define	GINTSTS_ERLYSUSP		(1<<10)
#define	GINTSTS_I2CINT			(1<<9)
#define	GINTSTS_ULPICKINT		(1<<8)
#define	GINTSTS_GOUTNAKEFF		(1<<7)
#define	GINTSTS_GINNAKEFF		(1<<6)
#define	GINTSTS_NPTXFEMP		(1<<5)
#define	GINTSTS_RXFLVL			(1<<4)
#define	GINTSTS_SOF			(1<<3)
#define	GINTSTS_OTGINT			(1<<2)
#define	GINTSTS_MODEMIS			(1<<1)
#define	GINTSTS_CURMOD			(1<<0)

/* Core Interrupt Mask Register */
#define	GINTMSK_WKUPINTMSK		(1<<31)
#define	GINTMSK_SESSREQINTMSK		(1<<30)
#define	GINTMSK_DISCONNINTMSK		(1<<29)
#define	GINTMSK_CONIDSTSCHNGMSK		(1<<28)
#define	GINTMSK_PTXFEMPMSK		(1<<26)
#define	GINTMSK_HCHINTMSK		(1<<25)
#define	GINTMSK_PRTINTMSK		(1<<24)
#define	GINTMSK_FETSUSPMSK		(1<<22)
#define	GINTMSK_INCOMPLPMSK		(1<<21)
#define	GINTMSK_INCOMPISOINMSK		(1<<20)
#define	GINTMSK_OEPINTMSK		(1<<19)
#define	GINTMSK_IEPINTMSK		(1<<18)
#define	GINTMSK_EPMISMSK		(1<<17)
#define	GINTMSK_EOPFMSK			(1<<15)
#define	GINTMSK_ISOOUTDROPMSK		(1<<14)
#define	GINTMSK_ENUMDONEMSK		(1<<13)
#define	GINTMSK_USBRSTMSK		(1<<12)
#define	GINTMSK_USBSUSPMSK		(1<<11)
#define	GINTMSK_ERLYSUSPMSK		(1<<10)
#define	GINTMSK_I2CINTMSK		(1<<9)
#define	GINTMSK_ULPICKINTMSK		(1<<8)
#define	GINTMSK_GOUTNAKEFFMSK		(1<<7)
#define	GINTMSK_GINNAKEFFMSK		(1<<6)
#define	GINTMSK_NPTXFEMPMSK		(1<<5)
#define	GINTMSK_RXFLVLMSK		(1<<4)
#define	GINTMSK_SOFMSK			(1<<3)
#define	GINTMSK_OTGINTMSK		(1<<2)
#define	GINTMSK_MODEMISMSK		(1<<1)
#define	GINTMSK_CURMODMSK		(1<<0)

/* Core Receive Status Queue Read Register (Host mode) */
#define	GRXSTSRH_PKTSTS_MASK		0x001e0000
#define	GRXSTSRH_PKTSTS_SHIFT		17
#define	GRXSTSRH_DPID_MASK		0x00018000
#define	GRXSTSRH_DPID_SHIFT		15
#define	GRXSTSRH_BCNT_MASK		0x00007ff0
#define	GRXSTSRH_BCNT_SHIFT		4
#define	GRXSTSRH_CHNUM_MASK		0x0000000f
#define	GRXSTSRH_CHNUM_SHIFT		0

/* Core Receive Status Queue Read Register (Device mode) */
#define	GRXSTSRD_FN_MASK		0x01e00000
#define	GRXSTSRD_FN_GET(x)		(((x) >> 21) & 15)
#define	GRXSTSRD_FN_SHIFT		21
#define	GRXSTSRD_PKTSTS_MASK		0x001e0000
#define	GRXSTSRD_PKTSTS_SHIFT		17
#define	GRXSTSRH_IN_DATA		(2<<17)
#define	GRXSTSRH_IN_COMPLETE		(3<<17)
#define	GRXSTSRH_DT_ERROR		(5<<17)
#define	GRXSTSRH_HALTED			(7<<17)
#define	GRXSTSRD_GLOB_OUT_NAK		(1<<17)
#define	GRXSTSRD_OUT_DATA		(2<<17)
#define	GRXSTSRD_OUT_COMPLETE		(3<<17)
#define	GRXSTSRD_STP_COMPLETE		(4<<17)
#define	GRXSTSRD_STP_DATA		(6<<17)
#define	GRXSTSRD_DPID_MASK		0x00018000
#define	GRXSTSRD_DPID_SHIFT		15
#define	GRXSTSRD_DPID_DATA0		(0<<15)
#define	GRXSTSRD_DPID_DATA1		(2<<15)
#define	GRXSTSRD_DPID_DATA2		(1<<15)
#define	GRXSTSRD_DPID_MDATA		(3<<15)
#define	GRXSTSRD_BCNT_MASK		0x00007ff0
#define	GRXSTSRD_BCNT_GET(x)		(((x) >> 4) & 0x7FF)
#define	GRXSTSRD_BCNT_SHIFT		4
#define	GRXSTSRD_CHNUM_MASK		0x0000000f
#define	GRXSTSRD_CHNUM_GET(x)		((x) & 15)
#define	GRXSTSRD_CHNUM_SHIFT		0

/* Core Receive FIFO Size Register */
#define	GRXFSIZ_RXFDEP_MASK		0x0000ffff
#define	GRXFSIZ_RXFDEP_SHIFT		0

/* Core Non-Periodic Transmit FIFO Size Register */
#define	GNPTXFSIZ_NPTXFDEP_MASK		0xffff0000
#define	GNPTXFSIZ_NPTXFDEP_SHIFT	0
#define	GNPTXFSIZ_NPTXFSTADDR_MASK	0x0000ffff
#define	GNPTXFSIZ_NPTXFSTADDR_SHIFT	16

/* Core Non-Periodic Transmit FIFO/Queue Status Register */
#define	GNPTXSTS_NPTXQTOP_SHIFT		24
#define	GNPTXSTS_NPTXQTOP_MASK		0x7f000000
#define	GNPTXSTS_NPTXQSPCAVAIL_SHIFT	16
#define	GNPTXSTS_NPTXQSPCAVAIL_MASK	0x00ff0000
#define	GNPTXSTS_NPTXFSPCAVAIL_SHIFT	0
#define	GNPTXSTS_NPTXFSPCAVAIL_MASK	0x0000ffff

/* Core I2C Access Register */
#define	GI2CCTL_BSYDNE_SC		(1<<31)
#define	GI2CCTL_RW			(1<<30)
#define	GI2CCTL_I2CDATSE0		(1<<28)
#define	GI2CCTL_I2CDEVADR_SHIFT		26
#define	GI2CCTL_I2CDEVADR_MASK		0x0c000000
#define	GI2CCTL_I2CSUSPCTL		(1<<25)
#define	GI2CCTL_ACK			(1<<24)
#define	GI2CCTL_I2CEN			(1<<23)
#define	GI2CCTL_ADDR_SHIFT		16
#define	GI2CCTL_ADDR_MASK		0x007f0000
#define	GI2CCTL_REGADDR_SHIFT		8
#define	GI2CCTL_REGADDR_MASK		0x0000ff00
#define	GI2CCTL_RWDATA_SHIFT		0
#define	GI2CCTL_RWDATA_MASK		0x000000ff

/* Core Vendor Control Register */
#define	GPVNDCTL_DISULPIDRVR		(1<<31)
#define	GPVNDCTL_VSTSDONE		(1<<27)
#define	GPVNDCTL_VSTSBSY		(1<<26)
#define	GPVNDCTL_NEWREGREQ		(1<<25)
#define	GPVNDCTL_REGWR			(1<<22)
#define	GPVNDCTL_REGADDR_SHIFT		16
#define	GPVNDCTL_REGADDR_MASK		0x003f0000
#define	GPVNDCTL_VCTRL_SHIFT		8
#define	GPVNDCTL_VCTRL_MASK		0x0000ff00
#define	GPVNDCTL_REGDATA_SHIFT		0
#define	GPVNDCTL_REGDATA_MASK		0x000000ff

/* Core General Purpose I/O Register */
#define	GGPIO_GPO_SHIFT			16
#define	GGPIO_GPO_MASK			0xffff0000
#define	GGPIO_GPI_SHIFT			0
#define	GGPIO_GPI_MASK			0x0000ffff

/* Core HW Config1 Register */
#define	GHWCFG1_GET_DIR(x, n)		(((x) >> (2 * (n))) & 3)
#define	GHWCFG1_BIDIR			0
#define	GHWCFG1_IN			1
#define	GHWCFG1_OUT			2

/* Core HW Config2 Register */
#define	GHWCFG2_TKNQDEPTH_SHIFT		26
#define	GHWCFG2_TKNQDEPTH_MASK		0x7c000000
#define	GHWCFG2_PTXQDEPTH_SHIFT		24
#define	GHWCFG2_PTXQDEPTH_MASK		0x03000000
#define	GHWCFG2_NPTXQDEPTH_SHIFT	22
#define	GHWCFG2_NPTXQDEPTH_MASK		0x00c00000
#define	GHWCFG2_MPI			(1<<20)
#define	GHWCFG2_DYNFIFOSIZING		(1<<19)
#define	GHWCFG2_PERIOSUPPORT		(1<<18)
#define	GHWCFG2_NUMHSTCHNL_SHIFT	14
#define	GHWCFG2_NUMHSTCHNL_MASK		0x0003c000
#define	GHWCFG2_NUMHSTCHNL_GET(x)	((((x) >> 14) & 15) + 1)
#define	GHWCFG2_NUMDEVEPS_SHIFT		10
#define	GHWCFG2_NUMDEVEPS_MASK		0x00003c00
#define	GHWCFG2_NUMDEVEPS_GET(x)	((((x) >> 10) & 15) + 1)
#define	GHWCFG2_FSPHYTYPE_SHIFT		8
#define	GHWCFG2_FSPHYTYPE_MASK		0x00000300
#define	GHWCFG2_HSPHYTYPE_SHIFT		6
#define	GHWCFG2_HSPHYTYPE_MASK		0x000000c0
#define	GHWCFG2_SINGPNT			(1<<5)
#define	GHWCFG2_OTGARCH_SHIFT		3
#define	GHWCFG2_OTGARCH_MASK		0x00000018
#define	GHWCFG2_OTGMODE_SHIFT		0
#define	GHWCFG2_OTGMODE_MASK		0x00000007

#define OTGARCH_SLAVE_ONLY		0
#define OTGARCH_EXT_DMA			1
#define OTGARCH_INT_DMA			2

#define OTGMODE_HNP_SRP_OTG		0
#define OTGMODE_SRP_ONLY_OTG		1
#define OTGMODE_NO_HNP_SRP_OTG		2
#define OTGMODE_SRP_CAPABLE_DEVICE	3
#define OTGMODE_NO_SRP_CAPABLE_DEVICE	4
#define OTGMODE_SRP_CAPABLE_HOST	5
#define OTGMODE_NO_SRP_CAPABLE_HOST	6

#define HSPHYTYPE_UTMI			1
#define HSPHYTYPE_ULPI			2
#define HSPHYTYPE_UTMI_ULPI		3

/* Core HW Config3 Register */
#define	GHWCFG3_DFIFODEPTH_SHIFT	16
#define	GHWCFG3_DFIFODEPTH_MASK		0xffff0000
#define	GHWCFG3_DFIFODEPTH_GET(x)	((x) >> 16)
#define	GHWCFG3_RSTTYPE			(1<<11)
#define	GHWCFG3_OPTFEATURE		(1<<10)
#define	GHWCFG3_VNDCTLSUPT		(1<<9)
#define	GHWCFG3_I2CINTSEL		(1<<8)
#define	GHWCFG3_OTGEN			(1<<7)
#define	GHWCFG3_PKTSIZEWIDTH_SHIFT	4
#define	GHWCFG3_PKTSIZEWIDTH_MASK	0x00000070
#define	GHWCFG3_PKTSIZE_GET(x)		(0x10<<(((x) >> 4) & 7))
#define	GHWCFG3_XFERSIZEWIDTH_SHIFT	0
#define	GHWCFG3_XFERSIZEWIDTH_MASK	0x0000000f
#define	GHWCFG3_XFRRSIZE_GET(x)		(0x400<<(((x) >> 0) & 15))

/* Core HW Config4 Register */
#define	GHWCFG4_NUM_IN_EP_GET(x)	((((x) >> 26) & 15) + 1)
#define	GHWCFG4_SESSENDFLTR		(1<<24)
#define	GHWCFG4_BVALIDFLTR		(1<<23)
#define	GHWCFG4_AVALIDFLTR		(1<<22)
#define	GHWCFG4_VBUSVALIDFLTR		(1<<21)
#define	GHWCFG4_IDDGFLTR		(1<<20)
#define	GHWCFG4_NUMCTLEPS_SHIFT		16
#define	GHWCFG4_NUMCTLEPS_MASK		0x000f0000
#define	GHWCFG4_NUMCTLEPS_GET(x)	(((x) >> 16) & 15)
#define	GHWCFG4_PHYDATAWIDTH_SHIFT	14
#define	GHWCFG4_PHYDATAWIDTH_MASK	0x0000c000
#define	GHWCFG4_AHBFREQ			(1<<5)
#define	GHWCFG4_ENABLEPWROPT		(1<<4)
#define	GHWCFG4_NUMDEVPERIOEPS_SHIFT	0
#define	GHWCFG4_NUMDEVPERIOEPS_MASK	0x0000000f
#define	GHWCFG4_NUMDEVPERIOEPS_GET(x)	(((x) >> 0) & 15)

/* Core LPM Configuration Register */
#define	GLPMCFG_HSIC_CONN		(1<<30)

/* Host Periodic Transmit FIFO Size Register */
#define	HPTXFSIZ_PTXFSIZE_SHIFT		16
#define	HPTXFSIZ_PTXFSIZE_MASK		0xffff0000
#define	HPTXFSIZ_PTXFSTADDR_SHIFT	0
#define	HPTXFSIZ_PTXFSTADDR_MASK	0x0000ffff

/* Device Periodic Transmit FIFO#n Size Register */
#define	DPTXFSIZN_DPTXFSIZE_SHIFT	16
#define	DPTXFSIZN_DPTXFSIZE_MASK	0xffff0000
#define	DPTXFSIZN_PTXFSTADDR_SHIFT	0
#define	DPTXFSIZN_PTXFSTADDR_MASK	0x0000ffff

/* Device Transmit FIFO#n Register */
#define	DIEPTXFN_INEPNTXFDEP_SHIFT	16
#define	DIEPTXFN_INEPNTXFDEP_MASK	0xffff0000
#define	DIEPTXFN_INEPNTXFSTADDR_SHIFT	0
#define	DIEPTXFN_INEPNTXFSTADDR_MASK	0x0000ffff

/* Host Configuration Register */
#define	HCFG_MODECHANGERDY		(1<<31)
#define	HCFG_PERSCHEDENABLE		(1<<26)
#define	HCFG_FLENTRIES_SHIFT		24
#define	HCFG_FLENTRIES_MASK		0x03000000
#define	HCFG_FLENTRIES_8		(0)
#define	HCFG_FLENTRIES_16		(1)
#define	HCFG_FLENTRIES_32		(2)
#define	HCFG_FLENTRIES_64		(3)
#define	HCFG_MULTISEGDMA		(1<<23)
#define	HCFG_32KHZSUSPEND		(1<<7)
#define	HCFG_FSLSSUPP			(1<<2)
#define	HCFG_FSLSPCLKSEL_SHIFT		0
#define	HCFG_FSLSPCLKSEL_MASK		0x00000003

/* Host Frame Interval Register */
#define	HFIR_RELOADCTRL			(1<<16)
#define	HFIR_FRINT_SHIFT		0
#define	HFIR_FRINT_MASK			0x0000ffff

/* Host Frame Number / Frame Remaining Register */
#define	HFNUM_FRREM_SHIFT		16
#define	HFNUM_FRREM_MASK		0xffff0000
#define	HFNUM_FRNUM_SHIFT		0
#define	HFNUM_FRNUM_MASK		0x0000ffff

/* Host Periodic Transmit FIFO / Queue Status Register */
#define	HPTXSTS_ODD			(1<<31)
#define	HPTXSTS_CHAN_SHIFT		27
#define	HPTXSTS_CHAN_MASK		0x78000000
#define	HPTXSTS_TOKEN_SHIFT		25
#define	HPTXSTS_TOKEN_MASK		0x06000000
#define	HPTXSTS_TOKEN_ZL		0
#define	HPTXSTS_TOKEN_PING		1
#define	HPTXSTS_TOKEN_DISABLE		2
#define	HPTXSTS_TERMINATE		(1<<24)
#define	HPTXSTS_PTXQSPCAVAIL_SHIFT	16
#define	HPTXSTS_PTXQSPCAVAIL_MASK	0x00ff0000
#define	HPTXSTS_PTXFSPCAVAIL_SHIFT	0
#define	HPTXSTS_PTXFSPCAVAIL_MASK	0x0000ffff

/* Host All Channels Interrupt Register */
#define	HAINT_HAINT_SHIFT		0
#define	HAINT_HAINT_MASK		0x0000ffff

/* Host All Channels Interrupt Mask Register */
#define	HAINTMSK_HAINTMSK_SHIFT		0
#define	HAINTMSK_HAINTMSK_MASK		0x0000ffff

/* Host Port Control and Status Rgeister */
#define	HPRT_PRTSPD_SHIFT		17
#define	HPRT_PRTSPD_MASK		0x00060000
#define	HPRT_PRTSPD_HIGH		0
#define	HPRT_PRTSPD_FULL		1
#define	HPRT_PRTSPD_LOW			2
#define	HPRT_PRTSPD_MASK		0x00060000
#define	HPRT_PRTTSTCTL_SHIFT		13
#define	HPRT_PRTTSTCTL_MASK		0x0001e000
#define	HPRT_PRTPWR			(1<<12)
#define	HPRT_PRTLNSTS_SHIFT		10
#define	HPRT_PRTLNSTS_MASK		0x00000c00
#define	HPRT_PRTRST			(1<<8)
#define	HPRT_PRTSUSP			(1<<7)
#define	HPRT_PRTRES			(1<<6)
#define	HPRT_PRTOVRCURRCHNG		(1<<5)
#define	HPRT_PRTOVRCURRACT		(1<<4)
#define	HPRT_PRTENCHNG			(1<<3)
#define	HPRT_PRTENA			(1<<2)
#define	HPRT_PRTCONNDET			(1<<1)
#define	HPRT_PRTCONNSTS			(1<<0)

/* Host Channel 0 Characteristic Register */
#define	HCCHAR_CHENA			(1<<31)
#define	HCCHAR_CHDIS			(1<<30)
#define	HCCHAR_ODDFRM			(1<<29)
#define	HCCHAR_DEVADDR_SHIFT		22
#define	HCCHAR_DEVADDR_MASK		0x1fc00000
#define	HCCHAR_MC_SHIFT			20
#define	HCCHAR_MC_MASK			0x00300000
#define	HCCHAR_EPTYPE_SHIFT		18
#define	HCCHAR_EPTYPE_MASK		0x000c0000
#define	HCCHAR_LSPDDEV			(1<<17)
#define	HCCHAR_EPDIR			(1<<15)
#define	HCCHAR_EPDIR_IN			(1<<15)
#define	HCCHAR_EPDIR_OUT		0
#define	HCCHAR_EPNUM_SHIFT		11
#define	HCCHAR_EPNUM_MASK		0x00007800
#define	HCCHAR_MPS_SHIFT		0
#define	HCCHAR_MPS_MASK			0x000007ff

/* Host Channel 0 Split Control Register */
#define	HCSPLT_SPLTENA			(1<<31)
#define	HCSPLT_COMPSPLT			(1<<16)
#define	HCSPLT_XACTPOS_SHIFT		14
#define	HCSPLT_XACTPOS_MASK		0x0000c000
#define	HCSPLT_HUBADDR_SHIFT		7
#define	HCSPLT_HUBADDR_MASK		0x00003f80
#define	HCSPLT_PRTADDR_SHIFT		0
#define	HCSPLT_PRTADDR_MASK		0x0000007f

#define	HCINT_ERRORS \
    (HCINT_BBLERR | HCINT_XACTERR)
#define	HCINT_RETRY \
    (HCINT_DATATGLERR | HCINT_FRMOVRUN | HCINT_NAK)

/* Host Channel 0 Interrupt Register */
#define	HCINT_SOFTWARE_ONLY		(1<<20)	/* BSD only */
#define	HCINT_DATATGLERR		(1<<10)
#define	HCINT_FRMOVRUN			(1<<9)
#define	HCINT_BBLERR			(1<<8)
#define	HCINT_XACTERR			(1<<7)
#define	HCINT_NYET			(1<<6)
#define	HCINT_ACK			(1<<5)
#define	HCINT_NAK			(1<<4)
#define	HCINT_STALL			(1<<3)
#define	HCINT_AHBERR			(1<<2)
#define	HCINT_CHHLTD			(1<<1)
#define	HCINT_XFERCOMPL			(1<<0)

/* Host Channel 0 Interrupt Mask Register */
#define	HCINTMSK_DATATGLERRMSK		(1<<10)
#define	HCINTMSK_FRMOVRUNMSK		(1<<9)
#define	HCINTMSK_BBLERRMSK		(1<<8)
#define	HCINTMSK_XACTERRMSK		(1<<7)
#define	HCINTMSK_NYETMSK		(1<<6)
#define	HCINTMSK_ACKMSK			(1<<5)
#define	HCINTMSK_NAKMSK			(1<<4)
#define	HCINTMSK_STALLMSK		(1<<3)
#define	HCINTMSK_AHBERRMSK		(1<<2)
#define	HCINTMSK_CHHLTDMSK		(1<<1)
#define	HCINTMSK_XFERCOMPLMSK		(1<<0)

/* Host Channel 0 Transfer Size Register */
#define	HCTSIZ_DOPNG			(1<<31)
#define	HCTSIZ_PID_SHIFT		29
#define	HCTSIZ_PID_MASK			0x60000000
#define	HCTSIZ_PID_DATA0		0
#define	HCTSIZ_PID_DATA2		1
#define	HCTSIZ_PID_DATA1		2
#define	HCTSIZ_PID_MDATA		3
#define	HCTSIZ_PID_SETUP		3
#define	HCTSIZ_PKTCNT_SHIFT		19
#define	HCTSIZ_PKTCNT_MASK		0x1ff80000
#define	HCTSIZ_XFERSIZE_SHIFT		0
#define	HCTSIZ_XFERSIZE_MASK		0x0007ffff
#define	HCTSIZ_NTD_SHIFT		8
#define	HCTSIZ_NTD_MASK			0x0000ff00
#define	HCTSIZ_SCHINFO_SHIFT		0
#define	HCTSIZ_SCHINFO_MASK		0x000000ff

/* Device Configuration Register */
#define	DCFG_EPMISCNT_SHIFT		18
#define	DCFG_EPMISCNT_MASK		0x007c0000
#define	DCFG_PERFRINT_SHIFT		11
#define	DCFG_PERFRINT_MASK		0x00001800
#define	DCFG_DEVADDR_SHIFT		4
#define	DCFG_DEVADDR_MASK		0x000007f0
#define	DCFG_DEVADDR_SET(x)		(((x) & 0x7F) << 4)
#define	DCFG_NZSTSOUTHSHK		(1<<2)
#define	DCFG_DEVSPD_SHIFT		0
#define	DCFG_DEVSPD_MASK		0x00000003
#define	DCFG_DEVSPD_SET(x)		((x) & 0x3)
#define	DCFG_DEVSPD_HI			0
#define	DCFG_DEVSPD_FULL20		1
#define	DCFG_DEVSPD_FULL10		3

/* Device Control Register */
#define	DCTL_PWRONPRGDONE		(1<<11)
#define	DCTL_CGOUTNAK			(1<<10)
#define	DCTL_SGOUTNAK			(1<<9)
#define	DCTL_CGNPINNAK			(1<<8)
#define	DCTL_SGNPINNAK			(1<<7)
#define	DCTL_TSTCTL_SHIFT		4
#define	DCTL_TSTCTL_MASK		0x00000070
#define	DCTL_GOUTNAKSTS			(1<<3)
#define	DCTL_GNPINNAKSTS		(1<<2)
#define	DCTL_SFTDISCON			(1<<1)
#define	DCTL_RMTWKUPSIG			(1<<0)

/* Device Status Register */
#define	DSTS_SOFFN_SHIFT		8
#define	DSTS_SOFFN_MASK			0x003fff00
#define	DSTS_SOFFN_GET(x)		(((x) >> 8) & 0x3FFF)
#define	DSTS_ERRTICERR			(1<<3)
#define	DSTS_ENUMSPD_SHIFT		1
#define	DSTS_ENUMSPD_MASK		0x00000006
#define	DSTS_ENUMSPD_GET(x)		(((x) >> 1) & 3)
#define	DSTS_ENUMSPD_HI			0
#define	DSTS_ENUMSPD_FULL20		1
#define	DSTS_ENUMSPD_LOW10		2
#define	DSTS_ENUMSPD_FULL10		3
#define	DSTS_SUSPSTS			(1<<0)

/* Device IN EP Common Mask Register */
#define	DIEPMSK_TXFIFOUNDRNMSK		(1<<8)
#define	DIEPMSK_INEPNAKEFFMSK		(1<<6)
#define	DIEPMSK_INTKNEPMISMSK		(1<<5)
#define	DIEPMSK_INTKNTXFEMPMSK		(1<<4)
#define	DIEPMSK_FIFOEMPTY		(1<<4)
#define	DIEPMSK_TIMEOUTMSK		(1<<3)
#define	DIEPMSK_AHBERRMSK		(1<<2)
#define	DIEPMSK_EPDISBLDMSK		(1<<1)
#define	DIEPMSK_XFERCOMPLMSK		(1<<0)

/* Device OUT EP Common Mask Register */
#define	DOEPMSK_OUTPKTERRMSK		(1<<8)
#define	DOEPMSK_BACK2BACKSETUP		(1<<6)
#define	DOEPMSK_OUTTKNEPDISMSK		(1<<4)
#define	DOEPMSK_FIFOEMPTY		(1<<4)
#define	DOEPMSK_SETUPMSK		(1<<3)
#define	DOEPMSK_AHBERRMSK		(1<<2)
#define	DOEPMSK_EPDISBLDMSK		(1<<1)
#define	DOEPMSK_XFERCOMPLMSK		(1<<0)

/* Device IN EP Interrupt Register */
#define	DIEPINT_TXFIFOUNDRN		(1<<8)
#define	DIEPINT_INEPNAKEFF		(1<<6)
#define	DIEPINT_INTKNEPMIS		(1<<5)
#define	DIEPINT_INTKNTXFEMP		(1<<4)
#define	DIEPINT_TIMEOUT			(1<<3)
#define	DIEPINT_AHBERR			(1<<2)
#define	DIEPINT_EPDISBLD		(1<<1)
#define	DIEPINT_XFERCOMPL		(1<<0)

/* Device OUT EP Interrupt Register */
#define	DOEPINT_OUTPKTERR		(1<<8)
#define	DOEPINT_BACK2BACKSETUP		(1<<6)
#define	DOEPINT_OUTTKNEPDIS		(1<<4)
#define	DOEPINT_SETUP			(1<<3)
#define	DOEPINT_AHBERR			(1<<2)
#define	DOEPINT_EPDISBLD		(1<<1)
#define	DOEPINT_XFERCOMPL		(1<<0)

/* Device All EP Interrupt Register */
#define	DAINT_INEPINT_MASK		0xffff0000
#define	DAINT_INEPINT_SHIFT		0
#define	DAINT_OUTEPINT_MASK		0x0000ffff
#define	DAINT_OUTEPINT_SHIFT		16

/* Device All EP Interrupt Mask Register */
#define	DAINTMSK_INEPINT_MASK		0xffff0000
#define	DAINTMSK_INEPINT_SHIFT		0
#define	DAINTMSK_OUTEPINT_MASK		0x0000ffff
#define	DAINTMSK_OUTEPINT_SHIFT		16

/* Device IN Token Queue Read Register 1 */
#define	DTKNQR1_EPTKN_SHIFT		8
#define	DTKNQR1_EPTKN_MASK		0xffffff00
#define	DTKNQR1_WRAPBIT			(1<<7)
#define	DTKNQR1_INTKNWPTR_SHIFT		0
#define	DTKNQR1_INTKNWPTR_MASK		0x0000001f

/* Device VBUS Discharge Register */
#define	DVBUSDIS_DVBUSDIS_SHIFT		0
#define	DVBUSDIS_DVBUSDIS_MASK		0x0000ffff

/* Device VBUS Pulse Register */
#define	DVBUSPULSE_DVBUSPULSE_SHIFT	0
#define	DVBUSPULSE_DVBUSPULSE_MASK	0x00000fff

/* Device Tresholding Control Register */
#define	DTHRCTL_ARBPRKEN		(1<<27)
#define	DTHRCTL_RXTHRLEN_SHIFT		17
#define	DTHRCTL_RXTHRLEN_MASK		0x03fe0000
#define	DTHRCTL_RXTHREN			(1<<16)
#define	DTHRCTL_TXTHRLEN_SHIFT		2
#define	DTHRCTL_TXTHRLEN_MASK		0x000007fc
#define	DTHRCTL_ISOTHREN		(1<<1)
#define	DTHRCTL_NONISOTHREN		(1<<0)

/* Device IN EPs Empty Interrupt Mask Register */
#define	DIEPEMPMSK_INEPTXFEMPMSK_SHIFT	0
#define	DIEPEMPMSK_INEPTXFEMPMSK_MASK	0x0000ffff

/* Device IN Endpoint Control Register */
#define	DIEPCTL_EPENA			(1<<31)
#define	DIEPCTL_EPDIS			(1<<30)
#define	DIEPCTL_SETD1PID		(1<<29)
#define	DIEPCTL_SETD0PID		(1<<28)
#define	DIEPCTL_SNAK			(1<<27)
#define	DIEPCTL_CNAK			(1<<26)
#define	DIEPCTL_TXFNUM_SHIFT		22
#define	DIEPCTL_TXFNUM_MASK		0x03c00000
#define	DIEPCTL_TXFNUM_SET(n)		(((n) & 15) << 22)
#define	DIEPCTL_STALL			(1<<21)
#define	DIEPCTL_EPTYPE_SHIFT		18
#define	DIEPCTL_EPTYPE_MASK		0x000c0000
#define	DIEPCTL_EPTYPE_SET(n)		(((n) & 3) << 18)
#define	DIEPCTL_EPTYPE_CONTROL		0
#define	DIEPCTL_EPTYPE_ISOC		1
#define	DIEPCTL_EPTYPE_BULK		2
#define	DIEPCTL_EPTYPE_INTERRUPT	3
#define	DIEPCTL_NAKSTS			(1<<17)
#define	DIEPCTL_USBACTEP		(1<<15)
#define	DIEPCTL_NEXTEP_SHIFT		11
#define	DIEPCTL_NEXTEP_MASK		0x00007800
#define	DIEPCTL_MPS_SHIFT		0
#define	DIEPCTL_MPS_MASK		0x000007ff
#define	DIEPCTL_MPS_SET(n)		((n) & 0x7FF)
#define	DIEPCTL_MPS_64			(0<<0)
#define	DIEPCTL_MPS_32			(1<<0)
#define	DIEPCTL_MPS_16			(2<<0)
#define	DIEPCTL_MPS_8			(3<<0)

/* Device OUT Endpoint Control Register */
#define	DOEPCTL_EPENA			(1<<31)
#define	DOEPCTL_EPDIS			(1<<30)
#define	DOEPCTL_SETD1PID		(1<<29)
#define	DOEPCTL_SETD0PID		(1<<28)
#define	DOEPCTL_SNAK			(1<<27)
#define	DOEPCTL_CNAK			(1<<26)
#define	DOEPCTL_FNUM_SET(n)		(((n) & 15) << 22)
#define	DOEPCTL_STALL			(1<<21)
#define	DOEPCTL_EPTYPE_SHIFT		18
#define	DOEPCTL_EPTYPE_MASK		0x000c0000
#define	DOEPCTL_EPTYPE_SET(n)		(((n) & 3) << 18)
#define	DOEPCTL_NAKSTS			(1<<17)
#define	DOEPCTL_USBACTEP		(1<<15)
#define	DOEPCTL_MPS_SHIFT		0
#define	DOEPCTL_MPS_MASK		0x000007ff
#define	DOEPCTL_MPS_SET(n)		((n) & 0x7FF)
#define	DOEPCTL_MPS_64			(0<<0)
#define	DOEPCTL_MPS_32			(1<<0)
#define	DOEPCTL_MPS_16			(2<<0)
#define	DOEPCTL_MPS_8			(3<<0)

/* common bits for DIEPCTL and DOEPCTL */
#define	DXEPINT_TXFEMP			(1<<7)
#define	DXEPINT_SETUP			(1<<3)
#define	DXEPINT_XFER_COMPL		(1<<0)

/* Device IN Endpoint Transfer Size Register */
#define	DIEPTSIZ_XFERSIZE_MASK		0x0007ffff
#define	DIEPTSIZ_XFERSIZE_SHIFT		0
#define	DIEPTSIZ_PKTCNT_MASK		0x1ff80000
#define	DIEPTSIZ_PKTCNT_SHIFT		19
#define	DIEPTSIZ_MC_MASK		0x60000000
#define	DIEPTSIZ_MC_SHIFT		29

/* Device OUT Endpoint Transfer Size Register */
#define	DOEPTSIZ_XFERSIZE_MASK		0x0007ffff
#define	DOEPTSIZ_XFERSIZE_SHIFT		0
#define	DOEPTSIZ_PKTCNT_MASK		0x1ff80000
#define	DOEPTSIZ_PKTCNT_SHIFT		19
#define	DOEPTSIZ_MC_MASK		0x60000000
#define	DOEPTSIZ_MC_SHIFT		29

/* common bits for DIEPTSIZ and DOEPTSIZ */
#define	DXEPTSIZ_SET_MULTI(n)		(((n) & 3) << 29)
#define	DXEPTSIZ_SET_NPKT(n)		(((n) & 0x3FF) << 19)
#define	DXEPTSIZ_GET_NPKT(n)		(((n) >> 19) & 0x3FF)
#define	DXEPTSIZ_SET_NBYTES(n)		(((n) & 0x7FFFFF) << 0)
#define	DXEPTSIZ_GET_NBYTES(n)		(((n) >> 0) & 0x7FFFFF)

/* Power and clock gating CSR */
#define PCGCCTL_DEEP_SLEEP		(1<<7)
#define PCGCCTL_PHY_IN_SLEEP		(1<<6)
#define PCGCCTL_ENBL_SLEEP_GATING	(1<<5)
#define PCGCCTL_PHYSUSPEND		(1<<4)
#define PCGCCTL_RSTPDWNMODULE		(1<<3)
#define PCGCCTL_PWRCLMP			(1<<2)
#define PCGCCTL_GATEHCLK		(1<<1)
#define PCGCCTL_STOPPCLK		(1<<0)

/*****************************************************************/

struct dotg_host_dma_desc {
	uint32_t status;
	uint32_t buffer;
} dotg_host_dma_desc_t;

/* DMA Status quadlet */
/* for non-isochronous transfers */
#define HDDS_BYTES(x)			((x) & 0x0001ffff)
#define HDDS_OFFSET(x)			((x) & 0x007e0000)
#define HDDS_GET_OFFSET(x)		(HDDS_GET_OFFSET(x) >> 17)
#define HDDS_QTD			(1<<23)
#define HDDS_SUP			(1<<24)

/* for isochronous transfers */
#define HDDS_IBYTES(x)			((x) & 0x00000fff)

#define HDDS_IOC			(1<<25)
#define HDDS_EOL			(1<<26)
#define HDDS_STS(x)			(((x) & 3) << 28)
#define HDDS_A				(1<<31)

struct dotg_dev_dma_desc {
	uint32_t status;
	uint32_t buffer;
} dotg_dev_dma_desc_t;

/* DMA Status quadlet */
/* for non-isochronous transfers */
#define DDDS_BYTES(x)			((x) & 0xffff)
#define DDDS_MTRF			(1<<23)
#define DDDS_SR				(1<<24)

/* for isochronous transfers */
#define DDDS_RXBYTES(x)			((x) & 0x000007ff)
#define DDDS_TXBYTES(x)			((x) & 0x00000fff)
#define DDDS_FRAMENUM(x)		((x) & 0x007ff000)
#define DDDS_GET_FRAMENUM(x)		(DDDS_FRAMENUM(x) >> 12)
#define DDDS_PID(x)			((x) & 0x01800000)
#define DDDS_GET_PID(x)			(DDDS_PID(x) >> 23)

#define DDDS_IOC			(1<<25)
#define DDDS_SP				(1<<26)
#define DDDS_L				(1<<27)
#define DDDS_STS(x)			(((x) & 3) << 28)
#define DDDS_BS(x)			(((x) & 3) << 30)

#define STS_SUCCESS			0
#define STS_BUFFLUSH			1
#define STS_BUFERR			3

#define BS_HOST_READY			0
#define BS_DMA_BUSY			1
#define BS_DMA_DONE			2
#define BS_HOST_BUSY			3

#endif					/* _DWC_OTGREG_H_ */
