/*	$NetBSD: ka820.c,v 1.56 2014/03/26 08:01:21 christos Exp $	*/
/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
 *	@(#)ka820.c	7.4 (Berkeley) 12/16/90
 */

/*
 * KA820 specific CPU code.  (Note that the VAX8200 uses a KA820, not
 * a KA8200.  Sigh.)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ka820.c,v 1.56 2014/03/26 08:01:21 christos Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/proc.h>

#include <machine/ka820.h>
#include <machine/nexus.h>
#include <machine/clock.h>
#include <machine/scb.h>
#include <machine/mainbus.h>

#include <dev/cons.h>

#include <dev/bi/bireg.h>
#include <dev/bi/bivar.h>

#include <vax/vax/crx.h>

#include "ioconf.h"
#include "locators.h"

struct ka820port *ka820port_ptr;
struct rx50device *rx50device_ptr;
static volatile struct ka820clock *ka820_clkpage;
static int mastercpu;

static int ka820_match(device_t, cfdata_t, void *);
static void ka820_attach(device_t, device_t, void*);
static void ka820_memerr(void);
static void ka820_conf(void);
static int ka820_mchk(void *);
static int ka820_gettime(struct timeval *);
static void ka820_settime(struct timeval *);
static void rxcdintr(void *);
static void vaxbierr(void *);

static const char * const ka820_devs[] = { "bi", NULL };

const struct cpu_dep ka820_calls = {
	.cpu_mchk	= ka820_mchk,
	.cpu_memerr	= ka820_memerr,
	.cpu_conf	= ka820_conf,
	.cpu_gettime	= ka820_gettime,
	.cpu_settime	= ka820_settime,
	.cpu_devs	= ka820_devs,
	.cpu_vups	= 3,      /* ~VUPS */
	.cpu_scbsz	= 5,	/* SCB pages */
};

#if defined(MULTIPROCESSOR)
static void ka820_startslave(struct cpu_info *);
static void ka820_send_ipi(struct cpu_info *);
static void ka820_txrx(int, const char *, ...) __printflike(2, 3);
static void ka820_sendstr(int, const char *);
static void ka820_sergeant(int);
static int rxchar(void);
static void ka820_putc(int);
static void ka820_cnintr(void);
static void ka820_ipintr(void *);
cons_decl(gen);

const struct cpu_mp_dep ka820_mp_dep = {
	.cpu_startslave	= ka820_startslave,
	.cpu_send_ipi	= ka820_send_ipi,
	.cpu_cnintr	= ka820_cnintr,
};
#endif

CFATTACH_DECL_NEW(cpu_bi, 0,
    ka820_match, ka820_attach, NULL, NULL);

#ifdef notyet
extern struct pte BRAMmap[];
extern struct pte EEPROMmap[];
char bootram[KA820_BRPAGES * VAX_NBPG];
char eeprom[KA820_EEPAGES * VAX_NBPG];
#endif

static int
ka820_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bi_attach_args * const ba = aux;

	if (bus_space_read_2(ba->ba_iot, ba->ba_ioh, BIREG_DTYPE) != BIDT_KA820)
		return 0;

	if (cf->cf_loc[BICF_NODE] != BICF_NODE_DEFAULT &&
	    cf->cf_loc[BICF_NODE] != ba->ba_nodenr)
		return 0;

	return 1;
}

static void
ka820_attach(device_t parent, device_t self, void *aux)
{
	struct bi_attach_args * const ba = aux;
	struct cpu_info *ci;
	char c;
	int csr;
	u_short rev;

	rev = bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_DTYPE) >> 16;
	c = rev & 0x8000 ? '5' : '0';
	mastercpu = mfpr(PR_BINID);
	cpu_setmodel("VAX 82%c0", c);
	printf(": ka82%c (%s) CPU rev %d, u patch rev %d, sec patch %d\n",
	    c, mastercpu == ba->ba_nodenr ? "master" : "slave",
	    ((rev >> 11) & 15), ((rev >> 1) &1023), rev & 1);

	/* Allow for IPINTR */
	bus_space_write_4(ba->ba_iot, ba->ba_ioh,
	    BIREG_IPINTRMSK, BIIPINTR_MASK);

#if defined(MULTIPROCESSOR)
	if (ba->ba_nodenr != mastercpu) {
		v_putc = ka820_putc;	/* Need special console handling */
		cpu_slavesetup(self, ba->ba_nodenr);
		return;
	}
#endif

	ci = curcpu();
	self->dv_private = ci;	/* eww. but curcpu() is already too */
				/* entrenched to change */
	ci->ci_slotid = ba->ba_nodenr;
	ci->ci_cpuid = device_unit(self);
	ci->ci_dev = self;

