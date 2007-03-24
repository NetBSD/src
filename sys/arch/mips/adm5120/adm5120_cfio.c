/* $NetBSD: adm5120_cfio.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $ */

/*-
 * Copyright (c) 2007 David Young.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm5120_cfio.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_mainbusvar.h>
#include <mips/adm5120/include/adm5120_extiovar.h>

#ifdef CFIO_DEBUG
int cfio_debug = 1;
#define	CFIO_DPRINTF(__fmt, ...)		\
do {						\
	if (cfio_debug)				\
		printf((__fmt), __VA_ARGS__);	\
} while (/*CONSTCOND*/0)
#else /* !CFIO_DEBUG */
#define	CFIO_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* CFIO_DEBUG */

static struct {
	bus_space_handle_t	ch_handle[2];
	int			ch_inuse;
	int			ch_parent;
} cf_handles[16];

static int cf_handle_alloc(int);
static void cf_handle_free(int);
static void cf_bs_unmap(void *, bus_space_handle_t, bus_size_t, int);
static int cf_bs_map(void *, bus_addr_t, bus_size_t, int,
    bus_space_handle_t *, int);
static int cf_bs_subregion(void *, bus_space_handle_t, bus_size_t,
    bus_size_t, bus_space_handle_t *);
static int cf_bs_translate(void *, bus_addr_t, bus_size_t, int,
    struct mips_bus_space_translation *);
static int cf_bs_get_window(void *, int, struct mips_bus_space_translation *);
static int cf_bs_translate(void *, bus_addr_t, bus_size_t, int,
    struct mips_bus_space_translation *);
