/* $NetBSD: arch-ia64.h,v 1.1.1.1.10.2 2006/05/11 23:27:14 elad Exp $ */
/******************************************************************************
 * arch-ia64/hypervisor-if.h
 * 
 * Guest OS interface to IA64 Xen.
 */

#ifndef __HYPERVISOR_IF_IA64_H__
#define __HYPERVISOR_IF_IA64_H__

#ifdef __XEN__
#define __DEFINE_GUEST_HANDLE(name, type) \
    typedef struct { type *p; } __guest_handle_ ## name
#else
#define __DEFINE_GUEST_HANDLE(name, type) \
    typedef type * __guest_handle_ ## name
#endif

#define DEFINE_GUEST_HANDLE(name) __DEFINE_GUEST_HANDLE(name, name)
#define GUEST_HANDLE(name)        __guest_handle_ ## name

#ifndef __ASSEMBLY__
/* Guest handles for primitive C types. */
__DEFINE_GUEST_HANDLE(uchar, unsigned char);
__DEFINE_GUEST_HANDLE(uint,  unsigned int);
__DEFINE_GUEST_HANDLE(ulong, unsigned long);
DEFINE_GUEST_HANDLE(char);
DEFINE_GUEST_HANDLE(int);
DEFINE_GUEST_HANDLE(long);
DEFINE_GUEST_HANDLE(void);
#endif

/* Maximum number of virtual CPUs in multi-processor guests. */
/* WARNING: before changing this, check that shared_info fits on a page */
#define MAX_VIRT_CPUS 4

#ifndef __ASSEMBLY__

#define MAX_NR_SECTION  32  /* at most 32 memory holes */
typedef struct {
    unsigned long start;  /* start of memory hole */
    unsigned long end;    /* end of memory hole */
} mm_section_t;

typedef struct {
    unsigned long mfn : 56;
    unsigned long type: 8;
} pmt_entry_t;

#define GPFN_MEM          (0UL << 56) /* Guest pfn is normal mem */
#define GPFN_FRAME_BUFFER (1UL << 56) /* VGA framebuffer */
#define GPFN_LOW_MMIO     (2UL << 56) /* Low MMIO range */
#define GPFN_PIB          (3UL << 56) /* PIB base */
#define GPFN_IOSAPIC      (4UL << 56) /* IOSAPIC base */
#define GPFN_LEGACY_IO    (5UL << 56) /* Legacy I/O base */
#define GPFN_GFW          (6UL << 56) /* Guest Firmware */
#define GPFN_HIGH_MMIO    (7UL << 56) /* High MMIO range */

#define GPFN_IO_MASK     (7UL << 56)  /* Guest pfn is I/O type */
#define GPFN_INV_MASK    (31UL << 59) /* Guest pfn is invalid */

#define INVALID_MFN       (~0UL)

#define MEM_G   (1UL << 30)
#define MEM_M   (1UL << 20)

#define MMIO_START       (3 * MEM_G)
#define MMIO_SIZE        (512 * MEM_M)

#define VGA_IO_START     0xA0000UL
#define VGA_IO_SIZE      0x20000

#define LEGACY_IO_START  (MMIO_START + MMIO_SIZE)
#define LEGACY_IO_SIZE   (64*MEM_M)

#define IO_PAGE_START (LEGACY_IO_START + LEGACY_IO_SIZE)
#define IO_PAGE_SIZE  PAGE_SIZE

#define STORE_PAGE_START (IO_PAGE_START + IO_PAGE_SIZE)
#define STORE_PAGE_SIZE	 PAGE_SIZE

#define IO_SAPIC_START   0xfec00000UL
#define IO_SAPIC_SIZE    0x100000

#define PIB_START 0xfee00000UL
#define PIB_SIZE 0x100000

#define GFW_START        (4*MEM_G -16*MEM_M)
#define GFW_SIZE         (16*MEM_M)

/*
 * NB. This may become a 64-bit count with no shift. If this happens then the 
 * structure size will still be 8 bytes, so no other alignments will change.
 */
typedef struct {
    unsigned int  tsc_bits;      /* 0: 32 bits read from the CPU's TSC. */
    unsigned int  tsc_bitshift;  /* 4: 'tsc_bits' uses N:N+31 of TSC.   */
} tsc_timestamp_t; /* 8 bytes */

