/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * dhcpcd: BPF arp and bootp filtering
 * Copyright (c) 2006-2019 Roy Marples <roy@marples.name>
 * All rights reserved

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
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef BPF_HEADER
#define BPF_HEADER

#define	BPF_READING		(1U << 0)
#define	BPF_EOF			(1U << 1)
#define	BPF_PARTIALCSUM		(1U << 2)
#define	BPF_BCAST		(1U << 3)

/*
 * Even though we program the BPF filter should we trust it?
 * On Linux at least there is a window between opening the socket,
 * binding the interface and setting the filter where we receive data.
 * This data is NOT checked OR flushed and IS returned when reading.
 * We have no way of flushing it other than reading these packets!
 * But we don't know if they passed the filter or not ..... so we need
 * to validate each and every packet that comes through ourselves as well.
 * Even if Linux does fix this sorry state, who is to say other kernels
 * don't have bugs causing a similar effect?
 *
 * As such, let's strive to keep the filters just for pattern matching
 * to avoid waking dhcpcd up.
 *
 * If you want to be notified of any packet failing the BPF filter,
 * define BPF_DEBUG below.
 */
//#define	BPF_DEBUG

#include "dhcpcd.h"

extern const char *bpf_name;
size_t bpf_frame_header_len(const struct interface *);
int bpf_frame_bcast(const struct interface *, const char *frame);
int bpf_open(struct interface *, int (*)(struct interface *, int));
int bpf_close(struct interface *, int);
int bpf_attach(int, void *, unsigned int);
ssize_t bpf_send(const struct interface *, int, uint16_t, const void *, size_t);
ssize_t bpf_read(struct interface *, int, void *, size_t, unsigned int *);
int bpf_arp(struct interface *, int);
int bpf_bootp(struct interface *, int);
#endif