#if defined(MULTIPROCESSOR)
	/*
	 * Catch interprocessor interrupts.
	 */
	scb_vecalloc(KA820_INT_IPINTR, ka820_ipintr, ci, SCB_ISTACK, NULL);
#endif
	/* reset the console and enable the RX50 */
	ka820port_ptr = (void *)vax_map_physmem(KA820_PORTADDR, 1);
	csr = ka820port_ptr->csr;
	csr &= ~KA820PORT_RSTHALT;	/* ??? */
	csr |= KA820PORT_CONSCLR | KA820PORT_CRDCLR | KA820PORT_CONSEN |
		KA820PORT_RXIE;
	ka820port_ptr->csr = csr;
	bus_space_write_4(ba->ba_iot, ba->ba_ioh,
	    BIREG_INTRDES, ba->ba_intcpu);
	bus_space_write_4(ba->ba_iot, ba->ba_ioh, BIREG_VAXBICSR,
	    bus_space_read_4(ba->ba_iot, ba->ba_ioh, BIREG_VAXBICSR) |
	    BICSR_SEIE | BICSR_HEIE);
}

void
ka820_conf(void)
{
	/*
	 * Setup parameters necessary to read time from clock chip.
	 */
	ka820_clkpage = (void *)vax_map_physmem(KA820_CLOCKADDR, 1);

	/* Enable cache */
	mtpr(0, PR_CADR);

	/* Steal the interrupt vectors that are unique for us */
	scb_vecalloc(KA820_INT_RXCD, rxcdintr, NULL, SCB_ISTACK, NULL);
	scb_vecalloc(0x50, vaxbierr, NULL, SCB_ISTACK, NULL);

	/* XXX - should be done somewhere else */
	scb_vecalloc(SCB_RX50, crxintr, NULL, SCB_ISTACK, NULL);
	rx50device_ptr = (void *)vax_map_physmem(KA820_RX50ADDR, 1);
#if defined(MULTIPROCESSOR)
	mp_dep_call = &ka820_mp_dep;
#endif
}

void
vaxbierr(void *arg)
{
	if (cold == 0)
		panic("vaxbierr");
}

#ifdef notdef
/*
 * MS820 support.
 */
struct ms820regs {
	struct	biiregs biic;		/* BI interface chip */
	u_long	ms_gpr[4];		/* the four gprs (unused) */
	int	ms_csr1;		/* control/status register 1 */
	int	ms_csr2;		/* control/status register 2 */
};
#endif

#define	MEMRD(reg) bus_space_read_4(sc->sc_iot, sc->sc_ioh, (reg))
#define MEMWR(reg, val) bus_space_write_4(sc->sc_iot, sc->sc_ioh, (reg), (val))

#define	MSREG_CSR1	0x100
#define	MSREG_CSR2	0x104
/*
 * Bits in CSR1.
 */
#define MS1_ERRSUM	0x80000000	/* error summary (ro) */
#define MS1_ECCDIAG	0x40000000	/* ecc diagnostic (rw) */
#define MS1_ECCDISABLE	0x20000000	/* ecc disable (rw) */
#define MS1_MSIZEMASK	0x1ffc0000	/* mask for memory size (ro) */
#define MS1_RAMTYMASK	0x00030000	/* mask for ram type (ro) */
#define MS1_RAMTY64K	0x00000000	/* 64K chips */
#define MS1_RAMTY256K	0x00010000	/* 256K chips */
#define MS1_RAMTY1MB	0x00020000	/* 1MB chips */
					/* type 3 reserved */
#define MS1_CRDINH	0x00008000	/* inhibit crd interrupts (rw) */
#define MS1_MEMVALID	0x00004000	/* memory has been written (ro) */
#define MS1_INTLK	0x00002000	/* interlock flag (ro) */
#define MS1_BROKE	0x00001000	/* broken (rw) */
#define MS1_MBZ		0x00000880	/* zero */
#define MS1_MWRITEERR	0x00000400	/* rds during masked write (rw) */
#define MS1_CNTLERR	0x00000200	/* internal timing busted (rw) */
#define MS1_INTLV	0x00000100	/* internally interleaved (ro) */
#define MS1_DIAGC	0x0000007f	/* ecc diagnostic bits (rw) */

/*
 * Bits in CSR2.
 */
