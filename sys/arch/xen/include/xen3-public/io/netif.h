/* $NetBSD: netif.h,v 1.1.1.1.12.1 2006/05/24 15:48:25 tron Exp $ */
/******************************************************************************
 * netif.h
 * 
 * Unified network-device I/O interface for Xen guest OSes.
 * 
 * Copyright (c) 2003-2004, Keir Fraser
 */

#ifndef __XEN_PUBLIC_IO_NETIF_H__
#define __XEN_PUBLIC_IO_NETIF_H__

#include "ring.h"
#include "../grant_table.h"

/*
 * Note that there is *never* any need to notify the backend when enqueuing
 * receive requests (netif_rx_request_t). Notifications after enqueuing any
 * other type of message should be conditional on the appropriate req_event
 * or rsp_event field in the shared ring.
 */

/* Protocol checksum field is blank in the packet (hardware offload)? */
#define _NETTXF_csum_blank     (0)
#define  NETTXF_csum_blank     (1U<<_NETTXF_csum_blank)

/* Packet data has been validated against protocol checksum. */
#define _NETTXF_data_validated (1)
#define  NETTXF_data_validated (1U<<_NETTXF_data_validated)

typedef struct netif_tx_request {
    grant_ref_t gref;      /* Reference to buffer page */
    uint16_t offset;       /* Offset within buffer page */
    uint16_t flags;        /* NETTXF_* */
    uint16_t id;           /* Echoed in response message. */
    uint16_t size;         /* Packet size in bytes.       */
} netif_tx_request_t;

typedef struct netif_tx_response {
    uint16_t id;
    int16_t  status;       /* NETIF_RSP_* */
} netif_tx_response_t;

typedef struct {
    uint16_t    id;        /* Echoed in response message.        */
    grant_ref_t gref;      /* Reference to incoming granted frame */
} netif_rx_request_t;

/* Packet data has been validated against protocol checksum. */
#define _NETRXF_data_validated (0)
#define  NETRXF_data_validated (1U<<_NETRXF_data_validated)

/* Protocol checksum field is blank in the packet (hardware offload)? */
#define _NETRXF_csum_blank     (1)
#define  NETRXF_csum_blank     (1U<<_NETRXF_csum_blank)

typedef struct {
    uint16_t id;
    uint16_t offset;       /* Offset in page of start of received packet  */
    uint16_t flags;        /* NETRXF_* */
    int16_t  status;       /* -ve: BLKIF_RSP_* ; +ve: Rx'ed pkt size. */
} netif_rx_response_t;

/*
 * Generate netif ring structures and types.
 */

DEFINE_RING_TYPES(netif_tx, netif_tx_request_t, netif_tx_response_t);
DEFINE_RING_TYPES(netif_rx, netif_rx_request_t, netif_rx_response_t);

#define NETIF_RSP_DROPPED         -2
#define NETIF_RSP_ERROR           -1
#define NETIF_RSP_OKAY             0

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
