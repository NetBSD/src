/*	$NetBSD: p_nec_jc94.c,v 1.1 2001/06/13 15:30:38 soda Exp $	*/

/*
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>

#include <machine/autoconf.h>
#include <machine/platform.h>

#include <arc/jazz/rd94.h>
#include <arc/jazz/jazziovar.h>

void p_nec_jc94_init __P((void));

struct platform platform_nec_jc94 = {
	"NEC-JC94",
	"NEC W&S",
	" R4400 PCI",
	"Express 5800/230",
	"NEC",
	150, /* MHz ?? */
	c_nec_pci_mainbusdevs,
	platform_generic_match,
	p_nec_jc94_init,
	c_nec_pci_cons_init,
	jazzio_reset,
	c_nec_jazz_set_intr,
};

/*
 * jazzio bus configuration
 */

struct pica_dev nec_jc94_cpu[] = {
	{{ "timer",	-1, 0, },	(void *)RD94_SYS_IT_VALUE, },
	{{ "dallas_rtc", -1, 0, },	(void *)RD94_SYS_CLOCK, },
	{{ "lpt",	0, 0, },	(void *)RD94_SYS_PAR1, },
	{{ "fdc",	1, 0, },	(void *)RD94_SYS_FLOPPY, },
	{{ "AD1848",	2, 0, },	(void *)RD94_SYS_SOUND,},
	{{ "sonic",	3, 0, },	(void *)RD94_SYS_SONIC, },
	{{ "osiop",	5, 0, },	(void *)RD94_SYS_SCSI1, }, /*scsi(0)*/
	{{ "osiop",	4, 0, },	(void *)RD94_SYS_SCSI0, }, /*scsi(1)*/
	{{ "pckbd",	6, 0, },	(void *)RD94_SYS_KBD, },
	{{ "pms",	7, 0, },	(void *)RD94_SYS_KBD, },
	{{ "com",	8, 0, },	(void *)RD94_SYS_COM1, },
	{{ "com",	9, 0, },	(void *)RD94_SYS_COM2, },
	{{ NULL,	-1, 0, },	(void *)NULL, },
};

/*
 * critial i/o space, interrupt, and other chipset related initialization.
 */
void
p_nec_jc94_init()
{
	c_nec_pci_init();

	/* chipset-dependent jazzio bus configuration */
	jazzio_devconfig = nec_jc94_cpu;
}
