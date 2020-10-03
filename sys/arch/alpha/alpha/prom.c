/* $NetBSD: prom.c,v 1.58 2020/10/03 17:31:46 thorpej Exp $ */

/*
 * Copyright (c) 1992, 1994, 1995, 1996 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: prom.c,v 1.58 2020/10/03 17:31:46 thorpej Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <machine/rpb.h>
#include <machine/alpha.h>
#define	ENABLEPROM
#include <machine/prom.h>

#include <dev/cons.h>

/* XXX this is to fake out the console routines, while booting. */
struct consdev promcons = {
	.cn_getc = promcngetc,
	.cn_putc = promcnputc,
	.cn_pollc = nullcnpollc,
	.cn_dev = makedev(23,0),
	.cn_pri = 1
};

struct rpb	*hwrpb __read_mostly;
int		alpha_console;

extern struct prom_vec prom_dispatch_v;

bool		prom_interface_initialized;
int		prom_mapped = 1;	/* Is PROM still mapped? */

static kmutex_t	prom_lock;

static struct linux_kernel_params qemu_kernel_params;

#ifdef _PROM_MAY_USE_PROM_CONSOLE

pt_entry_t	prom_pte, saved_pte[1];	/* XXX */

static pt_entry_t *
prom_lev1map(void)
{
	struct alpha_pcb *apcb;

	/*
	 * Find the level 1 map that we're currently running on.
	 */
	apcb = (struct alpha_pcb *)
	    ALPHA_PHYS_TO_K0SEG((paddr_t)curlwp->l_md.md_pcbpaddr);

	return ((pt_entry_t *)ALPHA_PHYS_TO_K0SEG(apcb->apcb_ptbr << PGSHIFT));
}
#endif /* _PROM_MAY_USE_PROM_CONSOLE */

bool
prom_uses_prom_console(void)
{
#ifdef _PROM_MAY_USE_PROM_CONSOLE
	return (cputype == ST_DEC_21000);
#else
	return false;
#endif
}

static void
prom_init_cputype(const struct rpb * const rpb)
{
	cputype = rpb->rpb_type;
	if (cputype < 0) {
		/*
		 * At least some white-box systems have SRM which
		 * reports a systype that's the negative of their
		 * blue-box counterpart.
		 */
		cputype = -cputype;
	}
}

static void
prom_check_qemu(const struct rpb * const rpb)
{
	if (!alpha_is_qemu) {
		if (rpb->rpb_ssn[0] == 'Q' &&
		    rpb->rpb_ssn[1] == 'E' &&
		    rpb->rpb_ssn[2] == 'M' &&
		    rpb->rpb_ssn[3] == 'U') {
			alpha_is_qemu = true;
		}
	}
}

void
init_prom_interface(u_long ptb_pfn, struct rpb *rpb)
{

	if (prom_interface_initialized)
		return;

	struct crb *c;

	prom_init_cputype(rpb);
	prom_check_qemu(rpb);

	c = (struct crb *)((char *)rpb + rpb->rpb_crb_off);

	prom_dispatch_v.routine_arg = c->crb_v_dispatch;
	prom_dispatch_v.routine = c->crb_v_dispatch->entry_va;

	if (alpha_is_qemu) {
		/*
		 * Qemu has placed a Linux kernel parameter block
		 * at kernel_text[] - 0x6000.  We ensure the command
		 * line field is always NUL-terminated to simplify
		 * things later.
		 */
		extern char kernel_text[];
		memcpy(&qemu_kernel_params,
		       (void *)((vaddr_t)kernel_text - 0x6000),
		       sizeof(qemu_kernel_params));
		qemu_kernel_params.kernel_cmdline[
		    sizeof(qemu_kernel_params.kernel_cmdline) - 1] = '\0';
	}

#ifdef _PROM_MAY_USE_PROM_CONSOLE
	if (prom_uses_prom_console()) {
		/*
		 * XXX Save old PTE so we can remap the PROM, if
		 * XXX necessary.
		 */
		pt_entry_t * const l1pt =
		    (pt_entry_t *)ALPHA_PHYS_TO_K0SEG(ptb_pfn << PGSHIFT);
		prom_pte = l1pt[0] & ~PG_ASM;
	}
#endif /* _PROM_MAY_USE_PROM_CONSOLE */

	mutex_init(&prom_lock, MUTEX_DEFAULT, IPL_HIGH);
	prom_interface_initialized = true;
}