#define MS2_RDSERR	0x80000000	/* rds error (rw) */
#define MS2_HIERR	0x40000000	/* high error rate (rw) */
#define MS2_CRDERR	0x20000000	/* crd error (rw) */
#define MS2_ADRSERR	0x10000000	/* rds due to addr par err (rw) */
#define MS2_MBZ		0x0f000080	/* zero */
#define MS2_ADDR	0x00fffe00	/* address in error (relative) (ro) */
#define MS2_INTLVADDR	0x00000100	/* error was in bank 1 (ro) */
#define MS2_SYN		0x0000007f	/* error syndrome (ro, rw diag) */

static int ms820_match(device_t, cfdata_t, void *);
static void ms820_attach(device_t, device_t, void*);

struct mem_bi_softc {
	device_t sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

CFATTACH_DECL_NEW(mem_bi, sizeof(struct mem_bi_softc),
    ms820_match, ms820_attach, NULL, NULL);

static int
ms820_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bi_attach_args * const ba = aux;

	if (bus_space_read_2(ba->ba_iot, ba->ba_ioh, BIREG_DTYPE) != BIDT_MS820)
		return 0;

	if (cf->cf_loc[BICF_NODE] != BICF_NODE_DEFAULT &&
	    cf->cf_loc[BICF_NODE] != ba->ba_nodenr)
		return 0;

	return 1;
}

static void
ms820_attach(device_t parent, device_t self, void *aux)
{
	struct mem_bi_softc * const sc = device_private(self);
	struct bi_attach_args * const ba = aux;

	sc->sc_dev = self;
	sc->sc_iot = ba->ba_iot;
	sc->sc_ioh = ba->ba_ioh;

	if ((MEMRD(BIREG_VAXBICSR) & BICSR_STS) == 0)
		aprint_error(": failed self test\n");
	else
		aprint_normal(": size %dMB, %s chips\n", ((MEMRD(MSREG_CSR1) & 
		    MS1_MSIZEMASK) >> 20), (MEMRD(MSREG_CSR1) & MS1_RAMTYMASK
		    ? MEMRD(MSREG_CSR1) & MS1_RAMTY256K ? "256K":"1M":"64K"));

	MEMWR(BIREG_INTRDES, ba->ba_intcpu);
	MEMWR(BIREG_VAXBICSR, MEMRD(BIREG_VAXBICSR) | BICSR_SEIE | BICSR_HEIE);

	MEMWR(MSREG_CSR1, MS1_MWRITEERR | MS1_CNTLERR);
	MEMWR(MSREG_CSR2, MS2_RDSERR | MS2_HIERR | MS2_CRDERR | MS2_ADRSERR);
}

static void
ka820_memerr(void)
{
	struct mem_bi_softc *sc;
	int m, hard, csr1, csr2;
	const char *type;

static const char b1[] = "\20\40ERRSUM\37ECCDIAG\36ECCDISABLE\20CRDINH\17VALID\
\16INTLK\15BROKE\13MWRITEERR\12CNTLERR\11INTLV";
static const char b2[] = "\20\40RDS\37HIERR\36CRD\35ADRS";

	char sbuf[sizeof(b1) + 64], sbuf2[sizeof(b2) + 64];

	for (m = 0; m < mem_cd.cd_ndevs; m++) {
		sc = device_lookup_private(&mem_cd, m);
		if (sc == NULL)
			continue;
		csr1 = MEMRD(MSREG_CSR1);
		csr2 = MEMRD(MSREG_CSR2);
		snprintb(sbuf, sizeof(sbuf), b1, csr1);
		snprintb(sbuf2, sizeof(sbuf2), b2, csr2);
		aprint_error_dev(sc->sc_dev, "csr1=%s csr2=%s\n", sbuf, sbuf2);
		if ((csr1 & MS1_ERRSUM) == 0)
			continue;
		hard = 1;
		if (csr1 & MS1_BROKE)
			type = "broke";
		else if (csr1 & MS1_CNTLERR)
			type = "cntl err";
		else if (csr2 & MS2_ADRSERR)
			type = "address parity err";
		else if (csr2 & MS2_RDSERR)
			type = "rds err";
		else if (csr2 & MS2_CRDERR) {
			hard = 0;
			type = "";
		} else
			type = "mysterious error";
		aprint_error_dev(sc->sc_dev, "%s%s%s addr %x bank %x syn %x\n",
		    hard ? "hard error: " : "soft ecc",
		    type, csr2 & MS2_HIERR ?  " (+ other rds or crd err)" : "",
		    ((csr2 & MS2_ADDR) + MEMRD(BIREG_SADR)) >> 9,
		    (csr2 & MS2_INTLVADDR) != 0, csr2 & MS2_SYN);
		MEMWR(MSREG_CSR1, csr1 | MS1_CRDINH);
		MEMWR(MSREG_CSR2, csr2);
	}
}

