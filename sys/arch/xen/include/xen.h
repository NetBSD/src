/*	$NetBSD: xen.h,v 1.7 2004/05/07 15:51:04 cl Exp $	*/

/*
 *
 * Copyright (c) 2003, 2004 Keir Fraser (on behalf of the Xen team)
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef _XEN_H
#define _XEN_H

#ifndef _LOCORE

struct xen_netinfo {
	uint32_t xi_ifno;
	char *xi_root;
	uint32_t xi_ip[5];
};

union xen_cmdline_parseinfo {
	char			xcp_bootdev[16]; /* sizeof(dv_xname) */
	struct xen_netinfo	xcp_netinfo;
	char			xcp_console[16];
};

#define	XEN_PARSE_BOOTDEV	0
#define	XEN_PARSE_NETINFO	1
#define	XEN_PARSE_CONSOLE	2

void	xen_parse_cmdline(int, union xen_cmdline_parseinfo *);

void	xenconscn_attach(void);

void	xenmachmem_init(void);
void	xenprivcmd_init(void);
void	xenvfr_init(void);

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifdef XENDEBUG
void printk(const char *, ...);
void vprintk(const char *, va_list);
#endif

#endif

#define hypervisor_asm_ack(num) \
	movl	HYPERVISOR_shared_info,%eax		;\
	lock						;\
	btsl	$num,EVENTS_MASK(%eax)

#endif /* _XEN_H */

/******************************************************************************
 * os.h
 * 
 * random collection of macros and definition
 */

#ifndef _OS_H_
#define _OS_H_

/*
 * These are the segment descriptors provided for us by the hypervisor.
 * For now, these are hardwired -- guest OSes cannot update the GDT
 * or LDT.
 * 
 * It shouldn't be hard to support descriptor-table frobbing -- let me 
 * know if the BSD or XP ports require flexibility here.
 */


/*
 * these are also defined in hypervisor-if.h but can't be pulled in as
 * they are used in start of day assembly. Need to clean up the .h files
 * a bit more...
 */

#ifndef FLAT_RING1_CS
#define FLAT_RING1_CS		0x0819
#define FLAT_RING1_DS		0x0821
#define FLAT_RING3_CS		0x082b
#define FLAT_RING3_DS		0x0833
#endif

#define __KERNEL_CS        FLAT_RING1_CS
#define __KERNEL_DS        FLAT_RING1_DS

/* Everything below this point is not included by assembler (.S) files. */
#ifndef _LOCORE

#include <machine/hypervisor-ifs/hypervisor-if.h>

/* some function prototypes */
void trap_init(void);


/*
 * STI/CLI equivalents. These basically set and clear the virtual
 * event_enable flag in teh shared_info structure. Note that when
 * the enable bit is set, there may be pending events to be handled.
 * We may therefore call into do_hypervisor_callback() directly.
 */
#define unlikely(x)  __builtin_expect((x),0)
#define __save_flags(x)                                                       \
do {                                                                          \
    (x) = test_bit(EVENTS_MASTER_ENABLE_BIT,                                  \
                   &HYPERVISOR_shared_info->events_mask);                     \
    barrier();                                                                \
} while (0)

#define __restore_flags(x)                                                    \
do {                                                                          \
    shared_info_t *_shared = HYPERVISOR_shared_info;                          \
    if (x) set_bit(EVENTS_MASTER_ENABLE_BIT, &_shared->events_mask);          \
    barrier();                                                                \
} while (0)
/*     if ( unlikely(_shared->events) && (x) ) do_hypervisor_callback(NULL);     \ */

#define __cli()                                                               \
do {                                                                          \
    clear_bit(EVENTS_MASTER_ENABLE_BIT, &HYPERVISOR_shared_info->events_mask);\
    barrier();                                                                \
} while (0)

#define __sti()                                                               \
do {                                                                          \
    shared_info_t *_shared = HYPERVISOR_shared_info;                          \
    set_bit(EVENTS_MASTER_ENABLE_BIT, &_shared->events_mask);                 \
    barrier();                                                                \
} while (0)
/*     if ( unlikely(_shared->events) ) do_hypervisor_callback(NULL);            \ */
#define cli() __cli()
#define sti() __sti()
#define save_flags(x) __save_flags(x)
#define restore_flags(x) __restore_flags(x)
#define save_and_cli(x) __save_and_cli(x)
#define save_and_sti(x) __save_and_sti(x)



/* This is a barrier for the compiler only, NOT the processor! */
#define barrier() __asm__ __volatile__("": : :"memory")

#define __LOCK_PREFIX ""
#define __LOCK ""
#define __ADDR (*(volatile long *) addr)
/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } atomic_t;


#define xchg(ptr,v) \
        ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))
static inline unsigned long __xchg(unsigned long x, volatile void * ptr,
                                   int size)
{
    switch (size) {
    case 1:
        __asm__ __volatile__("xchgb %b0,%1"
                             :"=q" (x)
                             :"m" (*__xg(ptr)), "0" (x)
                             :"memory");
        break;
    case 2:
        __asm__ __volatile__("xchgw %w0,%1"
                             :"=r" (x)
                             :"m" (*__xg(ptr)), "0" (x)
                             :"memory");
        break;
    case 4:
        __asm__ __volatile__("xchgl %0,%1"
                             :"=r" (x)
                             :"m" (*__xg(ptr)), "0" (x)
                             :"memory");
        break;
    }
    return x;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to set
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.  
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile void * addr)
{
        int oldbit;

        __asm__ __volatile__( __LOCK_PREFIX
                "btrl %2,%1\n\tsbbl %0,%0"
                :"=r" (oldbit),"=m" (__ADDR)
                :"Ir" (nr) : "memory");
        return oldbit;
}

static __inline__ int constant_test_bit(int nr, const volatile void * addr)
{
    return ((1UL << (nr & 31)) & (((const volatile unsigned int *) addr)[nr >> 5])) != 0;
}

static __inline__ int variable_test_bit(int nr, volatile void * addr)
{
    int oldbit;
    
    __asm__ __volatile__(
        "btl %2,%1\n\tsbbl %0,%0"
        :"=r" (oldbit)
        :"m" (__ADDR),"Ir" (nr));
    return oldbit;
}

#define test_bit(nr,addr) \
(__builtin_constant_p(nr) ? \
 constant_test_bit((nr),(addr)) : \
 variable_test_bit((nr),(addr)))


/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void set_bit(int nr, volatile void * addr)
{
        __asm__ __volatile__( __LOCK_PREFIX
                "btsl %1,%0"
                :"=m" (__ADDR)
                :"Ir" (nr));
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void clear_bit(int nr, volatile void * addr)
{
        __asm__ __volatile__( __LOCK_PREFIX
                "btrl %1,%0"
                :"=m" (__ADDR)
                :"Ir" (nr));
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ void atomic_inc(atomic_t *v)
{
        __asm__ __volatile__(
                __LOCK "incl %0"
                :"=m" (v->counter)
                :"m" (v->counter));
}


#define rdtscll(val) \
     __asm__ __volatile__("rdtsc" : "=A" (val))


#endif /* !__ASSEMBLY__ */

#endif /* _OS_H_ */
