/*	$NetBSD: db_interface.c,v 1.51.2.1 2014/08/20 00:03:20 tls Exp $ */
/*	$OpenBSD: db_interface.c,v 1.2 1996/12/28 06:21:50 rahnds Exp $	*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_interface.c,v 1.51.2.1 2014/08/20 00:03:20 tls Exp $");

#define USERACC

#include "opt_ddb.h"
#include "opt_kgdb.h"
#include "opt_ppcarch.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <dev/cons.h>

#include <powerpc/db_machdep.h>
#include <powerpc/frame.h>
#include <powerpc/spr.h>
#include <powerpc/pte.h>
#include <powerpc/psl.h>

#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/spr.h>
#include <powerpc/oea/bat.h>
#include <powerpc/oea/cpufeat.h>
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/cpu.h>
#include <powerpc/ibm4xx/spr.h>
#include <machine/tlb.h>
#include <uvm/uvm_extern.h>
#endif

#ifdef PPC_BOOKE
#include <powerpc/booke/cpuvar.h>
#include <powerpc/booke/spr.h>
#endif

#ifdef DDB
#include <ddb/db_sym.h>
#include <ddb/db_command.h>
#include <ddb/db_extern.h>
#include <ddb/db_access.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/ddbvar.h>
#endif

#ifdef KGDB
#include <sys/kgdb.h>
#define db_printf printf
#endif

#include <dev/ofw/openfirm.h>

int	db_active = 0;

db_regs_t ddb_regs;

void ddb_trap(void);				/* Call into trap_subr.S */
int ddb_trap_glue(struct trapframe *);		/* Called from trap_subr.S */
#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
static void db_show_bat(db_expr_t, bool, db_expr_t, const char *);
static void db_show_mmu(db_expr_t, bool, db_expr_t, const char *);
#endif /* PPC_OEA || PPC_OEA64 || PPC_OEA64_BRIDGE */
#ifdef PPC_IBM4XX
static void db_ppc4xx_ctx(db_expr_t, bool, db_expr_t, const char *);
static void db_ppc4xx_pv(db_expr_t, bool, db_expr_t, const char *);
static void db_ppc4xx_reset(db_expr_t, bool, db_expr_t, const char *);
static void db_ppc4xx_tf(db_expr_t, bool, db_expr_t, const char *);
static void db_ppc4xx_dumptlb(db_expr_t, bool, db_expr_t, const char *);
static void db_ppc4xx_dcr(db_expr_t, bool, db_expr_t, const char *);
static db_expr_t db_ppc4xx_mfdcr(db_expr_t);
static void db_ppc4xx_mtdcr(db_expr_t, db_expr_t);
#ifdef USERACC
static void db_ppc4xx_useracc(db_expr_t, bool, db_expr_t, const char *);
#endif
#endif /* PPC_IBM4XX */

#ifdef PPC_BOOKE
static void db_ppcbooke_reset(db_expr_t, bool, db_expr_t, const char *);
static void db_ppcbooke_splhist(db_expr_t, bool, db_expr_t, const char *);
static void db_ppcbooke_tf(db_expr_t, bool, db_expr_t, const char *);
static void db_ppcbooke_dumptlb(db_expr_t, bool, db_expr_t, const char *);
#endif

