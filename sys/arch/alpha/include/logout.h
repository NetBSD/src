/* $NetBSD: logout.h,v 1.7.8.1 2012/04/17 00:05:55 yamt Exp $ */

/*
 * Copyright (c) 1998 by Matthew Jacob
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
 * Various Alpha OSF/1 PAL Logout error definitions.
 */

/*
 * Information gathered from: DEC documentation
 */

/*
 * Avanti (AlphaStation 200 and 400) Specific PALcode Exception Logout
 * Area Definitions
 */

/*
 * Avanti Specific common logout frame header.
 * *Almost* identical to the generic logout header listed in alpha_cpu.h.
 */

typedef struct {
	unsigned int	la_frame_size;		/* frame size */
	unsigned int	la_flags;		/* flags; see alpha_cpu.h */
	unsigned int	la_cpu_offset;		/* offset to CPU area */
	unsigned int	la_system_offset;	/* offset to system area */
	unsigned long	mcheck_code;		/* machine check code */
} mc_hdr_avanti;

/* Machine Check Codes */

/* SCB 660 Fatal Machine Checks */
#define AVANTI_RETRY_TIMEOUT		0x201L
#define	AVANTI_DMA_DATA_PARITY		0x202L
#define AVANTI_IO_PARITY		0x203L
#define AVANTI_TARGET_ABORT		0x204L
#define AVANTI_NO_DEVICE		0x205L
#define AVANTI_CORRRECTABLE_MEMORY	0x206L	/* Should never occur */
#define AVANTI_UNCORRECTABLE_PCI_MEMORY	0x207L
#define AVANTI_INVALID_PT_LOOKUP	0x208L
#define AVANTI_MEMORY			0x209L
#define AVANTI_BCACHE_TAG_ADDR_PARITY	0x20AL
#define AVANTI_BCACHE_TAG_CTRL_PARITY	0x20BL
#define AVANTI_NONEXISTENT_MEMORY	0x20CL
#define AVANTI_IO_BUS			0x20DL
#define AVANTI_BCACHE_TAG_PARITY	 0x80L
#define AVANTI_BCACHE_TAG_CTRL_PARITY2   0x82L

/* SCB 670 Processor Fatal Machine Checks */
#define AVANTI_HARD_ERROR		 0x84L
#define AVANTI_CORRECTABLE_ECC		 0x86L
#define AVANTI_NONCORRECTABLE_ECC	 0x88L
#define AVANTI_UNKNOWN_ERROR		 0x8AL
#define AVANTI_SOFT_ERROR		 0x8CL
#define AVANTI_BUGCHECK		 	 0x8EL
#define AVANTI_OS_BUGCHECK		 0x90L
#define AVANTI_DCACHE_FILL_PARITY 	 0x92L
#define AVANTI_ICACHE_FILL_PARITY	 0x94L

typedef struct {
	/* Registers from the CPU */
	uint64_t	paltemp[32];	/* PAL TEMP REGS.		*/
	uint64_t	exc_addr;	/* Address of excepting ins.	*/
	uint64_t	exc_sum;	/* Summary of arithmetic traps.	*/
	uint64_t	exc_mask;	/* Exception mask.		*/
	uint64_t	iccsr;
	uint64_t	pal_base;	/* Base address for PALcode.	*/
	uint64_t	hier;
	uint64_t	hirr;
	uint64_t	mm_csr;
	uint64_t	dc_stat;
	uint64_t	dc_addr;
	uint64_t	abox_ctl;
	uint64_t	biu_stat;	/* Bus Interface Unit Status.	*/
	uint64_t	biu_addr;
	uint64_t	biu_ctl;
	uint64_t	fill_syndrome;
	uint64_t	fill_addr;
	uint64_t	va;
	uint64_t	bc_tag;

	/* Registers from the cache and memory controller (21071-CA) */
	uint64_t	coma_gcr;	/* Error and Diag. Status.	*/
	uint64_t	coma_edsr;
	uint64_t	coma_ter;
	uint64_t	coma_elar;
	uint64_t	coma_ehar;
	uint64_t	coma_ldlr;
	uint64_t	coma_ldhr;
	uint64_t	coma_base0;
	uint64_t	coma_base1;
	uint64_t	coma_base2;
	uint64_t	coma_cnfg0;
	uint64_t	coma_cnfg1;
	uint64_t	coma_cnfg2;

	/* Registers from the PCI bridge (21071-DA) */
	uint64_t	epic_dcsr;	 /* Diag. Control and Status.	*/
	uint64_t	epic_pear;
	uint64_t	epic_sear;
	uint64_t	epic_tbr1;
	uint64_t	epic_tbr2;
	uint64_t	epic_pbr1;
	uint64_t	epic_pbr2;
	uint64_t	epic_pmr1;
	uint64_t	epic_pmr2;
	uint64_t	epic_harx1;
	uint64_t	epic_harx2;
	uint64_t	epic_pmlt;
	uint64_t	epic_tag0;
	uint64_t	epic_tag1;
	uint64_t	epic_tag2;
	uint64_t	epic_tag3;
	uint64_t	epic_tag4;
	uint64_t	epic_tag5;
	uint64_t	epic_tag6;
	uint64_t	epic_tag7;
	uint64_t	epic_data0;
	uint64_t	epic_data1;
	uint64_t	epic_data2;
	uint64_t	epic_data3;
	uint64_t	epic_data4;
	uint64_t	epic_data5;
	uint64_t	epic_data6;
	uint64_t	epic_data7;
} mc_uc_avanti;