static int cf_bs_get_window(void *, int, struct mips_bus_space_translation *);
static int cf_bs_alloc(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t,
    bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
static void cf_bs_free(void *, bus_space_handle_t, bus_size_t size);
static void *cf_bs_vaddr(void *, bus_space_handle_t);
static paddr_t cf_bs_mmap(void *, bus_addr_t, off_t off, int prot, int);
static void cf_bs_barrier(void *, bus_space_handle_t, bus_size_t,
    bus_size_t length, int);
static uint8_t cf_bs_r_1(void *, bus_space_handle_t, bus_size_t offset);
static void cf_bs_rm_1(void *, bus_space_handle_t, bus_size_t,
    uint8_t *, bus_size_t);
static void cf_bs_w_1(void *, bus_space_handle_t, bus_size_t, uint8_t val);
static void cf_bs_wm_1(void *, bus_space_handle_t, bus_size_t,
    const uint8_t *, bus_size_t);
static uint8_t cf_bs_rs_1(void *, bus_space_handle_t, bus_size_t offset);
static void cf_bs_rms_1(void *, bus_space_handle_t, bus_size_t,
    uint8_t *, bus_size_t);
static void cf_bs_ws_1(void *, bus_space_handle_t, bus_size_t, uint8_t val);
static void cf_bs_wms_1(void *, bus_space_handle_t, bus_size_t, const uint8_t *,
    bus_size_t);
static void cf_bs_sm_1(void *, bus_space_handle_t, bus_size_t, uint8_t,
    bus_size_t);

static const struct mips_bus_space cfio_space = {
	/* cookie */
	.bs_cookie = NULL,

	/* mapping/unmapping */
	.bs_map = cf_bs_map,
	.bs_unmap = cf_bs_unmap,
	.bs_subregion = cf_bs_subregion,

	.bs_translate = cf_bs_translate,
	.bs_get_window = cf_bs_get_window,
	.bs_alloc = cf_bs_alloc,
	.bs_free = cf_bs_free,
	.bs_vaddr = cf_bs_vaddr,
	.bs_mmap = cf_bs_mmap,
	.bs_barrier = cf_bs_barrier,
	.bs_r_1 = cf_bs_r_1,
	.bs_r_2 = NULL,
	.bs_r_4 = NULL,
	.bs_r_8 = NULL,
	.bs_rm_1 = cf_bs_rm_1,
	.bs_rm_2 = NULL,
	.bs_rm_4 = NULL,
	.bs_rm_8 = NULL,
	.bs_rr_1 = NULL,
	.bs_rr_2 = NULL,
	.bs_rr_4 = NULL,
	.bs_rr_8 = NULL,
	.bs_w_1 = cf_bs_w_1,
	.bs_w_2 = NULL,
	.bs_w_4 = NULL,
	.bs_w_8 = NULL,

	.bs_wm_1 = cf_bs_wm_1,
	.bs_wm_2 = NULL,
	.bs_wm_4 = NULL,
	.bs_wm_8 = NULL,

	.bs_wr_1 = NULL,
	.bs_wr_2 = NULL,
	.bs_wr_4 = NULL,
	.bs_wr_8 = NULL,

	/* read (single) stream */
	.bs_rs_1 = cf_bs_rs_1,
	.bs_rs_2 = NULL,
	.bs_rs_4 = NULL,
	.bs_rs_8 = NULL,

	/* read multiple stream */
	.bs_rms_1 = cf_bs_rms_1,
	.bs_rms_2 = NULL,
	.bs_rms_4 = NULL,
	.bs_rms_8 = NULL,

	/* read region stream */
	.bs_rrs_1 = NULL,
	.bs_rrs_2 = NULL,
	.bs_rrs_4 = NULL,
	.bs_rrs_8 = NULL,

	/* write (single) stream */
	.bs_ws_1 = cf_bs_ws_1,
	.bs_ws_2 = NULL,
	.bs_ws_4 = NULL,
	.bs_ws_8 = NULL,

	/* write multiple stream */
	.bs_wms_1 = cf_bs_wms_1,
	.bs_wms_2 = NULL,
	.bs_wms_4 = NULL,
	.bs_wms_8 = NULL,
					
	/* write region stream */
	.bs_wrs_1 = NULL,
	.bs_wrs_2 = NULL,
	.bs_wrs_4 = NULL,
	.bs_wrs_8 = NULL,

	/* set multiple */
	.bs_sm_1 = cf_bs_sm_1,
	.bs_sm_2 = NULL,
	.bs_sm_4 = NULL,
	.bs_sm_8 = NULL,

	/* set region */
	.bs_sr_1 = NULL,
	.bs_sr_2 = NULL,
	.bs_sr_4 = NULL,
	.bs_sr_8 = NULL,

	/* copy */
	.bs_c_1 = NULL,
	.bs_c_2 = NULL,
	.bs_c_4 = NULL,
	.bs_c_8 = NULL,
};

void
cfio_bus_mem_init(bus_space_tag_t cfio, bus_space_tag_t extio)
{
	*cfio = cfio_space;
	cfio->bs_cookie = (void *)extio;
}

static void
cf_bs_unmap(void *cookie, bus_space_handle_t bh, bus_size_t size, int acct)
{
	KASSERT(acct == 1);
	bus_space_unmap((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[1], size);
	bus_space_unmap((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[0], size);
	cf_handle_free(bh);
}

static void
cf_handle_free(int which)
{
	int i, parent;

	KASSERT(cf_handles[which].ch_inuse);
	for (i = 0; i < __arraycount(cf_handles); i++) {
		for (parent = cf_handles[i].ch_parent; parent != -1;
		     parent = cf_handles[parent].ch_parent) {
			if (parent == which) {
				CFIO_DPRINTF("%s: free %d (ancestor %d)\n",
				    __func__, i, which);
				cf_handles[i].ch_inuse = 0;
				break;
			}
		}
	}
	cf_handles[which].ch_inuse = 0;
	for (i = 0; i < __arraycount(cf_handles); i++) {
		if (!cf_handles[i].ch_inuse)
			cf_handles[i].ch_parent = -1;
	}
}

static int
cf_handle_alloc(int parent)
{
	int i;

	for (i = 0; i < __arraycount(cf_handles) && cf_handles[i].ch_inuse; i++)
		;
	if (i >= __arraycount(cf_handles))
		return -1;
	cf_handles[i].ch_inuse = 1;
	cf_handles[i].ch_parent = parent;
	return i;
}

static int
cf_bs_map(void *cookie, bus_addr_t addr, bus_size_t size, int flags,
    bus_space_handle_t *bhp, int acct)
{
	int bh, rc;

	KASSERT(acct == 1);

	if ((bh = cf_handle_alloc(-1)) == -1)
		return EBUSY;

	rc = bus_space_map((bus_space_tag_t)cookie,
	    addr + ADM5120_BASE_EXTIO1 - ADM5120_BASE_EXTIO0, size, flags,
	    &cf_handles[bh].ch_handle[1]);
	if (rc != 0)
		return rc;

	rc = bus_space_map((bus_space_tag_t)cookie, addr, size, flags,
	    &cf_handles[bh].ch_handle[0]);
	if (rc != 0) {
		bus_space_unmap((bus_space_tag_t)cookie,
		    cf_handles[bh].ch_handle[1], size);
		cf_handle_free(bh);
	} else
		*bhp = bh;

	return rc;
}

static int
cf_bs_subregion(void *cookie, bus_space_handle_t h, bus_size_t offset,
    bus_size_t size, bus_space_handle_t *bhp)
{
	int bh, rc;

	if ((bh = cf_handle_alloc(h)) == -1)
		return EBUSY;

	rc = bus_space_subregion((bus_space_tag_t)cookie,
	    cf_handles[h].ch_handle[1], offset, size,
	    &cf_handles[bh].ch_handle[1]);
	if (rc != 0)
		return rc;

	rc = bus_space_subregion((bus_space_tag_t)cookie,
	    cf_handles[h].ch_handle[0], offset, size,
	    &cf_handles[bh].ch_handle[0]);
	if (rc != 0)
		cf_handle_free(bh);
	else
		*bhp = bh;

	return rc;
}

static int
cf_bs_translate(void *cookie, bus_addr_t addr, bus_size_t len, int flags,
    struct mips_bus_space_translation *mbst)
{
	panic("%s: not implemented\n", __func__);
}

static int
cf_bs_get_window(void *cookie, int window,
		    struct mips_bus_space_translation *mbst)
{
	panic("%s: not implemented\n", __func__);
}

static int
cf_bs_alloc(void *cookie, bus_addr_t reg_start,
    bus_addr_t reg_end, bus_size_t size, bus_size_t alignment,
    bus_size_t boundary, int flags, bus_addr_t *addrp,
    bus_space_handle_t *bhp)
{
	bus_addr_t tmp;
	int bh, rc;

	if ((bh = cf_handle_alloc(-1)) == -1)
		return EBUSY;

	rc = bus_space_alloc((bus_space_tag_t)cookie,
	    reg_start + ADM5120_BASE_EXTIO1 - ADM5120_BASE_EXTIO0,
	    reg_end + ADM5120_BASE_EXTIO1 - ADM5120_BASE_EXTIO0,
	    size, alignment, boundary, flags, &tmp,
	    &cf_handles[bh].ch_handle[1]);
	if (rc != 0)
		return rc;

	rc = bus_space_alloc((bus_space_tag_t)cookie, reg_start, reg_end, size,
	    alignment, boundary, flags, addrp, &cf_handles[bh].ch_handle[0]);
	if (rc != 0) {
		bus_space_free((bus_space_tag_t)cookie,
		    cf_handles[bh].ch_handle[1], size);
		cf_handle_free(bh);
	} else
		*bhp = bh;

	return rc;
}

static void
cf_bs_free(void *cookie, bus_space_handle_t bh, bus_size_t size)
{
	bus_space_free((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[0],
	    size);
	bus_space_free((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[1],
	    size);
	cf_handle_free(bh);
}

static void *
cf_bs_vaddr(void *cookie, bus_space_handle_t bh)
{
	panic("%s: not implemented", __func__);
}

static paddr_t
cf_bs_mmap(void *cookie, bus_addr_t addr, off_t off, int prot, int flags)
{
	panic("%s: not implemented", __func__);
}

static void	
cf_bs_barrier(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    bus_size_t length, int flags)
{
	bus_space_barrier((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[0],
	    offset, length, flags);
	bus_space_barrier((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[1],
	    offset, length, flags);
}

/* read (single) */
static uint8_t
cf_bs_r_1(void *cookie, bus_space_handle_t bh, bus_size_t offset)
{
	(void)bus_space_read_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[0], offset);
	return bus_space_read_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[1], offset);
}

/* read multiple */
static void
cf_bs_rm_1(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    uint8_t *datap, bus_size_t count)
{
	while (count-- > 0)
		*datap++ = cf_bs_r_1(cookie, bh, offset);
}

/* write (single) */
static void
cf_bs_w_1(void *cookie, bus_space_handle_t bh, bus_size_t offset, uint8_t val)
{
	bus_space_write_1((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[0],
	    offset, val);
	bus_space_write_1((bus_space_tag_t)cookie, cf_handles[bh].ch_handle[1],
	    offset, val);
}

/* write multiple */
static void
cf_bs_wm_1(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    const uint8_t *datap, bus_size_t count)
{
	while (count-- > 0)
		cf_bs_w_1(cookie, bh, offset, *datap++);
}

/* read (single) stream */
static uint8_t
cf_bs_rs_1(void *cookie, bus_space_handle_t bh, bus_size_t offset)
{
	(void)bus_space_read_stream_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[0], offset);
	return bus_space_read_stream_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[1], offset);
}

/* read multiple stream */
static void
cf_bs_rms_1(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    uint8_t *datap, bus_size_t count)
{
	while (count-- > 0)
		*datap++ = cf_bs_rs_1(cookie, bh, offset);
}

/* write (single) stream */
static void
cf_bs_ws_1(void *cookie, bus_space_handle_t bh, bus_size_t offset, uint8_t val)
{
	bus_space_write_stream_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[0], offset, val);
	bus_space_write_stream_1((bus_space_tag_t)cookie,
	    cf_handles[bh].ch_handle[1], offset, val);
}

/* write multiple stream */
static void
cf_bs_wms_1(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    const uint8_t *datap, bus_size_t count)
{
	while (count-- > 0)
		cf_bs_ws_1(cookie, bh, offset, *datap++);
}

/* set multiple */
static void
cf_bs_sm_1(void *cookie, bus_space_handle_t bh, bus_size_t offset,
    uint8_t value, bus_size_t count)
{
	while (count-- > 0)
		cf_bs_w_1(cookie, bh, offset, value);
}
