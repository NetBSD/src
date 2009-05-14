/*	$NetBSD: sdp_session.c,v 1.2 2009/05/14 19:12:45 plunky Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Iain Hibbert.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: sdp_session.c,v 1.2 2009/05/14 19:12:45 plunky Exp $");

#include <sys/socket.h>
#include <sys/un.h>

#include <errno.h>
#include <sdp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sdp-int.h"

/*
 * open session with remote Bluetooth SDP server
 */
struct sdp_session *
_sdp_open(const bdaddr_t *laddr, const bdaddr_t *raddr)
{
	struct sdp_session *	ss;
	struct sockaddr_bt	sa;
	struct linger		li;
	socklen_t		len;

	ss = calloc(1, sizeof(struct sdp_session));
	if (ss == NULL)
		goto fail;

	ss->s = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);
	if (ss->s == -1)
		goto fail;

	memset(&li, 0, sizeof(li));
	li.l_onoff = 1;
	li.l_linger = 5;
	if (setsockopt(ss->s, SOL_SOCKET, SO_LINGER, &li, sizeof(li)) == -1)
		goto fail;

	if (laddr == NULL)
		laddr = BDADDR_ANY;

	memset(&sa, 0, sizeof(sa));
	sa.bt_len = sizeof(sa);
	sa.bt_family = AF_BLUETOOTH;
	bdaddr_copy(&sa.bt_bdaddr, laddr);
	if (bind(ss->s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		goto fail;

	sa.bt_psm = L2CAP_PSM_SDP;
	bdaddr_copy(&sa.bt_bdaddr, raddr);
	if (connect(ss->s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		goto fail;

	len = sizeof(ss->imtu);
	if (getsockopt(ss->s, BTPROTO_L2CAP, SO_L2CAP_IMTU, &ss->imtu, &len) == -1)
		goto fail;

	ss->ibuf = malloc(ss->imtu);
	if (ss->ibuf == NULL)
		goto fail;

	return ss;

fail:
	_sdp_close(ss);
	return NULL;
}

/*
 * open session with local SDP server
 */
struct sdp_session *
_sdp_open_local(const char *control)
{
	struct sdp_session *	ss;
	struct sockaddr_un	sa;

	ss = calloc(1, sizeof(struct sdp_session));
	if (ss == NULL)
		goto fail;

	ss->s = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (ss->s == -1)
		goto fail;

	if (control == NULL)
		control = SDP_LOCAL_PATH;

	memset(&sa, 0, sizeof(sa));
	sa.sun_len = sizeof(sa);
	sa.sun_family = AF_LOCAL;
	strlcpy(sa.sun_path, control, sizeof(sa.sun_path));
	if (connect(ss->s, (struct sockaddr *)&sa, sizeof(sa)) == -1)
		goto fail;

	ss->imtu = L2CAP_MTU_DEFAULT;

	ss->ibuf = malloc(ss->imtu);
	if (ss->ibuf == NULL)
		goto fail;

	return ss;

fail:
	_sdp_close(ss);
	return NULL;
}

/*
 * close session and release all resources
 */
void
_sdp_close(struct sdp_session *ss)
{

	if (ss == NULL)
		return;

	if (ss->s != -1)
		close(ss->s);

	if (ss->ibuf != NULL)
		free(ss->ibuf);

	if (ss->rbuf != NULL)
		free(ss->rbuf);

	free(ss);
}

/*
 * internal function; send a PDU on session
 *
 * caller provides an iovec array with an empty slot at the beginning for
 * PDU header, num is total iovec count.
 */
bool
_sdp_send_pdu(struct sdp_session *ss, uint8_t pid, struct iovec *iov, int num)
{
	sdp_pdu_t	pdu;
	ssize_t		len, nw;
	int		i;

	for (len = 0, i = 1; i < num; i++)
		len += iov[i].iov_len;

	if (len > UINT16_MAX) {
		errno = EMSGSIZE;
		return false;
	}

	ss->tid += 1;

	pdu.pid = pid;
	pdu.tid = htobe16(ss->tid);
	pdu.len = htobe16(len);

	iov[0].iov_base = &pdu;
	iov[0].iov_len = sizeof(pdu);

	do {
		nw = writev(ss->s, iov, num);
	} while (nw == -1 && errno == EINTR);

	if ((size_t)nw != sizeof(pdu) + len) {
		errno = EIO;
		return false;
	}

	return true;
}

/*
 * internal function; receive a PDU on session
 *
 * validate the PDU and transaction IDs and data length, stores
 * received data in the session incoming buffer.
 */
ssize_t
_sdp_recv_pdu(struct sdp_session *ss, uint8_t pid)
{
	struct iovec	iov[2];
	sdp_pdu_t	pdu;
	ssize_t		nr;

	iov[0].iov_base = &pdu;
	iov[0].iov_len = sizeof(pdu);

	iov[1].iov_base = ss->ibuf;
	iov[1].iov_len = ss->imtu;

	do {
		nr = readv(ss->s, iov, __arraycount(iov));
	} while (nr == -1 && errno == EINTR);

	if (nr == -1)
		return -1;

	if ((size_t)nr < sizeof(pdu)) {
		errno = EIO;
		return -1;
	}

	pdu.tid = be16toh(pdu.tid);
	pdu.len = be16toh(pdu.len);

	if (pid != pdu.pid
	    || ss->tid != pdu.tid
	    || (size_t)nr != sizeof(pdu) + pdu.len) {
		if (pdu.pid == SDP_PDU_ERROR_RESPONSE
		    && pdu.len == sizeof(uint16_t))
			errno = _sdp_errno(be16dec(ss->ibuf));
		else
			errno = EIO;

		return -1;
	}

	return pdu.len;
}

/*
 * translate ErrorCode to errno
 */
int
_sdp_errno(uint16_t ec)
{

	switch (ec) {
	case SDP_ERROR_CODE_INVALID_SERVICE_RECORD_HANDLE:
		return ENOATTR;

	case SDP_ERROR_CODE_INVALID_SDP_VERSION:
	case SDP_ERROR_CODE_INVALID_REQUEST_SYNTAX:
	case SDP_ERROR_CODE_INVALID_PDU_SIZE:
	case SDP_ERROR_CODE_INVALID_CONTINUATION_STATE:
	case SDP_ERROR_CODE_INSUFFICIENT_RESOURCES:
	default:
		return EIO;
	}
}
