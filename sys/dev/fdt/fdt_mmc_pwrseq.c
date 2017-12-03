/* $NetBSD: fdt_mmc_pwrseq.c,v 1.1.2.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_mmc_pwrseq.c,v 1.1.2.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <libfdt.h>
#include <dev/fdt/fdtvar.h>

struct fdtbus_mmc_pwrseq {
	device_t mps_dev;
	int mps_phandle;
	const struct fdtbus_mmc_pwrseq_func *mps_funcs;

	struct fdtbus_mmc_pwrseq *mps_next;
};

static struct fdtbus_mmc_pwrseq *fdtbus_mps = NULL;

int
fdtbus_register_mmc_pwrseq(device_t dev, int phandle,
    const struct fdtbus_mmc_pwrseq_func *funcs)
{
	struct fdtbus_mmc_pwrseq *mps;

	mps = kmem_alloc(sizeof(*mps), KM_SLEEP);
	mps->mps_dev = dev;
	mps->mps_phandle = phandle;
	mps->mps_funcs = funcs;

	mps->mps_next = fdtbus_mps;
	fdtbus_mps = mps;

	return 0;
}

static struct fdtbus_mmc_pwrseq *
fdtbus_get_mmc_pwrseq(int phandle)
{
	struct fdtbus_mmc_pwrseq *mps;

	for (mps = fdtbus_mps; mps; mps = mps->mps_next) {
		if (mps->mps_phandle == phandle) {
			return mps;
		}
	}

	return NULL;
}

struct fdtbus_mmc_pwrseq *
fdtbus_mmc_pwrseq_get(int phandle)
{
	const u_int *p;
	int mps_phandle;

	p = fdtbus_get_prop(phandle, "mmc-pwrseq", NULL);
	if (p == NULL)
		return NULL;

	mps_phandle = fdtbus_get_phandle_from_native(be32toh(p[0]));
	return fdtbus_get_mmc_pwrseq(mps_phandle);
}

void
fdtbus_mmc_pwrseq_pre_power_on(struct fdtbus_mmc_pwrseq *mps)
{
	if (mps->mps_funcs->pre_power_on)
		mps->mps_funcs->pre_power_on(mps->mps_dev);
}

void
fdtbus_mmc_pwrseq_post_power_on(struct fdtbus_mmc_pwrseq *mps)
{
	if (mps->mps_funcs->post_power_on)
		mps->mps_funcs->post_power_on(mps->mps_dev);
}

void
fdtbus_mmc_pwrseq_power_off(struct fdtbus_mmc_pwrseq *mps)
{
	if (mps->mps_funcs->power_off)
		mps->mps_funcs->power_off(mps->mps_dev);
}

void
fdtbus_mmc_pwrseq_reset(struct fdtbus_mmc_pwrseq *mps)
{
	if (mps->mps_funcs->reset)
		mps->mps_funcs->reset(mps->mps_dev);
}
