/*	$NetBSD: bt_dev.c,v 1.1 2009/08/03 15:59:42 plunky Exp $	*/

/*-
 * Copyright (c) 2009 Iain Hibbert
 * Copyright (c) 2009 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Iain Hibbert for Itronix Inc.
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
__RCSID("$NetBSD: bt_dev.c,v 1.1 2009/08/03 15:59:42 plunky Exp $");

#include <sys/event.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/uio.h>

#include <bluetooth.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
bt_devaddr(const char *name, bdaddr_t *addr)
{
	struct btreq btr;
	bdaddr_t bdaddr;
	int s, rv;

	if (name == NULL) {
		errno = EINVAL;
		return 0;
	}

	if (addr == NULL)
		addr = &bdaddr;

	if (bt_aton(name, addr))
		return bt_devname(NULL, addr);

	memset(&btr, 0, sizeof(btr));
	strncpy(btr.btr_name, name, HCI_DEVNAME_SIZE);

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return 0;

	rv = ioctl(s, SIOCGBTINFO, &btr);
	close(s);

	if (rv == -1)
		return 0;

	if ((btr.btr_flags & BTF_UP) == 0) {
		errno = ENXIO;
		return 0;
	}

	bdaddr_copy(addr, &btr.btr_bdaddr);
	return 1;
}

int
bt_devname(char *name, const bdaddr_t *bdaddr)
{
	struct btreq btr;
	int s, rv;

	if (bdaddr == NULL) {
		errno = EINVAL;
		return 0;
	}

	memset(&btr, 0, sizeof(btr));
	bdaddr_copy(&btr.btr_bdaddr, bdaddr);

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return 0;

	rv = ioctl(s, SIOCGBTINFOA, &btr);
	close(s);

	if (rv == -1)
		return 0;

	if ((btr.btr_flags & BTF_UP) == 0) {
		errno = ENXIO;
		return 0;
	}

	if (name != NULL)
		strlcpy(name, btr.btr_name, HCI_DEVNAME_SIZE);

	return 1;
}

int
bt_devopen(const char *name, int options)
{
	struct sockaddr_bt	sa;
	int			opt, s;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;

	if (name != NULL && !bt_devaddr(name, &sa.bt_bdaddr))
		return -1;

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return -1;

	opt = 1;

	if ((options & BTOPT_DIRECTION) && setsockopt(s, BTPROTO_HCI,
	    SO_HCI_DIRECTION, &opt, sizeof(opt)) == -1) {
		close(s);
		return -1;
	}

	if ((options & BTOPT_TIMESTAMP) && setsockopt(s, SOL_SOCKET,
	    SO_TIMESTAMP, &opt, sizeof(opt)) == -1) {
		close(s);
		return -1;
	}

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		close(s);
		return -1;
	}

	if (name != NULL
	    && connect(s, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
		close(s);
		return -1;
	}

	return s;
}

ssize_t
bt_devsend(int s, uint16_t opcode, void *param, size_t plen)
{
	hci_cmd_hdr_t	hdr;
	struct iovec	iov[2];
	ssize_t		n;

	if (plen > UINT8_MAX
	    || (plen == 0 && param != NULL)
	    || (plen != 0 && param == NULL)) {
		errno = EINVAL;
		return -1;
	}

	hdr.type = HCI_CMD_PKT;
	hdr.opcode = htole16(opcode);
	hdr.length = (uint8_t)plen;

	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);

	iov[1].iov_base = param;
	iov[1].iov_len = plen;

	while ((n = writev(s, iov, __arraycount(iov))) == -1) {
		if (errno == EINTR)
			continue;

		return -1;
	}

	return n;
}

ssize_t
bt_devrecv(int s, void *buf, size_t size, time_t to)
{
	struct kevent	ev;
	struct timespec ts;
	uint8_t		*p;
	ssize_t		n;
	int		kq;

	if (buf == NULL || size == 0) {
		errno = EINVAL;
		return -1;
	}

	if (to >= 0) {	/* timeout is optional */
		kq = kqueue();
		if (kq == -1)
			return -1;

		EV_SET(&ev, s, EVFILT_READ, EV_ADD, 0, 0, 0);

		ts.tv_sec = to;
		ts.tv_nsec = 0;

		while (kevent(kq, &ev, 1, &ev, 1, &ts) == -1) {
			if (errno == EINTR)
				continue;

			close(kq);
			return -1;
		}

		close(kq);

		if (ev.data == 0) {
			errno = ETIMEDOUT;
			return -1;
		}
	}

	while ((n = recv(s, buf, size, 0)) == -1) {
		if (errno == EINTR)
			continue;

		return -1;
	}

	if (n == 0)
		return 0;

	p = buf;
	switch (p[0]) {	/* validate that they get complete packets */
	case HCI_CMD_PKT:
		if (sizeof(hci_cmd_hdr_t) > (size_t)n
		    || sizeof(hci_cmd_hdr_t) + p[3] != (size_t)n)
			break;

		return n;

	case HCI_ACL_DATA_PKT:
		if (sizeof(hci_acldata_hdr_t) > (size_t)n
		    || sizeof(hci_acldata_hdr_t) + le16dec(p + 3) != (size_t)n)
			break;

		return n;

	case HCI_SCO_DATA_PKT:
		if (sizeof(hci_scodata_hdr_t) > (size_t)n
		    || sizeof(hci_scodata_hdr_t) + p[3] != (size_t)n)
			break;

		return n;

	case HCI_EVENT_PKT:
		if (sizeof(hci_event_hdr_t) > (size_t)n
		    || sizeof(hci_event_hdr_t) + p[2] != (size_t)n)
			break;

		return n;

	default:
		break;
	}

	errno = EIO;
	return -1;
}

