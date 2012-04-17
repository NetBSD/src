/* $NetBSD: event_channel.h,v 1.4.10.1 2012/04/17 00:07:08 yamt Exp $ */
/******************************************************************************
 * event_channel.h
 * 
 * Event channels between domains.
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
 *
 * Copyright (c) 2003-2004, K A Fraser.
 */

#ifndef __XEN_PUBLIC_EVENT_CHANNEL_H__
#define __XEN_PUBLIC_EVENT_CHANNEL_H__

#include "xen.h"

/*
 * Prototype for this hypercall is:
 *  int event_channel_op(int cmd, void *args)
 * @cmd  == EVTCHNOP_??? (event-channel operation).
 * @args == Operation-specific extra arguments (NULL if none).
 */

typedef uint32_t evtchn_port_t;
DEFINE_XEN_GUEST_HANDLE(evtchn_port_t);

/*
 * EVTCHNOP_alloc_unbound: Allocate a port in domain <dom> and mark as
 * accepting interdomain bindings from domain <remote_dom>. A fresh port
 * is allocated in <dom> and returned as <port>.
 * NOTES:
 *  1. If the caller is unprivileged then <dom> must be DOMID_SELF.
 *  2. <rdom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_alloc_unbound    6
struct evtchn_alloc_unbound {
    /* IN parameters */
    domid_t dom, remote_dom;
    /* OUT parameters */
    evtchn_port_t port;
};
typedef struct evtchn_alloc_unbound evtchn_alloc_unbound_t;

/*
 * EVTCHNOP_bind_interdomain: Construct an interdomain event channel between
 * the calling domain and <remote_dom>. <remote_dom,remote_port> must identify
 * a port that is unbound and marked as accepting bindings from the calling
 * domain. A fresh port is allocated in the calling domain and returned as
 * <local_port>.
 * NOTES:
 *  2. <remote_dom> may be DOMID_SELF, allowing loopback connections.
 */
#define EVTCHNOP_bind_interdomain 0
struct evtchn_bind_interdomain {
    /* IN parameters. */
    domid_t remote_dom;
    evtchn_port_t remote_port;
    /* OUT parameters. */
    evtchn_port_t local_port;
};
typedef struct evtchn_bind_interdomain evtchn_bind_interdomain_t;

/*
 * EVTCHNOP_bind_virq: Bind a local event channel to VIRQ <irq> on specified
 * vcpu.
 * NOTES:
 *  1. Virtual IRQs are classified as per-vcpu or global. See the VIRQ list
 *     in xen.h for the classification of each VIRQ.
 *  2. Global VIRQs must be allocated on VCPU0 but can subsequently be
 *     re-bound via EVTCHNOP_bind_vcpu.
 *  3. Per-vcpu VIRQs may be bound to at most one event channel per vcpu.
 *     The allocated event channel is bound to the specified vcpu and the
 *     binding cannot be changed.
 */
