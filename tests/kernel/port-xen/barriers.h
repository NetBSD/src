/* $NetBSD: barriers.h,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

struct barrier;

struct barrier * vm_barrier_create(size_t);
/* This opens the barrier for all waiting threads */
void vm_barrier_reset(struct barrier *); 
void vm_barrier_destroy(struct barrier *);
void vm_barrier_hold(struct barrier *);