/*
 * Internal handler for bt_devreq(), do the actual request.
 */
static int
bt__devreq(int s, struct bt_devreq *req, time_t t_end)
{
	uint8_t			buf[HCI_EVENT_PKT_SIZE], *p;
	hci_event_hdr_t		ev;
	hci_command_status_ep	cs;
	hci_command_compl_ep	cc;
	time_t			to;
	ssize_t			n;

	n = bt_devsend(s, req->opcode, req->cparam, req->clen);
	if (n == -1)
		return errno;

	for (;;) {
		to = t_end - time(NULL);
		if (to < 0)
			return ETIMEDOUT;

		p = buf;
		n = bt_devrecv(s, buf, sizeof(buf), to);
		if (n == -1)
			return errno;

		if (sizeof(ev) > (size_t)n || p[0] != HCI_EVENT_PKT)
			return EIO;

		memcpy(&ev, p, sizeof(ev));
		p += sizeof(ev);
		n -= sizeof(ev);

		if (ev.event == req->event)
			break;

		if (ev.event == HCI_EVENT_COMMAND_STATUS) {
			if (sizeof(cs) > (size_t)n)
				return EIO;

			memcpy(&cs, p, sizeof(cs));
			p += sizeof(cs);
			n -= sizeof(cs);

			if (le16toh(cs.opcode) == req->opcode) {
				if (cs.status != 0)
					return EIO;

				if (req->event == 0)
					break;
			}

			continue;
		}

		if (ev.event == HCI_EVENT_COMMAND_COMPL) {
			if (sizeof(cc) > (size_t)n)
				return EIO;

			memcpy(&cc, p, sizeof(cc));
			p += sizeof(cc);
			n -= sizeof(cc);

			if (le16toh(cc.opcode) == req->opcode)
				break;

			continue;
		}
	}

	/* copy out response data */
	if (req->rlen >= (size_t)n) {
		req->rlen = n;
		memcpy(req->rparam, p, req->rlen);
	} else if (req->rlen > 0)
		return EIO;

	return 0;
}

int
bt_devreq(int s, struct bt_devreq *req, time_t to)
{
	struct bt_devfilter	new, old;
	int			error;

	if (req == NULL || to < 0
	    || (req->rlen == 0 && req->rparam != NULL)
	    || (req->rlen != 0 && req->rparam == NULL)) {
		errno = EINVAL;
		return -1;
	}

	memset(&new, 0, sizeof(new));
	bt_devfilter_pkt_set(&new, HCI_EVENT_PKT);
	bt_devfilter_evt_set(&new, HCI_EVENT_COMMAND_COMPL);
	bt_devfilter_evt_set(&new, HCI_EVENT_COMMAND_STATUS);

	if (req->event != 0)
		bt_devfilter_evt_set(&new, req->event);

	if (bt_devfilter(s, &new, &old) == -1)
		return -1;

	error = bt__devreq(s, req, to + time(NULL));

	(void)bt_devfilter(s, &old, NULL);

	if (error != 0) {
		errno = error;
		return -1;
	}

	return 0;
}

