/* $NetBSD: memory.h,v 1.1.1.1.6.1 2006/04/22 11:38:11 simonb Exp $ */
/******************************************************************************
 * memory.h
 * 
 * Memory reservation and information.
 * 
 * Copyright (c) 2005, Keir Fraser <keir@xensource.com>
 */

#ifndef __XEN_PUBLIC_MEMORY_H__
#define __XEN_PUBLIC_MEMORY_H__

/*
 * Increase or decrease the specified domain's memory reservation. Returns a
 * -ve errcode on failure, or the # extents successfully allocated or freed.
 * arg == addr of struct xen_memory_reservation.
 */
#define XENMEM_increase_reservation 0
#define XENMEM_decrease_reservation 1
typedef struct xen_memory_reservation {

    /*
     * MFN bases of extents to free (XENMEM_decrease_reservation).
     * MFN bases of extents that were allocated (XENMEM_increase_reservation).
     */
    unsigned long *extent_start;

    /* Number of extents, and size/alignment of each (2^extent_order pages). */
    unsigned long  nr_extents;
    unsigned int   extent_order;

    /*
     * XENMEM_increase_reservation: maximum # bits addressable by the user
     * of the allocated region (e.g., I/O devices often have a 32-bit
     * limitation even in 64-bit systems). If zero then the user has no
     * addressing restriction.
     * XENMEM_decrease_reservation: unused.
     */
    unsigned int   address_bits;

    /*
     * Domain whose reservation is being changed.
     * Unprivileged domains can specify only DOMID_SELF.
     */
    domid_t        domid;

} xen_memory_reservation_t;

/*
 * Returns the maximum machine frame number of mapped RAM in this system.
 * This command always succeeds (it never returns an error code).
 * arg == NULL.
 */
#define XENMEM_maximum_ram_page     2

/*
 * Returns the current or maximum memory reservation, in pages, of the
 * specified domain (may be DOMID_SELF). Returns -ve errcode on failure.
 * arg == addr of domid_t.
 */
#define XENMEM_current_reservation  3
#define XENMEM_maximum_reservation  4

/*
 * Returns a list of MFN bases of 2MB extents comprising the machine_to_phys
 * mapping table. Architectures which do not have a m2p table do not implement
 * this command.
 * arg == addr of xen_machphys_mfn_list_t.
 */
#define XENMEM_machphys_mfn_list    5
typedef struct xen_machphys_mfn_list {
    /*
     * Size of the 'extent_start' array. Fewer entries will be filled if the
     * machphys table is smaller than max_extents * 2MB.
     */
    unsigned int max_extents;
    
    /*
     * Pointer to buffer to fill with list of extent starts. If there are
     * any large discontiguities in the machine address space, 2MB gaps in
     * the machphys table will be represented by an MFN base of zero.
     */
    unsigned long *extent_start;

    /*
     * Number of extents written to the above array. This will be smaller
     * than 'max_extents' if the machphys table is smaller than max_e * 2MB.
     */
    unsigned int nr_extents;
} xen_machphys_mfn_list_t;

#endif /* __XEN_PUBLIC_MEMORY_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
