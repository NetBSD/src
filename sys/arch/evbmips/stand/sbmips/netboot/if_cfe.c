/* $NetBSD: if_cfe.c,v 1.2.6.2 2017/12/03 11:36:11 jdolecek Exp $ */

/*
 * Copyright (c) 1997 Christopher G. Demetriou.  All rights reserved.
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
 */

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libkern/libkern.h>

#include "stand/sbmips/common/common.h"
#include "stand/sbmips/common/bbinfo.h"
#include "stand/sbmips/common/cfe_api.h"
#include "stand/sbmips/common/cfe_ioctl.h"

int cfenet_probe(struct netif *, void *);
int cfenet_match(struct netif *, void *);
void cfenet_init(struct iodesc *, void *);
int cfenet_get(struct iodesc *, void *, size_t, saseconds_t);
int cfenet_put(struct iodesc *, void *, size_t);
void cfenet_end(struct netif *);

extern struct netif_stats	cfenet_stats[];

struct netif_dif cfenet_ifs[] = {
/*	dif_unit	dif_nsel	dif_stats	dif_private	*/
{	0,		1,		&cfenet_stats[0],	0,		},
};
#define NCFENET_IFS (sizeof(cfenet_ifs) / sizeof(cfenet_ifs[0]))

struct netif_stats cfenet_stats[NCFENET_IFS];

struct netif_driver prom_netif_driver = {
	"cfe",			/* netif_bname */
	cfenet_match,		/* netif_match */
	cfenet_probe,		/* netif_probe */
	cfenet_init,		/* netif_init */
	cfenet_get,		/* netif_get */
	cfenet_put,		/* netif_put */
	cfenet_end,		/* netif_end */
	cfenet_ifs,		/* netif_ifs */
	NCFENET_IFS		/* netif_nifs */
};

int
cfenet_match(struct netif *nif, void *machdep_hint)
{

	return (1);
}

int
cfenet_probe(struct netif *nif, void *machdep_hint)
{

	return 0;
}

int
cfenet_put(struct iodesc *desc, void *pkt, size_t len)
{

    cfe_write(booted_dev_fd,pkt,len);

    return len;
}


int
cfenet_get(struct iodesc *desc, void *pkt, size_t len, saseconds_t timeout)
{
	satime_t t;
	int cc;

	t = getsecs();
	cc = 0;
	while (((getsecs() - t) < timeout) && !cc) {
	    cc = cfe_read(booted_dev_fd,pkt,len);
	    if (cc < 0) break;
	    break;
	}

	return cc;
}

void
cfenet_init(struct iodesc *desc, void *machdep_hint)
{
	u_int8_t eaddr[6];
	int res;

	res = cfe_ioctl(booted_dev_fd,IOCTL_ETHER_GETHWADDR,eaddr,sizeof(eaddr),NULL,0);

	if (res < 0) {
	    printf("boot: boot device name does not contain ethernet address.\n");
	    goto punt;
	    }

	memcpy(desc->myea, eaddr,6);

	printf("boot: ethernet address: %s\n", ether_sprintf(desc->myea));
	return;

punt:
	halt();

}

void
cfenet_end(struct netif *nif)
{

	/* nothing to do */
}