int
bt_devfilter(int s, const struct bt_devfilter *new, struct bt_devfilter *old)
{
	socklen_t	len;

	if (new == NULL && old == NULL) {
		errno = EINVAL;
		return -1;
	}

	len = sizeof(struct hci_filter);

	if (old != NULL) {
		if (getsockopt(s, BTPROTO_HCI,
		    SO_HCI_PKT_FILTER, &old->packet_mask, &len) == -1
		    || len != sizeof(struct hci_filter))
			return -1;

		if (getsockopt(s, BTPROTO_HCI,
		    SO_HCI_EVT_FILTER, &old->event_mask, &len) == -1
		    || len != sizeof(struct hci_filter))
			return -1;
	}

	if (new != NULL) {
		if (setsockopt(s, BTPROTO_HCI,
		    SO_HCI_PKT_FILTER, &new->packet_mask, len) == -1)
			return -1;

		if (setsockopt(s, BTPROTO_HCI,
		    SO_HCI_EVT_FILTER, &new->event_mask, len) == -1)
			return -1;
	}

	return 0;
}

void
bt_devfilter_pkt_set(struct bt_devfilter *filter, uint8_t type)
{

	hci_filter_set(type, &filter->packet_mask);
}

void
bt_devfilter_pkt_clr(struct bt_devfilter *filter, uint8_t type)
{

	hci_filter_clr(type, &filter->packet_mask);
}

int
bt_devfilter_pkt_tst(const struct bt_devfilter *filter, uint8_t type)
{

	return hci_filter_test(type, &filter->packet_mask);
}

void
bt_devfilter_evt_set(struct bt_devfilter *filter, uint8_t event)
{

	hci_filter_set(event, &filter->event_mask);
}

void
bt_devfilter_evt_clr(struct bt_devfilter *filter, uint8_t event)
{

	hci_filter_clr(event, &filter->event_mask);
}

int
bt_devfilter_evt_tst(const struct bt_devfilter *filter, uint8_t event)
{

	return hci_filter_test(event, &filter->event_mask);
}

/*
 * Internal function used by bt_devinquiry to find the first
 * active device.
 */
static int
bt__devany_cb(int s, const struct bt_devinfo *info, void *arg)
{

	if ((info->enabled)) {
		strlcpy(arg, info->devname, HCI_DEVNAME_SIZE + 1);
		return 1;
	}

	return 0;
}

/*
 * Internal function used by bt_devinquiry to insert inquiry
 * results to an array. Make sure that a bdaddr only appears
 * once in the list and always use the latest result.
 */
static void
bt__devresult(struct bt_devinquiry *ii, int *count, int max_count,
    bdaddr_t *ba, uint8_t psrm, uint8_t pspm, uint8_t *cl, uint16_t co,
    int8_t rssi, uint8_t *data)
{
	int	n;

	for (n = 0; ; n++, ii++) {
		if (n == *count) {
			if (*count == max_count)
				return;

			(*count)++;
			break;
		}

		if (bdaddr_same(&ii->bdaddr, ba))
			break;
	}

	bdaddr_copy(&ii->bdaddr, ba);
	ii->pscan_rep_mode = psrm;
	ii->pscan_period_mode = pspm;
	ii->clock_offset = le16toh(co);
	ii->rssi = rssi;

	if (cl != NULL)
		memcpy(ii->dev_class, cl, HCI_CLASS_SIZE);

	if (data != NULL)
		memcpy(ii->data, data, 240);
}

int
bt_devinquiry(const char *name, time_t to, int max_rsp,
    struct bt_devinquiry **iip)
{
	uint8_t			buf[HCI_EVENT_PKT_SIZE], *p;
	struct bt_devfilter	f;
	hci_event_hdr_t		ev;
	hci_command_status_ep	sp;
	hci_inquiry_cp		cp;
	hci_inquiry_result_ep	ip;
	hci_inquiry_response	ir;
	hci_rssi_result_ep	rp;
	hci_rssi_response	rr;
	hci_extended_result_ep	ep;
	struct bt_devinquiry	*ii;
	int			count, i, s;
	time_t			t_end;
	ssize_t			n;

