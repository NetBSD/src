/* $NetBSD: xenbus_comms.c,v 1.12.2.3 2009/11/01 21:43:29 jym Exp $ */
/******************************************************************************
 * xenbus_comms.c
 *
 * Low level code to talks to Xen Store: ringbuffer and event channel.
 *
 * Copyright (C) 2005 Rusty Russell, IBM Corporation
 * 
 * This file may be distributed separately from the Linux kernel, or
 * incorporated into other software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xenbus_comms.c,v 1.12.2.3 2009/11/01 21:43:29 jym Exp $");

#include <sys/types.h>
#include <sys/null.h> 
#include <sys/errno.h> 
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <xen/xen.h>	/* for xendomain_is_dom0() */
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <xen/xenbus.h>
#include "xenbus_comms.h"

#undef XENDEBUG
#ifdef XENDEBUG
#define XENPRINTF(x) printf x
#else
#define XENPRINTF(x)
#endif

struct xenstore_domain_interface *xenstore_interface;

extern int xenstored_ready; 
// static DECLARE_WORK(probe_work, xenbus_probe, NULL);

static int wake_waiting(void *);
static int check_indexes(XENSTORE_RING_IDX, XENSTORE_RING_IDX);
static void *get_output_chunk(XENSTORE_RING_IDX, XENSTORE_RING_IDX,
    char *, uint32_t *);
static const void *get_input_chunk(XENSTORE_RING_IDX, XENSTORE_RING_IDX,
    const char *, uint32_t *);


static inline struct xenstore_domain_interface *
xenstore_domain_interface(void)
{
	return xenstore_interface;
}

static int
wake_waiting(void *arg)
{
	if (__predict_false(xenstored_ready == 0 && xendomain_is_dom0())) {
		xenstored_ready = 1; 
		wakeup(&xenstored_ready);
	} 

	wakeup(&xenstore_interface);
	return 1;
}

static int
check_indexes(XENSTORE_RING_IDX cons, XENSTORE_RING_IDX prod)
{
	return ((prod - cons) <= XENSTORE_RING_SIZE);
}

static void *
get_output_chunk(XENSTORE_RING_IDX cons,
			      XENSTORE_RING_IDX prod,
			      char *buf, uint32_t *len)
{
	*len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(prod);
	if ((XENSTORE_RING_SIZE - (prod - cons)) < *len)
		*len = XENSTORE_RING_SIZE - (prod - cons);
	return buf + MASK_XENSTORE_IDX(prod);
}

static const void *
get_input_chunk(XENSTORE_RING_IDX cons,
				   XENSTORE_RING_IDX prod,
				   const char *buf, uint32_t *len)
{
	*len = XENSTORE_RING_SIZE - MASK_XENSTORE_IDX(cons);
	if ((prod - cons) < *len)
		*len = prod - cons;
	return buf + MASK_XENSTORE_IDX(cons);
}

int
xb_write(const void *data, unsigned len)
{
	struct xenstore_domain_interface *intf = xenstore_domain_interface();
	XENSTORE_RING_IDX cons, prod;

	int s = spltty();

	while (len != 0) {
		void *dst;
		unsigned int avail;

		while ((intf->req_prod - intf->req_cons) == XENSTORE_RING_SIZE) {
			XENPRINTF(("xb_write tsleep\n"));
			tsleep(&xenstore_interface, PRIBIO, "wrst", 0);
			XENPRINTF(("xb_write tsleep done\n"));
		}

		/* Read indexes, then verify. */
		cons = intf->req_cons;
		prod = intf->req_prod;
		xen_rmb();
		if (!check_indexes(cons, prod)) {
			splx(s);
			return EIO;
		}

		dst = get_output_chunk(cons, prod, intf->req, &avail);
		if (avail == 0)
			continue;
		if (avail > len)
			avail = len;

		memcpy(dst, data, avail);
		data = (const char *)data + avail;
		len -= avail;

		/* Other side must not see new header until data is there. */
		xen_rmb();
		intf->req_prod += avail;
		xen_rmb();

		hypervisor_notify_via_evtchn(xen_start_info.store_evtchn);
	}

	splx(s);
	return 0;
}

int
xb_read(void *data, unsigned len)
{
	struct xenstore_domain_interface *intf = xenstore_domain_interface();
	XENSTORE_RING_IDX cons, prod;

	int s = spltty();

	while (len != 0) {
		unsigned int avail;
		const char *src;

		while (intf->rsp_cons == intf->rsp_prod)
			tsleep(&xenstore_interface, PRIBIO, "rdst", 0);

		/* Read indexes, then verify. */
		cons = intf->rsp_cons;
		prod = intf->rsp_prod;
		xen_rmb();
		if (!check_indexes(cons, prod)) {
			XENPRINTF(("xb_read EIO\n"));
			splx(s);
			return EIO;
		}

		src = get_input_chunk(cons, prod, intf->rsp, &avail);
		if (avail == 0)
			continue;
		if (avail > len)
			avail = len;

		/* We must read header before we read data. */
		xen_rmb();

		memcpy(data, src, avail);
		data = (char *)data + avail;
		len -= avail;

		/* Other side must not see free space until we've copied out */
		xen_rmb();
		intf->rsp_cons += avail;
		xen_rmb();

		XENPRINTF(("Finished read of %i bytes (%i to go)\n",
		    avail, len));

		hypervisor_notify_via_evtchn(xen_start_info.store_evtchn);
	}

	splx(s);
	return 0;
}

/* Set up interrupt handler of store event channel. */
int
xb_init_comms(device_t dev)
{
	int err;
	int evtchn;

	evtchn = xen_start_info.store_evtchn;

	err = event_set_handler(evtchn, wake_waiting, NULL, IPL_TTY, "xenbus");
	if (err) {
		aprint_error_dev(dev, "request irq failed %i\n", err);
		return err;
	}

	hypervisor_enable_event(evtchn);
	aprint_verbose_dev(dev, "using event channel %d\n", evtchn);
	return 0;
}

void
xb_suspend_comms(device_t dev) {

	int err;
	int evtchn;

	evtchn = xen_start_info.store_evtchn;

	hypervisor_mask_event(evtchn);

	err = event_remove_handler(evtchn, wake_waiting, NULL);
	aprint_verbose_dev(dev, "removed event channel %d\n", evtchn);

}

/*
 * Local variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 *  c-indent-level: 8
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
