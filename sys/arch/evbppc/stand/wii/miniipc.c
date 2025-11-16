/* $NetBSD: miniipc.c,v 1.1 2025/11/16 20:11:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <lib/libsa/stand.h>

#include <machine/pio.h>

#include "miniipc.h"
#include "sdmmc.h"
#include "cache.h"
#include "timer.h"

#define MINIIPC_INFO_PTR		0x13fffffc

#define HW_BASE				0x0d800000
#define	HW_IPC_PPCMSG			(HW_BASE + 0x00)
#define  HW_IPC_PPCMSG_INDEX_OUT_MASK	__BITS(31, 16)
#define  HW_IPC_PPCMSG_INDEX_IN_MASK	__BITS(15, 0)
#define HW_IPC_PPCCTRL			(HW_BASE + 0x04)
#define  HW_IPC_PPCCTRL_IN		__BIT(2)
#define  HW_IPC_PPCCTRL_OUT		__BIT(0)
#define HW_IPC_ARMMSG			(HW_BASE + 0x08)
#define  HW_IPC_ARMMSG_INDEX_OUT_MASK	__BITS(15, 0)

/* IPC request flags */
#define IPC_SLOW			0x00
#define IPC_FAST			0x01

/* IPC devices and requests */
#define IPC_DEV_SYS			0x00
#define  IPC_SYS_PING			0x0000
#define IPC_DEV_SDMMC			0x07
#define  IPC_SDMMC_ACK			0x0000
#define  IPC_SDMMC_READ			0x0001
#define  IPC_SDMMC_WRITE		0x0002
#define  IPC_SDMMC_STATE		0x0003
#define  IPC_SDMMC_SIZE			0x0004

static volatile ipc_info_header_t *infohdr;
static uint32_t index_in;
static uint32_t index_out;
static uint32_t request_tag;

static uint32_t
miniipc_next_index_in(void)
{
	return (index_in + 1) % infohdr->ipc_in_size;
}

static uint32_t
miniipc_next_index_out(void)
{
	return (index_out + 1) % infohdr->ipc_out_size;
}

bool
miniipc_probe(void)
{
	paddr_t pa;

	pa = in32(MINIIPC_INFO_PTR);
	if (pa == 0 || pa == 0xffffffff) {
		printf("No MINI IPC pointer at 0x%x\n", MINIIPC_INFO_PTR);
		return false;
	}
	infohdr = (ipc_info_header_t *)pa;
	cache_dcbi((void *)infohdr, sizeof(*infohdr));

	if (memcmp((void *)infohdr->magic, "IPC", 3) != 0) {
		return false;
	}

	index_in = __SHIFTOUT(in32(HW_IPC_PPCMSG), HW_IPC_PPCMSG_INDEX_IN_MASK);
	index_out = __SHIFTOUT(in32(HW_IPC_PPCMSG), HW_IPC_PPCMSG_INDEX_OUT_MASK);

	miniipc_ping();

	return true;
}

