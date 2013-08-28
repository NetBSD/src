/*	$NetBSD: psychoreg.h,v 1.17.18.1 2013/08/28 23:59:22 rmind Exp $ */

/*
 * Copyright (c) 1999 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1998, 1999 Eduardo E. Horvath
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#ifndef _SPARC64_DEV_PSYCHOREG_H_
#define _SPARC64_DEV_PSYCHOREG_H_

/*
 * Sun4u PCI definitions.  Here's where we deal w/the machine
 * dependencies of psycho and the PCI controller on the UltraIIi.
 *
 * All PCI registers are bit-swapped, however they are not byte-swapped.
 * This means that they must be accessed using little-endian access modes,
 * either map the pages little-endian or use little-endian ASIs.
 *
 * PSYCHO implements two PCI buses, A and B.
 */

struct psychoreg {
	struct upareg {
		uint64_t	upa_portid;	/* UPA port ID register */		/* 1fe.0000.0000 */
		uint64_t	upa_config;	/* UPA config register */		/* 1fe.0000.0008 */
	} sys_upa;

	uint64_t	psy_csr;		/* PSYCHO control/status register */	/* 1fe.0000.0010 */
	/* 
	 * 63     59     55     50     45     4        3       2     1      0
	 * +------+------+------+------+--//---+--------+-------+-----+------+
	 * | IMPL | VERS | MID  | IGN  |  xxx  | APCKEN | APERR | IAP | MODE |
	 * +------+------+------+------+--//---+--------+-------+-----+------+
	 *
	 */
#define PSYCHO_GCSR_IMPL(csr)	((u_int)(((csr) >> 60) & 0xf))
#define PSYCHO_GCSR_VERS(csr)	((u_int)(((csr) >> 56) & 0xf))
#define PSYCHO_GCSR_MID(csr)	((u_int)(((csr) >> 51) & 0x1f))
#define PSYCHO_GCSR_IGN(csr)	((u_int)(((csr) >> 46) & 0x1f))
#define PSYCHO_CSR_APCKEN	8	/* UPA addr parity check enable */
#define PSYCHO_CSR_APERR	4	/* UPA addr parity error */
#define PSYCHO_CSR_IAP		2	/* invert UPA address parity */
#define PSYCHO_CSR_MODE		1	/* UPA/PCI handshake */

	uint64_t	pad0;
	uint64_t	psy_ecccr;		/* ECC control register */		/* 1fe.0000.0020 */
	uint64_t	reserved;							/* 1fe.0000.0028 */
	uint64_t	psy_ue_afsr;		/* Uncorrectable Error AFSR */		/* 1fe.0000.0030 */
#define	PSYCHO_UE_AFSR_BITS	"\177\020"				\
	"b\27BLK\0b\070P_DTE\0b\071S_DTE\0b\072S_DWR\0b\073S_DRD\0b"	\
	"\075P_DWR\0b\076P_DRD\0\0"
	uint64_t	psy_ue_afar;		/* Uncorrectable Error AFAR */		/* 1fe.0000.0038 */
	uint64_t	psy_ce_afsr;		/* Correctable Error AFSR */		/* 1fe.0000.0040 */
	uint64_t	psy_ce_afar;		/* Correctable Error AFAR */		/* 1fe.0000.0048 */

	uint64_t	pad1[22];

	struct perfmon {
		uint64_t	pm_cr;		/* Performance monitor control reg */	/* 1fe.0000.0100 */
		uint64_t	pm_count;	/* Performance monitor counter reg */	/* 1fe.0000.0108 */
	} psy_pm;

	uint64_t	pad2[30];

	struct iommureg psy_iommu;							/* 1fe.0000.0200,0210 */

	uint64_t	pad3[317];

	uint64_t	pcia_slot0_int;		/* PCI bus a slot 0 irq map reg */	/* 1fe.0000.0c00 */
	uint64_t	pcia_slot1_int;		/* PCI bus a slot 1 irq map reg */	/* 1fe.0000.0c08 */
	uint64_t	pcia_slot2_int;		/* PCI bus a slot 2 irq map reg (IIi)*/	/* 1fe.0000.0c10 */
	uint64_t	pcia_slot3_int;		/* PCI bus a slot 3 irq map reg (IIi)*/	/* 1fe.0000.0c18 */
	uint64_t	pcib_slot0_int;		/* PCI bus b slot 0 irq map reg */	/* 1fe.0000.0c20 */
	uint64_t	pcib_slot1_int;		/* PCI bus b slot 1 irq map reg */	/* 1fe.0000.0c28 */
	uint64_t	pcib_slot2_int;		/* PCI bus b slot 2 irq map reg */	/* 1fe.0000.0c30 */
	uint64_t	pcib_slot3_int;		/* PCI bus b slot 3 irq map reg */	/* 1fe.0000.0c38 */

