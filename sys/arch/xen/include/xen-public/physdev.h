/* $NetBSD: physdev.h,v 1.2 2005/03/09 22:39:20 bouyer Exp $ */

/*
 * (c) 2004 - Rolf Neugebauer - Intel Research Cambridge
 * (c) 2004 - Keir Fraser - University of Cambridge
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

/* -*-  Mode:C; c-basic-offset:4; tab-width:4 -*-
 ****************************************************************************
 * Description: Interface for domains to access physical devices on the PCI bus
 */

#ifndef __XEN_PUBLIC_PHYSDEV_H__
#define __XEN_PUBLIC_PHYSDEV_H__

/* Commands to HYPERVISOR_physdev_op() */
#define PHYSDEVOP_PCI_CFGREG_READ       0
#define PHYSDEVOP_PCI_CFGREG_WRITE      1
#define PHYSDEVOP_PCI_INITIALISE_DEVICE 2
#define PHYSDEVOP_PCI_PROBE_ROOT_BUSES  3
#define PHYSDEVOP_IRQ_UNMASK_NOTIFY     4
#define PHYSDEVOP_IRQ_STATUS_QUERY      5

/* Read from PCI configuration space. */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
    u32 reg;                          /* 12 */
    u32 len;                          /* 16 */
    /* OUT */
    u32 value;                        /* 20 */
} PACKED physdevop_pci_cfgreg_read_t; /* 24 bytes */

/* Write to PCI configuration space. */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
    u32 reg;                          /* 12 */
    u32 len;                          /* 16 */
    u32 value;                        /* 20 */
} PACKED physdevop_pci_cfgreg_write_t; /* 24 bytes */

/* Do final initialisation of a PCI device (e.g., last-moment IRQ routing). */
typedef struct {
    /* IN */
    u32 bus;                          /*  0 */
    u32 dev;                          /*  4 */
    u32 func;                         /*  8 */
} PACKED physdevop_pci_initialise_device_t; /* 12 bytes */

/* Find the root buses for subsequent scanning. */
typedef struct {
    /* OUT */
    u32 busmask[256/32];              /*  0 */
} PACKED physdevop_pci_probe_root_buses_t; /* 32 bytes */

typedef struct {
    /* IN */
    u32 irq;                          /*  0 */
    /* OUT */
/* Need to call PHYSDEVOP_IRQ_UNMASK_NOTIFY when the IRQ has been serviced? */
#define PHYSDEVOP_IRQ_NEEDS_UNMASK_NOTIFY (1<<0)
    u32 flags;                        /*  4 */
} PACKED physdevop_irq_status_query_t; /* 8 bytes */

typedef struct _physdev_op_st 
{
    u32 cmd;                          /*  0 */
    u32 __pad;                        /*  4 */
    union {                           /*  8 */
        physdevop_pci_cfgreg_read_t       pci_cfgreg_read;
        physdevop_pci_cfgreg_write_t      pci_cfgreg_write;
        physdevop_pci_initialise_device_t pci_initialise_device;
        physdevop_pci_probe_root_buses_t  pci_probe_root_buses;
        physdevop_irq_status_query_t      irq_status_query;
        u8                                __dummy[32];
    } PACKED u;
} PACKED physdev_op_t; /* 40 bytes */

#endif /* __XEN_PUBLIC_PHYSDEV_H__ */
