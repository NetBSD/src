/*	$NetBSD: hci.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $	*/

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
/*
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: hci.c,v 1.1 2006/06/19 15:44:56 gdamore Exp $");

#include <sys/ioctl.h>
#include <sys/time.h>
#include <bluetooth.h>
#include <errno.h>
#include <event.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "bthcid.h"

static struct event	hci_ev;

static void process_hci
		(int, short, void *);

static int process_pin_code_request_event
		(int, struct sockaddr_bt *, bdaddr_t *);
static int process_link_key_request_event
		(int, struct sockaddr_bt *, bdaddr_t *);
static int process_link_key_notification_event
		(int, struct sockaddr_bt *, hci_link_key_notification_ep *);

static int send_link_key_reply
		(int, struct sockaddr_bt *, bdaddr_t *, uint8_t *);
static int send_hci_cmd
		(int, struct sockaddr_bt *, uint16_t, size_t, void *);

static char dev_name[HCI_DEVNAME_SIZE];

/* Initialise HCI Events */
int
init_hci(bdaddr_t *bdaddr)
{
	struct sockaddr_bt	sa;
	struct hci_filter	filter;
	int			hci;

	hci = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (hci < 0)
		return -1;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, bdaddr);
	if (bind(hci, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		close(hci);
		return -1;
	}

	memset(&filter, 0, sizeof(filter));
	hci_filter_set(HCI_EVENT_PIN_CODE_REQ, &filter);
	hci_filter_set(HCI_EVENT_LINK_KEY_REQ, &filter);
	hci_filter_set(HCI_EVENT_LINK_KEY_NOTIFICATION, &filter);

	if (setsockopt(hci, BTPROTO_HCI, SO_HCI_EVT_FILTER,
			(const void *)&filter, sizeof(filter)) < 0) {
		close(hci);
		return -1;
	}

	event_set(&hci_ev, hci, EV_READ | EV_PERSIST, process_hci, NULL);
	if (event_add(&hci_ev, NULL) < 0) {
		close(hci);
		return -1;
	}

	return 0;
}

/* Process an HCI event */
static void
process_hci(int sock, short ev, void *arg)
{
	char			 buffer[HCI_EVENT_PKT_SIZE];
	hci_event_hdr_t		*event = (hci_event_hdr_t *)buffer;
	struct sockaddr_bt	 addr;
	int			 n;
	socklen_t		 size;

	size = sizeof(addr);
	n = recvfrom(sock, buffer, sizeof(buffer), 0,
			(struct sockaddr *) &addr, &size);
	if (n < 0) {
		syslog(LOG_ERR, "Could not receive from HCI socket. "
				"%s (%d)", strerror(errno), errno);

		return;
	}

	if (event->type != HCI_EVENT_PKT) {
		syslog(LOG_ERR, "Received unexpected HCI packet, "
				"type=%#x", event->type);

		return;
	}

	if (!bt_devname(dev_name, &addr.bt_bdaddr))
		strlcpy(dev_name, "unknown", sizeof(dev_name));

	switch (event->event) {
	case HCI_EVENT_PIN_CODE_REQ:
		process_pin_code_request_event(sock, &addr,
					    (bdaddr_t *)(event + 1));
		break;

	case HCI_EVENT_LINK_KEY_REQ:
		process_link_key_request_event(sock, &addr,
					    (bdaddr_t *)(event + 1));
		break;

	case HCI_EVENT_LINK_KEY_NOTIFICATION:
		process_link_key_notification_event(sock, &addr,
			(hci_link_key_notification_ep *)(event + 1));
		break;

	default:
		syslog(LOG_ERR, "Received unexpected HCI event, "
				"event=%#x", event->event);
		break;
	}

	return;
}

/* Process PIN_Code_Request event */
static int
process_pin_code_request_event(int sock, struct sockaddr_bt *addr,
		bdaddr_t *bdaddr)
{
	uint8_t	*pin;

	syslog(LOG_DEBUG, "Got PIN_Code_Request event from %s, "
			  "remote bdaddr %s",
			  dev_name,
			  bt_ntoa(bdaddr, NULL));

	pin = lookup_pin(&addr->bt_bdaddr, bdaddr);
	if (pin != NULL)
		return send_pin_code_reply(sock, addr, bdaddr, pin);

	if (send_client_request(&addr->bt_bdaddr, bdaddr, sock) == 0)
		return send_pin_code_reply(sock, addr, bdaddr, NULL);

	return 0;
}