void
init_bootstrap_console(void)
{
	char buf[4];

	/* init_prom_interface() has already been called. */
	if (! prom_interface_initialized) {
		prom_halt(1);
	}

	prom_getenv(PROM_E_TTY_DEV, buf, sizeof(buf));
	alpha_console = buf[0] - '0';

	/* XXX fake out the console routines, for now */
	cn_tab = &promcons;
}

bool
prom_qemu_getenv(const char *var, char *buf, size_t buflen)
{
	const size_t varlen = strlen(var);
	const char *sp;

	if (!alpha_is_qemu) {
		return false;
	}

	sp = qemu_kernel_params.kernel_cmdline;
	for (;;) {
		sp = strstr(sp, var);
		if (sp == NULL) {
			return false;
		}
		sp += varlen;
		if (*sp++ != '=') {
			continue;
		}
		/* Found it. */
		break;
	}

	while (--buflen) {
		if (*sp == ' ' || *sp == '\t' || *sp == '\0') {
			break;
		}
		*buf++ = *sp++;
	}
	*buf = '\0';

	return true;
}

#ifdef _PROM_MAY_USE_PROM_CONSOLE
static void prom_cache_sync(void);
#endif

void
prom_enter(void)
{

	mutex_enter(&prom_lock);

#ifdef _PROM_MAY_USE_PROM_CONSOLE
	/*
	 * If we have not yet switched out of the PROM's context
	 * (i.e. the first one after alpha_init()), then the PROM
	 * is still mapped, regardless of the `prom_mapped' setting.
	 */
	if (! prom_mapped) {
		if (!prom_uses_prom_console())
			panic("prom_enter");
		{
			pt_entry_t *lev1map;

			lev1map = prom_lev1map();	/* XXX */
			saved_pte[0] = lev1map[0];	/* XXX */
			lev1map[0] = prom_pte;		/* XXX */
		}
		prom_cache_sync();			/* XXX */
	}
#endif
}

void
prom_leave(void)
{

#ifdef _PROM_MAY_USE_PROM_CONSOLE
	/*
	 * See comment above.
	 */
	if (! prom_mapped) {
		if (!prom_uses_prom_console())
			panic("prom_leave");
		{
			pt_entry_t *lev1map;

			lev1map = prom_lev1map();	/* XXX */
			lev1map[0] = saved_pte[0];	/* XXX */
		}
		prom_cache_sync();			/* XXX */
	}
#endif
	mutex_exit(&prom_lock);
}

#ifdef _PROM_MAY_USE_PROM_CONSOLE
static void
prom_cache_sync(void)
{
	ALPHA_TBIA();
	alpha_pal_imb();
}
#endif

/*
 * promcnputc:
 *
 * Remap char before passing off to prom.
 *
 * Prom only takes 32 bit addresses. Copy char somewhere prom can
 * find it. This routine will stop working after pmap_rid_of_console
 * is called in alpha_init. This is due to the hard coded address
 * of the console area.
 */
void
promcnputc(dev_t dev, int c)
{
	prom_return_t ret;
	unsigned char *to = (unsigned char *)0x20000000;

	/* XXX */
	if (alpha_is_qemu)
		return;

	prom_enter();
	*to = c;

	do {
		ret.bits = prom_putstr(alpha_console, to, 1);
	} while ((ret.u.retval & 1) == 0);

	prom_leave();
}

/*
 * promcngetc:
 *
 * Wait for the prom to get a real char and pass it back.
 */