#define EVTCHNOP_bind_virq        1
struct evtchn_bind_virq {
    /* IN parameters. */
    uint32_t virq;
    uint32_t vcpu;
    /* OUT parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_bind_virq evtchn_bind_virq_t;

/*
 * EVTCHNOP_bind_pirq: Bind a local event channel to PIRQ <irq>.
 * NOTES:
 *  1. A physical IRQ may be bound to at most one event channel per domain.
 *  2. Only a sufficiently-privileged domain may bind to a physical IRQ.
 */
#define EVTCHNOP_bind_pirq        2
struct evtchn_bind_pirq {
    /* IN parameters. */
    uint32_t pirq;
#define BIND_PIRQ__WILL_SHARE 1
    uint32_t flags; /* BIND_PIRQ__* */
    /* OUT parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_bind_pirq evtchn_bind_pirq_t;

/*
 * EVTCHNOP_bind_ipi: Bind a local event channel to receive events.
 * NOTES:
 *  1. The allocated event channel is bound to the specified vcpu. The binding
 *     may not be changed.
 */
#define EVTCHNOP_bind_ipi         7
struct evtchn_bind_ipi {
    uint32_t vcpu;
    /* OUT parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_bind_ipi evtchn_bind_ipi_t;

/*
 * EVTCHNOP_close: Close a local event channel <port>. If the channel is
 * interdomain then the remote end is placed in the unbound state
 * (EVTCHNSTAT_unbound), awaiting a new connection.
 */
#define EVTCHNOP_close            3
struct evtchn_close {
    /* IN parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_close evtchn_close_t;

/*
 * EVTCHNOP_send: Send an event to the remote end of the channel whose local
 * endpoint is <port>.
 */
#define EVTCHNOP_send             4
struct evtchn_send {
    /* IN parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_send evtchn_send_t;

/*
 * EVTCHNOP_status: Get the current status of the communication channel which
 * has an endpoint at <dom, port>.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may obtain the status of an event
 *     channel for which <dom> is not DOMID_SELF.
 */
#define EVTCHNOP_status           5
struct evtchn_status {
    /* IN parameters */
    domid_t  dom;
    evtchn_port_t port;
    /* OUT parameters */
#define EVTCHNSTAT_closed       0  /* Channel is not in use.                 */
#define EVTCHNSTAT_unbound      1  /* Channel is waiting interdom connection.*/
#define EVTCHNSTAT_interdomain  2  /* Channel is connected to remote domain. */
#define EVTCHNSTAT_pirq         3  /* Channel is bound to a phys IRQ line.   */
#define EVTCHNSTAT_virq         4  /* Channel is bound to a virtual IRQ line */
#define EVTCHNSTAT_ipi          5  /* Channel is bound to a virtual IPI line */
    uint32_t status;
    uint32_t vcpu;                 /* VCPU to which this channel is bound.   */
    union {
        struct {
            domid_t dom;
        } unbound; /* EVTCHNSTAT_unbound */
        struct {
            domid_t dom;
            evtchn_port_t port;
        } interdomain; /* EVTCHNSTAT_interdomain */
        uint32_t pirq;      /* EVTCHNSTAT_pirq        */
        uint32_t virq;      /* EVTCHNSTAT_virq        */
    } u;
};
typedef struct evtchn_status evtchn_status_t;

/*
 * EVTCHNOP_bind_vcpu: Specify which vcpu a channel should notify when an
 * event is pending.
 * NOTES:
 *  1. IPI-bound channels always notify the vcpu specified at bind time.
 *     This binding cannot be changed.
 *  2. Per-VCPU VIRQ channels always notify the vcpu specified at bind time.
 *     This binding cannot be changed.
 *  3. All other channels notify vcpu0 by default. This default is set when
 *     the channel is allocated (a port that is freed and subsequently reused
 *     has its binding reset to vcpu0).
 */
#define EVTCHNOP_bind_vcpu        8
struct evtchn_bind_vcpu {
    /* IN parameters. */
    evtchn_port_t port;
    uint32_t vcpu;
};
typedef struct evtchn_bind_vcpu evtchn_bind_vcpu_t;

/*
 * EVTCHNOP_unmask: Unmask the specified local event-channel port and deliver
 * a notification to the appropriate VCPU if an event is pending.
 */
#define EVTCHNOP_unmask           9
struct evtchn_unmask {
    /* IN parameters. */
    evtchn_port_t port;
};
typedef struct evtchn_unmask evtchn_unmask_t;

/*
 * EVTCHNOP_reset: Close all event channels associated with specified domain.
 * NOTES:
 *  1. <dom> may be specified as DOMID_SELF.
 *  2. Only a sufficiently-privileged domain may specify other than DOMID_SELF.
 */
#define EVTCHNOP_reset           10
struct evtchn_reset {
    /* IN parameters. */
    domid_t dom;
};
typedef struct evtchn_reset evtchn_reset_t;

/*
 * Argument to event_channel_op_compat() hypercall. Superceded by new
 * event_channel_op() hypercall since 0x00030202.
 */
struct evtchn_op {
    uint32_t cmd; /* EVTCHNOP_* */
    union {
        struct evtchn_alloc_unbound    alloc_unbound;
        struct evtchn_bind_interdomain bind_interdomain;
        struct evtchn_bind_virq        bind_virq;
        struct evtchn_bind_pirq        bind_pirq;
        struct evtchn_bind_ipi         bind_ipi;
        struct evtchn_close            close;
        struct evtchn_send             send;
        struct evtchn_status           status;
        struct evtchn_bind_vcpu        bind_vcpu;
        struct evtchn_unmask           unmask;
    } u;
};
typedef struct evtchn_op evtchn_op_t;
DEFINE_XEN_GUEST_HANDLE(evtchn_op_t);

#endif /* __XEN_PUBLIC_EVENT_CHANNEL_H__ */

/*
 * Local variables:
 * mode: C
 * c-set-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
