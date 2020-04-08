/*
 * Reimplementation of winpcap pcap-remote.c
 * Copyright (c) 2002 - 2005 NetGroup, Politecnico di Torino (Italy)
 * Copyright (c) 2005 - 2008 CACE Technologies, Davis (California)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Politecnico di Torino, CACE Technologies 
 * nor the names of its contributors may be used to endorse or promote 
 * products derived from this software without specific prior written 
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pcap-int.h"

#ifdef NEED_STRERROR_H
#include "strerror.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <time.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "pcap-rpcap-unix.h"

#define RPCAP_IFACE "rpcap://"

/* default */
#define RPCAP_DEFAULT_NETPORT 2002

/* version */
#define RPCAP_VERSION_EXPERIMENTAL 0

/* packets */
#define RPCAP_MSG_ERROR 1				/*!< Message that keeps an error notification */
#define RPCAP_MSG_OPEN_REQ 3			/*!< Request to open a remote device */
#define RPCAP_MSG_STARTCAP_REQ 4		/*!< Request to start a capture on a remote device */
#define RPCAP_MSG_UPDATEFILTER_REQ 5	/*!< Send a compiled filter into the remote device */
#define RPCAP_MSG_PACKET 7				/*!< This is a 'data' message, which carries a network packet */
#define RPCAP_MSG_AUTH_REQ 8			/*!< Message that keeps the authentication parameters */
#define RPCAP_MSG_STATS_REQ 9			/*!< It requires to have network statistics */

#define RPCAP_UPDATEFILTER_BPF 1			/*!< This code tells us that the filter is encoded with the BPF/NPF syntax */

static unsigned short
get16(const unsigned char *buf)
{
	unsigned short val;

	val = buf[0];
	val = val << 8 | buf[1];
	return val;
}

static void
put16(unsigned char *buf, unsigned int val)
{
	buf[0] = val >> 8;
	buf[1] = val >> 0;
}

static unsigned int
get32(const unsigned char *buf)
{
	unsigned int val;

	val = buf[0];
	val = val << 8 | buf[1];
	val = val << 8 | buf[2];
	val = val << 8 | buf[3];
	return val;
}

static void
put32(unsigned char *buf, unsigned int val)
{
	buf[0] = val >> 24;
	buf[1] = val >> 16;
	buf[2] = val >> 8;
	buf[3] = val >> 0;
}

static int
rpcap_recv_pkt(pcap_t *p, int fd, unsigned char *recv_buf, unsigned int buflen)
{
	static unsigned char discard[1024];

	size_t mlen;
	int ret;
	unsigned char *buf;
	unsigned int len;
	unsigned int pkt_len;

	unsigned char hdr[8];
	int pkt_type;

/* read header loop */
	buf = hdr;
	len = 8;

	ret = 0;
	do {
		buf += ret;
		len -= ret;

		do {
			ret = read(fd, buf, len);
			if (p->break_loop) {
				snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "break-loop");
				p->break_loop = 0;
				return -2;
			}
		} while (ret == -1 && errno == EINTR);
	} while (ret > 0 && len-ret);

	if (ret <= 0) {
		if (!ret)
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection closed");
		else
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection error (%s)", strerror(errno));
		return -1;
	}

	if (hdr[0] != RPCAP_VERSION_EXPERIMENTAL) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: incorrect reply version (%.2x)", hdr[0]);
		return -1;
	}

	pkt_type = (unsigned char) hdr[1];
	pkt_len  = get32(&hdr[4]);

	if (pkt_type == RPCAP_MSG_ERROR) {
		recv_buf = (unsigned char *)p->errbuf;
		buflen = PCAP_ERRBUF_SIZE-1;
	}

	buf = recv_buf;

