/*	$NetBSD: arcbios.c,v 1.2 2001/07/08 22:57:10 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>

#include <dev/cons.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

const struct arcbios_spb *ARCBIOS_SPB;
const struct arcbios_fv *ARCBIOS;

int	arcbios_cngetc(dev_t);
void	arcbios_cnputc(dev_t, int);

struct consdev arcbios_cn = {
	NULL, NULL, arcbios_cngetc, arcbios_cnputc, nullcnpollc, NULL,
	    NODEV, CN_NORMAL,
};

/*
 * arcbios_init:
 *
 *	Initialize the ARC BIOS.
 */
int
arcbios_init(vaddr_t pblkva)
{

	ARCBIOS_SPB = (struct arcbios_spb *) pblkva;
	
	switch (ARCBIOS_SPB->SPBSignature) {
	case ARCBIOS_SPB_SIGNATURE:
	case ARCBIOS_SPB_SIGNATURE_1:
		/* Okay. */
		break;

	default:
		/* Don't know what this is. */
		return (1);
	}

	/* Initialize our pointer to the firmware vector. */
	ARCBIOS = ARCBIOS_SPB->FirmwareVector;

	/* Initialize the bootstrap console. */
	cn_tab = &arcbios_cn;

	return (0);
}

/****************************************************************************
 * ARC component tree walking routines.
 ****************************************************************************/

static void
arcbios_subtree_walk(struct arcbios_component *node,
    void (*func)(struct arcbios_component *, struct arcbios_treewalk_context *),
    struct arcbios_treewalk_context *atc)
{

	for (node = (*ARCBIOS->GetChild)(node);
	     node != NULL && atc->atc_terminate == 0;
	     node = (*ARCBIOS->GetPeer)(node)) {
		(*func)(node, atc);
		if (atc->atc_terminate)
			return;
		arcbios_subtree_walk(node, func, atc);
	}
}

void
arcbios_tree_walk(void (*func)(struct arcbios_component *,
    struct arcbios_treewalk_context *), void *cookie)
{
	struct arcbios_treewalk_context atc;

	atc.atc_cookie = cookie;
	atc.atc_terminate = 0;

	arcbios_subtree_walk(NULL, func, &atc);
}

/****************************************************************************
 * Bootstrap console routines.
 ****************************************************************************/

int
arcbios_cngetc(dev_t dev)
{
	uint32_t count;
	char c;

	(*ARCBIOS->Read)(ARCBIOS_STDIN, &c, 1, &count);
	return (c);
}

void
arcbios_cnputc(dev_t dev, int c)
{
	uint32_t count;
	char ch = c;

	(*ARCBIOS->Write)(ARCBIOS_STDOUT, &ch, 1, &count);
}
