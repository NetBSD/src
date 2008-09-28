/*-
 * Copyright (c) 2000 Alfred Perlstein <alfred@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: accf_data.c,v 1.1.4.2 2008/09/28 10:40:57 mjf Exp $");

#define ACCEPT_FILTER_MOD

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/lkm.h>
#include <sys/sysctl.h>
#include <sys/signalvar.h>
#include <sys/socketvar.h>
#include <netinet/accept_filter.h>

/* accept filter that holds a socket until data arrives */

static void	sohasdata(struct socket *so, void *arg, int waitflag);

static struct accept_filter accf_data_filter = {
	"dataready",
	sohasdata,
	NULL,
	NULL,
	{NULL,}
};

void accf_dataattach(int);
void accf_dataattach(int num)
{
	accept_filt_generic_mod_event(NULL, LKM_E_LOAD, &accf_data_filter);
}

/*
 * This code is to make DATA ready accept filer as LKM.
 * To compile as LKM we need to move this set of code into
 * another file and include this file for compilation when
 * making LKM
 */

#ifdef _LKM
static int accf_data_handle(struct lkm_table * lkmtp, int cmd);
int accf_data_lkmentry(struct lkm_table * lkmtp, int cmd, int ver);

MOD_MISC("accf_data");

static int accf_data_handle(struct lkm_table * lkmtp, int cmd)
{
	return accept_filt_generic_mod_event(lkmtp, cmd, &accf_data_filter);
}

/*
 * the module entry point.
 */
int
accf_data_lkmentry(struct lkm_table *lkmtp, int cmd, int ver)
{
	DISPATCH(lkmtp, cmd, ver, accf_data_handle, accf_data_handle,
	    accf_data_handle)
}
#endif

static void
sohasdata(struct socket *so, void *arg, int waitflag)
{

	if (!soreadable(so))
		return;

	so->so_upcall = NULL;
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	soisconnected(so);
	return;
}
