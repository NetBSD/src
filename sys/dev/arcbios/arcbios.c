/*	$NetBSD: arcbios.c,v 1.4 2001/09/08 01:39:11 rafal Exp $	*/

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

#include <sys/systm.h>
#include <dev/cons.h>
#include <sys/conf.h>

#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

const struct arcbios_spb *ARCBIOS_SPB;
const struct arcbios_fv *ARCBIOS;

char arcbios_sysid_vendor[ARCBIOS_SYSID_FIELDLEN + 1];
char arcbios_sysid_product[ARCBIOS_SYSID_FIELDLEN + 1];

char arcbios_system_identifier[64 + 1];

int	arcbios_cngetc(dev_t);
void	arcbios_cnputc(dev_t, int);

void	arcbios_fetch_system_identifier(struct arcbios_component *,
	    struct arcbios_treewalk_context *);

struct consdev arcbios_cn = {
	NULL, NULL, arcbios_cngetc, arcbios_cnputc, nullcnpollc, NULL,
	    NODEV, CN_NORMAL,
};

cdev_decl(arcbios_tty);

/*
 * arcbios_init:
 *
 *	Initialize the ARC BIOS.
 */
int
arcbios_init(vaddr_t pblkva)
{
	int maj;
	struct arcbios_sysid *sid;

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

	/* Find the ARC BIOS console device major (needed by cnopen) */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == arcbios_ttyopen)
			break;

	arcbios_cn.cn_dev = makedev(maj, 0);

	/* Initialize the bootstrap console. */
	cn_tab = &arcbios_cn;

	/*
	 * Fetch the system ID.
	 */
	sid = (*ARCBIOS->GetSystemId)();
	if (sid != NULL) {
		memcpy(arcbios_sysid_vendor, sid->VendorId,
		    sizeof(sid->VendorId));
		arcbios_sysid_vendor[sizeof(sid->VendorId)] = '\0';

		memcpy(arcbios_sysid_product, sid->ProductId,
		    sizeof(sid->ProductId));
		arcbios_sysid_product[sizeof(sid->ProductId)] = '\0';
	}

	/*
	 * Fetch the identifier string from the `system' component.
	 * Machdep code will use this to initialize the system type.
	 */
	arcbios_tree_walk(arcbios_fetch_system_identifier, NULL);

	return (0);
}

void
arcbios_fetch_system_identifier(struct arcbios_component *node,
    struct arcbios_treewalk_context *atc)
{

	switch (node->Class) {
	case COMPONENT_CLASS_SystemClass:
		arcbios_component_id_copy(node,
		    arcbios_system_identifier,
		    sizeof(arcbios_system_identifier));
		atc->atc_terminate = 1;
		break;

	default:
		break;
	}
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

void
arcbios_component_id_copy(struct arcbios_component *node,
    char *dst, size_t dstsize)
{

	dstsize--;
	if (dstsize > node->IdentifierLength)
		dstsize = node->IdentifierLength;
	memcpy(dst, node->Identifier, dstsize);
	dst[dstsize] = '\0';
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
