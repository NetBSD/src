
#include <sys/types.h>
#include <sys/atomic.h>

#ifndef __HAVE_ATOMIC64_OPS

/* XXX: Not so atomic, could use mutexes but not worth it */
uint64_t
atomic_cas_64(volatile uint64_t *ptr, uint64_t old, uint64_t new) {
	uint64_t prev = *ptr;
	if (prev == old)
		*ptr = new;
	return prev;
}

void
atomic_add_64(volatile uint64_t *ptr, int64_t delta) {
	*ptr += delta;
}

void
atomic_inc_64(volatile uint64_t *ptr) {
	++(*ptr);
}

void
atomic_dec_64(volatile uint64_t *ptr) {
	--(*ptr);
}

uint64_t
atomic_add_64_nv(volatile uint64_t *ptr, int64_t delta) {
	return *ptr += delta;
}

#endif