	if (iip == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (name == NULL) {
		if (bt_devenum(bt__devany_cb, buf) == -1)
			return -1;

		name = (const char *)buf;
	}

	s = bt_devopen(name, 0);
	if (s == -1)
		return -1;

	memset(&f, 0, sizeof(f));
	bt_devfilter_pkt_set(&f, HCI_EVENT_PKT);
	bt_devfilter_evt_set(&f, HCI_EVENT_COMMAND_STATUS);
	bt_devfilter_evt_set(&f, HCI_EVENT_INQUIRY_COMPL);
	bt_devfilter_evt_set(&f, HCI_EVENT_INQUIRY_RESULT);
	bt_devfilter_evt_set(&f, HCI_EVENT_RSSI_RESULT);
	bt_devfilter_evt_set(&f, HCI_EVENT_EXTENDED_RESULT);
	if (bt_devfilter(s, &f, NULL) == -1) {
		close(s);
		return -1;
	}

	/*
	 * silently adjust number of reponses to fit in uint8_t
	 */
	if (max_rsp < 1)
		max_rsp = 8;
	else if (max_rsp > UINT8_MAX)
		max_rsp = UINT8_MAX;

	ii = calloc((size_t)max_rsp, sizeof(struct bt_devinquiry));
	if (ii == NULL) {
		close(s);
		return -1;
	}

	/*
	 * silently adjust timeout value so that inquiry_length
	 * falls into the range 0x01->0x30 (unit is 1.28 seconds)
	 */
	if (to < 1)
		to = 5;
	else if (to == 1)
		to = 2;
	else if (to > 62)
		to = 62;

	/* General Inquiry LAP is 0x9e8b33 */
	cp.lap[0] = 0x33;
	cp.lap[1] = 0x8b;
	cp.lap[2] = 0x9e;
	cp.inquiry_length = (uint8_t)(to * 100 / 128);
	cp.num_responses = (uint8_t)max_rsp;

	if (bt_devsend(s, HCI_CMD_INQUIRY, &cp, sizeof(cp)) == -1)
		goto fail;

	count = 0;

	for (t_end = time(NULL) + to + 1; to > 0; to = t_end - time(NULL)) {
		p = buf;
		n = bt_devrecv(s, buf, sizeof(buf), to);
		if (n == -1)
			goto fail;

		if (sizeof(ev) > (size_t)n) {
			errno = EIO;
			goto fail;
		}

		memcpy(&ev, p, sizeof(ev));
		p += sizeof(ev);
		n -= sizeof(ev);

		switch (ev.event) {
		case HCI_EVENT_COMMAND_STATUS:
			if (sizeof(sp) > (size_t)n)
				break;

			memcpy(&sp, p, sizeof(sp));

			if (le16toh(sp.opcode) != HCI_CMD_INQUIRY
			    || sp.status == 0)
				break;

			errno = EIO;
			goto fail;

		case HCI_EVENT_INQUIRY_COMPL:
			close(s);
			*iip = ii;
			return count;

		case HCI_EVENT_INQUIRY_RESULT:
			if (sizeof(ip) > (size_t)n)
				break;

			memcpy(&ip, p, sizeof(ip));
			p += sizeof(ip);
			n -= sizeof(ip);

			if (sizeof(ir) * ip.num_responses != (size_t)n)
				break;

			for (i = 0; i < ip.num_responses; i++) {
				memcpy(&ir, p, sizeof(ir));
				p += sizeof(ir);

				bt__devresult(ii, &count, max_rsp,
					&ir.bdaddr,
					ir.page_scan_rep_mode,
					ir.page_scan_period_mode,
					ir.uclass,
					ir.clock_offset,
					0,		/* rssi */
					NULL);		/* extended data */
			}

			break;

		case HCI_EVENT_RSSI_RESULT:
			if (sizeof(rp) > (size_t)n)
				break;

			memcpy(&rp, p, sizeof(rp));
			p += sizeof(rp);
			n -= sizeof(rp);

			if (sizeof(rr) * rp.num_responses != (size_t)n)
				break;

			for (i = 0; i < rp.num_responses; i++) {
				memcpy(&rr, p, sizeof(rr));
				p += sizeof(rr);

				bt__devresult(ii, &count, max_rsp,
					&rr.bdaddr,
					rr.page_scan_rep_mode,
					0,	/* page scan period mode */
					rr.uclass,
					rr.clock_offset,
					rr.rssi,
					NULL);		/* extended data */
			}

			break;

		case HCI_EVENT_EXTENDED_RESULT:
			if (sizeof(ep) != (size_t)n)
				break;

			memcpy(&ep, p, sizeof(ep));

			if (ep.num_responses != 1)
				break;

			bt__devresult(ii, &count, max_rsp,
				&ep.bdaddr,
				ep.page_scan_rep_mode,
				0,	/* page scan period mode */
				ep.uclass,
				ep.clock_offset,
				ep.rssi,
				ep.response);

			break;

		default:
			break;
		}
	}

	errno = ETIMEDOUT;

fail:
	free(ii);
	close(s);
	return -1;
}

/*
 * Internal version of bt_devinfo. Fill in the devinfo structure
 * with the socket handle provided. If the device is present and
 * active, the socket will be left connected to the device.
 */
static int
bt__devinfo(int s, const char *name, struct bt_devinfo *info)
{
	struct sockaddr_bt		sa;
	struct bt_devreq		req;
	struct btreq			btr;
	hci_read_buffer_size_rp		bp;
	hci_read_local_features_rp	fp;

	memset(&btr, 0, sizeof(btr));
	strncpy(btr.btr_name, name, HCI_DEVNAME_SIZE);

	if (ioctl(s, SIOCGBTINFO, &btr) == -1)
		return -1;

	memset(info, 0, sizeof(struct bt_devinfo));
	memcpy(info->devname, btr.btr_name, HCI_DEVNAME_SIZE);
	bdaddr_copy(&info->bdaddr, &btr.btr_bdaddr);
	info->enabled = ((btr.btr_flags & BTF_UP) ? 1 : 0);

	info->sco_size = btr.btr_sco_mtu;
	info->acl_size = btr.btr_acl_mtu;
	info->cmd_free = btr.btr_num_cmd;
	info->sco_free = btr.btr_num_sco;
	info->acl_free = btr.btr_num_acl;

	info->link_policy_info = btr.btr_link_policy;
	info->packet_type_info = btr.btr_packet_type;

	if (ioctl(s, SIOCGBTSTATS, &btr) == -1)
		return -1;

	info->cmd_sent = btr.btr_stats.cmd_tx;
	info->evnt_recv = btr.btr_stats.evt_rx;
	info->acl_recv = btr.btr_stats.acl_rx;
	info->acl_sent = btr.btr_stats.acl_tx;
	info->sco_recv = btr.btr_stats.sco_rx;
	info->sco_sent = btr.btr_stats.sco_tx;
	info->bytes_recv = btr.btr_stats.byte_rx;
	info->bytes_sent = btr.btr_stats.byte_tx;

	/* can only get the rest from enabled devices */
	if ((info->enabled) == 0)
		return 0;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, &info->bdaddr);