	uint64_t	pad4[120];

	uint64_t	scsi_int_map;		/* SCSI interrupt map reg */		/* 1fe.0000.1000 */
	uint64_t	ether_int_map;		/* ethernet interrupt map reg */	/* 1fe.0000.1008 */
	uint64_t	bpp_int_map;		/* parallel interrupt map reg */	/* 1fe.0000.1010 */
	uint64_t	audior_int_map;		/* audio record interrupt map reg */	/* 1fe.0000.1018 */
	uint64_t	audiop_int_map;		/* audio playback interrupt map reg */	/* 1fe.0000.1020 */
	uint64_t	power_int_map;		/* power fail interrupt map reg */	/* 1fe.0000.1028 */
	uint64_t	ser_kbd_ms_int_map;	/* serial/kbd/mouse interrupt map reg *//* 1fe.0000.1030 */
	uint64_t	fd_int_map;		/* floppy interrupt map reg */		/* 1fe.0000.1038 */
	uint64_t	spare_int_map;		/* spare interrupt map reg */		/* 1fe.0000.1040 */
	uint64_t	kbd_int_map;		/* kbd [unused] interrupt map reg */	/* 1fe.0000.1048 */
	uint64_t	mouse_int_map;		/* mouse [unused] interrupt map reg */	/* 1fe.0000.1050 */
	uint64_t	serial_int_map;		/* second serial interrupt map reg */	/* 1fe.0000.1058 */
	uint64_t	timer0_int_map;		/* timer 0 interrupt map reg */		/* 1fe.0000.1060 */
	uint64_t	timer1_int_map;		/* timer 1 interrupt map reg */		/* 1fe.0000.1068 */
	uint64_t	ue_int_map;		/* UE interrupt map reg */		/* 1fe.0000.1070 */
	uint64_t	ce_int_map;		/* CE interrupt map reg */		/* 1fe.0000.1078 */
	uint64_t	pciaerr_int_map;	/* PCI bus a error interrupt map reg */	/* 1fe.0000.1080 */
	uint64_t	pciberr_int_map;	/* PCI bus b error interrupt map reg */	/* 1fe.0000.1088 */
	uint64_t	pwrmgt_int_map;		/* power mgmt wake interrupt map reg */	/* 1fe.0000.1090 */
	uint64_t	ffb0_int_map;		/* FFB0 graphics interrupt map reg */	/* 1fe.0000.1098 */
	uint64_t	ffb1_int_map;		/* FFB1 graphics interrupt map reg */	/* 1fe.0000.10a0 */
	
	uint64_t	pad5[107];

	/* Note: clear interrupt 0 registers are not really used */
	uint64_t	pcia0_clr_int[4];	/* PCI a slot 0 clear int regs 0..7 */	/* 1fe.0000.1400-1418 */
	uint64_t	pcia1_clr_int[4];	/* PCI a slot 1 clear int regs 0..7 */	/* 1fe.0000.1420-1438 */
	uint64_t	pcia2_clr_int[4];	/* PCI a slot 2 clear int regs 0..7 */	/* 1fe.0000.1440-1458 */
	uint64_t	pcia3_clr_int[4];	/* PCI a slot 3 clear int regs 0..7 */	/* 1fe.0000.1480-1478 */
	uint64_t	pcib0_clr_int[4];	/* PCI b slot 0 clear int regs 0..7 */	/* 1fe.0000.1480-1498 */
	uint64_t	pcib1_clr_int[4];	/* PCI b slot 1 clear int regs 0..7 */	/* 1fe.0000.14a0-14b8 */
	uint64_t	pcib2_clr_int[4];	/* PCI b slot 2 clear int regs 0..7 */	/* 1fe.0000.14c0-14d8 */
	uint64_t	pcib3_clr_int[4];	/* PCI b slot 3 clear int regs 0..7 */	/* 1fe.0000.14d0-14f8 */

	uint64_t	pad6[96];

