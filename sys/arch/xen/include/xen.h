/*	$NetBSD: xen.h,v 1.8.2.3 2004/09/18 14:42:53 skrll Exp $	*/

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

/* some function prototypes */
void trap_init(void);


/*
 * STI/CLI equivalents. These basically set and clear the virtual
 * event_enable flag in teh shared_info structure. Note that when
 * the enable bit is set, there may be pending events to be handled.
 * We may therefore call into do_hypervisor_callback() directly.
 */

#define __save_flags(x)							\
do {									\
	(x) = x86_atomic_test_bit(&HYPERVISOR_shared_info->events_mask,	\
		EVENTS_MASTER_ENABLE_BIT);				\
	__insn_barrier();						\
} while (0)

#define __restore_flags(x)						\
do {									\
	shared_info_t *_shared = HYPERVISOR_shared_info;		\
	if (x) x86_atomic_set_bit(&_shared->events_mask,		\
		EVENTS_MASTER_ENABLE_BIT);				\
	__insn_barrier();						\
} while (0)
/*     if (__predict_false(_shared->events) && (x)) do_hypervisor_callback(NULL);     \ */

#define __cli()								\
do {									\
	x86_atomic_clear_bit(&HYPERVISOR_shared_info->events_mask,	\
		EVENTS_MASTER_ENABLE_BIT);				\
	    __insn_barrier();						\
} while (0)

#define __sti()								\
do {									\
	shared_info_t *_shared = HYPERVISOR_shared_info;		\
	x86_atomic_set_bit(&_shared->events_mask,			\
		EVENTS_MASTER_ENABLE_BIT);				\
    __insn_barrier();							\
} while (0)
/*     if (__predict_false(_shared->events)) do_hypervisor_callback(NULL); \ */

#define cli()			__cli()
#define sti()			__sti()
#define save_flags(x)		__save_flags(x)
#define restore_flags(x)	__restore_flags(x)
#define save_and_cli(x)		__save_and_cli(x)
#define save_and_sti(x)		__save_and_sti(x)

#ifdef MULTIPROCESSOR
#define __LOCK_PREFIX "lock; "
#else
#define __LOCK_PREFIX ""
#endif

static __inline__ unsigned long
x86_atomic_xchg(unsigned long *ptr, unsigned long val)
{
	unsigned long result;

        __asm __volatile("xchgl %0,%1"
	    :"=r" (result)
	    :"m" (*ptr), "0" (val)
	    :"memory");

	return result;
}

static __inline__ int
x86_atomic_test_and_clear_bit(volatile void *ptr, int bitno)
{
        int result;

        __asm __volatile(__LOCK_PREFIX
	    "btrl %2,%1 ;"
	    "sbbl %0,%0"
	    :"=r" (result), "=m" (*(volatile uint32_t *)(ptr))
	    :"Ir" (bitno) : "memory");
        return result;
}

static __inline int
x86_constant_test_bit(const volatile void *ptr, int bitno)
{
	return ((1UL << (bitno & 31)) &
	    (((const volatile uint32_t *) ptr)[bitno >> 5])) != 0;
}

static __inline int
x86_variable_test_bit(const volatile void *ptr, int bitno)
{
	int result;
    
	__asm __volatile(
		"btl %2,%1 ;"
		"sbbl %0,%0"
		:"=r" (result)
		:"m" (*(volatile uint32_t *)(ptr)), "Ir" (bitno));
	return result;
}

#define x86_atomic_test_bit(ptr, bitno) \
	(__builtin_constant_p(bitno) ? \
	 x86_constant_test_bit((ptr),(bitno)) : \
	 variable_test_bit((ptr),(bitno)))

static __inline void
x86_atomic_set_bit(volatile void *ptr, int bitno)
{
        __asm __volatile(__LOCK_PREFIX
	    "btsl %1,%0"
	    :"=m" (*(volatile uint32_t *)(ptr))
	    :"Ir" (bitno));
}

static __inline void
x86_atomic_clear_bit(volatile void *ptr, int bitno)
{
        __asm __volatile(__LOCK_PREFIX
	    "btrl %1,%0"
	    :"=m" (*(volatile uint32_t *)(ptr))
	    :"Ir" (bitno));
}

#endif /* !__ASSEMBLY__ */

#endif /* _OS_H_ */