/* Process Link_Key_Request event */
static int
process_link_key_request_event(int sock, struct sockaddr_bt *addr,
		bdaddr_t *bdaddr)
{
	uint8_t		*key;

	syslog(LOG_DEBUG,
		"Got Link_Key_Request event from %s, remote bdaddr %s",
		dev_name, bt_ntoa(bdaddr, NULL));

	key = lookup_key(&addr->bt_bdaddr, bdaddr);

	if (key != NULL) {
		syslog(LOG_DEBUG, "Found Key, remote bdaddr %s",
				bt_ntoa(bdaddr, NULL));

		return send_link_key_reply(sock, addr, bdaddr, key);
	}

	syslog(LOG_DEBUG, "Could not find link key for remote bdaddr %s",
			bt_ntoa(bdaddr, NULL));

	return send_link_key_reply(sock, addr, bdaddr, NULL);
}

/* Send PIN_Code_[Negative]_Reply */
int
send_pin_code_reply(int sock, struct sockaddr_bt *addr,
	bdaddr_t *bdaddr, uint8_t *pin)
{
	int	n;

	if (pin != NULL) {
		hci_pin_code_rep_cp	 cp;

		syslog(LOG_DEBUG, "Sending PIN_Code_Reply to %s "
				  "for remote bdaddr %s",
				  dev_name,
				  bt_ntoa(bdaddr, NULL));

		bdaddr_copy(&cp.bdaddr, bdaddr);
		memcpy(cp.pin, pin, HCI_PIN_SIZE);

		n = HCI_PIN_SIZE;
		while (n > 0 && pin[n - 1] == 0)
			n--;
		cp.pin_size = n;

		n = send_hci_cmd(sock, addr,
				HCI_CMD_PIN_CODE_REP, sizeof(cp), &cp);

	} else {
		syslog(LOG_DEBUG, "Sending PIN_Code_Negative_Reply to %s "
				  "for remote bdaddr %s",
				  dev_name,
				  bt_ntoa(bdaddr, NULL));

		n = send_hci_cmd(sock, addr, HCI_CMD_PIN_CODE_NEG_REP,
					sizeof(bdaddr_t), bdaddr);
	}

	if (n < 0) {
		syslog(LOG_ERR, "Could not send PIN code reply to %s "
				"for remote bdaddr %s. %s (%d)",
				dev_name,
				bt_ntoa(bdaddr, NULL),
				strerror(errno),
				errno);

		return -1;
	}

	return 0;
}

/* Send Link_Key_[Negative]_Reply */
static int
send_link_key_reply(int sock, struct sockaddr_bt *addr,
		bdaddr_t *bdaddr, uint8_t *key)
{
	int	n;

	if (key != NULL) {
		hci_link_key_rep_cp	cp;

		bdaddr_copy(&cp.bdaddr, bdaddr);
		memcpy(&cp.key, key, sizeof(cp.key));

		syslog(LOG_DEBUG, "Sending Link_Key_Reply to %s "
				"for remote bdaddr %s",
				dev_name, bt_ntoa(bdaddr, NULL));

		n = send_hci_cmd(sock, addr, HCI_CMD_LINK_KEY_REP, sizeof(cp), &cp);
	} else {
		hci_link_key_neg_rep_cp	cp;

		bdaddr_copy(&cp.bdaddr, bdaddr);

		syslog(LOG_DEBUG, "Sending Link_Key_Negative_Reply to %s "
				"for remote bdaddr %s",
				dev_name, bt_ntoa(bdaddr, NULL));

		n = send_hci_cmd(sock, addr, HCI_CMD_LINK_KEY_NEG_REP, sizeof(cp), &cp);
	}

	if (n < 0) {
		syslog(LOG_ERR, "Could not send link key reply to %s "
				"for remote bdaddr %s. %s (%d)",
				dev_name, bt_ntoa(bdaddr, NULL),
				strerror(errno), errno);
		return -1;
	}

	return 0;
}

/* Process Link_Key_Notification event */
static int
process_link_key_notification_event(int sock, struct sockaddr_bt *addr,
		hci_link_key_notification_ep *ep)
{

	syslog(LOG_DEBUG, "Got Link_Key_Notification event from %s, "
			"remote bdaddr %s",
			dev_name,
			bt_ntoa(&ep->bdaddr, NULL));

	save_key(&addr->bt_bdaddr, &ep->bdaddr, ep->key);
	return 0;
}

/* Send HCI Command Packet to socket */
static int
send_hci_cmd(int sock, struct sockaddr_bt *sa, uint16_t opcode, size_t len, void *buf)
{
	char msg[HCI_CMD_PKT_SIZE];
	hci_cmd_hdr_t *h = (hci_cmd_hdr_t *)msg;

	h->type = HCI_CMD_PKT;
	h->opcode = htole16(opcode);
	h->length = len;

	if (len > 0)
		memcpy(msg + sizeof(hci_cmd_hdr_t), buf, len);

	return sendto(sock, msg, sizeof(hci_cmd_hdr_t) + len, 0,
			(struct sockaddr *)sa, sizeof(*sa));
}
