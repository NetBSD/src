/*	$NetBSD: client.c,v 1.3 2006/09/26 19:18:19 plunky Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: client.c,v 1.3 2006/09/26 19:18:19 plunky Exp $");

#include <sys/ioctl.h>
#include <sys/queue.h>
#include <sys/time.h>
#include <sys/un.h>
#include <bluetooth.h>
#include <errno.h>
#include <event.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bthcid.h"

/*
 * A client is anybody who connects to our control socket to
 * receive PIN requests.
 */
struct client {
	struct event		ev;
	int			fd;		/* client descriptor */
	LIST_ENTRY(client)	next;
};

/*
 * PIN cache items are made when we have sent a client pin
 * request. The event is used to expire the item.
 */
struct item {
	struct event	 ev;
	bdaddr_t	 laddr;			/* local device BDADDR */
	bdaddr_t	 raddr;			/* remote device BDADDR */
	uint8_t		 pin[HCI_PIN_SIZE];	/* PIN */
	int		 hci;			/* HCI socket */
	LIST_ENTRY(item) next;
};

static struct event		control_ev;

static LIST_HEAD(,client)	client_list;
static LIST_HEAD(,item)		item_list;

static void process_control	(int, short, void *);
static void process_client	(int, short, void *);
static void process_item	(int, short, void *);

#define PIN_REQUEST_TIMEOUT	30	/* Request is valid */
#define PIN_TIMEOUT		300	/* PIN is valid */

int
init_control(const char *name, mode_t mode)
{
	struct sockaddr_un	un;
	int			ctl;

	LIST_INIT(&client_list);
	LIST_INIT(&item_list);

	if (name == NULL)
		return 0;

	if (unlink(name) < 0 && errno != ENOENT)
		return -1;

	ctl = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (ctl < 0)
		return -1;

	memset(&un, 0, sizeof(un));
	un.sun_len = sizeof(un);
	un.sun_family = AF_LOCAL;
	strlcpy(un.sun_path, name, sizeof(un.sun_path));
	if (bind(ctl, (struct sockaddr *)&un, sizeof(un)) < 0) {
		close(ctl);
		return -1;
	}

	if (chmod(name, mode) < 0) {
		close(ctl);
		unlink(name);
		return -1;
	}

	if (listen(ctl, 10) < 0) {
		close(ctl);
		unlink(name);
		return -1;
	}

	event_set(&control_ev, ctl, EV_READ | EV_PERSIST, process_control, NULL);
	if (event_add(&control_ev, NULL) < 0) {
		close(ctl);
		unlink(name);
		return -1;
	}

	return 0;
}

/* Process control socket event */
static void
process_control(int sock, short ev, void *arg)
{
	struct sockaddr_un	un;
	socklen_t		n;
	int			fd;
	struct client		*cl;

	n = sizeof(un);
	fd = accept(sock, (struct sockaddr *)&un, &n);
	if (fd < 0) {
		syslog(LOG_ERR, "Could not accept PIN client connection");
		return;
	}

	n = 1;
	if (ioctl(fd, FIONBIO, &n) < 0) {
		syslog(LOG_ERR, "Could not set non blocking IO for client");
		close(fd);
		return;
	}

	cl = malloc(sizeof(struct client));
	if (cl == NULL) {
		syslog(LOG_ERR, "Could not malloc client");
		close(fd);
		return;
	}

	memset(cl, 0, sizeof(struct client));
	cl->fd = fd;

	event_set(&cl->ev, fd, EV_READ | EV_PERSIST, process_client, cl);
	if (event_add(&cl->ev, NULL) < 0) {
		syslog(LOG_ERR, "Could not add client event");
		free(cl);
		close(fd);
		return;
	}

	syslog(LOG_DEBUG, "New Client");
	LIST_INSERT_HEAD(&client_list, cl, next);
}

