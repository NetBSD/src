/* $NetBSD: zbbus.c,v 1.8 2003/07/15 03:35:51 lukem Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: zbbus.c,v 1.8 2003/07/15 03:35:51 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <mips/sibyte/include/zbbusvar.h>

#include "locators.h"

static int	zbbus_match(struct device *, struct cfdata *, void *);
static void	zbbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(zbbus, sizeof(struct device),
    zbbus_match, zbbus_attach, NULL, NULL);

static int	zbbus_print(void *, const char *);
static int	zbbus_submatch(struct device *, struct cfdata *, void *);
static const char *zbbus_entity_type_name(enum zbbus_entity_type type);

static int	zbbus_attached;

static const struct zbbus_attach_locs sb1250_zbbus_devs[] = {
	{	0, 	ZBBUS_ENTTYPE_CPU	},
	{	1, 	ZBBUS_ENTTYPE_CPU	},
	{	4, 	ZBBUS_ENTTYPE_SCD	},
	{	2, 	ZBBUS_ENTTYPE_BRZ	},
	{	3, 	ZBBUS_ENTTYPE_OBIO	},
};
static const int sb1250_zbbus_dev_count =
    sizeof sb1250_zbbus_devs / sizeof sb1250_zbbus_devs[0];

static int
zbbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	if (zbbus_attached)
		return (0);
	return 1;
}

static void
zbbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct zbbus_attach_args za;
	int i;

	printf("\n");
	zbbus_attached = 1;

	sb1250_icu_init();

	for (i = 0; i < sb1250_zbbus_dev_count; i++) {
		memset(&za, 0, sizeof za);
		za.za_locs = sb1250_zbbus_devs[i];
		config_found_sm(self, &za, zbbus_print, zbbus_submatch);
	}

	return;
}

int
zbbus_print(void *aux, const char *pnp)
{
	struct zbbus_attach_args *zap = aux;

	if (pnp)
		aprint_normal("%s at %s",
		    zbbus_entity_type_name(zap->za_locs.za_type), pnp);
	aprint_normal(" busid %d", zap->za_locs.za_busid);
	return (UNCONF);
}

static int
zbbus_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct zbbus_attach_args *zap = aux;

	if (cf->cf_loc[ZBBUSCF_BUSID] != ZBBUSCF_BUSID_DEFAULT &&
	    cf->cf_loc[ZBBUSCF_BUSID] != zap->za_locs.za_busid)
		return (0);

	return (config_match(parent, cf, aux));
}

static const char *
zbbus_entity_type_name(enum zbbus_entity_type type)
{

	switch (type) {
	case ZBBUS_ENTTYPE_CPU:
		return ("cpu");
	case ZBBUS_ENTTYPE_SCD:
		return ("sbscd");
	case ZBBUS_ENTTYPE_BRZ:
		return ("sbbrz");
	case ZBBUS_ENTTYPE_OBIO:
		return ("sbobio");
	}
	panic("zbbus_entity_type_name");
	return ("panic");
}