#ifdef DDB
const struct db_command db_machine_command_table[] = {
#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
	{ DDB_ADD_CMD("bat",	db_show_bat,		0,
	  "Print BAT register translations", NULL,NULL) },
	{ DDB_ADD_CMD("mmu",	db_show_mmu,		0,
	  "Print MMU registers", NULL,NULL) },
#endif /* PPC_OEA || PPC_OEA64 || PPC_OEA64_BRIDGE */
#ifdef PPC_IBM4XX
	{ DDB_ADD_CMD("ctx",	db_ppc4xx_ctx,		0,
	  "Print process MMU context information", NULL,NULL) },
	{ DDB_ADD_CMD("pv",	db_ppc4xx_pv,		0,
	  "Print PA->VA mapping information",
	  "address",
	  "   address:\tphysical address to look up") },
	{ DDB_ADD_CMD("reset",	db_ppc4xx_reset,	0,
	  "Reset the system ", NULL,NULL) },
	{ DDB_ADD_CMD("tf",	db_ppc4xx_tf,		0,
	  "Display the contents of the trapframe",
	  "address",
	  "   address:\tthe struct trapframe to print") },
	{ DDB_ADD_CMD("tlb",	db_ppc4xx_dumptlb,	0,
	  "Display instruction translation storage buffer information.",
	  NULL,NULL) },
	{ DDB_ADD_CMD("dcr",	db_ppc4xx_dcr,		CS_MORE|CS_SET_DOT,
	  "Set the DCR register",
	  "dcr",
	  "   dcr:\tNew DCR value (between 0x0 and 0x3ff)") },
#ifdef USERACC
	{ DDB_ADD_CMD("user",	db_ppc4xx_useracc,	0,
	   "Display user memory.", "[address][,count]",
	   "   address:\tuserspace address to start\n"
	   "   count:\tnumber of bytes to display") },
#endif
#endif /* PPC_IBM4XX */
#ifdef PPC_BOOKE
	{ DDB_ADD_CMD("reset",	db_ppcbooke_reset,	0,
	  "Reset the system ", NULL,NULL) },
	{ DDB_ADD_CMD("tf",	db_ppcbooke_tf,		0,
	  "Display the contents of the trapframe",
	  "address",
	  "   address:\tthe struct trapframe to print") },
	{ DDB_ADD_CMD("splhist", db_ppcbooke_splhist,	0,
	  "Display the splraise/splx splx",
	  NULL, NULL) },
	{ DDB_ADD_CMD("tlb",	db_ppcbooke_dumptlb,	0,
	  "Display instruction translation storage buffer information.",
	  NULL,NULL) },
#endif /* PPC_BOOKE */
	{ DDB_ADD_CMD(NULL,	NULL,			0,
	  NULL,NULL,NULL) }
};

void
cpu_Debugger(void)
{
#ifdef PPC_BOOKE
	const register_t msr = mfmsr();
	__asm volatile("wrteei 0\n\ttweq\t1,1");
	mtmsr(msr);
	__asm volatile("isync");
#else
	ddb_trap();
#endif
}
#endif /* DDB */

int
ddb_trap_glue(struct trapframe *tf)
{
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	if ((tf->tf_srr1 & PSL_PR) == 0)
		return kdb_trap(tf->tf_exc, tf);
#else /* PPC_OEA */
	if ((tf->tf_srr1 & PSL_PR) == 0 &&
	    (tf->tf_exc == EXC_TRC ||
	     tf->tf_exc == EXC_RUNMODETRC ||
	     (tf->tf_exc == EXC_PGM && (tf->tf_srr1 & 0x20000)) ||
	     tf->tf_exc == EXC_BPT ||
	     tf->tf_exc == EXC_DSI)) {
		int type = tf->tf_exc;
		if (type == EXC_PGM && (tf->tf_srr1 & 0x20000)) {
			type = T_BREAKPOINT;
		}
		return kdb_trap(type, tf);
	}
#endif
	return 0;
}

