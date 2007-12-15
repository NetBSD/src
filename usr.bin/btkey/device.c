/*	$NetBSD: device.c,v 1.2 2007/12/15 16:03:30 perry Exp $	*/

/*-
 * Copyright (c) 2007 Iain Hibbert
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
 * 3. The name of the author may not be used to endorse or promote products
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
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: device.c,v 1.2 2007/12/15 16:03:30 perry Exp $");

#include <bluetooth.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "btkey.h"

/*
 * read/write stored link keys packet, with space for one key
 */
struct stored_link_keys {
	uint8_t		num_keys;
	struct {
		bdaddr_t addr;
		uint8_t	 key[HCI_KEY_SIZE];
	} key[1];
} __packed;

/*
 * generic request
 *
 *	send command 'opcode' with command packet 'cptr' of size 'clen'
 *	call 'func_cc' on command_complete event
 *	call 'func_ev' on event 'event'
 *	callbacks return -1 (failure), 0 (continue) or 1 (success)
 */
static bool
hci_req(uint16_t opcode, void *cptr, size_t clen, int (*func_cc)(void *),
	uint8_t event, int (*func_ev)(void *))
{
	uint8_t buf[sizeof(hci_cmd_hdr_t) + HCI_CMD_PKT_SIZE];
	struct sockaddr_bt sa;
	struct hci_filter f;
	hci_cmd_hdr_t *hdr;
	hci_event_hdr_t *ep;
	int fd, rv;

	memset(&f, 0, sizeof(f));
	hci_filter_set(HCI_EVENT_COMMAND_COMPL, &f);
	if (event != 0) hci_filter_set(event, &f);

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, &laddr);

	hdr = (hci_cmd_hdr_t *)buf;
	hdr->type = HCI_CMD_PKT;
	hdr->opcode = htole16(opcode);
	hdr->length = clen;

	memcpy(buf + sizeof(hci_cmd_hdr_t), cptr, clen);

	rv = -1;

	if ((fd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI)) < 0
	    || setsockopt(fd, BTPROTO_HCI, SO_HCI_EVT_FILTER, &f, sizeof(f)) < 0
	    || bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0
	    || connect(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0
	    || send(fd, buf, sizeof(hci_cmd_hdr_t) + clen, 0) < 0)
		goto done;

	ep = (hci_event_hdr_t *)buf;
	for (;;) {
		if (recv(fd, buf, sizeof(buf), 0) < 0)
			goto done;

		if (ep->event == HCI_EVENT_COMMAND_COMPL) {
			hci_command_compl_ep *cc;

			cc = (hci_command_compl_ep *)(ep + 1);
			if (opcode != le16toh(cc->opcode))
				continue;

			rv = func_cc(cc + 1);
			if (rv == 0)
				continue;

			goto done;
		}

		if (event != 0 && event == ep->event) {
			rv = func_ev(ep + 1);
			if (rv == 0)
				continue;

			goto done;
		}
	}

done:
	if (fd >= 0) close(fd);
	return rv > 0 ? true : false;
}

/*
 * List keys on device
 */

static int
list_device_cc(void *arg)
{
	hci_read_stored_link_key_rp *rp = arg;

	if (rp->status)
		return -1;

	printf("\n");
	printf("read %d keys (max %d)\n", rp->num_keys_read, rp->max_num_keys);

	return 1;
}

static int
list_device_ev(void *arg)
{
	struct stored_link_keys *ep = arg;
	int i;

	for (i = 0 ; i < ep->num_keys ; i++) {
		printf("\n");
		print_addr("bdaddr", &ep->key[i].addr);
		print_key("device key", ep->key[i].key);
	}

	return 0;
}

bool
list_device(void)
{
	hci_read_stored_link_key_cp cp;

	bdaddr_copy(&cp.bdaddr, BDADDR_ANY);
	cp.read_all = 0x01;

	return hci_req(HCI_CMD_READ_STORED_LINK_KEY,
			&cp, sizeof(cp), list_device_cc,
			HCI_EVENT_RETURN_LINK_KEYS, list_device_ev);
}

/*
 * Read key from device
 */

static int
read_device_cc(void *arg)
{

	/* if we got here, no key was found */
	return -1;
}

static int
read_device_ev(void *arg)
{
	struct stored_link_keys *ep = arg;

	if (ep->num_keys != 1
	    || !bdaddr_same(&ep->key[0].addr, &raddr))
		return 0;

	memcpy(key, ep->key[0].key, HCI_KEY_SIZE);
	return 1;
}

bool
read_device(void)
{
	hci_read_stored_link_key_cp cp;

	bdaddr_copy(&cp.bdaddr, &raddr);
	cp.read_all = 0x00;

	return hci_req(HCI_CMD_READ_STORED_LINK_KEY,
			&cp, sizeof(cp), read_device_cc,
			HCI_EVENT_RETURN_LINK_KEYS, read_device_ev);
}

/*
 * Write key to device
 */
static int
write_device_cc(void *arg)
{
	hci_write_stored_link_key_rp *rp = arg;

	if (rp->status || rp->num_keys_written != 1)
		return -1;

	return 1;
}

bool
write_device(void)
{
	struct stored_link_keys cp;

	cp.num_keys = 1;
	bdaddr_copy(&cp.key[0].addr, &raddr);
	memcpy(cp.key[0].key, key, HCI_KEY_SIZE);

	return hci_req(HCI_CMD_WRITE_STORED_LINK_KEY,
			&cp, sizeof(cp), write_device_cc,
			0, NULL);
}

/*
 * Clear key from device
 */
static int
clear_device_cc(void *arg)
{
	hci_delete_stored_link_key_rp *rp = arg;

	if (rp->status || rp->num_keys_deleted != 1)
		return -1;

	return 1;
}

bool
clear_device(void)
{
	hci_delete_stored_link_key_cp cp;

	cp.delete_all = 0x00;
	bdaddr_copy(&cp.bdaddr, &raddr);

	return hci_req(HCI_CMD_DELETE_STORED_LINK_KEY,
			&cp, sizeof(cp), clear_device_cc,
			0, NULL);
}
