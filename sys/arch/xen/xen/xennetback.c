/*      $NetBSD: xennetback.c,v 1.1.2.1 2005/02/16 13:48:04 bouyer Exp $      */

/*
 * Copyright (c) 2005 Manuel Bouyer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/kernel.h>

#include <machine/xen.h>
#include <machine/evtchn.h>
#include <machine/ctrl_if.h>

#include <machine/xen-public/io/domain_controller.h>

/*
 * Backend block device driver for Xen
 */

static void xnetback_ctrlif_rx(ctrl_msg_t *, unsigned long);

/* state of a xnetback instance */
typedef enum {CONNECTED, DISCONNECTED} xnetback_state_t;

/* we keep the xnetback instances in a linked list */
struct xnetback_instance {
	SLIST_ENTRY(xnetback_instance) next; 
	domid_t domid;		/* attached to this domain */
	uint32_t handle;	/* domain-specific handle */
	xnetback_state_t status;
};

SLIST_HEAD(, xnetback_instance) xnetback_instances;

static struct xnetback_instance *xnetif_lookup(domid_t, uint32_t);


void
xennetback_init()
{
	ctrl_msg_t cmsg;
	netif_be_driver_status_t st;

	if ( !(xen_start_info.flags & SIF_INITDOMAIN) &&
	     !(xen_start_info.flags & SIF_NET_BE_DOMAIN))
		return;

	printf("xennetback_init\n");

	/*
	 * initialize the backend driver, register the control message handler
	 * and send driver up message.
	 */
	SLIST_INIT(&xnetback_instances);
	(void)ctrl_if_register_receiver(CMSG_NETIF_BE, xnetback_ctrlif_rx,
	    CALLBACK_IN_BLOCKING_CONTEXT);

	cmsg.type      = CMSG_NETIF_BE;
	cmsg.subtype   = CMSG_NETIF_BE_DRIVER_STATUS;
	cmsg.length    = sizeof(netif_be_driver_status_t);
	st.status      = NETIF_DRIVER_STATUS_UP;
	memcpy(cmsg.msg, &st, sizeof(st));
	ctrl_if_send_message_block(&cmsg, NULL, 0, 0);
}

static void
xnetback_ctrlif_rx(ctrl_msg_t *msg, unsigned long id)
{
	struct xnetback_instance *xneti;

	printf("xnetback msg %d\n", msg->subtype);
	switch (msg->subtype) {
	case CMSG_NETIF_BE_CREATE:
	{
		netif_be_create_t *req = (netif_be_create_t *)&msg->msg[0];
		if (msg->length != sizeof(netif_be_create_t))
			goto error;
		if (xnetif_lookup(req->domid, req->netif_handle) != NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_EXISTS;
			goto end;
		}
		xneti = malloc(sizeof(struct xnetback_instance), M_DEVBUF,
		    M_NOWAIT | M_ZERO);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_OUT_OF_MEMORY;
			goto end;
		}
		xneti->domid = req->domid;
		xneti->handle = req->netif_handle;
		xneti->status = DISCONNECTED;
		req->status = NETIF_BE_STATUS_OKAY;
		SLIST_INSERT_HEAD(&xnetback_instances, xneti, next);
		break;
	}
	case CMSG_NETIF_BE_DESTROY:
	{
		netif_be_destroy_t *req = (netif_be_destroy_t *)&msg->msg[0];
		if (msg->length != sizeof(netif_be_destroy_t))
			goto error;
		xneti = xnetif_lookup(req->domid, req->netif_handle);
		if (xneti == NULL) {
			req->status = NETIF_BE_STATUS_INTERFACE_NOT_FOUND;
			goto end;
		}
		if (xneti->status == CONNECTED) {
			req->status = NETIF_BE_STATUS_INTERFACE_CONNECTED;
			goto end;
		}
		SLIST_REMOVE(&xnetback_instances,
		    xneti, xnetback_instance, next);
		free(xneti, M_DEVBUF);
		req->status = NETIF_BE_STATUS_OKAY;
		break;
	}
	case CMSG_NETIF_BE_CONNECT:
	{
		netif_be_connect_t *req = (netif_be_connect_t *)&msg->msg[0];
		if (msg->length != sizeof(netif_be_connect_t))
			goto error;
		req->status = NETIF_BE_STATUS_ERROR;
		break;
	}
	case CMSG_NETIF_BE_DISCONNECT:
	{
		netif_be_disconnect_t *req =
		    (netif_be_disconnect_t *)&msg->msg[0];
		if (msg->length != sizeof(netif_be_disconnect_t))
			goto error;
		req->status = NETIF_BE_STATUS_ERROR;
		break;
	}
	default:
error:
		printf("xnetback: wrong message subtype %d len %d\n",
		    msg->subtype, msg->length);
		msg->length = 0;
	}
end:
	printf("xnetback msg rep %d\n", msg->length);
	ctrl_if_send_response(msg);
	return;
}

static struct xnetback_instance *
xnetif_lookup(domid_t dom , uint32_t handle)
{
	struct xnetback_instance *xneti;

	SLIST_FOREACH(xneti, &xnetback_instances, next) {
		if (xneti->domid == dom && xneti->handle == handle)
			return xneti;
	}
	return NULL;
}