	uint64_t	scsi_clr_int;		/* SCSI clear int reg */		/* 1fe.0000.1800 */
	uint64_t	ether_clr_int;		/* ethernet clear int reg */		/* 1fe.0000.1808 */
	uint64_t	bpp_clr_int;		/* parallel clear int reg */		/* 1fe.0000.1810 */
	uint64_t	audior_clr_int;		/* audio record clear int reg */	/* 1fe.0000.1818 */
	uint64_t	audiop_clr_int;		/* audio playback clear int reg */	/* 1fe.0000.1820 */
	uint64_t	power_clr_int;		/* power fail clear int reg */		/* 1fe.0000.1828 */
	uint64_t	ser_kb_ms_clr_int;	/* serial/kbd/mouse clear int reg */	/* 1fe.0000.1830 */
	uint64_t	fd_clr_int;		/* floppy clear int reg */		/* 1fe.0000.1838 */
	uint64_t	spare_clr_int;		/* spare clear int reg */		/* 1fe.0000.1840 */
	uint64_t	kbd_clr_int;		/* kbd [unused] clear int reg */	/* 1fe.0000.1848 */
	uint64_t	mouse_clr_int;		/* mouse [unused] clear int reg */	/* 1fe.0000.1850 */
	uint64_t	serial_clr_int;		/* second serial clear int reg */	/* 1fe.0000.1858 */
	uint64_t	timer0_clr_int;		/* timer 0 clear int reg */		/* 1fe.0000.1860 */
	uint64_t	timer1_clr_int;		/* timer 1 clear int reg */		/* 1fe.0000.1868 */
	uint64_t	ue_clr_int;		/* UE clear int reg */			/* 1fe.0000.1870 */
	uint64_t	ce_clr_int;		/* CE clear int reg */			/* 1fe.0000.1878 */
	uint64_t	pciaerr_clr_int;	/* PCI bus a error clear int reg */	/* 1fe.0000.1880 */
	uint64_t	pciberr_clr_int;	/* PCI bus b error clear int reg */	/* 1fe.0000.1888 */
	uint64_t	pwrmgt_clr_int;		/* power mgmt wake clr interrupt reg */	/* 1fe.0000.1890 */

	uint64_t	pad7[45];

	uint64_t	intr_retry_timer;	/* interrupt retry timer */		/* 1fe.0000.1a00 */

	uint64_t	pad8[63];

	struct timer_counter {
		uint64_t	tc_count;	/* timer/counter 0/1 count register */	/* 1fe.0000.1c00,1c10 */
		uint64_t	tc_limit;	/* timer/counter 0/1 limit register */	/* 1fe.0000.1c08,1c18 */
	} tc[2];

	uint64_t	pci_dma_write_sync;	/* PCI DMA write sync register (IIi) */	/* 1fe.0000.1c20 */

	uint64_t	pad9[123];

	struct pci_ctl {
		uint64_t	pci_csr;	/* PCI a/b control/status register */	/* 1fe.0000.2000,4000 */
		uint64_t	pad10;
		uint64_t	pci_afsr;	/* PCI a/b AFSR register */		/* 1fe.0000.2010,4010 */
		uint64_t	pci_afar;	/* PCI a/b AFAR register */		/* 1fe.0000.2018,4018 */
		uint64_t	pci_diag;	/* PCI a/b diagnostic register */	/* 1fe.0000.2020,4020 */
		uint64_t	pci_tasr;	/* PCI target address space reg (IIi)*/	/* 1fe.0000.2028,4028 */

		uint64_t	pad11[250];

		/* This is really the IOMMU's, not the PCI bus's */
		struct iommu_strbuf pci_strbuf;						/* 1fe.0000.2800-210 */
#define psy_iommu_strbuf psy_pcictl[0].pci_strbuf
		
		uint64_t	pad12[765];
	} psy_pcictl[2];			/* For PCI a and b */

	/* NB: FFB0 and FFB1 intr map regs also appear at 1fe.0000.6000 and 1fe.0000.8000 respectively */
	uint64_t	pad13[2048];

	uint64_t	dma_scb_diag0;		/* DMA scoreboard diag reg 0 */		/* 1fe.0000.a000 */
	uint64_t	dma_scb_diag1;		/* DMA scoreboard diag reg 1 */		/* 1fe.0000.a008 */

	uint64_t	pad14[126];

	uint64_t	iommu_svadiag;		/* IOMMU virtual addr diag reg */	/* 1fe.0000.a400 */
	uint64_t	iommu_tlb_comp_diag;	/* IOMMU TLB tag compare diag reg */	/* 1fe.0000.a408 */
	
	uint64_t	pad15[30];

	uint64_t	iommu_queue_diag[16];	/* IOMMU LRU queue diag */		/* 1fe.0000.a500-a578 */
	uint64_t	tlb_tag_diag[16];	/* TLB tag diag */			/* 1fe.0000.a580-a5f8 */
	uint64_t	tlb_data_diag[16];	/* TLB data RAM diag */			/* 1fe.0000.a600-a678 */