static int
miniipc_sendrecv(const ipc_request_t *req_in, ipc_request_t *req_out)
{
	volatile ipc_request_t *req;
	int retry, error = 0;

	req = &infohdr->ipc_in[index_in];

	*req = *req_in;
	req->tag = ++request_tag;

#ifdef MINIIPC_DEBUG
	printf("IPC[%u] -> req code 0x%x tag %u args %u %u %u %u %u %u\n",
	    index_in, req->code, req->tag,
	    req->args[0], req->args[1], req->args[2], req->args[3],
	    req->args[4], req->args[5]);
#endif

	cache_dcbf((void *)req, sizeof(*req));

	req = &infohdr->ipc_out[index_out];
	cache_dcbf((void *)req, sizeof(*req));

	index_in = miniipc_next_index_in();

	out32(HW_IPC_PPCMSG,
	    (in32(HW_IPC_PPCMSG) & ~HW_IPC_PPCMSG_INDEX_IN_MASK) |
	     __SHIFTIN(index_in, HW_IPC_PPCMSG_INDEX_IN_MASK));
	out32(HW_IPC_PPCCTRL, in32(HW_IPC_PPCCTRL) | HW_IPC_PPCCTRL_OUT);

	retry = 10000000;
	while (__SHIFTOUT(in32(HW_IPC_ARMMSG), HW_IPC_ARMMSG_INDEX_OUT_MASK) ==
	       index_out) {
		timer_udelay(1);
		if (--retry == 0) {
#ifdef MINIIPC_DEBUG
			printf("HW_IPC_PPCMSG: 0x%x\n", in32(HW_IPC_PPCMSG));
			printf("HW_IPC_ARMMSG: 0x%x\n", in32(HW_IPC_ARMMSG));
#endif
			error = ETIMEDOUT;
			break;
		}
	}

	req = &infohdr->ipc_out[index_out];
	cache_dcbi((void *)req, sizeof(*req));

#ifdef MINIIPC_DEBUG
	printf("IPC[%u] <- req code 0x%x tag %u args %u %u %u %u %u %u\n",
	    index_out, req->code, req->tag,
	    req->args[0], req->args[1], req->args[2], req->args[3],
	    req->args[4], req->args[5]);
#endif

	*req_out = *req;

#ifdef MINIIPC_DEBUG
	printf(" DUMP OUTPUT RING\n");
	for (int n = 0; n < infohdr->ipc_out_size; n++) {
		req = &infohdr->ipc_out[n];
		cache_dcbi((void *)req, sizeof(*req));
		printf(" debug IPC[%u] <- req code 0x%x tag %u args %u %u %u %u %u %u\n",
		    n, req->code, req->tag,
		    req->args[0], req->args[1], req->args[2], req->args[3],
		    req->args[4], req->args[5]);
	}
	printf(" END OUTPUT RING\n");
#endif

	index_out = miniipc_next_index_out();
	out32(HW_IPC_PPCMSG,
	    (in32(HW_IPC_PPCMSG) & ~HW_IPC_PPCMSG_INDEX_OUT_MASK) |
	     __SHIFTIN(index_out, HW_IPC_PPCMSG_INDEX_OUT_MASK));

	return error;
}

int
miniipc_ping(void)
{
	ipc_request_t req = {
		.flags = IPC_FAST,
		.device = IPC_DEV_SYS,
		.req = IPC_SYS_PING,
	};

	return miniipc_sendrecv(&req, &req);
}

int
miniipc_sdmmc_ack(uint32_t *ack)
{
	ipc_request_t req = {
		.flags = IPC_SLOW,
		.device = IPC_DEV_SDMMC,
		.req = IPC_SDMMC_ACK,
	};
	int error;

	error = miniipc_sendrecv(&req, &req);
	if (error != 0) {
		*ack = 0;
		return error;
	}

	*ack = req.args[0];
	return 0;
}

int
miniipc_sdmmc_state(uint32_t *state)
{
	ipc_request_t req = {
		.flags = IPC_SLOW,
		.device = IPC_DEV_SDMMC,
		.req = IPC_SDMMC_STATE,
	};
	int error;

	error = miniipc_sendrecv(&req, &req);
	if (error != 0) {
		*state = 0;
		return error;
	}

	*state = req.args[0];
	return 0;
}

int
miniipc_sdmmc_size(uint32_t *size)
{
	ipc_request_t req = {
		.flags = IPC_SLOW,
		.device = IPC_DEV_SDMMC,
		.req = IPC_SDMMC_SIZE,
	};
	int error;

	error = miniipc_sendrecv(&req, &req);
	if (error != 0) {
		*size = 0;
		return error;
	}

	*size = req.args[0];
	return 0;
}

int
miniipc_sdmmc_read(uint32_t start_blkno, uint32_t nblks, void *buf)
{
	ipc_request_t req = {
		.flags = IPC_SLOW,
		.device = IPC_DEV_SDMMC,
		.req = IPC_SDMMC_READ,
		.args = { start_blkno, nblks, (uint32_t)buf },
	};
	int error;

	if (((uint32_t)buf & (CACHE_LINE_SIZE - 1)) != 0) {
		cache_dcbf(buf, nblks * SDMMC_BLOCK_SIZE);
	}
	error = miniipc_sendrecv(&req, &req);
	if (error != 0) {
		return error;
	} else if (req.args[0] != 0) {
		printf("read block %u (nblks = %u) failed: %d\n",
		    start_blkno, nblks, (int)req.args[0]);
		error = EIO;
		return error;
	}
	cache_dcbi(buf, nblks * SDMMC_BLOCK_SIZE);

	return 0;
}
