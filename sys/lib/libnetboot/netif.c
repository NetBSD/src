/*
 * Copyright (c) 1993 Adam Glass
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
 *	This product includes software developed by Adam Glass.
 * 4. The name of the Author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Adam Glass ``AS IS'' AND
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
 *
 *	$Id: netif.c,v 1.3 1994/02/14 21:53:04 mycroft Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <time.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "salibc.h"

#include "netboot.h"
#include "netif.h"

/*
 * netif_init:
 *
 * initialize the generic network interface layer
 */

void
netif_init()
{
	struct netif_driver *drv;
	int d, i;
    
#ifdef DEBUG
	if (netif_debug)
		printf("netif_init: called\n");
#endif
	for (d = 0; d < n_netif_drivers; d++) {
		drv = netif_drivers[d];
		for (i = 0; i < drv->netif_nifs; i++)
			drv->netif_ifs[i].dif_used = 0;
	}
}

int
netif_match(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{
	struct netif_driver *drv = nif->nif_driver;

#ifdef DEBUG
#ifdef notdef
	if (netif_debug)
		printf("%s%d: netif_match (%d)\n", drv->netif_bname,
		    nif->nif_unit, nif->nif_sel);
#endif
#endif
	return drv->netif_match(nif, machdep_hint);
}

struct netif *
netif_select(machdep_hint)
	void *machdep_hint;
{
	int d, u, unit_done, s;
	struct netif_driver *drv;
	struct netif cur_if;
	static struct netif best_if;
	int best_val;
	int val;

	best_val = 0;
	best_if.nif_driver = NULL;

#ifdef DEBUG
	if (netif_debug)
		printf("netif_select: %d interfaces\n", n_netif_drivers);
#endif

	for (d = 0; d < n_netif_drivers; d++) {
		cur_if.nif_driver = netif_drivers[d];
		drv = cur_if.nif_driver;

		for (u = 0; u < drv->netif_nifs; u++) {
			cur_if.nif_unit = u;
			unit_done = 0;
		
#ifdef DEBUG
			if (netif_debug)
				printf("\t%s%d:", drv->netif_bname,
				    cur_if.nif_unit);
#endif

			for (s = 0; s < drv->netif_ifs[u].dif_nsel; s++) {
				cur_if.nif_sel = s;

				if (drv->netif_ifs[u].dif_used & (1 << s)) {
#ifdef DEBUG
					if (netif_debug)
						printf(" [%d used]", s);
#endif
					continue;
				}

				val = netif_match(&cur_if, machdep_hint);
#ifdef DEBUG
				if (netif_debug)
					printf(" [%d -> %d]", s, val);
#endif
				if (val > best_val) {
					best_val = val;
					best_if = cur_if;
				}
			}
#ifdef DEBUG
			if (netif_debug)
				printf("\n");
#endif
		}
	}

	if (best_if.nif_driver == NULL)
		return NULL;

	best_if.nif_driver->
	    netif_ifs[best_if.nif_unit].dif_used |= (1 << best_if.nif_sel);

#ifdef DEBUG
	if (netif_debug)
		printf("netif_select: %s%d(%d) wins\n",
			best_if.nif_driver->netif_bname,
			best_if.nif_unit, best_if.nif_sel);
#endif
	return &best_if;
}

int
netif_probe(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{
	struct netif_driver *drv = nif->nif_driver;

#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_probe\n", drv->netif_bname, nif->nif_unit);
#endif
	return drv->netif_probe(nif, machdep_hint);
}

void
netif_attach(nif, desc, machdep_hint)
	struct netif *nif;
	struct iodesc *desc;
	void *machdep_hint;
{
	struct netif_driver *drv = nif->nif_driver;

#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_attach\n", drv->netif_bname, nif->nif_unit);
#endif
	desc->io_netif = nif; 
#ifdef DIAGNOSTIC
	if (drv->netif_init == NULL)
		panic("%s%d: no netif_init support\n", drv->netif_bname,
		    nif->nif_unit);
#endif
	drv->netif_init(desc, machdep_hint);
	bzero(drv->netif_ifs[nif->nif_unit].dif_stats, 
	    sizeof(struct netif_stats));
}

void
netif_detach(nif)
	struct netif *nif;
{
	struct netif_driver *drv = nif->nif_driver;

#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_detach\n", drv->netif_bname, nif->nif_unit);
#endif
#ifdef DIAGNOSTIC
	if (drv->netif_end == NULL)
		panic("%s%d: no netif_end support\n", drv->netif_bname,
		    nif->nif_unit);
#endif
	drv->netif_end(nif);
}

int
netif_get(desc, pkt, len, timo)
	struct iodesc *desc;
	void *pkt;
	int len;
	time_t timo;
{
	struct netif *nif = desc->io_netif;
	struct netif_driver *drv = desc->io_netif->nif_driver;
	int rv;

#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_get\n", drv->netif_bname, nif->nif_unit);
#endif
#ifdef DIAGNOSTIC
	if (drv->netif_get == NULL)
		panic("%s%d: no netif_get support\n", drv->netif_bname,
		    nif->nif_unit);
#endif
	rv = drv->netif_get(desc, pkt, len, timo);
#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_get returning %d\n", drv->netif_bname,
		    nif->nif_unit, rv);
#endif
	return rv;
}

int
netif_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	int len;
{
	struct netif *nif = desc->io_netif;
	struct netif_driver *drv = desc->io_netif->nif_driver;
	int rv;

#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_put\n", drv->netif_bname, nif->nif_unit);
#endif
#ifdef DIAGNOSTIC
	if (drv->netif_put == NULL)
		panic("%s%d: no netif_put support\n", drv->netif_bname,
		    nif->nif_unit);
#endif
	rv = drv->netif_put(desc, pkt, len);
#ifdef DEBUG
	if (netif_debug)
		printf("%s%d: netif_put returning %d\n", drv->netif_bname,
		    nif->nif_unit, rv);
#endif
	return rv;
}
