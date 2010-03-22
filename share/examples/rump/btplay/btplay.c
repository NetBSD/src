/*      $NetBSD: btplay.c,v 1.1 2010/03/22 12:21:37 pooka Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Little demo showing how to use bluetooth stack in rump.  Searches
 * for a peer and prints a little info for it.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <netbt/bluetooth.h>
#include <netbt/hci.h>

#include <bluetooth.h>
#include <err.h>
#include <paths.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const char *btclasses[] = {
	"misc.",
	"computer",
	"phone",
	"LAN",
	"audio-video",
	"peripheral",
	"imaging",
	"wearable",
	"toy",
};

int
main(int argc, char *argv[])
{
	struct sockaddr_bt sbt;
	bdaddr_t peeraddr;
	uint8_t msg[1024];
	hci_inquiry_cp inq;
	hci_cmd_hdr_t *cmd;
	hci_event_hdr_t *evp;
	struct hci_filter filt;
	struct btreq btr;
	int s, gotpeer = 0;

	rump_boot_sethowto(RUMP_AB_VERBOSE);
	rump_init();

	s = rump_sys_socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		err(1, "socket");

	/* enable first device, print name and address */
	memset(&btr, 0, sizeof(btr));
	if (rump_sys_ioctl(s, SIOCNBTINFO, &btr) == -1)
		err(1, "no bt device?");

	btr.btr_flags |= BTF_UP;
	if (rump_sys_ioctl(s, SIOCSBTFLAGS, &btr) == -1)
		err(1, "raise interface");
	if (rump_sys_ioctl(s, SIOCGBTINFO, &btr) == -1)
		err(1, "reget info");

	memset(&sbt, 0, sizeof(sbt));
	sbt.bt_len = sizeof(sbt);
	sbt.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sbt.bt_bdaddr, &btr.btr_bdaddr);

	if (rump_sys_bind(s, (struct sockaddr *)&sbt, sizeof(sbt)) == -1)
		err(1, "bind");
	if (rump_sys_connect(s, (struct sockaddr *)&sbt, sizeof(sbt)) == -1)
		err(1, "connect");

	printf("device %s, addr %s\n",
	    btr.btr_name, bt_ntoa(&btr.btr_bdaddr, NULL));

	/* allow to receive various messages we need later */
	memset(&filt, 0, sizeof(filt));
	hci_filter_set(HCI_EVENT_COMMAND_COMPL, &filt);
	hci_filter_set(HCI_EVENT_INQUIRY_RESULT, &filt);
	hci_filter_set(HCI_EVENT_INQUIRY_COMPL, &filt);
	hci_filter_set(HCI_EVENT_REMOTE_NAME_REQ_COMPL, &filt);
	if (rump_sys_setsockopt(s, BTPROTO_HCI, SO_HCI_EVT_FILTER,
	    &filt, sizeof(filt)) == -1)
		err(1, "setsockopt");

	cmd = (void *)msg;
	evp = (void *)msg;

	/* discover peer.  first, send local access profile general inquiry */
	inq.lap[0] = 0x33;
	inq.lap[1] = 0x8b;
	inq.lap[2] = 0x9e;
	inq.inquiry_length = 4;
	inq.num_responses = 1;

	cmd->type = HCI_CMD_PKT;
	cmd->opcode = htole16(HCI_CMD_INQUIRY);
	cmd->length = sizeof(inq);
	memcpy(cmd+1, &inq, sizeof(inq));

	if (rump_sys_sendto(s, msg, sizeof(*cmd)+sizeof(inq), 0, NULL, 0) == -1)
		err(1, "send inquiry");
	memset(msg, 0, sizeof(msg));
	if (rump_sys_recvfrom(s, msg, sizeof(msg), 0, NULL, NULL) == -1)
		err(1, "recv inq response");
	if (evp->event != HCI_EVENT_COMMAND_COMPL)
		errx(1, "excepted command compl");

	/* then, wait for response */
	for (;;) {
		if (rump_sys_recvfrom(s, msg, sizeof(msg), 0, NULL, NULL) == -1)
			err(1, "recv inq result");

		if (evp->event == HCI_EVENT_INQUIRY_COMPL)
			break;
		if (evp->event == HCI_EVENT_INQUIRY_RESULT) {
			hci_inquiry_response *r;
			unsigned class;

			r = (void *)(msg + sizeof(hci_event_hdr_t)
			    + sizeof(hci_inquiry_result_ep));
			bdaddr_copy(&peeraddr, &r[0]. bdaddr);
			printf("my peer: %s, ", bt_ntoa(&peeraddr, NULL));
			class = r[0].uclass[1] & 0x1f;
			printf("major class: %d (%s)\n", class,
			    class < __arraycount(btclasses)
			      ? btclasses[class] : "unknown");
			gotpeer = 1;
		}
	}

	/* found peer.  ask for its name */
	if (gotpeer) {
		hci_remote_name_req_cp nreq;
		hci_remote_name_req_compl_ep *nresp;

		memset(&nreq, 0, sizeof(nreq));
		bdaddr_copy(&nreq.bdaddr, &peeraddr);

		cmd->type = HCI_CMD_PKT;
		cmd->opcode = htole16(HCI_CMD_REMOTE_NAME_REQ);
		cmd->length = sizeof(nreq);
		memcpy(cmd+1, &nreq, sizeof(nreq));

		if (rump_sys_sendto(s, msg, sizeof(*cmd)+sizeof(nreq), 0,
		    NULL, 0) == -1)
			err(1, "send name req");
		memset(msg, 0, sizeof(msg));
		if (rump_sys_recvfrom(s, msg, sizeof(msg), 0, NULL, NULL) == -1)
			err(1, "recv inq response");
		if (evp->event != HCI_EVENT_REMOTE_NAME_REQ_COMPL)
			errx(1, "excepted nreq compl");

		nresp = (void *)(evp+1);
		printf("peer name: %s\n", nresp->name);
	}

	return 0;
}
