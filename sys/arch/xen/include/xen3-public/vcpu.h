/******************************************************************************
 * vcpu.h
 * 
 * VCPU initialisation, query, and hotplug.
 * 
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_PUBLIC_VCPU_H__
#define __XEN_PUBLIC_VCPU_H__

/*
 * Prototype for this hypercall is:
 *  int vcpu_op(int cmd, int vcpuid, void *extra_args)
 * @cmd        == VCPUOP_??? (VCPU operation).
 * @vcpuid     == VCPU to operate on.
 * @extra_args == Operation-specific extra arguments (NULL if none).
 */

/*
 * Initialise a VCPU. Each VCPU can be initialised only once. A 
 * newly-initialised VCPU will not run until it is brought up by VCPUOP_up.
 * 
 * @extra_arg == pointer to vcpu_guest_context structure containing initial
 *               state for the VCPU.
 */
#define VCPUOP_initialise           0

/*
 * Bring up a VCPU. This makes the VCPU runnable. This operation will fail
 * if the VCPU has not been initialised (VCPUOP_initialise).
 */
#define VCPUOP_up                   1

/*
 * Bring down a VCPU (i.e., make it non-runnable).
 * There are a few caveats that callers should observe:
 *  1. This operation may return, and VCPU_is_up may return false, before the
 *     VCPU stops running (i.e., the command is asynchronous). It is a good
 *     idea to ensure that the VCPU has entered a non-critical loop before
 *     bringing it down. Alternatively, this operation is guaranteed
 *     synchronous if invoked by the VCPU itself.
 *  2. After a VCPU is initialised, there is currently no way to drop all its
 *     references to domain memory. Even a VCPU that is down still holds
 *     memory references via its pagetable base pointer and GDT. It is good
 *     practise to move a VCPU onto an 'idle' or default page table, LDT and
 *     GDT before bringing it down.
 */
#define VCPUOP_down                 2

/* Returns 1 if the given VCPU is up. */
#define VCPUOP_is_up                3

#endif /* __XEN_PUBLIC_VCPU_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