int
kdb_trap(int type, void *v)
{
	struct trapframe *tf = v;

#ifdef DDB
	if (db_recover != 0 && (type != -1 && type != T_BREAKPOINT)) {
		db_error("Faulted in DDB; continuing...\n");
		/* NOTREACHED */
	}
#endif

	/* XXX Should switch to kdb's own stack here. */

	memcpy(DDB_REGS->r, tf->tf_fixreg, 32 * sizeof(u_int32_t));
	DDB_REGS->iar = tf->tf_srr0;
	DDB_REGS->msr = tf->tf_srr1;
	DDB_REGS->lr = tf->tf_lr;
	DDB_REGS->ctr = tf->tf_ctr;
	DDB_REGS->cr = tf->tf_cr;
	DDB_REGS->xer = tf->tf_xer;
#ifdef PPC_OEA
	DDB_REGS->mq = tf->tf_mq;
#elif defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	DDB_REGS->dear = tf->tf_dear;
	DDB_REGS->esr = tf->tf_esr;
	DDB_REGS->pid = tf->tf_pid;
#endif

#ifdef DDB
	db_active++;
	cnpollc(1);
	db_trap(type, 0);
	cnpollc(0);
	db_active--;
#elif defined(KGDB)
	if (!kgdb_trap(type, DDB_REGS))
		return 0;
#endif

	/* KGDB isn't smart about advancing PC if we
	 * take a breakpoint trap after kgdb_active is set.
	 * Therefore, we help out here.
	 */
	if (IS_BREAKPOINT_TRAP(type, 0)) {
		int bkpt;
		db_read_bytes(PC_REGS(DDB_REGS),BKPT_SIZE,(void *)&bkpt);
		if (bkpt== BKPT_INST) {
			PC_REGS(DDB_REGS) += BKPT_SIZE;
		}
	}

	memcpy(tf->tf_fixreg, DDB_REGS->r, 32 * sizeof(u_int32_t));
	tf->tf_srr0 = DDB_REGS->iar;
	tf->tf_srr1 = DDB_REGS->msr;
	tf->tf_lr = DDB_REGS->lr;
	tf->tf_ctr = DDB_REGS->ctr;
	tf->tf_cr = DDB_REGS->cr;
	tf->tf_xer = DDB_REGS->xer;
#ifdef PPC_OEA
	tf->tf_mq = DDB_REGS->mq;
#endif
#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
	tf->tf_dear = DDB_REGS->dear;
	tf->tf_esr = DDB_REGS->esr;
	tf->tf_pid = DDB_REGS->pid;
#endif

	return 1;
}

#if defined (PPC_OEA) || defined(PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
static void
print_battranslation(struct bat *bat, unsigned int blidx)
{
	static const char batsizes[][6] = {
		"128KB",
		"256KB",
		"512KB",
		"1MB",
		"2MB",
		"4MB",
		"8MB",
		"16MB",
		"32MB",
		"64MB",
		"128MB",
		"256MB",
		"512MB",
		"1GB",
		"2GB",
		"4GB",
	};
	vsize_t len;

	len = (0x20000L << blidx) - 1;
	db_printf("\t%08lx %08lx %5s: 0x%08lx..0x%08lx -> 0x%08lx physical\n",
	    bat->batu, bat->batl, batsizes[blidx], bat->batu & ~len,
	    (bat->batu & ~len) + len, bat->batl & ~len);
}

static void
print_batmodes(register_t super, register_t user, register_t pp)
{
	static const char *const accessmodes[] = {
		"none",
		"ro soft",
		"read/write",
		"read only"
	};

	db_printf("\tvalid: %c%c  access: %-10s  memory:",
	    super ? 'S' : '-', user ? 'U' : '-', accessmodes[pp]);
}

static void
print_wimg(register_t wimg)
{
	if (wimg & BAT_W)
		db_printf(" wrthrough");
	if (wimg & BAT_I)
		db_printf(" nocache");
	if (wimg & BAT_M)
		db_printf(" coherent");
	if (wimg & BAT_G)
		db_printf(" guard");
}

static void
print_bat(struct bat *bat)
{
	if ((bat->batu & BAT_V) == 0) {
		db_printf("\tdisabled\n\n");
		return;
	}
	print_battranslation(bat,
	    30 - __builtin_clz((bat->batu & (BAT_XBL|BAT_BL))|2));
	print_batmodes(bat->batu & BAT_Vs, bat->batu & BAT_Vu,
	    bat->batl & BAT_PP);
	print_wimg(bat->batl & BAT_WIMG);
	db_printf("\n");
}

#ifdef PPC_OEA601
static void
print_bat601(struct bat *bat)
{
	if ((bat->batl & BAT601_V) == 0) {
		db_printf("\tdisabled\n\n");
		return;
	}
	print_battranslation(bat, 32 - __builtin_clz(bat->batl & BAT601_BSM));
	print_batmodes(bat->batu & BAT601_Ks, bat->batu & BAT601_Ku,
	    bat->batu & BAT601_PP);
	print_wimg(bat->batu & (BAT601_W | BAT601_I | BAT601_M));
	db_printf("\n");
}
#endif

