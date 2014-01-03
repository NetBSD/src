#include <sys/cdefs.h>
 __RCSID("$NetBSD: bpf.c,v 1.1.1.6 2014/01/03 22:10:42 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <net/bpf.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "common.h"
#include "dhcp.h"
#include "ipv4.h"
#include "bpf-filter.h"

int
ipv4_opensocket(struct interface *ifp, int protocol)
{
	struct dhcp_state *state;
	int fd = -1;
	struct ifreq ifr;
	int buf_len = 0;
	struct bpf_version pv;
	struct bpf_program pf;
#ifdef BIOCIMMEDIATE
	int flags;
#endif
#ifdef _PATH_BPF
	fd = open(_PATH_BPF, O_RDWR | O_NONBLOCK);
#else
	char *device;
	int n = 0;

	device = malloc(sizeof(char) * PATH_MAX);
	if (device == NULL)
		return -1;
	do {
		snprintf(device, PATH_MAX, "/dev/bpf%d", n++);
		fd = open(device, O_RDWR | O_NONBLOCK);
	} while (fd == -1 && errno == EBUSY);
	free(device);
#endif

	if (fd == -1)
		return -1;

	state = D_STATE(ifp);

	if (ioctl(fd, BIOCVERSION, &pv) == -1)
		goto eexit;
	if (pv.bv_major != BPF_MAJOR_VERSION ||
	    pv.bv_minor < BPF_MINOR_VERSION) {
		syslog(LOG_ERR, "BPF version mismatch - recompile");
		goto eexit;
	}

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(fd, BIOCSETIF, &ifr) == -1)
		goto eexit;

	/* Get the required BPF buffer length from the kernel. */
	if (ioctl(fd, BIOCGBLEN, &buf_len) == -1)
		goto eexit;
	if (state->buffer_size != (size_t)buf_len) {
		free(state->buffer);
		state->buffer = malloc(buf_len);
		if (state->buffer == NULL)
			goto eexit;
		state->buffer_size = buf_len;
		state->buffer_len = state->buffer_pos = 0;
	}

#ifdef BIOCIMMEDIATE
	flags = 1;
	if (ioctl(fd, BIOCIMMEDIATE, &flags) == -1)
		goto eexit;
#endif

	/* Install the DHCP filter */
	if (protocol == ETHERTYPE_ARP) {
		pf.bf_insns = UNCONST(arp_bpf_filter);
		pf.bf_len = arp_bpf_filter_len;
	} else {
		pf.bf_insns = UNCONST(dhcp_bpf_filter);
		pf.bf_len = dhcp_bpf_filter_len;
	}
	if (ioctl(fd, BIOCSETF, &pf) == -1)
		goto eexit;
	if (set_cloexec(fd) == -1)
		goto eexit;
	return fd;

eexit:
	free(state->buffer);
	state->buffer = NULL;
	close(fd);
	return -1;
}

ssize_t
ipv4_sendrawpacket(const struct interface *ifp, int protocol,
    const void *data, ssize_t len)
{
	struct iovec iov[2];
	struct ether_header hw;
	int fd;
	const struct dhcp_state *state;

	memset(&hw, 0, ETHER_HDR_LEN);
	memset(&hw.ether_dhost, 0xff, ETHER_ADDR_LEN);
	hw.ether_type = htons(protocol);
	iov[0].iov_base = &hw;
	iov[0].iov_len = ETHER_HDR_LEN;
	iov[1].iov_base = UNCONST(data);
	iov[1].iov_len = len;
	state = D_CSTATE(ifp);
	if (protocol == ETHERTYPE_ARP)
		fd = state->arp_fd;
	else
		fd = state->raw_fd;
	return writev(fd, iov, 2);
}

/* BPF requires that we read the entire buffer.
 * So we pass the buffer in the API so we can loop on >1 packet. */
ssize_t
ipv4_getrawpacket(struct interface *ifp, int protocol,
    void *data, ssize_t len, int *partialcsum)
{
	int fd = -1;
	struct bpf_hdr packet;
	ssize_t bytes;
	const unsigned char *payload;
	struct dhcp_state *state;

	state = D_STATE(ifp);
	if (protocol == ETHERTYPE_ARP)
		fd = state->arp_fd;
	else
		fd = state->raw_fd;

	if (partialcsum != NULL)
		*partialcsum = 0; /* Not supported on BSD */

	for (;;) {
		if (state->buffer_len == 0) {
			bytes = read(fd, state->buffer, state->buffer_size);
			if (bytes == -1)
				return errno == EAGAIN ? 0 : -1;
			else if ((size_t)bytes < sizeof(packet))
				return -1;
			state->buffer_len = bytes;
			state->buffer_pos = 0;
		}
		bytes = -1;
		memcpy(&packet, state->buffer + state->buffer_pos,
		    sizeof(packet));
		if (packet.bh_caplen != packet.bh_datalen)
			goto next; /* Incomplete packet, drop. */
		if (state->buffer_pos + packet.bh_caplen + packet.bh_hdrlen >
		    state->buffer_len)
			goto next; /* Packet beyond buffer, drop. */
		payload = state->buffer + packet.bh_hdrlen + ETHER_HDR_LEN;
		bytes = packet.bh_caplen - ETHER_HDR_LEN;
		if (bytes > len)
			bytes = len;
		memcpy(data, payload, bytes);
next:
		state->buffer_pos += BPF_WORDALIGN(packet.bh_hdrlen +
		    packet.bh_caplen);
		if (state->buffer_pos >= state->buffer_len)
			state->buffer_len = state->buffer_pos = 0;
		if (bytes != -1)
			return bytes;
	}
}