struct pt_fpreg {
    union {
        unsigned long bits[2];
        long double __dummy;    /* force 16-byte alignment */
    } u;
};

typedef struct cpu_user_regs{
    /* The following registers are saved by SAVE_MIN: */
    unsigned long b6;  /* scratch */
    unsigned long b7;  /* scratch */

    unsigned long ar_csd; /* used by cmp8xchg16 (scratch) */
    unsigned long ar_ssd; /* reserved for future use (scratch) */

    unsigned long r8;  /* scratch (return value register 0) */
    unsigned long r9;  /* scratch (return value register 1) */
    unsigned long r10; /* scratch (return value register 2) */
    unsigned long r11; /* scratch (return value register 3) */

    unsigned long cr_ipsr; /* interrupted task's psr */
    unsigned long cr_iip;  /* interrupted task's instruction pointer */
    unsigned long cr_ifs;  /* interrupted task's function state */

    unsigned long ar_unat; /* interrupted task's NaT register (preserved) */
    unsigned long ar_pfs;  /* prev function state  */
    unsigned long ar_rsc;  /* RSE configuration */
    /* The following two are valid only if cr_ipsr.cpl > 0: */
    unsigned long ar_rnat;  /* RSE NaT */
    unsigned long ar_bspstore; /* RSE bspstore */

    unsigned long pr;  /* 64 predicate registers (1 bit each) */
    unsigned long b0;  /* return pointer (bp) */
    unsigned long loadrs;  /* size of dirty partition << 16 */

    unsigned long r1;  /* the gp pointer */
    unsigned long r12; /* interrupted task's memory stack pointer */
    unsigned long r13; /* thread pointer */

    unsigned long ar_fpsr;  /* floating point status (preserved) */
    unsigned long r15;  /* scratch */

 /* The remaining registers are NOT saved for system calls.  */

    unsigned long r14;  /* scratch */
    unsigned long r2;  /* scratch */
    unsigned long r3;  /* scratch */
    unsigned long r16;  /* scratch */
    unsigned long r17;  /* scratch */
    unsigned long r18;  /* scratch */
    unsigned long r19;  /* scratch */
    unsigned long r20;  /* scratch */
    unsigned long r21;  /* scratch */
    unsigned long r22;  /* scratch */
    unsigned long r23;  /* scratch */
    unsigned long r24;  /* scratch */
    unsigned long r25;  /* scratch */
    unsigned long r26;  /* scratch */
    unsigned long r27;  /* scratch */
    unsigned long r28;  /* scratch */
    unsigned long r29;  /* scratch */
    unsigned long r30;  /* scratch */
    unsigned long r31;  /* scratch */
    unsigned long ar_ccv;  /* compare/exchange value (scratch) */

    /*
     * Floating point registers that the kernel considers scratch:
     */
    struct pt_fpreg f6;  /* scratch */
    struct pt_fpreg f7;  /* scratch */
    struct pt_fpreg f8;  /* scratch */
    struct pt_fpreg f9;  /* scratch */
    struct pt_fpreg f10;  /* scratch */
    struct pt_fpreg f11;  /* scratch */
    unsigned long r4;  /* preserved */
    unsigned long r5;  /* preserved */
    unsigned long r6;  /* preserved */
    unsigned long r7;  /* preserved */
    unsigned long eml_unat;    /* used for emulating instruction */
    unsigned long rfi_pfs;     /* used for elulating rfi */

}cpu_user_regs_t;

typedef union {
    unsigned long value;
    struct {
        int a_int:1;
        int a_from_int_cr:1;
        int a_to_int_cr:1;
        int a_from_psr:1;
        int a_from_cpuid:1;
        int a_cover:1;
        int a_bsw:1;
        long reserved:57;
    };
} vac_t;

typedef union {
    unsigned long value;
    struct {
        int d_vmsw:1;
        int d_extint:1;
        int d_ibr_dbr:1;
        int d_pmc:1;
        int d_to_pmd:1;
        int d_itm:1;
        long reserved:58;
    };
} vdc_t;

