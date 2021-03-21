/*	$NetBSD: ka860.c,v 1.34.74.1 2021/03/21 21:09:08 thorpej Exp $	*/
/*
 * Copyright (c) 1986, 1988 Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)ka860.c	7.4 (Berkeley) 12/16/90
 */

/*
 * VAX 8600 specific routines.
 * Also contains abus spec's and memory init routines.
 *
 * Todo: Set up all four console lines in a VAX8600.
 * This is: local, remote, EMM and logical.
 *
 * ABus code added by Johnny Billquist 2010
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka860.c,v 1.34.74.1 2021/03/21 21:09:08 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/clock.h>
#include <machine/nexus.h>
#include <machine/ioa.h>
#include <machine/sid.h>
#include <machine/mainbus.h>

#include <vax/vax/gencons.h>

void	crlattach(void);

static	void	ka86_memerr(void);
static	int	ka86_mchk(void *);
static	void	ka86_reboot(int);
static	void	ka86_clrf(void);
static	void	ka860_conf(void);
static	void	ka86_attach_cpu(device_t);
static int abus_mainbus_match(device_t, cfdata_t, void *);
static void abus_mainbus_attach(device_t, device_t, void*);
static int abus_print(void *, const char *);

static const char * const ka86_devs[] = { "cpu", "abus", NULL };

const struct cpu_dep ka860_calls = {
	.cpu_mchk	= ka86_mchk,
	.cpu_memerr	= ka86_memerr,
	.cpu_conf	= ka860_conf,
	.cpu_gettime	= generic_gettime,
	.cpu_settime	= generic_settime,
	.cpu_vups	= 6,	/* ~VUPS */
	.cpu_scbsz	= 10,	/* SCB pages */
	.cpu_reboot	= ka86_reboot,
	.cpu_clrf	= ka86_clrf,
	.cpu_devs	= ka86_devs,
	.cpu_attach_cpu	= ka86_attach_cpu,
};

/*
 * 8600 memory register (MERG) bit definitions
 */
#define M8600_ICRD	0x400		/* inhibit crd interrupts */
#define M8600_TB_ERR	0xf00		/* translation buffer error mask */

/*
 * MDECC register
 */
#define M8600_ADDR_PE	0x080000	/* address parity error */
#define M8600_DBL_ERR	0x100000	/* data double bit error */
#define M8600_SNG_ERR	0x200000	/* data single bit error */
#define M8600_BDT_ERR	0x400000	/* bad data error */

/*
 * ESPA register is used to address scratch pad registers in the Ebox.
 * To access a register in the scratch pad, write the ESPA with the address
 * and then read the ESPD register.  
 *
 * NOTE:  In assmebly code, the mfpr instruction that reads the ESPD
 *	  register must immedately follow the mtpr instruction that setup
 *	  the ESPA register -- per the VENUS processor register spec.
 *
 * The scratchpad registers that are supplied for a single bit ECC 
 * error are:
 */
#define SPAD_MSTAT1	0x25		/* scratch pad mstat1 register	*/
#define SPAD_MSTAT2	0x26		/* scratch pad mstat2 register	*/
#define SPAD_MDECC	0x27		/* scratch pad mdecc register	*/
#define SPAD_MEAR	0x2a		/* scratch pad mear register	*/

#define M8600_MEMERR(mdecc) ((mdecc) & 0x780000)
#define M8600_HRDERR(mdecc) ((mdecc) & 0x580000)
#define M8600_SYN(mdecc) (((mdecc) >> 9) & 0x3f)
#define M8600_ADDR(mear) ((mear) & 0x3ffffffc)
#define M8600_ARRAY(mear) (((mear) >> 22) & 0x0f)

#define M8600_MDECC_BITS \
"\20\27BAD_DT_ERR\26SNG_BIT_ERR\25DBL_BIT_ERR\24ADDR_PE"

