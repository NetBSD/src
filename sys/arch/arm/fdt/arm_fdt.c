/* $NetBSD: arm_fdt.c,v 1.2 2017/05/30 21:12:41 jmcneill Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: arm_fdt.c,v 1.2 2017/05/30 21:12:41 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <machine/cpu.h>

#include <dev/fdt/fdtvar.h>
#include <dev/ofw/openfirm.h>

#include <arm/fdt/arm_fdtvar.h>

static int	arm_fdt_match(device_t, cfdata_t, void *);
static void	arm_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(arm_fdt, 0,
    arm_fdt_match, arm_fdt_attach, NULL, NULL);

static struct arm_platlist arm_platform_list =
    TAILQ_HEAD_INITIALIZER(arm_platform_list);

struct arm_fdt_cpu_hatch_cb {
	TAILQ_ENTRY(arm_fdt_cpu_hatch_cb) next;
	void (*cb)(void *, struct cpu_info *);
	void *priv;
};

static TAILQ_HEAD(, arm_fdt_cpu_hatch_cb) arm_fdt_cpu_hatch_cbs =
    TAILQ_HEAD_INITIALIZER(arm_fdt_cpu_hatch_cbs);

int
arm_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

void
arm_fdt_attach(device_t parent, device_t self, void *aux)
{
	const struct arm_platform *plat = arm_fdt_platform();
	struct fdt_attach_args faa;

	aprint_naive("\n");
	aprint_normal("\n");

	plat->init_attach_args(&faa);
	faa.faa_name = "";
	faa.faa_phandle = OF_peer(0);

	config_found(self, &faa, NULL);
}

const struct arm_platform *
arm_fdt_platform(void)
{
	static const struct arm_platform_info *booted_platform = NULL;

	if (booted_platform == NULL) {
		__link_set_decl(arm_platforms, struct arm_platform_info);
		struct arm_platform_info * const *info;
		const struct arm_platform_info *best_info = NULL;
		const int phandle = OF_peer(0);
		int match, best_match = 0;

		__link_set_foreach(info, arm_platforms) {
			const char * const compat[] = { (*info)->compat, NULL };
			match = of_match_compatible(phandle, compat);
			if (match > best_match) {
				best_match = match;
				best_info = *info;
			}
		}

		booted_platform = best_info;
	}

	return booted_platform == NULL ? NULL : booted_platform->ops;
}

void
arm_fdt_cpu_hatch_register(void *priv, void (*cb)(void *, struct cpu_info *))
{
	struct arm_fdt_cpu_hatch_cb *c;

	c = kmem_alloc(sizeof(*c), KM_SLEEP);
	c->priv = priv;
	c->cb = cb;
	TAILQ_INSERT_TAIL(&arm_fdt_cpu_hatch_cbs, c, next);
}

void
arm_fdt_cpu_hatch(struct cpu_info *ci)
{
	struct arm_fdt_cpu_hatch_cb *c;

	TAILQ_FOREACH(c, &arm_fdt_cpu_hatch_cbs, next)
		c->cb(c->priv, ci);
}