/*
 * Information gathered from: OSF/1 header files.
 */


/*
 * EV5 Specific OSF/1 Pal Code Exception Logout Area Definitions
 * (inspired from OSF/1 Header files).
 */

/*
 * EV5 Specific common logout frame header.
 * *Almost* identical to the generic logout header listed in alpha_cpu.h.
 */

typedef struct {
	unsigned int	la_frame_size;		/* frame size */
	unsigned int	la_flags;		/* flags; see alpha_cpu.h */
	unsigned int	la_cpu_offset;		/* offset to CPU area */
	unsigned int	la_system_offset;	/* offset to system area */
	unsigned long	mcheck_code;		/* machine check code */
} mc_hdr_ev5;

/* Machine Check Codes */
#define	EV5_CORRECTED		0x86L
#define	SYSTEM_CORRECTED	0x201L

/*
 * EV5 Specific Machine Check logout frame for uncorrectable errors.
 * This is used to log uncorrectable errors such as double bit ECC errors.
 *
 * This typically resides in the CPU offset area of the logout frame.
 */

typedef struct {
	uint64_t	shadow[8];	/* Shadow reg. 8-14, 25		*/
	uint64_t	paltemp[24];	/* PAL TEMP REGS.		*/
	uint64_t	exc_addr;	/* Address of excepting ins.	*/
	uint64_t	exc_sum;	/* Summary of arithmetic traps.	*/
	uint64_t	exc_mask;	/* Exception mask.		*/
	uint64_t	pal_base;	/* Base address for PALcode.	*/
	uint64_t	isr;		/* Interrupt Status Reg.	*/
	uint64_t	icsr;		/* CURRENT SETUP OF EV5 IBOX	*/
	uint64_t	ic_perr_stat;	/*
					 * I-CACHE Reg:
					 *	<13> IBOX Timeout
					 *	<12> TAG parity
					 *	<11> Data parity
					 */
	uint64_t	dc_perr_stat;	/* D-CACHE error Reg:
					 * Bits set to 1:
					 *  <2> Data error in bank 0
					 *  <3> Data error in bank 1
					 *  <4> Tag error in bank 0
					 *  <5> Tag error in bank 1
					 */
	uint64_t	va;		/* Effective VA of fault or miss. */
	uint64_t	mm_stat;	/*
					 * Holds the reason for D-stream
					 * fault or D-cache parity errors
					 */
	uint64_t	sc_addr;	/*
					 * Address that was being accessed
					 * when EV5 detected Secondary cache
					 * failure.
					 */
	uint64_t	sc_stat;	/*
					 * Helps determine if the error was
					 * TAG/Data parity(Secondary Cache)
					 */
	uint64_t	bc_tag_addr;	/* Contents of EV5 BC_TAG_ADDR	  */
	uint64_t	ei_addr;	/*
					 * Physical address of any transfer
					 * that is logged in the EV5 EI_STAT
					 */
	uint64_t	fill_syndrome;	/* For correcting ECC errors.	  */
	uint64_t	ei_stat;	/*
					 * Helps identify reason of any
					 * processor uncorrectable error
					 * at its external interface.	
					 */
	uint64_t	ld_lock;	/* Contents of EV5 LD_LOCK register*/
} mc_uc_ev5;
#define	EV5_IC_PERR_IBOXTMO	0x2000

/*
 * EV5 Specific Machine Check logout frame for correctable errors.
 *
 * This is used to log correctable errors such as Single bit ECC errors.
 */
typedef struct {
	uint64_t	ei_addr;	/*
					 * Physical address of any transfer
					 * that is logged in the EV5 EI_STAT
					 */
	uint64_t	fill_syndrome;	/* For correcting ECC errors.	  */
	uint64_t	ei_stat;	/*
					 * Helps identify reason of any
					 * processor uncorrectable error
					 * at its external interface.	
					 */
	uint64_t	isr;		/* Interrupt Status Reg. 	  */
} mc_cc_ev5;


#ifdef	_KERNEL
extern void ev5_logout_print(mc_hdr_ev5 *, mc_uc_ev5 *);
#endif

/*
 * EV6/67 specific Machine Check logout definitions
 * from DS20E Service Guide, EK-K8F6W-SV. A01
 */

struct ev6_logout_area {
	struct alpha_logout_area la;
	uint32_t mchk_code;
	uint32_t mchk_rev;
};

struct ev6_logout_proc {
	uint64_t i_stat;
	uint64_t dc_stat;
	uint64_t c_addr;
	uint64_t c_syndrome1;
	uint64_t c_syndrome0;
	uint64_t c_stat;
	uint64_t c_sts;
	uint64_t mm_stat;
	uint64_t exc_addr;
	uint64_t ier_cm;
	uint64_t isum;
	uint64_t _r;
	uint64_t pal_base;
	uint64_t i_ctl;
	uint64_t pctx;
};

struct ev6_logout_sys {
	uint64_t flags;
	uint64_t dir;
	uint64_t misc;
	uint64_t p0_error;
	uint64_t p1_error;
};