/* these are bits 0 to 6 in the summary field */
const char * const mc8200[] = {
	"cpu bad ipl",		"ucode lost err",
	"ucode par err",	"DAL par err",
	"BI bus err",		"BTB tag par",
	"cache tag par",
};
#define MC8200_BADIPL	0x01
#define MC8200_UERR	0x02
#define MC8200_UPAR	0x04
#define MC8200_DPAR	0x08
#define MC8200_BIERR	0x10
#define MC8200_BTAGPAR	0x20
#define MC8200_CTAGPAR	0x40

struct mc8200frame {
	int	mc82_bcnt;		/* byte count == 0x20 */
	int	mc82_summary;		/* summary parameter */
	int	mc82_param1;		/* parameter 1 */
	int	mc82_va;		/* va register */
	int	mc82_vap;		/* va prime register */
	int	mc82_ma;		/* memory address */
	int	mc82_status;		/* status word */
	int	mc82_epc;		/* error pc */
	int	mc82_upc;		/* micro pc */
	int	mc82_pc;		/* current pc */
	int	mc82_psl;		/* current psl */
};

static int
ka820_mchk(void *cmcf)
{
	struct mc8200frame *mcf = (struct mc8200frame *)cmcf;
	int i, type = mcf->mc82_summary;

	/* ignore BI bus errors during configuration */
	if (cold && type == MC8200_BIERR) {
		mtpr(PR_MCESR, 0xf);
		return (MCHK_RECOVERED);
	}

	/*
	 * SOME ERRORS ARE RECOVERABLE
	 * do it later
	 */
	printf("machine check %x: ", type);
	for (i = 0; i < sizeof (mc8200) / sizeof (mc8200[0]); i++)
		if (type & (1 << i))
			printf(" %s,", mc8200[i]);
	printf(" param1 %x\n", mcf->mc82_param1);
	printf(
"\tva %x va' %x ma %x pc %x psl %x\n\tstatus %x errpc %x upc %x\n",
		mcf->mc82_va, mcf->mc82_vap, mcf->mc82_ma,
		mcf->mc82_pc, mcf->mc82_psl,
		mcf->mc82_status, mcf->mc82_epc, mcf->mc82_upc);
	return (MCHK_PANIC);
}

#if defined(MULTIPROCESSOR)
#define	RXBUF	80
static char rxbuf[RXBUF];
static int got = 0, taken = 0;
static int expect = 0;
#endif
/*
 * Receive a character from logical console.
 */
static void
rxcdintr(void *arg)
{
	int c = mfpr(PR_RXCD);

	if (c == 0)
		return;

#if defined(MULTIPROCESSOR)
	if (expect == ((c >> 8) & 0xf))
		rxbuf[got++] = c & 0xff;

	if (got == RXBUF)
		got = 0;
#endif
}

#if defined(MULTIPROCESSOR)
int
rxchar(void)
{
	int ret;

	if (got == taken)
		return 0;

	ret = rxbuf[taken++];
	if (taken == RXBUF)
		taken = 0;
	return ret;
}
#endif

int
ka820_gettime(struct timeval *tvp)
{
	struct clock_ymdhms c;
	int s;

	while (ka820_clkpage->csr0 & KA820CLK_0_BUSY)
		;

	s = splhigh();
	c.dt_sec  = ka820_clkpage->sec;
	c.dt_min  = ka820_clkpage->min;
	c.dt_hour = ka820_clkpage->hr;
	c.dt_wday = ka820_clkpage->dayofwk;
	c.dt_day  = ka820_clkpage->day;
	c.dt_mon  = ka820_clkpage->mon;
	c.dt_year = ka820_clkpage->yr;
	splx(s);

	/* strange conversion */
	c.dt_sec  = ((c.dt_sec  << 7) | (c.dt_sec  >> 1)) & 0377;
	c.dt_min  = ((c.dt_min  << 7) | (c.dt_min  >> 1)) & 0377;
	c.dt_hour = ((c.dt_hour << 7) | (c.dt_hour >> 1)) & 0377;
	c.dt_wday = ((c.dt_wday << 7) | (c.dt_wday >> 1)) & 0377;
	c.dt_day  = ((c.dt_day  << 7) | (c.dt_day  >> 1)) & 0377;
	c.dt_mon  = ((c.dt_mon  << 7) | (c.dt_mon  >> 1)) & 0377;
	c.dt_year = ((c.dt_year << 7) | (c.dt_year >> 1)) & 0377;

	tvp->tv_sec = clock_ymdhms_to_secs(&c);
	return 0;
}

