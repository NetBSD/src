/*	$NetBSD: opensolaris.c,v 1.1.44.1 2018/06/25 07:25:25 pgoyette Exp $	*/
/*-
 * Copyright 2007 John Birrell <jb@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: head/sys/cddl/compat/opensolaris/kern/opensolaris.c 222670 2011-06-04 07:02:06Z avg $
 *
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/conf.h>
#include <sys/cpuvar.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/misc.h>
#include <sys/module.h>
#include <sys/mutex.h>

cpu_core_t	cpu_core[MAXCPUS];
solaris_cpu_t	solaris_cpu[MAXCPUS];

/*
 *  OpenSolaris subsystem initialisation.
 */
void
opensolaris_init(void *dummy)
{
	int i;

	/*
	 * "Enable" all CPUs even though they may not exist just so
	 * that the asserts work. On FreeBSD, if a CPU exists, it is
	 * enabled.
	 */
	for (i = 0; i < MAXCPUS; i++) {
		solaris_cpu[i].cpuid = i;
		solaris_cpu[i].cpu_flags &= CPU_ENABLE;
	}

}

void
opensolaris_fini(void *dummy)
{
}
