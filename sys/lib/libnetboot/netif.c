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
 * $Header: /cvsroot/src/sys/lib/libnetboot/Attic/netif.c,v 1.1 1993/10/13 05:41:35 cgd Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <time.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netboot.h"
#include "netif.h"

static int netif_newp = 0; /* an optimization to skip over exhausted netifs */

void netif_init()
{
    int i;
    
    if (debug)
	printf("netif_init: called\n");
    for (i =0; i < n_netif; i++) {
	netiftab[i]->netif_unit;
	netiftab[i]->netif_exhausted = 0;
    }
}
int netif_match(nif, machdep_hint, unitp)
     struct netif *nif;
     void *machdep_hint;
     int *unitp;
{
    if (debug)
	printf("netif_match: called\n");
    return nif->netif_match(machdep_hint, unitp);
}

struct netif *netif_select(machdep_hint)
     void *machdep_hint;
{
    int i;
    struct netif *best_if = NULL;
    int best_unit;
    int best_val;
    int val, unit;

    best_val = 0;
    if (debug)
	printf("network interfaces: %d\n", n_netif);
    for (i = netif_newp; i < n_netif; i++) {
	val = netif_match(netiftab[i], machdep_hint, &unit);
	if (val == 0) {
	    netiftab[i]->netif_exhausted = 1;
	    continue;
	}
	if (debug) 
	    printf("netif_select: %s%d = %d\n", netiftab[i]->netif_bname,
		   netiftab[i]->netif_unit, val);
	if (val > best_val) {
	    best_if = netiftab[i];
	    best_unit = unit;
	}
    }
    if (!best_if) return NULL;
    best_if->netif_unit = best_unit;
    return best_if;
}

int netif_probe(nif, machdep_hint)
     struct netif *nif;
     void *machdep_hint;
{
    if (debug)
	printf("netif_probe: called\n");
    return nif->netif_probe(machdep_hint);
}

void netif_attach(nif, desc, machdep_hint)
     struct netif *nif;
     struct iodesc *desc;
     void *machdep_hint;
{
    if (debug)
	printf("netif_attach: called\n");
    nif->netif_init(desc, machdep_hint);
    desc->io_netif = nif;
    bzero(desc->io_netif->netif_stats, *desc->io_netif->netif_stats);
}
void netif_detach(nif)
     struct netif *nif;
{
    if (debug)
	printf("netif_detach: called\n");
    nif->netif_end();
}
