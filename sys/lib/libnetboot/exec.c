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
 *	$Id: exec.c,v 1.3 1994/05/27 03:33:03 cgd Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>

#include <a.out.h>

#include "salibc.h"

#include "netboot.h"
#include "netif.h"

char *kern_names[] = {
    "netbsd", "onetbsd", "netbsd.old",
    MACHINE "bsd", "o" MACHINE "bsd", MACHINE "bsd.old"
    "vmunix", "ovmunix", "vmunix.old", NULL
};

void nfs_load_image(desc, image, ev)
     struct iodesc *desc;
     char *image;
     struct exec_var *ev;
{
    u_long t;
    
    t = gettenths();
    printf("%s: ", image);
    readseg(desc, ev->text_info.file_offset,
	    ev->text_info.segment_size, ev->text_info.segment_va);
    printf("%d", ev->text_info.segment_size);
    readseg(desc, ev->data_info.file_offset,
	    ev->data_info.segment_size, ev->data_info.segment_va);
    printf("+%d", ev->data_info.segment_size);
    printf("+%d", ev->bss_info.segment_size);
    t = gettenths() - t;
    printf(" [%d.%d seconds]\n", t/10, t%10);
}

unsigned int nfs_load(desc, nfsdp, image)
     struct iodesc *desc;
     struct nfs_diskless **nfsdp;
     char *image;
{
    struct exec_var ev;
    
    /* get fundamental parameters */
    if (machdep_exec_override(desc, &ev))
	if (netbsd_exec(desc, &ev))
	    if (machdep_exec(desc, &ev))
	panic("nfs_load: unable to understand image");
    
    machdep_exec_setup(&ev);

    nfs_load_image(desc, image, &ev);

    if (ev.nfs_disklessp)
	*nfsdp = (struct nfs_diskless *) ev.nfs_disklessp;
    return ev.real_entry_point;
}

int netbsd_exec_compute(exec, ev)
     struct exec exec;
     struct exec_var *ev;
{
    bzero(ev, sizeof(*ev));

    ev->text_info.file_offset = N_TXTOFF(exec);
    ev->text_info.segment_size = exec.a_text;
    ev->text_info.segment_addr = N_TXTADDR(exec) + exec.a_entry;

    ev->data_info.file_offset = N_DATOFF(exec);
    ev->data_info.segment_size = exec.a_data;
    ev->data_info.segment_addr = N_DATADDR(exec) + exec.a_entry;

    ev->bss_info.file_offset = 0;
    ev->bss_info.segment_size = exec.a_bss;
    ev->bss_info.segment_addr = N_DATADDR(exec)+exec.a_data + exec.a_entry;
    
    ev->entry_point = exec.a_entry;

    return 0;
}
int netbsd_exec(desc, ev)
     struct iodesc *desc;
     struct exec_var *ev;
{
    int cc, error;
    u_long midmag, magic;
    u_short mid;
    struct exec exec;

    cc = readdata(desc, 0, &exec, sizeof(exec));

    if (cc != sizeof(exec))
	panic("netbsd_exec: image appears truncated\n");
    if (cc < 0)
	panic("netbsd_exec: bad exec read\n");
    if (N_GETMID(exec) != MID_MACHINE) {
	error = 1;
	goto failed;
    }
    if (N_BADMAG(exec)) {
	error = 1;
	goto failed;
    }
    error = netbsd_exec_compute(exec, ev);
 failed:
    if (error)
	printf("netbsd_exec: bad exec? code %d\n", error);
    return error;
}

void nfs_exec(desc, kernel_override)
     struct iodesc *desc;
     char *kernel_override;
{
    time_t tenths;
    char *image;
    u_long ftype, bytes;
    u_char image_fh[NFS_FHSIZE];
    struct nfs_diskless *nfsd;
    unsigned int call_addr;

    nfsd = NULL;
    image = NULL;

    if (kernel_override) {
	image = kernel_override;
	/* Get image file handle and size */
	if (lookupfh(desc, image, image_fh, NULL, &bytes, &ftype) < 0)
	    panic("nfs_exec: lookup failed %s: %s", image, strerror(errno));
	if (ftype != NFREG)
	    panic("nfs_exec: bad image ftype %d", ftype);
    }
    else {
	int i;

	for (i = 0; kern_names[i]; i++) {
	    if (lookupfh(desc, kern_names[i], image_fh, NULL,
			 &bytes, &ftype) < 0)
		continue;
	    if (ftype != NFREG)
		panic("nfs_exec: %s, bad image ftype %d",
		      kern_names[i], ftype);
	    image = kern_names[i];
	    break;
	}
	if (!image)
	    panic("nfs_exec: couldn't find kernel");
    }
    printf("nfs_exec: %s\n", image);
    desc->fh = image_fh;
    call_addr = nfs_load(desc, &nfsd, image);

#if 0
    if (nfsd)
	nfs_diskless_munge(nfsd);
#endif    
    /* Now we're ready to fill in the nfs_diskless struct */
#if 0
    if (nfsd) 
	diskless_setup(desc, (struct nfs_diskless_v1 *)sym);
#endif
}