#define M8600_MSTAT1_BITS "\20\30CPR_PE_A\27CPR_PE_B\26ABUS_DT_PE\
\25ABUS_CTL_MSK_PE\24ABUS_ADR_PE\23ABUS_C/A_CYCLE\22ABUS_ADP_1\21ABUS_ADP_0\
\20TB_MISS\17BLK_HIT\16C0_TAG_MISS\15CHE_MISS\14TB_VAL_ERR\13TB_PTE_B_PE\
\12TB_PTE_A_PE\11TB_TAG_PE\10WR_DT_PE_B3\7WR_DT_PE_B2\6WR_DT_PE_B1\
\5WR_DT_PE_B0\4CHE_RD_DT_PE\3CHE_SEL\2ANY_REFL\1CP_BW_CHE_DT_PE"

#define M8600_MSTAT2_BITS "\20\20CP_BYT_WR\17ABUS_BD_DT_CODE\10MULT_ERR\
\7CHE_TAG_PE\6CHE_TAG_W_PE\5CHE_WRTN_BIT\4NXM\3CP-IO_BUF_ERR\2MBOX_LOCK"

/* log CRD errors */
void
ka86_memerr(void)
{
	int mdecc, mear, mstat1, mstat2, array;

	/*
	 * Scratchpad registers in the Ebox must be read by
	 * storing their ID number in ESPA and then immediately
	 * reading ESPD's contents with no other intervening
	 * machine instructions!
	 *
	 * The asm's below have a number of constants which
	 * are defined correctly above and in mtpr.h.
	 */
	__asm("mtpr $0x27,$0x4e; mfpr $0x4f,%0" : "=g" (mdecc));
			/* must acknowledge interrupt? */
	if (M8600_MEMERR(mdecc)) {
		__asm("mtpr $0x2a,$0x4e; mfpr $0x4f,%0" : "=g" (mear));
		__asm("mtpr $0x25,$0x4e; mfpr $0x4f,%0" : "=g" (mstat1));
		__asm("mtpr $0x26,$0x4e; mfpr $0x4f,%0" : "=g" (mstat2));
		array = M8600_ARRAY(mear);

		{
			char sbuf[256], sbuf2[256];

			printf("mcr0: ecc error, addr %x (array %d) syn %x\n",
				M8600_ADDR(mear), array, M8600_SYN(mdecc));

			snprintb(sbuf, sizeof(sbuf), M8600_MSTAT1_BITS, mstat1);
			snprintb(sbuf2, sizeof(sbuf2), M8600_MSTAT2_BITS, mstat2);
			printf("\tMSTAT1 = %s\n\tMSTAT2 = %s\n", sbuf, sbuf2);
		}

		mtpr(0, PR_EHSR);
		mtpr(mfpr(PR_MERG) | M8600_ICRD, PR_MERG);
	}
}

#define NMC8600 7
const char * const mc8600[] = {
	"unkn type",	"fbox error",	"ebox error",	"ibox error",
	"mbox error",	"tbuf error",	"mbox 1D error"
};
/* codes for above */
#define MC_FBOX		1
#define MC_EBOX		2
#define MC_IBOX		3
#define MC_MBOX		4
#define MC_TBUF		5
#define MC_MBOX1D	6

/* error bits */
#define MBOX_FE		0x8000		/* Mbox fatal error */
#define FBOX_SERV	0x10000000	/* Fbox service error */
#define IBOX_ERR	0x2000		/* Ibox error */
#define EBOX_ERR	0x1e00		/* Ebox error */
#define MBOX_1D		0x81d0000	/* Mbox 1D error */
#define EDP_PE		0x200

struct mc8600frame {
	int	mc86_bcnt;		/* byte count == 0x58 */
	int	mc86_ehmsts;
	int	mc86_evmqsav;
	int	mc86_ebcs;
	int	mc86_edpsr;
	int	mc86_cslint;
	int	mc86_ibesr;
	int	mc86_ebxwd1;
	int	mc86_ebxwd2;
	int	mc86_ivasav;
	int	mc86_vibasav;
	int	mc86_esasav;
	int	mc86_isasav;
	int	mc86_cpc;
	int	mc86_mstat1;
	int	mc86_mstat2;
	int	mc86_mdecc;
	int	mc86_merg;
	int	mc86_cshctl;
	int	mc86_mear;
	int	mc86_medr;
	int	mc86_accs;
	int	mc86_cses;
	int	mc86_pc;		/* trapped pc */
	int	mc86_psl;		/* trapped psl */
};