static void
db_show_bat(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct bat ibat[8];
	struct bat dbat[8];
	unsigned int cpuvers;
	u_int i;
	u_int maxbat = (oeacpufeat & OEACPU_HIGHBAT) ? 8 : 4;

	if (oeacpufeat & OEACPU_NOBAT)
		return;

	cpuvers = mfpvr() >> 16;

	ibat[0].batu = mfspr(SPR_IBAT0U);
	ibat[0].batl = mfspr(SPR_IBAT0L);
	ibat[1].batu = mfspr(SPR_IBAT1U);
	ibat[1].batl = mfspr(SPR_IBAT1L);
	ibat[2].batu = mfspr(SPR_IBAT2U);
	ibat[2].batl = mfspr(SPR_IBAT2L);
	ibat[3].batu = mfspr(SPR_IBAT3U);
	ibat[3].batl = mfspr(SPR_IBAT3L);
	if (maxbat == 8) {
		ibat[4].batu = mfspr(SPR_IBAT4U);
		ibat[4].batl = mfspr(SPR_IBAT4L);
		ibat[5].batu = mfspr(SPR_IBAT5U);
		ibat[5].batl = mfspr(SPR_IBAT5L);
		ibat[6].batu = mfspr(SPR_IBAT6U);
		ibat[6].batl = mfspr(SPR_IBAT6L);
		ibat[7].batu = mfspr(SPR_IBAT7U);
		ibat[7].batl = mfspr(SPR_IBAT7L);
	}

	if (cpuvers != MPC601) {
		/* The 601 has only four unified BATs */
		dbat[0].batu = mfspr(SPR_DBAT0U);
		dbat[0].batl = mfspr(SPR_DBAT0L);
		dbat[1].batu = mfspr(SPR_DBAT1U);
		dbat[1].batl = mfspr(SPR_DBAT1L);
		dbat[2].batu = mfspr(SPR_DBAT2U);
		dbat[2].batl = mfspr(SPR_DBAT2L);
		dbat[3].batu = mfspr(SPR_DBAT3U);
		dbat[3].batl = mfspr(SPR_DBAT3L);
		if (maxbat == 8) {
			dbat[4].batu = mfspr(SPR_DBAT4U);
			dbat[4].batl = mfspr(SPR_DBAT4L);
			dbat[5].batu = mfspr(SPR_DBAT5U);
			dbat[5].batl = mfspr(SPR_DBAT5L);
			dbat[6].batu = mfspr(SPR_DBAT6U);
			dbat[6].batl = mfspr(SPR_DBAT6L);
			dbat[7].batu = mfspr(SPR_DBAT7U);
			dbat[7].batl = mfspr(SPR_DBAT7L);
		}
	}

	for (i = 0; i < maxbat; i++) {
#ifdef PPC_OEA601
		if (cpuvers == MPC601) {
			db_printf("bat[%u]:\n", i);
			print_bat601(&ibat[i]);
		} else
#endif
		{
			db_printf("ibat[%u]:\n", i);
			print_bat(&ibat[i]);
			db_printf("dbat[%u]:\n", i);
			print_bat(&dbat[i]);
		}
	}
}

static void
db_show_mmu(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	paddr_t sdr1;

	__asm volatile ("mfsdr1 %0" : "=r"(sdr1));
	db_printf("sdr1\t\t0x%08lx\n", sdr1);

#if defined(PPC_OEA64) || defined(PPC_OEA64_BRIDGE)
	if (oeacpufeat & (OEACPU_64|OEACPU_64_BRIDGE)) {
		__asm volatile ("mfasr %0" : "=r"(sdr1));
		db_printf("asr\t\t0x%08lx\n", sdr1);
	}
#endif
#if defined(PPC_OEA) || defined(PPC_OEA64_BRIDGE)
	if ((oeacpufeat & OEACPU_64) == 0) {
		vaddr_t saddr = 0;
		for (u_int i = 0; i <= 0xf; i++) {
			register_t sr;
			if ((i & 3) == 0)
				db_printf("sr%d-%d\t\t", i, i+3);
			__asm volatile ("mfsrin %0,%1" : "=r"(sr) : "r"(saddr));
			db_printf("0x%08lx   %c", sr, (i&3) == 3 ? '\n' : ' ');
			saddr += 1 << ADDR_SR_SHFT;
		}
	}
#endif
}
#endif /* PPC_OEA || PPC_OEA64 || PPC_OEA64_BRIDGE */

