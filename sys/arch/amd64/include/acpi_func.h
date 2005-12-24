/*	$NetBSD: acpi_func.h,v 1.2 2005/12/24 20:06:47 perry Exp $	*/

#include <machine/cpufunc.h>

#if 0
#define	ACPI_DISABLE_IRQS()		disable_intr()
#define	ACPI_ENABLE_IRQS()		enable_intr()
#endif

#define	ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
do { \
	unsigned long dummy; \
	__asm volatile( \
	"1:	movl (%1),%%eax		;" \
	"	movl %%eax,%%edx	;" \
	"	andq %2,%%rdx		;" \
	"	btsl $0x1,%%edx		;" \
	"	adcl $0x0,%%edx		;" \
	"	lock			;" \
	"	cmpxchgl %%edx,(%1)	;" \
	"	jnz 1b			;" \
	"	cmpb $0x3,%%dl		;" \
	"	sbbl %%eax,%%eax	" \
	: "=a" (Acq), "=c" (dummy) \
	: "c" (GLptr), "i" (~1L) \
	: "dx"); \
} while (0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
do { \
	unsigned long dummy; \
	__asm volatile( \
	"1:	movl (%1),%%eax		;" \
	"	andq %2,%%rdx		;" \
	"	lock			;" \
	"	cmpxchgl %%edx,(%1)	;" \
	"	jnz 1b			;" \
	"	andl $0x1,%%eax		;" \
	: "=a" (Acq), "=c" (dummy) \
	: "c" (GLptr), "i" (~3L) \
	: "dx"); \
} while (0)

#define	ACPI_FLUSH_CPU_CACHE()		wbinvd()
