/*
 * Copyright (c) 1993 Adam Glass
 * All rights reserved.
 *
 * A lot of this code is derived material from the LBL bootbootp release,
 * thus their copyright below.
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
 * $Header: /cvsroot/src/sys/lib/libnetboot/Attic/boot_nfs.c,v 1.4 1993/10/15 13:43:16 cgd Exp $
 */
/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
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
 * @(#) $Header: /cvsroot/src/sys/lib/libnetboot/Attic/boot_nfs.c,v 1.4 1993/10/15 13:43:16 cgd Exp $ (LBL)
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <time.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>

#include "netboot.h"
#include "bootbootp.h"
#include "bootp.h"
#include "netif.h"

static	u_char rootfh[NFS_FHSIZE];
static	u_char swapfh[NFS_FHSIZE];
static	u_long swapblks;
static	time_t roottime;

void nfs_setup(desc)
     struct iodesc *desc;
{
    u_long ftype, bytes;

    bcopy(&desc->myea[4], &desc->myport, 2);
    
    printf("myip: %s", intoa(desc->myip));
    if (gateip)
	printf(", gateip: %s", intoa(gateip));
    if (mask)
	printf(", mask: %s", intoa(mask));
    printf("\n");
    printf("root: %s:%s\n", intoa(rootip), rootpath);
    printf("swap: %s:%s\n", intoa(swapip), swappath);
    
    /* Get root file handle and timestamp */
    desc->destip = rootip;
    getnfsfh(desc, rootpath, rootfh);
    desc->fh = rootfh;
    getnfsinfo(desc, &roottime, NULL, &ftype);
    if (ftype != NFDIR)
	panic("bad root ftype %d", ftype);
    if (debug)
	printf("nfs_setup: got root fh\n");

    desc->destip = swapip;
    getnfsfh(desc, swappath, swapfh);
    desc->fh = swapfh;
    getnfsinfo(desc, NULL, &bytes, &ftype);
    if (bytes % 512)
	printf("warning: swap is odd sized\n");
    if (ftype != NFREG)
	panic("bad swap ftype %d\n", ftype);
    swapblks = bytes / 512;
    if (debug)
	printf("nfs_setup: got swap fh\n");
    printf("swap: %d (%d blocks)\n", bytes, swapblks);
    desc->destip = rootip;
    desc->fh = rootfh;
}


unsigned int nfs_boot(kernel_override, machdep_hint)
     char *kernel_override;
     void *machdep_hint;
{
    /*
     * 0. get common ether addr if exists
     * 1. choose interface
     * 2. set up interface
     * 3. bootp or bootparam
     * 4. get filesystem fh information
     * 5. get swap information
     * 6. load kernel
     * 7. if nfs_diskless crud, do the right thign
     * 8. run kernel
     */
    struct iodesc desc;
    struct netif *nif;

    netif_init();
    while (1) {
	bzero(&desc, sizeof(desc));
	machdep_common_ether(desc.myea);
	nif = netif_select(machdep_hint);
	if (!nif) 
	    panic("netboot: no interfaces left untried");
	if (netif_probe(nif, machdep_hint)) {
	    printf("netboot: couldn't probe %s%d\n",
		   nif->netif_bname, nif->netif_unit);
	    continue;
	}
	netif_attach(nif, &desc, machdep_hint);

	get_bootinfo(&desc);
	nfs_setup(&desc);
	nfs_exec(&desc, kernel_override);
    	netif_detach(nif);
    }
}
