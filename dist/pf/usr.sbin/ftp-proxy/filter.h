/*	$NetBSD: filter.h,v 1.4 2011/02/10 14:04:30 rmind Exp $ */

/*
 * Copyright (c) 2004, 2005 Camiel Dobbelaar, <cd@sentia.nl>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _FTP_PROXY_FILTER_H_
#define _FTP_PROXY_FILTER_H_

typedef struct {
	void	(*init_filter)(char *, char *, int);
	int	(*add_filter)(uint32_t, uint8_t, struct sockaddr *,
		    struct sockaddr *, uint16_t);
	int	(*add_nat)(uint32_t, struct sockaddr *, struct sockaddr *,
		    uint16_t, struct sockaddr *, u_int16_t, u_int16_t);
	int	(*add_rdr)(uint32_t, struct sockaddr *, struct sockaddr *,
		    uint16_t, struct sockaddr *, uint16_t);
	int	(*server_lookup)(struct sockaddr *, struct sockaddr *,
		    struct sockaddr *);
	int	(*prepare_commit)(u_int32_t);
	int	(*do_commit)(void);
	int	(*do_rollback)(void);
} ftp_proxy_ops_t;

extern const ftp_proxy_ops_t	pf_fprx_ops;

#if defined(__NetBSD__)

extern const char *		netif;

#if defined(WITH_NPF)
extern const ftp_proxy_ops_t	npf_fprx_ops;
extern char *			npfopts;
#endif

#if defined(WITH_IPF)
extern const ftp_proxy_ops_t	ipf_fprx_ops;
#endif

#endif

#endif
