/*	$NetBSD: pic_discovery.c,v 1.7.6.1 2017/12/03 11:36:37 jdolecek Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic_discovery.c,v 1.7.6.1 2017/12/03 11:36:37 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>
#include <sys/intr.h>

#include <powerpc/pic/picvar.h>

#include <dev/marvell/gtvar.h>
#include <dev/marvell/gtintrreg.h>
#include <dev/marvell/gtintrvar.h>


__CTASSERT(sizeof(imask_t) == sizeof(uint64_t));

struct discovery_gpp_pic_ops;

struct discovery_pic_ops {
	struct pic_ops pic;
	union {
		uint64_t mask64;
		uint32_t mask32[2];
	} _mask;
#define enable_mask		_mask.mask64
#define enable_mask_high	_mask.mask32[1]
#define enable_mask_low		_mask.mask32[0]
};
struct discovery_gpp_pic_ops {
	struct pic_ops pic;

	int gpp_base;
};


static void discovery_enable_irq(struct pic_ops *, int, int);
static void discovery_disable_irq(struct pic_ops *, int);
static int discovery_get_irq(struct pic_ops *, int);
static void discovery_ack_irq(struct pic_ops *, int);

static void discovery_gpp_enable_irq(struct pic_ops *, int, int);
static void discovery_gpp_disable_irq(struct pic_ops *, int);
static int discovery_gpp_get_irq(struct pic_ops *, int);
static void discovery_gpp_ack_irq(struct pic_ops *, int);


struct pic_ops *
setup_discovery_pic(void)
{
	struct discovery_pic_ops *discovery;
	struct pic_ops *pic;

	discovery = kmem_alloc(sizeof(*discovery), KM_SLEEP);

	pic = &discovery->pic;
	pic->pic_numintrs = 64;
	pic->pic_cookie = (void *)NULL;			/* set later */
	pic->pic_enable_irq = discovery_enable_irq;
	pic->pic_reenable_irq = discovery_enable_irq;
	pic->pic_disable_irq = discovery_disable_irq;
	pic->pic_get_irq = discovery_get_irq;
	pic->pic_ack_irq = discovery_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	pic->pic_finish_setup = NULL;
	strcpy(pic->pic_name, "discovery");
	pic_add(pic);

	discovery->enable_mask = 0;

	return pic;
}

static void
discovery_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct discovery_pic_ops *discovery = (struct discovery_pic_ops *)pic;

	discovery->enable_mask |= (1 << irq);
	discovery_enable_intr(pic->pic_cookie, irq);
}

static void
discovery_disable_irq(struct pic_ops *pic, int irq)
{
	struct discovery_pic_ops *discovery = (struct discovery_pic_ops *)pic;

	discovery->enable_mask &= ~(1 << irq);
	discovery_disable_intr(pic->pic_cookie, irq);
}

static int
discovery_get_irq(struct pic_ops *pic, int mode)
{
	struct discovery_pic_ops *discovery = (struct discovery_pic_ops *)pic;
	uint32_t cause;
	int irq, base, msk;

	cause = discovery_mic_low(pic->pic_cookie) & discovery->enable_mask_low;
	if (cause)
		base = 0;
	else {
		cause = discovery_mic_high(pic->pic_cookie);
		cause &= discovery->enable_mask_high;
		if (!cause)
			return 255;
		base = 32;
	}
	for (irq = base, msk = 0x00000001; !(cause & msk); irq++, msk <<= 1);

	return irq;
}

static void
discovery_ack_irq(struct pic_ops *pic, int irq)
{

	/* nothing */
}


struct pic_ops *
setup_discovery_gpp_pic(void *discovery, int gpp_base)
{
	struct discovery_gpp_pic_ops *discovery_gpp;
	struct pic_ops *pic;

	discovery_gpp = kmem_alloc(sizeof(*discovery_gpp), KM_SLEEP);

	pic = &discovery_gpp->pic;
	pic->pic_numintrs = 32;
	pic->pic_cookie = discovery;
	pic->pic_enable_irq = discovery_gpp_enable_irq;
	pic->pic_reenable_irq = discovery_gpp_enable_irq;
	pic->pic_disable_irq = discovery_gpp_disable_irq;
	pic->pic_get_irq = discovery_gpp_get_irq;
	pic->pic_ack_irq = discovery_gpp_ack_irq;
	pic->pic_establish_irq = dummy_pic_establish_intr;
	pic->pic_finish_setup = NULL;
	strcpy(pic->pic_name, "discovery_gpp");
	pic_add(pic);

	discovery_gpp->gpp_base = gpp_base;

	return pic;
}

static void
discovery_gpp_enable_irq(struct pic_ops *pic, int irq, int type)
{
	struct discovery_gpp_pic_ops *discovery_gpp =
	    (struct discovery_gpp_pic_ops *)pic;
	struct discovery_pic_ops *discovery = discovery_gpp->pic.pic_cookie;

	discovery_gpp_enable_intr(discovery->pic.pic_cookie,
	    discovery_gpp->gpp_base + irq);
}

static void
discovery_gpp_disable_irq(struct pic_ops *pic, int irq)
{
	struct discovery_gpp_pic_ops *discovery_gpp =
	    (struct discovery_gpp_pic_ops *)pic;
	struct discovery_pic_ops *discovery = discovery_gpp->pic.pic_cookie;

	discovery_gpp_disable_intr(discovery->pic.pic_cookie,
	    discovery_gpp->gpp_base + irq);
}

static int
discovery_gpp_get_irq(struct pic_ops *pic, int mode)
{
	struct discovery_gpp_pic_ops *discovery_gpp =
	    (struct discovery_gpp_pic_ops *)pic;
	struct discovery_pic_ops *discovery = discovery_gpp->pic.pic_cookie;
	uint32_t cause, mask;
	int irq, msk, base;

	cause = discovery_gpp_cause(discovery->pic.pic_cookie);
	mask = discovery_gpp_mask(discovery->pic.pic_cookie);

	base = discovery_gpp->gpp_base;
	cause &= (mask & (0xff << base));
	if (!cause)
		return 255;

	for (irq = base, msk = (1 << base); !(cause & msk); irq++, msk <<= 1);

	return irq;
}

static void
discovery_gpp_ack_irq(struct pic_ops *pic, int irq)
{
	struct discovery_gpp_pic_ops *discovery_gpp =
	    (struct discovery_gpp_pic_ops *)pic;
	struct discovery_pic_ops *discovery = discovery_gpp->pic.pic_cookie;

	discovery_gpp_clear_cause(discovery->pic.pic_cookie,
	    discovery_gpp->gpp_base + irq);
}
