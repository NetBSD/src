/*	$NetBSD: acpi_func.h,v 1.1 2008/05/24 22:16:20 jmcneill Exp $	*/

#include <machine/cpufunc.h>

#include <sys/atomic.h>

#define GL_ACQUIRED	(-1)
#define GL_BUSY		0
#define GL_BIT_PENDING	1
#define GL_BIT_OWNED	2
#define GL_BIT_MASK	(GL_BIT_PENDING | GL_BIT_OWNED)

#define ACPI_ACQUIRE_GLOBAL_LOCK(GLptr, Acq) 				\
do { 									\
	(Acq) = acpi_acquire_global_lock(&((GLptr)->GlobalLock));	\
} while (0)

#define ACPI_RELEASE_GLOBAL_LOCK(GLptr, Acq) 				\
do {									\
	(Acq) = acpi_release_global_lock(&((GLptr)->GlobalLock));	\
} while (0)

static inline int
acpi_acquire_global_lock(uint32_t *lock)
{
	uint32_t new, old, val;

	do {
		old = *lock;
		new = ((old & ~GL_BIT_MASK) | GL_BIT_OWNED) |
		    ((old >> 1) & GL_BIT_PENDING);
		val = atomic_cas_32(lock, old, new);
	} while (__predict_false(val != old));

	return ((new < GL_BIT_MASK) ? GL_ACQUIRED : GL_BUSY);
}

static inline int
acpi_release_global_lock(uint32_t *lock)
{
	uint32_t new, old, val;

	do {
		old = *lock;
		new = old & ~GL_BIT_MASK;
		val = atomic_cas_32(lock, old, new);
	} while (__predict_false(val != old));

	return old & GL_BIT_PENDING;
}

#define	ACPI_FLUSH_CPU_CACHE()		wbinvd()