	if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) == -1
	    || connect(s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		return -1;

	memset(&req, 0, sizeof(req));
	req.opcode = HCI_CMD_READ_BUFFER_SIZE;
	req.rparam = &bp;
	req.rlen = sizeof(bp);

	if (bt_devreq(s, &req, 5) == -1)
		return -1;

	info->acl_pkts = bp.max_acl_size;
	info->sco_pkts = bp.max_sco_size;

	memset(&req, 0, sizeof(req));
	req.opcode = HCI_CMD_READ_LOCAL_FEATURES;
	req.rparam = &fp;
	req.rlen = sizeof(fp);

	if (bt_devreq(s, &req, 5) == -1)
		return -1;

	memcpy(info->features, fp.features, HCI_FEATURES_SIZE);

	return 0;
}

int
bt_devinfo(const char *name, struct bt_devinfo *info)
{
	int	rv, s;

	if (name == NULL || info == NULL) {
		errno = EINVAL;
		return -1;
	}

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return -1;

	rv = bt__devinfo(s, name, info);
	close(s);
	return rv;
}

int
bt_devenum(bt_devenum_cb_t cb, void *arg)
{
	struct btreq		btr;
	struct bt_devinfo	info;
	int			count, fd, rv, s;

	s = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (s == -1)
		return -1;

	memset(&btr, 0, sizeof(btr));
	count = 0;

	while (ioctl(s, SIOCNBTINFO, &btr) != -1) {
		count++;

		if (cb == NULL)
			continue;

		fd = socket(PF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
		if (fd == -1) {
			close(s);
			return -1;
		}

		if (bt__devinfo(fd, btr.btr_name, &info) == -1) {
			close(fd);
			close(s);
			return -1;
		}

		rv = (*cb)(fd, &info, arg);
		close(fd);
		if (rv != 0)
			break;
	}

	close(s);
	return count;
}