void
ka820_settime(struct timeval *tvp)
{
	struct clock_ymdhms c;

	clock_secs_to_ymdhms(tvp->tv_sec, &c);

	ka820_clkpage->csr1    = KA820CLK_1_SET;
	ka820_clkpage->sec     = ((c.dt_sec  << 1) | (c.dt_sec  >> 7)) & 0377;
	ka820_clkpage->min     = ((c.dt_min  << 1) | (c.dt_min  >> 7)) & 0377;
	ka820_clkpage->hr      = ((c.dt_hour << 1) | (c.dt_hour >> 7)) & 0377;
	ka820_clkpage->dayofwk = ((c.dt_wday << 1) | (c.dt_wday >> 7)) & 0377;
	ka820_clkpage->day     = ((c.dt_day  << 1) | (c.dt_day  >> 7)) & 0377;
	ka820_clkpage->mon     = ((c.dt_mon  << 1) | (c.dt_mon  >> 7)) & 0377;
	ka820_clkpage->yr      = ((c.dt_year << 1) | (c.dt_year >> 7)) & 0377;

	ka820_clkpage->csr1    = KA820CLK_1_GO;
}

#if defined(MULTIPROCESSOR)
static void
ka820_startslave(struct cpu_info *ci)
{
	const struct pcb *pcb = lwp_getpcb(ci->ci_data.cpu_onproc);
	const int id = ci->ci_slotid;
	int i;

	expect = id;
	/* First empty queue */
	for (i = 0; i < 10000; i++)
		if (rxchar())
			i = 0;
	ka820_txrx(id, "\020");		/* Send ^P to get attention */
	ka820_txrx(id, "I\r");			/* Init other end */
	ka820_txrx(id, "D/I 4 %x\r", (int)ci->ci_istack);	/* Interrupt stack */
	ka820_txrx(id, "D/I C %x\r", mfpr(PR_SBR));	/* SBR */
	ka820_txrx(id, "D/I D %x\r", mfpr(PR_SLR));	/* SLR */
	ka820_txrx(id, "D/I 10 %x\r", (int)pcb->pcb_paddr);	/* PCB for idle proc */
	ka820_txrx(id, "D/I 11 %x\r", mfpr(PR_SCBB));	/* SCB */
	ka820_txrx(id, "D/I 38 %x\r", mfpr(PR_MAPEN));	/* Enable MM */
	ka820_txrx(id, "S %x\r", (int)&vax_mp_tramp);	/* Start! */
	expect = 0;
	for (i = 0; i < 10000; i++)
		if (ci->ci_flags & CI_RUNNING)
			break;
	if (i == 10000)
		aprint_error_dev(ci->ci_dev, "(ID %d) failed starting??\n", id);
}

void
ka820_txrx(int id, const char *fmt, ...)
{
	char buf[20];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	ka820_sendstr(id, buf);
	ka820_sergeant(id);
}

static void
ka820_sendchr(int chr)
{
	/*
	 * It seems like mtpr to TXCD sets the V flag if it fails.
	 * Cannot check that flag in C...
	 */
	__asm volatile("1:;mtpr %0,$92;bvs 1b" :: "g"(chr));
}

void
ka820_sendstr(int id, const char *buf)
{
	u_int utchr;
	int ch, i;

	while (*buf) {
		utchr = *buf | id << 8;

		ka820_sendchr(utchr);
		buf++;
		i = 30000;
		while ((ch = rxchar()) == 0 && --i)
			;
		if (ch == 0)
			continue; /* failed */
	}
}

void
ka820_sergeant(int id)
{
	int i, ch, nserg;

	nserg = 0;
	for (i = 0; i < 30000; i++) {
		if ((ch = rxchar()) == 0)
			continue;
		if (ch == '>')
			nserg++;
		else
			nserg = 0;
		i = 0;
		if (nserg == 3)
			break;
	}
	/* What to do now??? */
}

/*
 * Write to master console.
 */
static volatile int ch = 0;

void
ka820_putc(int c)
{
	if (curcpu()->ci_flags & CI_MASTERCPU) {
		gencnputc(0, c);
		return;
	}
	ch = c;

	cpu_send_ipi(IPI_DEST_MASTER, IPI_SEND_CNCHAR);
	while (ch != 0)
		; /* Wait for master to handle */
}

/*
 * Got character IPI.
 */
void
ka820_cnintr(void)
{
	if (ch != 0)
		gencnputc(0, ch);
	ch = 0; /* Release slavecpu */
}

void
ka820_send_ipi(struct cpu_info *ci)
{
	mtpr(1 << ci->ci_cpuid, PR_IPIR);
}

void
ka820_ipintr(void *arg)
{
	cpu_handle_ipi();
}
#endif
