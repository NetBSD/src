/*	$NetBSD: if_prom.c,v 1.1 1996/09/18 20:03:10 cgd Exp $	*/

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
 */

#include <sys/param.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netif.h"
#include "include/prom.h"
#include "lib/libkern/libkern.h"

int prom_probe();
int prom_match();
void prom_init();
int prom_get();
int prom_put();
void prom_end();

extern struct netif_stats	prom_stats[];

struct netif_dif prom_ifs[] = {
/*	dif_unit	dif_nsel	dif_stats	dif_private	*/
{	0,		1,		&prom_stats[0],	0,		},
};

struct netif_stats prom_stats[NENTS(prom_ifs)];

struct netif_driver prom_netif_driver = {
	"prom",			/* netif_bname */
	prom_match,		/* netif_match */
	prom_probe,		/* netif_probe */
	prom_init,		/* netif_init */
	prom_get,		/* netif_get */
	prom_put,		/* netif_put */
	prom_end,		/* netif_end */
	prom_ifs,		/* netif_ifs */
	NENTS(prom_ifs)		/* netif_nifs */
};

int netfd;

int
prom_match(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{

	printf("prom_match(0x%lx, 0x%lx)\n", nif, machdep_hint);
	return (1);
}

int
prom_probe(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{

	printf("prom_probe(0x%lx, 0x%lx)\n", nif, machdep_hint);
	return 0;
}

int
prom_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	int len;
{

	/* printf("prom_put(0x%lx, 0x%lx, %d)\n", desc, pkt, len); */
	prom_write(netfd, len, pkt, 0);

	return len;
}


int
prom_get(desc, pkt, len, timeout)
	struct iodesc *desc;
	void *pkt;
	int len;
	time_t timeout;
{
        prom_return_t ret;
        time_t t;
        int cc;
	char hate[2000];

#if 0
	printf("prom_get(0x%lx, 0x%lx, %d, %d)\n", desc, pkt, len, timeout);
#endif

        t = getsecs();  
        cc = 0;                 
        while (((getsecs() - t) < timeout) && !cc) {
#if 1 /* TC machines' firmware */
                ret.bits = prom_read(netfd, 0, hate, 0);
#else
                ret.bits = prom_read(netfd, sizeof hate, hate, 0);
#endif
		if (ret.u.status == 0)
			cc += ret.u.retval;
        }

#if 0
	printf("got %d\n", cc);

	if (cc != len)
		printf("prom_get: cc = %d, len = %d\n", cc, len);
#endif

#if 1 /* TC machines' firmware */
	cc = min(cc, len);
#else
	cc = len;
#endif
	bcopy(hate, pkt, cc);

        return cc;
}

void
prom_init(desc, machdep_hint)
	struct iodesc *desc;
	void *machdep_hint;
{

        prom_return_t ret;
        char devname[64], tmpbuf[14];
        int devlen;

	printf("prom_init(0x%lx, 0x%lx)\n", desc, machdep_hint);

        ret.bits = prom_getenv(PROM_E_BOOTED_DEV, devname, sizeof(devname));
        devlen = ret.u.retval;
        printf("devlen = %d\n", devlen);
printf("booted_dev was: %s\n", devname);
        ret.bits = prom_open(devname, devlen + 1); 
        if (ret.u.status) {
                printf("open failed: %d\n", ret.u.status);
                return;
        }
        netfd = ret.u.retval;

	bzero(tmpbuf, sizeof(tmpbuf));
	prom_write(netfd, sizeof(tmpbuf), tmpbuf, 0);

	/* PLUG YOUR ENET ADDR IN HERE. */
#if 1
#if 1
	desc->myea[0] = 0x08;
	desc->myea[1] = 0x00;
	desc->myea[2] = 0x2b;
	desc->myea[3] = 0xbd;
	desc->myea[4] = 0x5d;
	desc->myea[5] = 0xfd;
#else
	desc->myea[0] = 0x08;
	desc->myea[1] = 0x00;
	desc->myea[2] = 0x2b;
	desc->myea[3] = 0x39;
	desc->myea[4] = 0x98;
	desc->myea[5] = 0x3d;
#endif
#else
	desc->myea[0] = 0x00;
	desc->myea[1] = 0x00;
	desc->myea[2] = 0xf8;
	desc->myea[3] = 0x21;
	desc->myea[4] = 0xa6;
	desc->myea[5] = 0x81;
#endif
}

void
prom_end(nif)
	struct netif *nif;
{

	printf("prom_end(0x%lx)\n", nif);
	prom_close(netfd);
}