/* read payload loop */
	if (pkt_len) {
		ret = 0;
		len = pkt_len;
		do {
			buf += ret;
			buflen -= ret;
			len -= ret;

			if (!buflen) {
				buf = discard;
				buflen = sizeof(discard);
			}

			mlen = (len < 0x7fff) ? len : 0x7fff;

			if (mlen > buflen)
				mlen = buflen;

			do {
				ret = read(fd, buf, mlen);
				if (p->break_loop) {
					p->break_loop = 0;
					snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "break-loop");
					return -2;
				}
			} while (ret == -1 && errno == EINTR);
		} while (ret > 0 && len-ret);

		if (ret <= 0) {
			if (!ret)
				snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection closed");
			else
				snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection error (%s)", strerror(errno));
			return -1;
		}
		buf += ret;
	}

	/* always NUL terminate errbuf, and signal error */
	if (pkt_type == RPCAP_MSG_ERROR) {
		*buf = '\0';
		return -1;
	}
	return pkt_len;
}

static int
rpcap_send_pkt(pcap_t *p, const unsigned char *send_buf, unsigned int len)
{
	size_t mlen;
	int ret;

/* send loop */
	ret = 0;
	do {
		send_buf += ret;
		len -= ret;

		mlen = (len < 0x7fff) ? len : 0x7fff;

		do {
			ret = write(p->fd, send_buf, mlen);
			if (p->break_loop) {
				p->break_loop = 0;
				snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "break-loop");
				return -2;
			}
		} while (ret == -1 && errno == EINTR);
	} while (ret > 0 && len-ret);

	if (ret <= 0) {
		if (!ret)
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection closed");
		else
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: connection error (%s)", strerror(errno));
		return -1;
	}
	return 0;
}

static int
rpcap_send_request(pcap_t *p, char type, unsigned char *buf, unsigned int payload_len)
{
	buf[0] = RPCAP_VERSION_EXPERIMENTAL;
	buf[1] = type;
	buf[2] = buf[3] = 0;
	put32(&buf[4], payload_len);

	return rpcap_send_pkt(p, buf, 8+payload_len);
}

static int
rpcap_send_request_auth(pcap_t *p, const char *username, const char *password)
{

	if (username || password) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: auth not supported (yet!)");
		return -1;

	} else {
		static const unsigned char login_null_pkt[16] = { 
			RPCAP_VERSION_EXPERIMENTAL,
			RPCAP_MSG_AUTH_REQ, 
			0, 0, 
			0, 0, 0, 8,

			0, 0, 0, 0, 0, 0, 0, 0
		};

		if (rpcap_send_pkt(p, login_null_pkt, sizeof(login_null_pkt)))
			return -1;
	}

	return rpcap_recv_pkt(p, p->fd, NULL, 0);
}

static int
rpcap_send_request_open(pcap_t *p, const char *interface)
{
	const size_t interface_len = strlen(interface);

	unsigned char buf_open[8+255] = { 
		RPCAP_VERSION_EXPERIMENTAL, 
		RPCAP_MSG_OPEN_REQ, 
		0, 0, 
		0, 0, 0, interface_len
	};

	unsigned char reply_buf[8];
	int reply_len;

	if (interface_len > 255) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "rpcap: maximum interface length: 255");
		return -1;
	}

	memcpy(buf_open + 8, interface, interface_len);
	if (rpcap_send_pkt(p, buf_open, 8 + interface_len))
		return -1;

	reply_len = rpcap_recv_pkt(p, p->fd, reply_buf, sizeof(reply_buf));
	if (reply_len != sizeof(reply_buf)) {
		if (reply_len >= 0)
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Bad protocol (openreply: %u)", reply_len);
		return -1;
	}

	p->linktype = get32(&reply_buf[0]);
	p->tzoff    = get32(&reply_buf[4]);

	return 0;
}