/* Process client response packet */
static void
process_client(int sock, short ev, void *arg)
{
	bthcid_pin_response_t	 rp;
	struct timeval		 tv;
	struct sockaddr_bt	 sa;
	struct client		*cl = arg;
	struct item		*item;
	int			 n;

	n = recv(sock, &rp, sizeof(rp), 0);
	if (n != sizeof(rp)) {
		if (n != 0)
			syslog(LOG_ERR, "Bad Client");

		close(sock);
		LIST_REMOVE(cl, next);
		free(cl);

		syslog(LOG_DEBUG, "Client Closed");
		return;
	}

	syslog(LOG_DEBUG, "New PIN for %s", bt_ntoa(&rp.raddr, NULL));

	LIST_FOREACH(item, &item_list, next) {
		if (bdaddr_same(&rp.laddr, &item->laddr) == 0
		    || bdaddr_same(&rp.raddr, &item->raddr) == 0)
			continue;

		evtimer_del(&item->ev);
		if (item->hci != -1) {
			memset(&sa, 0, sizeof(sa));
			sa.bt_len = sizeof(sa);
			sa.bt_family = AF_BLUETOOTH;
			bdaddr_copy(&sa.bt_bdaddr, &item->laddr);

			send_pin_code_reply(item->hci, &sa, &item->raddr, rp.pin);
		}
		goto newpin;
	}

	item = malloc(sizeof(struct item));
	if (item == NULL) {
		syslog(LOG_ERR, "Item allocation failed");
		return;
	}

	memset(item, 0, sizeof(struct item));
	bdaddr_copy(&item->laddr, &rp.laddr);
	bdaddr_copy(&item->raddr, &rp.raddr);
	evtimer_set(&item->ev, process_item, item);
	LIST_INSERT_HEAD(&item_list, item, next);

newpin:
	memcpy(item->pin, rp.pin, HCI_PIN_SIZE);
	item->hci = -1;

	tv.tv_sec = PIN_TIMEOUT;
	tv.tv_usec = 0;

	if (evtimer_add(&item->ev, &tv) < 0) {
		syslog(LOG_ERR, "Cannot add event timer for item");
		LIST_REMOVE(item, next);
		free(item);
	}
}

/* Send PIN request to client */
int
send_client_request(bdaddr_t *laddr, bdaddr_t *raddr, int hci)
{
	bthcid_pin_request_t	 cp;
	struct client		*cl;
	struct item		*item;
	int			 n = 0;
	struct timeval		 tv;

	memset(&cp, 0, sizeof(cp));
	bdaddr_copy(&cp.laddr, laddr);
	bdaddr_copy(&cp.raddr, raddr);
	cp.time = PIN_REQUEST_TIMEOUT;

	LIST_FOREACH(cl, &client_list, next) {
		if (send(cl->fd, &cp, sizeof(cp), 0) != sizeof(cp))
			syslog(LOG_ERR, "send PIN request failed");
		else
			n++;
	}

	if (n == 0)
		return 0;

	syslog(LOG_DEBUG, "Sent PIN requests to %d client%s.",
				n, (n == 1 ? "" : "s"));

	item = malloc(sizeof(struct item));
	if (item == NULL) {
		syslog(LOG_ERR, "Cannot allocate PIN request item");
		return 0;
	}

	memset(item, 0, sizeof(struct item));
	bdaddr_copy(&item->laddr, laddr);
	bdaddr_copy(&item->raddr, raddr);
	item->hci = hci;
	evtimer_set(&item->ev, process_item, item);

	tv.tv_sec = cp.time;
	tv.tv_usec = 0;

	if (evtimer_add(&item->ev, &tv) < 0) {
		syslog(LOG_ERR, "Cannot add request timer");
		free(item);
		return 0;
	}

	LIST_INSERT_HEAD(&item_list, item, next);
	return 1;
}

/* Process item event (by expiring it) */
static void
process_item(int fd, short ev, void *arg)
{
	struct item *item = arg;

	syslog(LOG_DEBUG, "PIN for %s expired", bt_ntoa(&item->raddr, NULL));

	LIST_REMOVE(item, next);
	free(item);
}

/* lookup PIN in item cache */
uint8_t *
lookup_pin(bdaddr_t *laddr, bdaddr_t *raddr)
{
	struct item *item;

	LIST_FOREACH(item, &item_list, next) {
		if (bdaddr_same(raddr, &item->raddr) == 0)
			continue;

		if (bdaddr_same(laddr, &item->laddr) == 0
		    && bdaddr_any(&item->laddr) == 0)
			continue;

		if (item->hci >= 0)
			break;

		syslog(LOG_DEBUG, "Matched PIN from cache");
		return item->pin;
	}

	return NULL;
}
