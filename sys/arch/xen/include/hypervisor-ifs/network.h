/*	$NetBSD: network.h,v 1.1.4.4 2004/09/21 13:24:45 skrll Exp $	*/

/*
 *
 * Copyright (c) 2003, 2004 Keir Fraser (on behalf of the Xen team)
 * All rights reserved.
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

/******************************************************************************
 * network.h
 *
 * ring data structures for buffering messages between hypervisor and
 * guestos's.  As it stands this is only used for network buffer exchange.
 *
 * This file also contains structures and interfaces for the per-domain
 * routing/filtering tables in the hypervisor.
 *
 */

#ifndef __RING_H__
#define __RING_H__

/*
 * Command values for block_io_op()
 */

#define NETOP_PUSH_BUFFERS    0  /* Notify Xen of new buffers on the rings. */
#define NETOP_FLUSH_BUFFERS   1  /* Flush all pending request buffers.      */
#define NETOP_RESET_RINGS     2  /* Reset ring indexes on a quiescent vif.  */
#define NETOP_GET_VIF_INFO    3  /* Query information for this vif.         */
typedef struct netop_st {
    unsigned int cmd; /* NETOP_xxx */
    unsigned int vif; /* VIF index */
    union {
        struct {
            unsigned long ring_mfn; /* Page frame containing net_ring_t. */
            unsigned char vmac[6];  /* Virtual Ethernet MAC address.     */
        } get_vif_info;
    } u;
} netop_t;


typedef struct tx_req_entry_st
{
    unsigned short id;
    unsigned short size;   /* packet size in bytes */
    unsigned long  addr;   /* machine address of packet */
} tx_req_entry_t;

typedef struct tx_resp_entry_st
{
    unsigned short id;
    unsigned char  status;
} tx_resp_entry_t;

typedef union tx_entry_st
{
    tx_req_entry_t  req;
    tx_resp_entry_t resp;
} tx_entry_t;


typedef struct rx_req_entry_st
{
    unsigned short id;
    unsigned long  addr;   /* machine address of PTE to swizzle */
} rx_req_entry_t;

typedef struct rx_resp_entry_st
{
    unsigned short id;
    unsigned short size;   /* received packet size in bytes */
    unsigned char  status; /* per descriptor status */
    unsigned char  offset; /* offset in page of received pkt */
} rx_resp_entry_t;

typedef union rx_entry_st
{
    rx_req_entry_t  req;
    rx_resp_entry_t resp;
} rx_entry_t;


#define TX_RING_SIZE 256
#define RX_RING_SIZE 256

#define MAX_DOMAIN_VIFS 8

/* This structure must fit in a memory page. */
typedef struct net_ring_st
{
    tx_entry_t tx_ring[TX_RING_SIZE];
    rx_entry_t rx_ring[RX_RING_SIZE];
} net_ring_t;

typedef struct net_idx_st
{
    /*
     * Guest OS places packets into ring at tx_req_prod.
     * Guest OS receives EVENT_NET when tx_resp_prod passes tx_event.
     * Guest OS places empty buffers into ring at rx_req_prod.
     * Guest OS receives EVENT_NET when rx_rssp_prod passes rx_event.
     */
    unsigned int tx_req_prod, tx_resp_prod, tx_event;
    unsigned int rx_req_prod, rx_resp_prod, rx_event;
} net_idx_t;

/*
 * Packet routing/filtering code follows:
 */

#define NETWORK_ACTION_ACCEPT   0
#define NETWORK_ACTION_COUNT    1

#define NETWORK_PROTO_ANY       0
#define NETWORK_PROTO_IP        1
#define NETWORK_PROTO_TCP       2
#define NETWORK_PROTO_UDP       3
#define NETWORK_PROTO_ARP       4

typedef struct net_rule_st 
{
    u32  src_addr;
    u32  dst_addr;
    u16  src_port;
    u16  dst_port;
    u32  src_addr_mask;
    u32  dst_addr_mask;
    u16  src_port_mask;
    u16  dst_port_mask;
    u16  proto;
    unsigned long src_vif;
    unsigned long dst_vif;
    u16  action;
} net_rule_t;

#define VIF_DOMAIN_MASK  0xfffff000UL
#define VIF_DOMAIN_SHIFT 12
#define VIF_INDEX_MASK   0x00000fffUL
#define VIF_INDEX_SHIFT  0

/* These are specified in the index if the dom is SPECIAL. */
#define VIF_SPECIAL      0xfffff000UL
#define VIF_UNKNOWN_INTERFACE   (VIF_SPECIAL | 0)
#define VIF_PHYSICAL_INTERFACE  (VIF_SPECIAL | 1)
#define VIF_ANY_INTERFACE       (VIF_SPECIAL | 2)

typedef struct vif_query_st
{
    unsigned int    domain;
    int             *buf;   /* reply buffer -- guest virtual address */
} vif_query_t;

typedef struct vif_getinfo_st
{
    unsigned int        domain;
    unsigned int        vif;

    /* domain & vif are supplied by dom0, the rest are response fields */
    long long           total_bytes_sent;
    long long           total_bytes_received;
    long long           total_packets_sent;
    long long           total_packets_received;

    /* Current scheduling parameters */
    unsigned long credit_bytes;
    unsigned long credit_usec;
} vif_getinfo_t;

/*
 * Set parameters associated with a VIF. Currently this is only scheduling
 * parameters --- permit 'credit_bytes' to be transmitted every 'credit_usec'.
 */
typedef struct vif_setparams_st
{
    unsigned int        domain;
    unsigned int        vif;
    unsigned long       credit_bytes;
    unsigned long       credit_usec;
} vif_setparams_t;

/* Network trap operations and associated structure. 
 * This presently just handles rule insertion and deletion, but will
 * evenually have code to add and remove interfaces.
 */

#define NETWORK_OP_ADDRULE      0
#define NETWORK_OP_DELETERULE   1
#define NETWORK_OP_GETRULELIST  2
#define NETWORK_OP_VIFQUERY     3
#define NETWORK_OP_VIFGETINFO   4
#define NETWORK_OP_VIFSETPARAMS 5

typedef struct network_op_st 
{
    unsigned long cmd;
    union
    {
        net_rule_t net_rule;
        vif_query_t vif_query;
        vif_getinfo_t vif_getinfo;
        vif_setparams_t vif_setparams;
    }
    u;
} network_op_t;

typedef struct net_rule_ent_st
{
    net_rule_t r;
    struct net_rule_ent_st *next;
} net_rule_ent_t;

/* Drop a new rule down to the network tables. */
int add_net_rule(net_rule_t *rule);

/* Descriptor status values */
#define RING_STATUS_OK               0  /* Everything is gravy. */
#define RING_STATUS_BAD_PAGE         1  /* What they gave us was pure evil */
#define RING_STATUS_DROPPED          2  /* Unrouteable packet */

#endif