static int
rpcap_send_request_start(pcap_t *p, struct in_addr *server_ip)
{
	unsigned char buf_start[8+12+8+8] = { 
		RPCAP_VERSION_EXPERIMENTAL, 
		RPCAP_MSG_STARTCAP_REQ, 
		0, 0, 
		0, 0, 0, 12+8+8,
/* rpcap_startcapreq (12B) */
		0xff, 0xff, 0xff, 0xff, /* snaplen */
		0xff, 0xff, 0xff, 0xff, /* timeout */
		0x00, 0x00, /* flags */
		0x00, 0x00, /* portdata */
/* rpcap_filter (8B+8B) */
		0x00, RPCAP_UPDATEFILTER_BPF, /* filtertype */
		0x00, 0x00,
		0x00, 0x00, 0x00, 0x01,

			0x00, 0x06,				/* ret */
			0x00,
			0x00,
			0x00, 0x00, 0xff, 0xff	/* #65535 */
	};

	struct sockaddr_in sin;
	unsigned char reply_buf[8];
	int reply_len;
	int fd;

	unsigned short portdata;

	put32(&buf_start[8], p->snapshot);      /* snaplen */
	put32(&buf_start[12], p->opt.timeout/2); /* read_timeout */

	if (rpcap_send_pkt(p, buf_start, sizeof(buf_start)))
		return -1;

	reply_len = rpcap_recv_pkt(p, p->fd, reply_buf, sizeof(reply_buf));
	if (reply_len != sizeof(reply_buf)) {
		if (reply_len >= 0)
			snprintf(p->errbuf, PCAP_ERRBUF_SIZE, "Bad protocol (startreply: %u)", reply_len);
		return -1;
	}

	get32(&reply_buf[0]);
	portdata = get16(&reply_buf[4]);

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, 
				"Can't create data socket %d:%s", 
				errno, pcap_strerror(errno));
		return -1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr = *server_ip;
	sin.sin_port = htons(portdata);

	if (connect(fd, (struct sockaddr *) &sin, sizeof(sin))) {
		snprintf(p->errbuf, PCAP_ERRBUF_SIZE, 
				"Can't connect to data socket (%d:%s)", 
				errno, pcap_strerror(errno));
		return -1;
	}
	p->selectable_fd = fd;

	return 0;
}

static int
rpcap_inject_common(pcap_t *handle, const void *buf, size_t size)
{
	snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "inject not supported with remote capture");
	return -1;
}

static int
rpcap_stats_common(pcap_t *handle, struct pcap_stat *stats)
{
	static const unsigned char buf_stats[8] = {
		RPCAP_VERSION_EXPERIMENTAL, 
		RPCAP_MSG_STATS_REQ, 
		0, 0, 
		0, 0, 0, 0
	};

	unsigned char reply_buf[16];
	int reply_len;

/* local */
#if 0
	stats->ps_recv = handle->md.packets_read;
	stats->ps_drop = 0;
	stats->ps_ifdrop = 0;
#endif

/* remote */
	if (rpcap_send_pkt(handle, buf_stats, sizeof(buf_stats)))
		return -1;

	reply_len = rpcap_recv_pkt(handle, handle->fd, reply_buf, sizeof(reply_buf));
	if (reply_len != sizeof(reply_buf)) {
		if (reply_len >= 0)
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "Bad protocol (statsreply: %u)", reply_len);
		return -1;
	}

	stats->ps_recv   = get32(&reply_buf[0]);
	stats->ps_ifdrop = get32(&reply_buf[4]);
	stats->ps_drop   = get32(&reply_buf[8]);
	return 0;
}

static int 
rpcap_setfilter_common(pcap_t *handle, struct bpf_program *prog)
{
	unsigned char *buf_setfilter;

	/* update local filter */
	if (install_bpf_program(handle, prog) == -1)
		return -1;

	/* update remote filter */
	if (prog->bf_len < 0xfffff) {
		unsigned int data_size = 8 + 8 * prog->bf_len;
		unsigned char *buf_filter;
		unsigned char *buf_insn;
		size_t i;

		buf_setfilter = malloc(8 + data_size);
		if (!buf_setfilter) {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "No memory for setfilter packet");
			return -1;
		}

		buf_filter = &buf_setfilter[8];
		buf_insn = &buf_filter[8];

		put16(&buf_filter[0], RPCAP_UPDATEFILTER_BPF);
		put16(&buf_filter[2], 0);
		put32(&buf_filter[4], prog->bf_len);

		for (i = 0; i < prog->bf_len; i++) {
			unsigned char *data = &buf_insn[i * 8];

			put16(&data[0], prog->bf_insns[i].code);
			data[2] = prog->bf_insns[i].jt;
			data[3] = prog->bf_insns[i].jf;
			put32(&data[4], prog->bf_insns[i].k);
		}

		if (rpcap_send_request(handle, RPCAP_MSG_UPDATEFILTER_REQ, buf_setfilter, data_size)) {
			free(buf_setfilter);
			return -1;
		}
		free(buf_setfilter);

		if (rpcap_recv_pkt(handle, handle->fd, NULL, 0) < 0)
			return -1;
	}
	return 0;
}

