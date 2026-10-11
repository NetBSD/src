/*	$NetBSD: isp1763ptd.c,v 1.1 2026/10/11 16:09:06 rkujawa Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/*
 * ISP1763A transfer descriptors.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isp1763ptd.c,v 1.1 2026/10/11 16:09:06 rkujawa Exp $");

#include <sys/param.h>
#include <sys/endian.h>

#include <dev/ic/isp1763var.h>

#define	ISP1763_CERR_MAX	3
#define	ISP1763_USA_EVERY	0xff
#define	ISP1763_USA_EVERY2	0x55
#define	ISP1763_USA_EVERY4	0x11
#define	ISP1763_USA_ONCE	0x01
#define	ISP1763_HS_IVAL_MS	4
#define	ISP1763_IVAL_MAX	9

void
isp1763hc_ptd_build(const struct isp1763hc_ptdreq *pr, uint32_t *dw)
{

	dw[0] = ISP1763_PTD_DW0_V |
	    __SHIFTIN(pr->pr_len, ISP1763_PTD_DW0_NBYTES) |
	    __SHIFTIN(pr->pr_mps, ISP1763_PTD_DW0_MPS) |
	    __SHIFTIN(pr->pr_mult, ISP1763_PTD_DW0_MULT) |
	    __SHIFTIN(pr->pr_ep & 1, ISP1763_PTD_DW0_EP0);

	dw[1] = __SHIFTIN(pr->pr_ep >> 1, ISP1763_PTD_DW1_EP31) |
	    __SHIFTIN(pr->pr_addr, ISP1763_PTD_DW1_ADDR) |
	    __SHIFTIN(pr->pr_token, ISP1763_PTD_DW1_TOKEN) |
	    __SHIFTIN(pr->pr_eptype, ISP1763_PTD_DW1_EPTYPE);
	if (pr->pr_split) {
		dw[1] |= ISP1763_PTD_DW1_SPLIT |
		    __SHIFTIN(pr->pr_se, ISP1763_PTD_DW1_SE) |
		    __SHIFTIN(pr->pr_port, ISP1763_PTD_DW1_PORT) |
		    __SHIFTIN(pr->pr_hub, ISP1763_PTD_DW1_HUB);
	}

	dw[2] = __SHIFTIN(pr->pr_uframe, ISP1763_PTD_DW2_UFRAME) |
	    __SHIFTIN(ISP1763_PTD_ADDR(pr->pr_mem), ISP1763_PTD_DW2_DATA);

	dw[3] = ISP1763_PTD_DW3_A |
	    __SHIFTIN(ISP1763_CERR_MAX, ISP1763_PTD_DW3_CERR);
	if (pr->pr_toggle)
		dw[3] |= ISP1763_PTD_DW3_DT;
	if (pr->pr_ping)
		dw[3] |= ISP1763_PTD_DW3_PING;

	dw[4] = __SHIFTIN(pr->pr_usa, ISP1763_PTD_DW4_USA);
	dw[5] = __SHIFTIN(pr->pr_uscs, ISP1763_PTD_DW5_USCS);
	dw[6] = 0;
	dw[7] = 0;
}

void
isp1763hc_ptd_encode(const uint32_t *dw, uint8_t *buf, int ndw)
{
	int i;

	for (i = 0; i < ndw; i++)
		le32enc(buf + 4 * i, dw[i]);
}

void
isp1763hc_ptd_decode(const uint8_t *buf, uint32_t *dw, int ndw)
{
	int i;

	for (i = 0; i < ndw; i++)
		dw[i] = le32dec(buf + 4 * i);
}

void
isp1763hc_intr_sched(bool hs, u_int ival, uint8_t *uframe, uint8_t *usa)
{
	u_int b;

	if (hs) {
		b = MIN(MAX(ival, 1), ISP1763_IVAL_MAX);
	} else {
		for (b = ISP1763_HS_IVAL_MS; b < ISP1763_IVAL_MAX &&
		    (2U << (b - ISP1763_HS_IVAL_MS)) <= ival; b++)
			continue;
	}

	switch (b) {
	case 1:
		*usa = ISP1763_USA_EVERY;
		break;
	case 2:
		*usa = ISP1763_USA_EVERY2;
		break;
	case 3:
		*usa = ISP1763_USA_EVERY4;
		break;
	default:
		*usa = ISP1763_USA_ONCE;
		break;
	}

	if (b <= ISP1763_HS_IVAL_MS)
		*uframe = 0;
	else
		*uframe = (1U << (b - ISP1763_HS_IVAL_MS - 1)) << 3;
}

uint16_t
isp1763hc_last_map(uint16_t busy)
{
	uint16_t last;

	last = 1;
	while ((busy >>= 1) != 0)
		last <<= 1;

	return last;
}