/* machine check */
int
ka86_mchk(void *cmcf)
{
	struct mc8600frame *mcf = (struct mc8600frame *)cmcf;
	int type;

	if (mcf->mc86_ebcs & MBOX_FE)
		mcf->mc86_ehmsts |= MC_MBOX;
	else if (mcf->mc86_ehmsts & FBOX_SERV)
		mcf->mc86_ehmsts |= MC_FBOX;
	else if (mcf->mc86_ebcs & EBOX_ERR) {
		if (mcf->mc86_ebcs & EDP_PE)
			mcf->mc86_ehmsts |= MC_MBOX;
		else
			mcf->mc86_ehmsts |= MC_EBOX;
	} else if (mcf->mc86_ehmsts & IBOX_ERR)
		mcf->mc86_ehmsts |= MC_IBOX;
	else if (mcf->mc86_mstat1 & M8600_TB_ERR)
		mcf->mc86_ehmsts |= MC_TBUF;
	else if ((mcf->mc86_cslint & MBOX_1D) == MBOX_1D)
		mcf->mc86_ehmsts |= MC_MBOX1D;

	type = mcf->mc86_ehmsts & 0x7;
	printf("machine check %x: %s\n", type,
	    type < NMC8600 ? mc8600[type] : "???");
	printf("\tehm.sts %x evmqsav %x ebcs %x edpsr %x cslint %x\n",
	    mcf->mc86_ehmsts, mcf->mc86_evmqsav, mcf->mc86_ebcs,
	    mcf->mc86_edpsr, mcf->mc86_cslint);
	printf("\tibesr %x ebxwd %x %x ivasav %x vibasav %x\n",
	    mcf->mc86_ibesr, mcf->mc86_ebxwd1, mcf->mc86_ebxwd2,
	    mcf->mc86_ivasav, mcf->mc86_vibasav);
	printf("\tesasav %x isasav %x cpc %x mstat %x %x mdecc %x\n",
	    mcf->mc86_esasav, mcf->mc86_isasav, mcf->mc86_cpc,
	    mcf->mc86_mstat1, mcf->mc86_mstat2, mcf->mc86_mdecc);
	printf("\tmerg %x cshctl %x mear %x medr %x accs %x cses %x\n",
	    mcf->mc86_merg, mcf->mc86_cshctl, mcf->mc86_mear,
	    mcf->mc86_medr, mcf->mc86_accs, mcf->mc86_cses);
	printf("\tpc %x psl %x\n", mcf->mc86_pc, mcf->mc86_psl);
	mtpr(0, PR_EHSR);
	return (MCHK_PANIC);
}

struct ka86 {
	unsigned snr:12,
		 plant:4,
		 eco:7,
		 v8650:1,
		 type:8;
};

/* The manufacturing plant information comes from EK-86XV1-MG-003
 * VAX 86XX System Maintenance Guide
 * /bqt
 */

static const char * const manuf[] = {"Unknown",
			      "Galway, Ireland",
			      "Franklin, MA",
			      "Burlington, VT",
			      "Marlboro, MA"};

const int mindex[] = {0,1,1,1, 0,2,2,2, 0,0,3,3, 3,4,4,4};

void
ka860_conf(void)
{
	/* Enable cache */
	mtpr(3, PR_CSWP);

	/* enable CRD reports */
	mtpr(mfpr(PR_MERG) & ~M8600_ICRD, PR_MERG);
}