#if defined(PPC_IBM4XX) || defined(PPC_BOOKE)
db_addr_t
branch_taken(int inst, db_addr_t pc, db_regs_t *regs)
{

	if ((inst & M_B ) == I_B || (inst & M_B ) == I_BL) {
		db_expr_t off;
		off = ((db_expr_t)((inst & 0x03fffffc) << 6)) >> 6;
		return (((inst & 0x2) ? 0 : pc) + off);
	}

	if ((inst & M_BC) == I_BC || (inst & M_BC) == I_BCL) {
		db_expr_t off;
		off = ((db_expr_t)((inst & 0x0000fffc) << 16)) >> 16;
		return (((inst & 0x2) ? 0 : pc) + off);
	}

	if ((inst & M_RTS) == I_RTS || (inst & M_RTS) == I_BLRL)
		return (regs->lr);

	if ((inst & M_BCTR) == I_BCTR || (inst & M_BCTR) == I_BCTRL)
		return (regs->ctr);

	db_printf("branch_taken: can't figure out branch target for 0x%x!\n",
	    inst);
	return (0);
}
#endif /* PPC_IBM4XX || PPC_BOOKE */


#ifdef PPC_IBM4XX
#ifdef DDB
static void
db_ppc4xx_ctx(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct proc *p;

	/* XXX LOCKING XXX */
	for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
		if (p->p_stat) {
			db_printf("process %p:", p);
			db_printf("pid:%d pmap:%p ctx:%d %s\n",
				p->p_pid, p->p_vmspace->vm_map.pmap,
				p->p_vmspace->vm_map.pmap->pm_ctx,
				p->p_comm);
		}
	}
	return;
}

static void
db_ppc4xx_pv(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct pv_entry {
		struct pv_entry *pv_next;	/* Linked list of mappings */
		vaddr_t pv_va;			/* virtual address of mapping */
		struct pmap *pv_pm;
	};
	struct pv_entry *pa_to_pv(paddr_t);
	struct pv_entry *pv;

	if (!have_addr) {
		db_printf("pv: <pa>\n");
		return;
	}
	pv = pa_to_pv(addr);
	db_printf("pv at %p\n", pv);
	while (pv && pv->pv_pm) {
		db_printf("next %p va %p pmap %p\n", pv->pv_next,
			(void *)pv->pv_va, pv->pv_pm);
		pv = pv->pv_next;
	}
}

static void
db_ppc4xx_reset(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	printf("Reseting...\n");
	ppc4xx_reset();
}

static void
db_ppc4xx_tf(db_expr_t addr, bool have_addr, db_expr_t count, const char *modif)
{
	struct trapframe *tf;


	if (have_addr) {
		tf = (struct trapframe *)addr;

		db_printf("r0-r3:  \t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[0], tf->tf_fixreg[1],
			tf->tf_fixreg[2], tf->tf_fixreg[3]);
		db_printf("r4-r7:  \t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[4], tf->tf_fixreg[5],
			tf->tf_fixreg[6], tf->tf_fixreg[7]);
		db_printf("r8-r11: \t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[8], tf->tf_fixreg[9],
			tf->tf_fixreg[10], tf->tf_fixreg[11]);
		db_printf("r12-r15:\t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[12], tf->tf_fixreg[13],
			tf->tf_fixreg[14], tf->tf_fixreg[15]);
		db_printf("r16-r19:\t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[16], tf->tf_fixreg[17],
			tf->tf_fixreg[18], tf->tf_fixreg[19]);
		db_printf("r20-r23:\t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[20], tf->tf_fixreg[21],
			tf->tf_fixreg[22], tf->tf_fixreg[23]);
		db_printf("r24-r27:\t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[24], tf->tf_fixreg[25],
			tf->tf_fixreg[26], tf->tf_fixreg[27]);
		db_printf("r28-r31:\t%8.8lx %8.8lx %8.8lx %8.8lx\n",
			tf->tf_fixreg[28], tf->tf_fixreg[29],
			tf->tf_fixreg[30], tf->tf_fixreg[31]);

		db_printf("lr: %8.8lx cr: %8.8x xer: %8.8x ctr: %8.8lx\n",
			tf->tf_lr, tf->tf_cr, tf->tf_xer, tf->tf_ctr);
		db_printf("srr0(pc): %8.8lx srr1(msr): %8.8lx "
			"dear: %8.8lx esr: %8.8x\n",
			tf->tf_srr0, tf->tf_srr1, tf->tf_dear, tf->tf_esr);
		db_printf("exc: %8.8x pid: %8.8x\n",
			tf->tf_exc, tf->tf_pid);
	}
	return;
}

