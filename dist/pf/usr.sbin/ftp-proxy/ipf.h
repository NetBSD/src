/*	$NetBSD: ipf.h,v 1.1.2.1 2008/05/25 17:51:13 peter Exp $ */
/*	$OpenBSD: filter.h,v 1.4 2007/08/01 09:31:41 henning Exp $ */

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

extern int ipf_enabled;
extern const char *netif;

int ipf_add_filter(u_int32_t, u_int8_t, struct sockaddr *, struct sockaddr *,
    u_int16_t);
int ipf_add_nat(u_int32_t, struct sockaddr *, struct sockaddr *, u_int16_t,
    struct sockaddr *, u_int16_t, u_int16_t);
int ipf_add_rdr(u_int32_t, struct sockaddr *, struct sockaddr *, u_int16_t,
    struct sockaddr *, u_int16_t);
int ipf_do_commit(void);
int ipf_do_rollback(void);
void ipf_init_filter(char *, char *, int);
int ipf_prepare_commit(u_int32_t);
int ipf_server_lookup(struct sockaddr *, struct sockaddr *, struct sockaddr *);