void
ka86_attach_cpu(device_t self)
{
	struct ka86 * const ka86 = (void *)&vax_cpudata;
	int fpa;

	aprint_naive(": KA86%d, S/N %d, Rev. %c, manufactured in %s.\n",
	    ka86->v8650 ? 5 : 0, ka86->snr, ka86->eco+64,
	    manuf[mindex[ka86->plant]]);
	aprint_normal(": KA86%d, S/N %d, Rev. %c, manufactured in %s.\n",
	    ka86->v8650 ? 5 : 0, ka86->snr, ka86->eco+64,
	    manuf[mindex[ka86->plant]]);
	fpa = mfpr(PR_ACCS);
	if (fpa & 255) {
		aprint_naive_dev(self,
		    "FPA present: type %d, serial number %d\n", 
		    fpa & 255, fpa >> 16);
		aprint_normal_dev(self,
		    "FPA present: type %d, serial number %d\n", 
		    fpa & 255, fpa >> 16);
		mtpr(0x8000, PR_ACCS);
	} else {
		aprint_naive_dev(self, "no FPA\n");
		aprint_normal_dev(self, "no FPA\n");
        }

	/*
	 * Init CPU.
	 * Attach crl first.
	 */

	crlattach();
}

/*
 * Clear restart flag.
 */
void
ka86_clrf(void)
{
	/*
	 * We block all interrupts here so that there won't be any
	 * interrupts for an ongoing printout.
	 */
	int s = splhigh(), old = mfpr(PR_TXCS);

#define	WAIT	while ((mfpr(PR_TXCS) & GC_RDY) == 0) ;

	WAIT;

	/* Enable channel to console */
	mtpr(GC_LT|GC_WRT, PR_TXCS);
	WAIT;

	/* clear warm start flag */
	mtpr(GC_CWFL, PR_TXDB);
	WAIT;

	/* clear cold start flag */
	mtpr(GC_CCFL, PR_TXDB);
	WAIT;	

	/* restore old state */
	mtpr(old|GC_WRT, PR_TXCS);
	splx(s);
}

void
ka86_reboot(int howto)
{
	WAIT;

	/* Enable channel to console */ 
	mtpr(GC_LT|GC_WRT, PR_TXCS);
	WAIT;   

	mtpr(GC_BTFL, PR_TXDB);
	WAIT;

	__asm("halt");
}


CFATTACH_DECL_NEW(abus_mainbus, 0,
                  abus_mainbus_match, abus_mainbus_attach, NULL, NULL);

int abus_mainbus_match(device_t parent, cfdata_t self, void *aux)
{
  return (vax_bustype == VAX_ABUS);
}

void abus_mainbus_attach(device_t parent, device_t self, void *aux)
{
        struct mainbus_attach_args * const ma = aux;
        struct abus_attach_args aa;

        unsigned int tmp;
	volatile struct sbia_regs *sbiar;

        int     type, i;

        aprint_naive("\n");
        aprint_normal("\n");

	for (i = 0; i < NIOA8600; i++) {
		sbiar = (struct sbia_regs *)vax_map_physmem((paddr_t)IOA8600(i),
		    (IOAMAPSIZ / VAX_NBPG));
		if (badaddr(sbiar, 4)) {
			vax_unmap_physmem((vaddr_t)sbiar,
			    (IOAMAPSIZ / VAX_NBPG));
			continue;
		}

		tmp = sbiar->sbi_cfg;
		type = tmp & IOA_TYPMSK;

                switch (type) {

                case IOA_SBIA:

                        aa.aa_base = (bus_addr_t)(SBIA8600(i));
                        aa.aa_num = i;
                        aa.aa_name = "sbi";
                        aa.aa_type = tmp;
                        aa.aa_iot = ma->ma_iot;
                        aa.aa_dmat = ma->ma_dmat;

			sbiar->sbi_errsum = -1;
			sbiar->sbi_error = 0x1000;
			sbiar->sbi_fltsts = 0xc0000;

                        config_found(self, &aa, abus_print, CFARG_EOL);
                        break;

                default:
                  aprint_naive("IOAdapter %#x unsupported\n", type);
                  aprint_normal("IOAdapter %#x unsupported\n", type);
		}

		vax_unmap_physmem((vaddr_t)sbiar, (IOAMAPSIZ / VAX_NBPG));
	}
}

int abus_print(void *aux, const char *name)
{
  struct abus_attach_args *aa = aux;

  if (name) {
    aprint_naive("%s%d at %s\n", aa->aa_name, aa->aa_num, name);
    aprint_normal("%s%d at %s\n", aa->aa_name, aa->aa_num, name);
  }
  return UNSUPP;
}