	uint64_t	pad16[48];

	uint64_t	pci_int_diag;		/* PCI int state diag reg */		/* 1fe.0000.a800 */
	uint64_t	obio_int_diag;		/* OBIO and misc int state diag reg */	/* 1fe.0000.a808 */

	uint64_t	pad17[254];

	struct strbuf_diag {
		uint64_t	strbuf_data_diag[128];	/* streaming buffer data RAM diag */	/* 1fe.0000.b000-b3f8 */
		uint64_t	strbuf_error_diag[128];	/* streaming buffer error status diag *//* 1fe.0000.b400-b7f8 */
		uint64_t	strbuf_pg_tag_diag[16];	/* streaming buffer page tag diag */	/* 1fe.0000.b800-b878 */
		uint64_t	pad18[16];
		uint64_t	strbuf_ln_tag_diag[16];	/* streaming buffer line tag diag */ /* 1fe.0000.b900-b978 */
		uint64_t	pad19[208];	
	} psy_strbufdiag[2];					/* For PCI a and b */
	
	/* 1fe.0000.d000-f058 */
	uint64_t	pad20[1036];
	/* US-IIe and II'i' only */
	uint64_t        stick_cmp_low;
	uint64_t        stick_cmp_high;
	uint64_t        stick_count_low;
	uint64_t        stick_count_high;
	uint64_t        estar_mode;

	/* 
	 * Here is the rest of the map, which we're not specifying:
	 *
	 * 1fe.0100.0000 - 1fe.01ff.ffff	PCI configuration space
	 * 1fe.0100.0000 - 1fe.0100.00ff	PCI B configuration header
	 * 1fe.0101.0000 - 1fe.0101.00ff	PCI A configuration header
	 * 1fe.0200.0000 - 1fe.0200.ffff	PCI A I/O space
	 * 1fe.0201.0000 - 1fe.0201.ffff	PCI B I/O space
	 * 1ff.0000.0000 - 1ff.7fff.ffff	PCI A memory space
	 * 1ff.8000.0000 - 1ff.ffff.ffff	PCI B memory space
	 *
	 * NB: config and I/O space can use 1-4 byte accesses, not 8 byte
	 * accesses.  Memory space can use any sized accesses.
	 *
	 * Note that the SUNW,sabre/SUNW,simba combinations found on the
	 * Ultra5 and Ultra10 machines uses slightly differrent addresses
	 * than the above.  This is mostly due to the fact that the APB is
	 * a multi-function PCI device with two PCI bridges, and the U2P is
	 * two separate PCI bridges.  It uses the same PCI configuration
	 * space, though the configuration header for each PCI bus is
	 * located differently due to the SUNW,simba PCI busses being
	 * function 0 and function 1 of the APB, whereas the psycho's are
	 * each their own PCI device.  The I/O and memory spaces are each
	 * split into 8 equally sized areas (8x2MB blocks for I/O space,
	 * and 8x512MB blocks for memory space).  These are allocated in to
	 * either PCI A or PCI B, or neither in the APB's `I/O Address Map
	 * Register A/B' (0xde) and `Memory Address Map Register A/B' (0xdf)
	 * registers of each simba.  We must ensure that both of the
	 * following are correct (the prom should do this for us):
	 *
	 *    (PCI A Memory Address Map) & (PCI B Memory Address Map) == 0
	 *
	 *    (PCI A I/O Address Map) & (PCI B I/O Address Map) == 0
	 *
	 * 1fe.0100.0000 - 1fe.01ff.ffff	PCI configuration space
	 * 1fe.0100.0800 - 1fe.0100.08ff	PCI B configuration header
	 * 1fe.0100.0900 - 1fe.0100.09ff	PCI A configuration header
	 * 1fe.0200.0000 - 1fe.02ff.ffff	PCI I/O space (divided)
	 * 1ff.0000.0000 - 1ff.ffff.ffff	PCI memory space (divided) 
	 */
};

#define STICK_CMP_LOW	0xf060
#define STICK_CMP_HIGH	0xf068
#define STICK_CNT_LOW	0xf070
#define STICK_CNT_HIGH	0xf078
#define ESTAR_MODE	0xf080

/* what the bits mean! */

