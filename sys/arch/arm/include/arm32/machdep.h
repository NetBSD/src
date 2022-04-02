/* $NetBSD: machdep.h,v 1.36 2022/04/02 11:16:07 skrll Exp $ */

#ifndef _ARM32_MACHDEP_H_
#define _ARM32_MACHDEP_H_

#ifdef _KERNEL

#define INIT_ARM_STACK_SHIFT	12
#define INIT_ARM_STACK_SIZE	(1 << INIT_ARM_STACK_SHIFT)
#define INIT_ARM_TOTAL_STACK	(INIT_ARM_STACK_SIZE * MAXCPUS)

/* Define various stack sizes in pages */
#ifndef IRQ_STACK_SIZE
#define IRQ_STACK_SIZE	1
#endif
#ifndef ABT_STACK_SIZE
#define ABT_STACK_SIZE	1
#endif
#ifndef UND_STACK_SIZE
#define UND_STACK_SIZE	1
#endif
#ifndef FIQ_STACK_SIZE
#define FIQ_STACK_SIZE	1
#endif

extern void (*cpu_reset_address)(void);
extern paddr_t cpu_reset_address_paddr;

extern void (*cpu_powerdown_address)(void);

extern u_int data_abort_handler_address;
extern u_int prefetch_abort_handler_address;
// extern u_int undefined_handler_address;
#define	undefined_handler_address (curcpu()->ci_undefsave[2])

struct bootmem_info {
	paddr_t bmi_start;
	paddr_t bmi_kernelstart;
	paddr_t bmi_kernelend;
	paddr_t bmi_end;
	pv_addrqh_t bmi_freechunks;
	pv_addrqh_t bmi_chunks;		/* sorted list of memory to be mapped */
	pv_addr_t bmi_freeblocks[4];
	/*
	 * These need to be static for pmap's kernel_pt list.
	 */
	pv_addr_t bmi_vector_l2pt;
	pv_addr_t bmi_io_l2pt;
	pv_addr_t bmi_l2pts[32];	// for large memory disks.
	u_int bmi_freepages;
	u_int bmi_nfreeblocks;
};

extern struct bootmem_info bootmem_info;

extern char *booted_kernel;
extern u_long kern_vtopdiff;

/* misc prototypes used by the many arm machdeps */
void cortex_pmc_ccnt_init(void);
void cpu_hatch(struct cpu_info *, u_int, void (*)(struct cpu_info *));
void halt(void);
void parse_mi_bootargs(char *);
void data_abort_handler(trapframe_t *);
void prefetch_abort_handler(trapframe_t *);
void undefinedinstruction_bounce(trapframe_t *);
void dumpsys(void);

/*
 * note that we use void * as all the platforms have different ideas on what
 * the structure is
 */
vaddr_t initarm(void *);
struct pmap_devmap;
struct boot_physmem;

void cpu_startup_hook(void);
void cpu_startup_default(void);

static inline paddr_t
aarch32_kern_vtophys(vaddr_t va)
{
	return va - kern_vtopdiff;
}

static inline vaddr_t
aarch32_kern_phystov(paddr_t pa)
{
	return pa + kern_vtopdiff;
}

#define KERN_VTOPHYS(va)	aarch32_kern_vtophys(va)
#define KERN_PHYSTOV(pa)	aarch32_kern_phystov(pa)

void cpu_kernel_vm_init(paddr_t, psize_t);

void arm32_bootmem_init(paddr_t memstart, psize_t memsize, paddr_t kernelstart);
void arm32_kernel_vm_init(vaddr_t kvm_base, vaddr_t vectors,
	vaddr_t iovbase /* (can be zero) */,
	const struct pmap_devmap *devmap, bool mapallmem_p);
vaddr_t initarm_common(vaddr_t kvm_base, vsize_t kvm_size,
        const struct boot_physmem *bp, size_t nbp);

void uartputc(int);

/* from arm/arm32/intr.c */
void dosoftints(void);
void set_spl_masks(void);
#ifdef DIAGNOSTIC
void dump_spl_masks(void);
#endif

/* cpu_onfault */
int cpu_set_onfault(struct faultbuf *) __returns_twice;
void cpu_jump_onfault(struct trapframe *, const struct faultbuf *, int);

static inline void
cpu_unset_onfault(void)
{
	curpcb->pcb_onfault = NULL;
}

static inline void
cpu_enable_onfault(struct faultbuf *fb)
{
	curpcb->pcb_onfault = fb;
}

static inline struct faultbuf *
cpu_disable_onfault(void)
{
	struct faultbuf * const fb = curpcb->pcb_onfault;
	if (fb != NULL)
		curpcb->pcb_onfault = NULL;
	return fb;
}

#endif	/* _KERNEL */

#endif	/* _ARM32_MACHDEP_H_ */
