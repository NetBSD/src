/*
 * BPF BSD interface
 * SPDX-License-Identifier: BSD-2-Clause
 * Copyright (c) 2006-2025 Roy Marples <roy@marples.name>
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

#include <sys/ioctl.h>
#include <sys/socket.h>

#include <net/bpf.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bpf.h"
#ifdef __sun
#include "bpf-dlpi.h"
#endif
#include "logerr.h"

const char *bpf_name = "Berkeley Packet Filter";

struct bpf *
bpf_open(const struct interface *ifp,
    int (*filter)(const struct bpf *, const struct in_addr *),
    const struct in_addr *ia)
{
	struct bpf *bpf;
	struct bpf_version pv = { .bv_major = 0, .bv_minor = 0 };
	struct ifreq ifr = { .ifr_flags = 0 };
	int ibuf_len = 0;
#ifdef O_CLOEXEC
#define BPF_OPEN_FLAGS O_RDWR | O_NONBLOCK | O_CLOEXEC
#else
#define BPF_OPEN_FLAGS O_RDWR | O_NONBLOCK
#endif
#ifdef BIOCIMMEDIATE
	unsigned int flags;
#endif
#ifndef O_CLOEXEC
	int fd_opts;
#endif

	bpf = calloc(1, sizeof(*bpf));
	if (bpf == NULL)
		return NULL;

	/* /dev/bpf is a cloner on modern kernels */
	bpf->bpf_fd = open("/dev/bpf", BPF_OPEN_FLAGS);

	/* Support older kernels where /dev/bpf is not a cloner */
	if (bpf->bpf_fd == -1) {
		char device[32];
		int n = 0;

		do {
			snprintf(device, sizeof(device), "/dev/bpf%d", n++);
			bpf->bpf_fd = open(device, BPF_OPEN_FLAGS);
		} while (bpf->bpf_fd == -1 && errno == EBUSY);
	}

	if (bpf->bpf_fd == -1)
		goto eexit;

	bpf->bpf_ifp = ifp;
	bpf->bpf_flags = BPF_EOF;

#ifndef O_CLOEXEC
	if ((fd_opts = fcntl(bpf->bpf_fd, F_GETFD)) == -1 ||
	    fcntl(bpf->bpf_fd, F_SETFD, fd_opts | FD_CLOEXEC) == -1)
		goto eexit;
#endif

	if (ioctl(bpf->bpf_fd, BIOCVERSION, &pv) == -1)
		goto eexit;
	if (pv.bv_major != BPF_MAJOR_VERSION ||
	    pv.bv_minor < BPF_MINOR_VERSION) {
		logerrx("BPF version mismatch - recompile");
		goto eexit;
	}

	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(bpf->bpf_fd, BIOCSETIF, &ifr) == -1)
		goto eexit;

#ifdef BIOCIMMEDIATE
	flags = 1;
	if (ioctl(bpf->bpf_fd, BIOCIMMEDIATE, &flags) == -1)
		goto eexit;
#endif

	if (filter(bpf, ia) != 0)
		goto eexit;

	/* Get the required BPF buffer length from the kernel. */
	if (ioctl(bpf->bpf_fd, BIOCGBLEN, &ibuf_len) == -1)
		goto eexit;

	bpf->bpf_size = (size_t)ibuf_len;
	bpf->bpf_buffer = malloc(bpf->bpf_size);
	if (bpf->bpf_buffer == NULL)
		goto eexit;

#ifdef __sun
	if (bpf_dlpi_open(bpf) == -1)
		goto eexit;
#endif

	return bpf;

eexit:
	bpf_close(bpf);
	return NULL;
}

/* BPF requires that we read the entire buffer.
 * So we pass the buffer in the API so we can loop on >1 packet. */