static const char *const tlbsizes[] = {
	  "1kB",
	  "4kB",
	 "16kB",
	 "64kB",
	"256kB",
	  "1MB",
	  "4MB",
	 "16MB"
};

static void
db_ppc4xx_dumptlb(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	int i, zone, tlbsize;
	u_int zpr, pid, opid, msr;
	u_long tlblo, tlbhi, tlbmask;

	zpr = mfspr(SPR_ZPR);
	for (i = 0; i < NTLB; i++) {
		__asm volatile("mfmsr %3;"
			"mfpid %4;"
			"li %0,0;"
			"mtmsr %0;"
			"sync; isync;"
			"tlbrelo %0,%5;"
			"tlbrehi %1,%5;"
			"mfpid %2;"
			"mtpid %4;"
			"mtmsr %3;"
			"sync; isync"
			: "=&r" (tlblo), "=&r" (tlbhi), "=r" (pid),
			"=&r" (msr), "=&r" (opid) : "r" (i));

		if (strchr(modif, 'v') && !(tlbhi & TLB_VALID))
			continue;

		tlbsize = (tlbhi & TLB_SIZE_MASK) >> TLB_SIZE_SHFT;
		/* map tlbsize 0 .. 7 to masks for 1kB .. 16MB */
		tlbmask = ~(1 << (tlbsize * 2 + 10)) + 1;

		if (have_addr && ((tlbhi & tlbmask) != (addr & tlbmask)))
			continue;

		zone = (tlblo & TLB_ZSEL_MASK) >> TLB_ZSEL_SHFT;
		db_printf("tlb%c%2d", tlbhi & TLB_VALID ? ' ' : '*', i);
		db_printf("  PID %3d EPN 0x%08lx %-5s",
		    pid,
		    tlbhi & tlbmask,
		    tlbsizes[tlbsize]);
		db_printf("  RPN 0x%08lx  ZONE %2d%c  %s %s %c%c%c%c%c %s",
		    tlblo & tlbmask,
		    zone,
		    "NTTA"[(zpr >> ((15 - zone) * 2)) & 3],
		    tlblo & TLB_EX ? "EX" : "  ",
		    tlblo & TLB_WR ? "WR" : "  ",
		    tlblo & TLB_W ? 'W' : ' ',
		    tlblo & TLB_I ? 'I' : ' ',
		    tlblo & TLB_M ? 'M' : ' ',
		    tlblo & TLB_G ? 'G' : ' ',
		    tlbhi & TLB_ENDIAN ? 'E' : ' ',
		    tlbhi & TLB_U0 ? "U0" : "  ");
		db_printf("\n");
	}
}

static void
db_ppc4xx_dcr(db_expr_t address, bool have_addr, db_expr_t count,
    const char *modif)
{
	db_expr_t new_value;
	db_expr_t addr;

	if (address < 0 || address > 0x3ff)
		db_error("Invalid DCR address (Valid range is 0x0 - 0x3ff)\n");

	addr = address;

	while (db_expression(&new_value)) {
		db_printf("dcr 0x%lx\t\t%s = ", addr,
		    db_num_to_str(db_ppc4xx_mfdcr(addr)));
		db_ppc4xx_mtdcr(addr, new_value);
		db_printf("%s\n", db_num_to_str(db_ppc4xx_mfdcr(addr)));
		addr += 1;
	}

	if (addr == address) {
		db_next = (db_addr_t)addr + 1;
		db_prev = (db_addr_t)addr;
		db_printf("dcr 0x%lx\t\t%s\n", addr,
		    db_num_to_str(db_ppc4xx_mfdcr(addr)));
	} else {
		db_next = (db_addr_t)addr;
		db_prev = (db_addr_t)addr - 1;
	}

	db_skip_to_eol();
}

