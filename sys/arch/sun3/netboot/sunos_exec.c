/*	$NetBSD: sunos_exec.c,v 1.3 1994/10/26 09:11:57 cgd Exp $	*/

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
#include <sys/cdefs.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <a.out.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include "netboot/netboot.h"
#include "netboot/netif.h"
#include "netboot/exec_var.h"
#include "config.h"

int sunos_exec(desc, ev)
     struct iodesc *desc;
     struct exec_var *ev;
{
    int cc, error;
    struct exec exec;
    unsigned short magic;
    
    cc = readdata(desc, 0, &exec, sizeof(exec));
    if (cc < 0)
	panic("netbsd_exec: bad exec read\n");
    if (N_BADMAG(exec)) {
	error = 1;
	goto failed;
    }
    if (N_GETMID(exec) != 2) {
	error = 1;
	goto failed;
    }
 failed:
    if (error)
	printf("netbsd_exec: bad exec? code %d\n", error);
    error = netbsd_exec_compute(exec, ev);
    return error;
}