static int
rpcap_read_unix(pcap_t *handle, int max_packets, pcap_handler callback, u_char *user)
{
	struct pcap_pkthdr pkth;
	const unsigned char *pkt;
	const unsigned char *buf;
	unsigned int pkt_len;
	int count = 0;

	pkt_len = rpcap_recv_pkt(handle, handle->selectable_fd, handle->buffer, handle->bufsize);
	if (pkt_len < 20) {
		if (pkt_len == 0) {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "pkt_len check failed (%u < 20)", pkt_len);
			return -1;
		}
		return pkt_len;
	}
	buf = handle->buffer;

/* local */
#if 0
	gettimeofday(&pkth.ts, NULL);
	pkth.caplen = pkth.len = pkt_len - 20
#endif
/* remote */
	pkth.ts.tv_sec = get32(&buf[0]);
	pkth.ts.tv_usec = get32(&buf[4]);
	pkth.caplen = get32(&buf[8]);
	pkth.len = get32(&buf[12]);
	/* pktnr */
	pkt = &buf[20];

	/* sanity caplen */
	if (pkt_len - 20 < pkth.caplen) {
		/* pkt.caplen = pkt_len - 20; */
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "pkth.caplen check failed (%u < %u)", pkt_len - 20, pkth.caplen);
		return -1;
	}

	if (handle->fcode.bf_insns == NULL ||
			bpf_filter(handle->fcode.bf_insns, pkt, pkth.len, pkth.caplen)) 
	{
//		handle->md.packets_read++;
		callback(user, &pkth, pkt);
		count++;
	}
	return count;
}

static void
rpcap_cleanup(pcap_t *handle)
{
	if (handle->selectable_fd != handle->fd) {
		int fd = handle->selectable_fd;

		if (fd != -1) {
			handle->selectable_fd = -1;
			close(fd);
		}
	}
	pcap_cleanup_live_common(handle);
}