ssize_t
bpf_read(struct bpf *bpf, void *data, size_t len)
{
	ssize_t bytes;
	struct bpf_hdr packet;
	size_t hdr_max;
	const uint8_t *payload;

	bpf->bpf_flags &= ~BPF_EOF;
	if (bpf->bpf_len == 0) {
		bytes = read(bpf->bpf_fd, bpf->bpf_buffer, bpf->bpf_size);
#ifdef __sun
		/* After 2^31 bytes, the kernel offset overflows.
		 * To work around this bug, lseek 0. */
		if (bytes == -1 && errno == EINVAL) {
			lseek(bpf->bpf_fd, 0, SEEK_SET);
			return 0;
		}
#endif
		if (bytes == -1 || bytes == 0)
			return bytes;
		bpf->bpf_len = (size_t)bytes;
		bpf->bpf_pos = 0;
	}

	if (bpf->bpf_pos + sizeof(packet) > bpf->bpf_len) {
		errno = EINVAL;
		goto err;
	}

	payload = (const uint8_t *)bpf->bpf_buffer + bpf->bpf_pos;
	memcpy(&packet, payload, sizeof(packet));

	hdr_max = SIZE_MAX - packet.bh_caplen;
	if (packet.bh_hdrlen > hdr_max) {
		errno = EOVERFLOW;
		goto err;
	}
	if (packet.bh_hdrlen + packet.bh_caplen > bpf->bpf_len - bpf->bpf_pos) {
		errno = EBADMSG;
		goto err;
	}

	payload += packet.bh_hdrlen;
	if (packet.bh_caplen > len)
		bytes = (ssize_t)len;
	else
		bytes = (ssize_t)packet.bh_caplen;

	if (bpf_frame_bcast(bpf->bpf_ifp, payload) == 0)
		bpf->bpf_flags |= BPF_BCAST;
	else
		bpf->bpf_flags &= ~BPF_BCAST;
	memcpy(data, payload, (size_t)bytes);

	bpf->bpf_pos += BPF_WORDALIGN(packet.bh_hdrlen + packet.bh_caplen);
	if (bpf->bpf_pos >= bpf->bpf_len) {
		bpf->bpf_len = bpf->bpf_pos = 0;
		bpf->bpf_flags |= BPF_EOF;
	}
	return bytes;

err:
	bpf->bpf_len = bpf->bpf_pos = 0;
	bpf->bpf_flags |= BPF_EOF;
	return -1;
}

int
bpf_setfilter(const struct bpf *bpf, void *filter, unsigned int filter_len)
{
	struct bpf_program pf = { .bf_insns = filter, .bf_len = filter_len };

	/* Install the filter. */
	return ioctl(bpf->bpf_fd, BIOCSETF, &pf);
}

int
bpf_setwfilter(const struct bpf *bpf, void *filter, unsigned int filter_len)
{
#ifdef BIOCSETWF
	struct bpf_program pf = { .bf_insns = filter, .bf_len = filter_len };

	/* Install the filter. */
	return ioctl(bpf->bpf_fd, BIOCSETWF, &pf);
#else
#warning No BIOCSETWF support - a compromised BPF can be used as a raw socket
	UNUSED(bpf);
	UNUSED(filter);
	UNUSED(filter_len);
	errno = ENOSYS;
	return -1;
#endif
}

int
bpf_lockfilter(const struct bpf *bpf)
{
#ifdef BIOCLOCK
	return ioctl(bpf->bpf_fd, BIOCLOCK);
#else
	UNUSED(bpf);
	errno = ENOSYS;
	return -1;
#endif
}

#if !defined(__sun)
/* SunOS is special too - sending via BPF goes nowhere. */
ssize_t
bpf_writev(const struct bpf *bpf, struct iovec *iov, int iovcnt)
{
	return writev(bpf->bpf_fd, iov, iovcnt);
}
#endif

void
bpf_close(struct bpf *bpf)
{
#ifdef __sun
	bpf_dlpi_close(bpf);
#endif
	if (bpf->bpf_fd != -1)
		close(bpf->bpf_fd);
	free(bpf->bpf_buffer);
	free(bpf);
}