typedef struct {
    vac_t   vac;
    vdc_t   vdc;
    unsigned long  virt_env_vaddr;
    unsigned long  reserved1[29];
    unsigned long  vhpi;
    unsigned long  reserved2[95];
    union {
        unsigned long  vgr[16];
        unsigned long bank1_regs[16]; // bank1 regs (r16-r31) when bank0 active
    };
    union {
        unsigned long  vbgr[16];
        unsigned long bank0_regs[16]; // bank0 regs (r16-r31) when bank1 active
    };
    unsigned long  vnat;
    unsigned long  vbnat;
    unsigned long  vcpuid[5];
    unsigned long  reserved3[11];
    unsigned long  vpsr;
    unsigned long  vpr;
    unsigned long  reserved4[76];
    union {
        unsigned long  vcr[128];
        struct {
            unsigned long dcr;  // CR0
            unsigned long itm;
            unsigned long iva;
            unsigned long rsv1[5];
            unsigned long pta;  // CR8
            unsigned long rsv2[7];
            unsigned long ipsr;  // CR16
            unsigned long isr;
            unsigned long rsv3;
            unsigned long iip;
            unsigned long ifa;
            unsigned long itir;
            unsigned long iipa;
            unsigned long ifs;
            unsigned long iim;  // CR24
            unsigned long iha;
            unsigned long rsv4[38];
            unsigned long lid;  // CR64
            unsigned long ivr;
            unsigned long tpr;
            unsigned long eoi;
            unsigned long irr[4];
            unsigned long itv;  // CR72
            unsigned long pmv;
            unsigned long cmcv;
            unsigned long rsv5[5];
            unsigned long lrr0;  // CR80
            unsigned long lrr1;
            unsigned long rsv6[46];
        };
    };
    union {
        unsigned long  reserved5[128];
        struct {
            unsigned long precover_ifs;
            unsigned long unat;  // not sure if this is needed until NaT arch is done
            int interrupt_collection_enabled; // virtual psr.ic
            int interrupt_delivery_enabled; // virtual psr.i
            int pending_interruption;
            int incomplete_regframe; // see SDM vol2 6.8
            unsigned long reserved5_1[4];
            int metaphysical_mode; // 1 = use metaphys mapping, 0 = use virtual
            int banknum; // 0 or 1, which virtual register bank is active
            unsigned long rrs[8]; // region registers
            unsigned long krs[8]; // kernel registers
            unsigned long pkrs[8]; // protection key registers
            unsigned long tmp[8]; // temp registers (e.g. for hyperprivops)
            // FIXME: tmp[8] temp'ly being used for virtual psr.pp
        };
    };
    unsigned long  reserved6[3456];
    unsigned long  vmm_avail[128];
    unsigned long  reserved7[4096];
} mapped_regs_t;

typedef struct {
    mapped_regs_t *privregs;
    int evtchn_vector;
} arch_vcpu_info_t;

typedef mapped_regs_t vpd_t;

typedef struct {
    unsigned int flags;
    unsigned long start_info_pfn;
} arch_shared_info_t;

typedef struct {
    unsigned long start;
    unsigned long size;
} arch_initrd_info_t;

#define IA64_COMMAND_LINE_SIZE 512
typedef struct vcpu_guest_context {
#define VGCF_FPU_VALID (1<<0)
#define VGCF_VMX_GUEST (1<<1)
#define VGCF_IN_KERNEL (1<<2)
    unsigned long flags;       /* VGCF_* flags */
    unsigned long pt_base;     /* PMT table base */
    unsigned long share_io_pg; /* Shared page for I/O emulation */
    unsigned long sys_pgnr;    /* System pages out of domain memory */
    unsigned long vm_assist;   /* VMASST_TYPE_* bitmap, now none on IPF */

    cpu_user_regs_t regs;
    arch_vcpu_info_t vcpu;
    arch_shared_info_t shared;
    arch_initrd_info_t initrd;
    char cmdline[IA64_COMMAND_LINE_SIZE];
} vcpu_guest_context_t;
DEFINE_GUEST_HANDLE(vcpu_guest_context_t);

#endif /* !__ASSEMBLY__ */

#endif /* __HYPERVISOR_IF_IA64_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