static int
rpcap_activate(pcap_t *handle)
{
	const char *dev = handle->opt.device;
	const char *tmp;

	char *host;
	char *username = NULL;
	char *password = NULL;
	const char *interface = NULL;
	int port;

	struct sockaddr_in sin;

	/* rpcap[s]://login:password@host:port/interface */
	if (strncmp(dev, RPCAP_IFACE, strlen(RPCAP_IFACE)) == 0) {
		port = RPCAP_DEFAULT_NETPORT;
		dev += strlen(RPCAP_IFACE);

	} /* else if (strncmp(dev, "rpcaps://", strlen("rpcaps://")) == 0) {
		port = RPCAP_DEFAULT_NETPORT_SSL;
		dev += strlen("rpcaps://");

	} */ else {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: invalid protocol");
		return PCAP_ERROR;
	}

	if ((tmp = strchr(dev, '@'))) {
		char *ptmp;

		if ((ptmp = strchr(dev, ':'))) {
			username = strndup(dev, ptmp-dev);
			password = strndup(ptmp+1, tmp-(ptmp+1));

		} else {
			username = strndup(dev, tmp-dev);

			/* XXX, ask for password? */
		}

		dev = tmp + 1;
	}

	if (*dev == '[') {
		tmp = strchr(dev, ']');
		if (!tmp) {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: invalid host (missing ']')");
			return PCAP_ERROR;
		}

		host = strndup(dev+1, tmp-(dev+1));

		dev = tmp + 1;
	} else {
		tmp = strchr(dev, ':');
		if (!tmp)
			tmp = strchr(dev, '/');

		if (tmp) {
			host = strndup(dev, tmp-dev);
			dev = tmp;
		} else
			host = strdup(dev);
	}

	if (*dev == ':') {
		char *end;

		if (dev[1] == '\0') {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: missing port");
			return PCAP_ERROR;
		}

		port = strtol(dev + 1, &end, 10);
		if (port < 1 || port > 65535) {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: invalid port");
			return PCAP_ERROR;
		}

		dev = end;
	}

	if (*dev == '/')
		interface = dev+1;

	if (!host) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: couldn't parse host");
		return PCAP_ERROR;
	}

	if (!interface) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: couldn't parse interface");
		return PCAP_ERROR;
	}

	/* XXX, gethostbyname, ipv6, etc... */
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(host);
	sin.sin_port = htons(port);
	if (sin.sin_addr.s_addr == INADDR_NONE) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "rpcap: not ipv4 address");
		goto free_fail;
	}

	/* Initialize some components of the pcap structure. */
	handle->offset = 20;
	handle->bufsize = handle->snapshot + handle->offset;
	handle->linktype = -1;	/* invalid for now */
	handle->read_op = rpcap_read_unix;
	handle->setfilter_op = rpcap_setfilter_common;
	handle->inject_op = rpcap_inject_common;
	handle->setdirection_op = NULL;
	handle->set_datalink_op = NULL;	/* not possible */
	handle->getnonblock_op = pcap_getnonblock_fd;
	handle->setnonblock_op = pcap_setnonblock_fd;
	handle->stats_op = rpcap_stats_common;
	handle->cleanup_op = rpcap_cleanup;

	/* Create socket */
	handle->fd = socket(AF_INET, SOCK_STREAM, 0);
	handle->selectable_fd = -1;
	if (handle->fd < 0) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, 
				"Can't create socket %d:%s", 
				errno, pcap_strerror(errno));
		goto close_fail;
	}

	if (connect(handle->fd, (struct sockaddr *) &sin, sizeof(sin))) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, 
				"Can't connect to %s:%d (%d:%s)", 
				host, port, errno, pcap_strerror(errno));
		goto free_fail;
	}

/* login */
	if (rpcap_send_request_auth(handle, username, password) < 0)
		goto close_fail;
	if (rpcap_send_request_open(handle, interface) < 0)
		goto close_fail;
	if (rpcap_send_request_start(handle, &(sin.sin_addr)) < 0)
		goto close_fail;

	handle->buffer = malloc(handle->bufsize);
	if (!handle->buffer) {
		snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, 
				"Can't allocate dump buffer: %s", 
				pcap_strerror(errno));
		goto close_fail;
	}

	if (handle->opt.rfmon) {
		/*
		 * Monitor mode doesn't apply to rpcap.
		 */
		rpcap_cleanup(handle);
		return PCAP_ERROR_RFMON_NOTSUP;
	}

	if (handle->opt.buffer_size != 0) {
		/*
		 * Set the socket buffer size to the specified value.
		 */
		if (setsockopt(handle->selectable_fd, SOL_SOCKET, SO_RCVBUF, &handle->opt.buffer_size, sizeof(handle->opt.buffer_size)) == -1) {
			snprintf(handle->errbuf, PCAP_ERRBUF_SIZE, "SO_RCVBUF: %s", pcap_strerror(errno));
			goto close_fail;
		}
	}
	free(username);
	free(password);
	free(host);

	return 0;

close_fail:
	rpcap_cleanup(handle);
free_fail:
	free(username);
	free(password);
	free(host);
	return PCAP_ERROR;
}

pcap_t *
rpcap_create(const char *device, char *err_str, int *is_ours)
{
	const char *cp = device;
	pcap_t *p;

	if (strncmp(cp, RPCAP_IFACE, strlen(RPCAP_IFACE)) == 0)
		cp += strlen(RPCAP_IFACE);
	else {
		*is_ours = 0;
		return NULL;
	}

	if (*cp == '\0') {
		*is_ours = 0;
		return NULL;
	}

	*is_ours = 1;

	p = pcap_create_common(__UNCONST(device), 16384);
	if (p == NULL)
		return NULL;

	p->activate_op = rpcap_activate;
	return p;
}