int
promcngetc(dev_t dev)
{
	prom_return_t ret;

	/* XXX */
	if (alpha_is_qemu)
		return 0;

	for (;;) {
		prom_enter();
	        ret.bits = prom_getc(alpha_console);
		prom_leave();
	        if (ret.u.status == 0 || ret.u.status == 1)
	                return (ret.u.retval);
	}
}

/*
 * promcnlookc:
 *
 * See if prom has a real char and pass it back.
 */
int
promcnlookc(dev_t dev, char *cp)
{
	prom_return_t ret;

	/* XXX */
	if (alpha_is_qemu)
		return 0;

	prom_enter();
	ret.bits = prom_getc(alpha_console);
	prom_leave();
	if (ret.u.status == 0 || ret.u.status == 1) {
		*cp = ret.u.retval;
		return 1;
	} else
		return 0;
}

int
prom_getenv(int id, char *buf, int len)
{
	unsigned char *to = (unsigned char *)0x20000000;
	prom_return_t ret;

	/* XXX */
	if (alpha_is_qemu)
		return 0;

	prom_enter();
	ret.bits = prom_getenv_disp(id, to, len);
	if (ret.u.status & 0x4)
		ret.u.retval = 0;
	len = uimin(len - 1, ret.u.retval);
	memcpy(buf, to, len);
	buf[len] = '\0';
	prom_leave();

	return len;
}

void
prom_halt(int halt)
{
	struct pcs *p;

	/*
	 * Turn off interrupts, for sanity.
	 */
	(void) splhigh();

	/*
	 * Set "boot request" part of the CPU state depending on what
	 * we want to happen when we halt.
	 */
	p = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);
	p->pcs_flags &= ~(PCS_RC | PCS_HALT_REQ);
	if (halt)
		p->pcs_flags |= PCS_HALT_STAY_HALTED;
	else
		p->pcs_flags |= PCS_HALT_WARM_BOOT;

	/*
	 * Halt the machine.
	 */
	alpha_pal_halt();
}

uint64_t
hwrpb_checksum(void)
{
	uint64_t *p, sum;
	int i;

	for (i = 0, p = (uint64_t *)hwrpb, sum = 0;
	    i < (offsetof(struct rpb, rpb_checksum) / sizeof (uint64_t));
	    i++, p++)
		sum += *p;

	return (sum);
}

void
hwrpb_primary_init(void)
{
	struct pcb *pcb;
	struct pcs *p;

	p = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);

	/* Initialize the primary's HWPCB and the Virtual Page Table Base. */
	pcb = lwp_getpcb(&lwp0);
	memcpy(p->pcs_hwpcb, &pcb->pcb_hw, sizeof(pcb->pcb_hw));
	hwrpb->rpb_vptb = VPTBASE;

	hwrpb->rpb_checksum = hwrpb_checksum();
}

void
hwrpb_restart_setup(void)
{
	struct pcs *p;

	/* Clear bootstrap-in-progress flag since we're done bootstrapping */
	p = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);
	p->pcs_flags &= ~PCS_BIP;

	/* when 'c'ontinuing from console halt, do a dump */
	hwrpb->rpb_rest_term = (uint64_t)&XentRestart;
	hwrpb->rpb_rest_term_val = 0x1;

	hwrpb->rpb_checksum = hwrpb_checksum();

	p->pcs_flags |= (PCS_RC | PCS_CV);
}

uint64_t
console_restart(struct trapframe *framep)
{
	struct pcs *p;

	/* Clear restart-capable flag, since we can no longer restart. */
	p = LOCATE_PCS(hwrpb, hwrpb->rpb_primary_cpu_id);
	p->pcs_flags &= ~PCS_RC;

	/* Fill in the missing frame slots */

	framep->tf_regs[FRAME_PS] = p->pcs_halt_ps;
	framep->tf_regs[FRAME_PC] = p->pcs_halt_pc;
	framep->tf_regs[FRAME_T11] = p->pcs_halt_r25;
	framep->tf_regs[FRAME_RA] = p->pcs_halt_r26;
	framep->tf_regs[FRAME_T12] = p->pcs_halt_r27;

	panic("user requested console halt");

	return (1);
}
