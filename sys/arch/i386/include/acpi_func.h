/*	$NetBSD: acpi_func.h,v 1.9 2008/04/11 11:24:41 jmcneill Exp $	*/

#include <machine/cpufunc.h>

#if 0
#define	ACPI_DISABLE_IRQS()		disable_intr()
#define	ACPI_ENABLE_IRQS()		enable_intr()
#endif

#define	ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) \
do { \
	__asm volatile( \
	"1:	movl %1,%%eax		;" \
	"	movl %%eax,%%edx	;" \
	"	andl %2,%%edx		;" \
	"	btsl $0x1,%%edx		;" \
	"	adcl $0x0,%%edx		;" \
	"	lock			;" \
	"	cmpxchgl %%edx,%1	;" \
	"	jnz 1b			;" \
	"	andb $0x3,%%dl		;" \
	"	cmpb $0x3,%%dl		;" \
	"	sbbl %%eax,%%eax	;" \
	: "=&a" (Acq), "+m" (*GLptr) \
	: "i" (~1L) \
	: "edx"); \
	(Acq) = -1; \
} while (0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) \
do { \
	__asm volatile( \
	"1:	movl %1,%%eax		;" \
	"	andl %2,%%edx		;" \
	"	lock			;" \
	"	cmpxchgl %%edx,%1	;" \
	"	jnz 1b			;" \
	"	andl $0x1,%%eax		;" \
	: "=&a" (Acq), "+m" (*GLptr) \
	: "i" (~3L) \
	: "edx"); \
} while (0)

#define	ACPI_FLUSH_CPU_CACHE()		wbinvd()
