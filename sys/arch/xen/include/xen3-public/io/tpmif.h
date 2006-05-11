/* $NetBSD: tpmif.h,v 1.1.1.1.10.2 2006/05/11 23:27:14 elad Exp $ */
/******************************************************************************
 * tpmif.h
 *
 * TPM I/O interface for Xen guest OSes.
 *
 * Copyright (c) 2005, IBM Corporation
 *
 * Author: Stefan Berger, stefanb@us.ibm.com
 * Grant table support: Mahadevan Gomathisankaran
 *
 * This code has been derived from tools/libxc/xen/io/netif.h
 *
 * Copyright (c) 2003-2004, Keir Fraser
 */

#ifndef __XEN_PUBLIC_IO_TPMIF_H__
#define __XEN_PUBLIC_IO_TPMIF_H__

#include "../grant_table.h"

typedef struct {
    unsigned long addr;   /* Machine address of packet.   */
    grant_ref_t ref;      /* grant table access reference */
    uint16_t unused;
    uint16_t size;        /* Packet size in bytes.        */
} tpmif_tx_request_t;

/*
 * The TPMIF_TX_RING_SIZE defines the number of pages the
 * front-end and backend can exchange (= size of array).
 */
typedef uint32_t TPMIF_RING_IDX;

#define TPMIF_TX_RING_SIZE 10

/* This structure must fit in a memory page. */

typedef struct {
    tpmif_tx_request_t req;
} tpmif_ring_t;

typedef struct {
    tpmif_ring_t ring[TPMIF_TX_RING_SIZE];
} tpmif_tx_interface_t;

#endif

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
