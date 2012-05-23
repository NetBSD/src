/*	$NetBSD: roaming.h,v 1.3.6.1 2012/05/23 10:07:05 yamt Exp $	*/
/* $OpenBSD: roaming.h,v 1.6 2011/12/07 05:44:38 djm Exp $ */
/*
 * Copyright (c) 2004-2009 AppGate Network Security AB
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

#ifndef ROAMING_H
#define ROAMING_H

#define DEFAULT_ROAMBUF	65536
#define MAX_ROAMBUF	(2*1024*1024) /* XXX arbitrary */
#define ROAMING_REQUEST	"roaming@appgate.com"

extern int roaming_enabled;
extern int resume_in_progress;

void	request_roaming(void);
int	get_snd_buf_size(void);
int	get_recv_buf_size(void);
void	add_recv_bytes(u_int64_t);
int	wait_for_roaming_reconnect(void);
void	roaming_reply(int, u_int32_t, void *);
void	set_out_buffer_size(size_t);
ssize_t	roaming_write(int, const void *, size_t, int *);
ssize_t	roaming_read(int, void *, size_t, int *);
size_t	roaming_atomicio(ssize_t (*)(int, void *, size_t), int, void *, size_t);
u_int64_t	get_recv_bytes(void);
u_int64_t	get_sent_bytes(void);
void	roam_set_bytes(u_int64_t, u_int64_t);
void	resend_bytes(int, u_int64_t *);
void	calculate_new_key(u_int64_t *, u_int64_t, u_int64_t);
int	resume_kex(void);

#endif /* ROAMING */
