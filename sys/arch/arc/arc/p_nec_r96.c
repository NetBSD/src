/*	$NetBSD: p_nec_r96.c,v 1.2 2003/01/31 22:07:52 tsutsui Exp $	*/

/*-
 * Copyright (C) 2000 Shuichiro URATA.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>

#include <machine/autoconf.h>
#include <machine/platform.h>

#include <arc/arc/arcbios.h>
#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>

static int p_nec_riscserver_2200_match __P((struct platform *));

struct platform platform_nec_r96 = {
	"NEC-R96",
	"MIPS DUO",
	"",
	"Express RISCserver",
	"NEC",
	150, /* MHz */
	c_jazz_eisa_mainbusdevs,
	platform_generic_match,
	c_nec_eisa_init,
	c_nec_eisa_cons_init,
	jazzio_reset,
	c_nec_jazz_set_intr,
};

struct platform platform_nec_riscserver_2200 = {
	"NEC-R96",
	"MIPS DUO",
	"",
	"RISCserver 2200",
	"NEC",
	200, /* MHz */
	c_jazz_eisa_mainbusdevs,
	p_nec_riscserver_2200_match,
	c_nec_eisa_init,
	c_jazz_eisa_cons_init,
	jazzio_reset,
	c_nec_jazz_set_intr,
};

static int
p_nec_riscserver_2200_match(p)
	struct platform *p;
{
	if (strcmp(arc_id, p->system_id) == 0 &&
	    (p->vendor_id == NULL || strcmp(arc_vendor_id, p->vendor_id) == 0)
	    && arc_cpu_l2cache_size == 2 * 1024 * 1024)
		return (2);

	return (0);
}
