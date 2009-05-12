/*	$NetBSD: sdp-int.h,v 1.1 2009/05/12 10:05:06 plunky Exp $	*/

/*
 * Copyright (c) 2001-2003 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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

#ifndef _SDP_INT_H_
#define _SDP_INT_H_

struct sdp_session {
	uint16_t	 tid;   /* current session transaction ID */
	uint16_t	 imtu;  /* incoming MTU */
	uint8_t		*ibuf;	/* incoming buffer */
	uint8_t		*rbuf;	/* response buffer */
	uint8_t		 cs[17];/* continuation state */
	int32_t		 s;     /* L2CAP socket */
};

/* sdp_session.c */
sdp_session_t	_sdp_open(const bdaddr_t *, const bdaddr_t *);
sdp_session_t	_sdp_open_local(const char *);
void		_sdp_close(sdp_session_t);
bool		_sdp_send_pdu(struct sdp_session *, uint8_t, struct iovec *, int);
ssize_t		_sdp_recv_pdu(struct sdp_session *, uint8_t);
int		_sdp_errno(uint16_t);

/* sdp_data.c */
bool		_sdp_data_print(const uint8_t *, const uint8_t *, int);

/* sdp_service.c */
bool		sdp_service_search_attribute(sdp_session_t, const sdp_data_t *,
		    const sdp_data_t *, sdp_data_t *);

#endif /* _SDP_INT_H_ */
