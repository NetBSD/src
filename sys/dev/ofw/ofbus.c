/*	$NetBSD: ofbus.c,v 1.8 1998/02/03 00:43:46 cgd Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/ofw/openfirm.h>

int ofbprobe __P((struct device *, struct cfdata *, void *));
void ofbattach __P((struct device *, struct device *, void *));
static int ofbprint __P((void *, const char *));

struct cfattach ofbus_ca = {
	sizeof(struct device), ofbprobe, ofbattach
};

struct cfattach ofroot_ca = {
	sizeof(struct device), ofbprobe, ofbattach
};
 
static int
ofbprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct ofprobe *ofp = aux;
	char name[64];

	(void)of_nodename(ofp->phandle, name, sizeof name);
	if (pnp)
		printf("%s at %s", name, pnp);
	else
		printf(" (%s)", name);
	return UNCONF;
}

int
ofbprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct ofprobe *ofp = aux;
	
	if (!OF_child(ofp->phandle))
		return 0;
	return 1;
}

void
ofbattach(parent, dev, aux)
	struct device *parent, *dev;
	void *aux;
{
	int child;
	char name[5];
	struct ofprobe *ofp = aux;
	struct ofprobe probe;
	int units;

	printf("\n");

	/*
	 * This is a hack to make the probe work on the scsi (and ide) bus.
	 * YES, I THINK IT IS A BUG IN THE OPENFIRMWARE TO NOT PROBE ALL
	 * DEVICES ON THESE BUSSES.
	 */
	units = 1;
	if (OF_getprop(ofp->phandle, "name", name, sizeof name) > 0) {
		if (!strcmp(name, "scsi"))
			units = 7; /* What about wide or hostid != 7?	XXX */
		else if (!strcmp(name, "ide"))
			units = 2;
	}

	for (child = OF_child(ofp->phandle); child; child = OF_peer(child)) {
		/*
		 * This is a hack to skip all the entries in the tree
		 * that aren't devices (packages, openfirmware etc.).
		 */
		if (OF_getprop(child, "device_type", name, sizeof name) < 0 &&
		    OF_getprop(child, "compatible", name, sizeof name) < 0)
			continue;
		probe.phandle = child;
		for (probe.unit = 0; probe.unit < units; probe.unit++)
			config_found(dev, &probe, ofbprint);
	}
}