/*
 * XXX Grossness Alert! XXX
 *
 * Please look away now if you don't like self-modifying code
 */
static u_int32_t db_ppc4xx_dcrfunc[4];

static db_expr_t
db_ppc4xx_mfdcr(db_expr_t reg)
{
	db_expr_t (*func)(void);

	reg = (((reg & 0x1f) << 5) | ((reg >> 5) & 0x1f)) << 11;
	db_ppc4xx_dcrfunc[0] = 0x7c0004ac;		/* sync */
	db_ppc4xx_dcrfunc[1] = 0x4c00012c;		/* isync */
	db_ppc4xx_dcrfunc[2] = 0x7c600286 | reg;	/* mfdcr reg, r3 */
	db_ppc4xx_dcrfunc[3] = 0x4e800020;		/* blr */

	__syncicache((void *)db_ppc4xx_dcrfunc, sizeof(db_ppc4xx_dcrfunc));
	func = (db_expr_t (*)(void))(void *)db_ppc4xx_dcrfunc;

	return ((*func)());
}

static void
db_ppc4xx_mtdcr(db_expr_t reg, db_expr_t val)
{
	db_expr_t (*func)(db_expr_t);

	reg = (((reg & 0x1f) << 5) | ((reg >> 5) & 0x1f)) << 11;
	db_ppc4xx_dcrfunc[0] = 0x7c0004ac;		/* sync */
	db_ppc4xx_dcrfunc[1] = 0x4c00012c;		/* isync */
	db_ppc4xx_dcrfunc[2] = 0x7c600386 | reg;	/* mtdcr r3, reg */
	db_ppc4xx_dcrfunc[3] = 0x4e800020;		/* blr */

	__syncicache((void *)db_ppc4xx_dcrfunc, sizeof(db_ppc4xx_dcrfunc));
	func = (db_expr_t (*)(db_expr_t))(void *)db_ppc4xx_dcrfunc;

	(*func)(val);
}

#ifdef USERACC
static void
db_ppc4xx_useracc(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	static paddr_t oldaddr = -1;
	int instr = 0;
	int data;
	extern vaddr_t opc_disasm(vaddr_t loc, int);


	if (!have_addr) {
		addr = oldaddr;
	}
	if (addr == -1) {
		db_printf("no address\n");
		return;
	}
	addr &= ~0x3; /* align */
	{
		const char *cp = modif;
		char c;
		while ((c = *cp++) != 0)
			if (c == 'i')
				instr = 1;
	}
	while (count--) {
		if (db_print_position() == 0) {
			/* Always print the address. */
			db_printf("%8.4lx:\t", addr);
		}
		oldaddr=addr;
		copyin((void *)addr, &data, sizeof(data));
		if (instr) {
			opc_disasm(addr, data);
		} else {
			db_printf("%4.4x\n", data);
		}
		addr += 4;
		db_end_line();
	}

}
#endif

#endif /* DDB */

#endif /* PPC_IBM4XX */

#ifdef PPC_BOOKE
static void
db_ppcbooke_reset(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	printf("Reseting...\n");
	(*cpu_md_ops.md_cpu_reset)();
}

static void
db_ppcbooke_splhist(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	dump_splhist(curcpu(), db_printf);
}

static void
db_ppcbooke_tf(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	if (!have_addr)
		return;

	dump_trapframe((const struct trapframe *)addr, db_printf);
}

static void
db_ppcbooke_dumptlb(db_expr_t addr, bool have_addr, db_expr_t count,
    const char *modif)
{
	tlb_dump(db_printf);
}
#endif /* PPC_BOOKE */
