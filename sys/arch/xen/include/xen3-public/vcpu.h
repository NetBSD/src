/* $NetBSD: vcpu.h,v 1.3 2006/05/07 10:56:37 bouyer Exp $ */
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

/*
 * Return information about the state and running time of a VCPU.
 * @extra_arg == pointer to vcpu_runstate_info structure.
 */
#define VCPUOP_get_runstate_info    4
typedef struct vcpu_runstate_info {
    /* VCPU's current state (RUNSTATE_*). */
    int      state;
    /* When was current state entered (system time, ns)? */
    uint64_t state_entry_time;
    /*
     * Time spent in each RUNSTATE_* (ns). The sum of these times is
     * guaranteed not to drift from system time.
     */
    uint64_t time[4];
} vcpu_runstate_info_t;

/* VCPU is currently running on a physical CPU. */
#define RUNSTATE_running  0

/* VCPU is runnable, but not currently scheduled on any physical CPU. */
#define RUNSTATE_runnable 1

/* VCPU is blocked (a.k.a. idle). It is therefore not runnable. */
#define RUNSTATE_blocked  2

/*
 * VCPU is not runnable, but it is not blocked.
 * This is a 'catch all' state for things like hotplug and pauses by the
 * system administrator (or for critical sections in the hypervisor).
 * RUNSTATE_blocked dominates this state (it is the preferred state).
 */
#define RUNSTATE_offline  3

/*
 * Register a shared memory area from which the guest may obtain its own
 * runstate information without needing to execute a hypercall.
 * Notes:
 *  1. The registered address may be virtual or physical, depending on the
 *     platform. The virtual address should be registered on x86 systems.
 *  2. Only one shared area may be registered per VCPU. The shared area is
 *     updated by the hypervisor each time the VCPU is scheduled. Thus
 *     runstate.state will always be RUNSTATE_running and
 *     runstate.state_entry_time will indicate the system time at which the
 *     VCPU was last scheduled to run.
 * @extra_arg == pointer to vcpu_register_runstate_memory_area structure.
 */
#define VCPUOP_register_runstate_memory_area 5
typedef struct vcpu_register_runstate_memory_area {
    union {
        struct vcpu_runstate_info *v;
        uint64_t p;
    } addr;
} vcpu_register_runstate_memory_area_t;

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