/* PCI [a|b] control/status register */
/* note that the sabre only has one set of PCI control/status registers */
#define	PCICTL_MRLM	0x0000001000000000LL	/* Memory Read Line/Multiple */
#define	PCICTL_SERR	0x0000000400000000LL	/* SERR asserted; W1C */
#define	PCICTL_ARB_PARK	0x0000000000200000LL	/* PCI arbitration parking */
#define	PCICTL_CPU_PRIO	0x0000000000100000LL	/* PCI arbitration parking */
#define	PCICTL_ARB_PRIO	0x00000000000f0000LL	/* PCI arbitration parking */
#define	PCICTL_ERRINTEN	0x0000000000000100LL	/* PCI error interrupt enable */
#define	PCICTL_RTRYWAIT 0x0000000000000080LL	/* PCI error interrupt enable */
#define	PCICTL_4ENABLE	0x000000000000000fLL	/* enable 4 PCI slots */
#define	PCICTL_6ENABLE	0x000000000000003fLL	/* enable 6 PCI slots */

/* the following registers only exist on US-IIe and US-II'i' */

/* STICK_CMP_HIGH */
#define STICK_DISABLE	0x80000000	/* disable STICK interrupt */

/*
 * ESTAR_MODE
 * CPU clock MUST remain above 66MHz, so we can't use 1/6 on a 400MHz chip
 */
#define ESTAR_FULL	0	/* full CPU speed */
#define ESTAR_DIV_2	1	/* 1/2 */
#define ESTAR_DIV_6	2	/* 1/6 */
/*
 * the following exist only on US-II'i' - that is the 2nd generation of US-IIe
 * CPUs that Sun decided to call US-IIi just to screw with everyone
 */
#define ESTAR_DIV_4	3	/* 1/4 */
#define ESTAR_DIV_8	4	/* 1/8 */

/*
 * these are the PROM structures we grovel
 */

/*
 * For the physical addresses split into 3 32 bit values, we decode
 * them like the following (IEEE1275 PCI Bus binding 2.0, 2.2.1.1
 * Numerical Representation):
 *
 * 	phys.hi cell:	npt000ss bbbbbbbb dddddfff rrrrrrrr
 * 	phys.mid cell:	hhhhhhhh hhhhhhhh hhhhhhhh hhhhhhhh
 * 	phys.lo cell:	llllllll llllllll llllllll llllllll
 *
 * where these bits affect the address' properties:
 *	n	not-relocatable
 *	p	prefetchable
 *	t	aliased (non-relocatable IO), below 1MB (memory) or
 *		below 64KB (reloc. IO)
 *	ss	address space code:
 *		00 - configuration space
 *		01 - I/O space
 *		10 - 32 bit memory space
 *		11 - 64 bit memory space
 *	bb..bb	8 bit bus number
 *	ddddd	5 bit device number
 *	fff	3 bit function number
 *	rr..rr	8 bit register number
 *	hh..hh	32 bit unsigned value
 *	ll..ll	32 bit unsigned value
 * the values of hh..hh and ll..ll are combined to form a larger number.
 *
 * For config space, we don't have to do much special.  For I/O space,
 * hh..hh must be zero, and if n == 0 ll..ll is the offset from the
 * start of I/O space, otherwise ll..ll is the I/O space.  For memory
 * space, hh..hh must be zero for the 32 bit space, and is the high 32
 * bits in 64 bit space, with ll..ll being the low 32 bits in both cases,
 * with offset handling being driver via `n == 0' as for I/O space.
 */

/* commonly used */
#define TAG2BUS(tag)	((tag) >> 16) & 0xff;
#define TAG2DEV(tag)	((tag) >> 11) & 0x1f;
#define TAG2FN(tag)	((tag) >> 8) & 0x7;

struct psycho_registers {
	uint32_t	phys_hi;
	uint32_t	phys_mid;
	uint32_t	phys_lo;
	uint32_t	size_hi;
	uint32_t	size_lo;
};

struct psycho_ranges {
	uint32_t	cspace;
	uint32_t	child_hi;
	uint32_t	child_lo;
	uint32_t	phys_hi;
	uint32_t	phys_lo;
	uint32_t	size_hi;
	uint32_t	size_lo;
};

struct psycho_interrupt_map {
	uint32_t	phys_hi;
	uint32_t	phys_mid;
	uint32_t	phys_lo;
	uint32_t	intr;
	int32_t		child_node;
	uint32_t	child_intr;
};

struct psycho_interrupt_map_mask {
	uint32_t	phys_hi;
	uint32_t	phys_mid;
	uint32_t	phys_lo;
	uint32_t	intr;
};

#endif /* _SPARC64_DEV_PSYCHOREG_H_ */
