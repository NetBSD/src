/*	$NetBSD: mainbus.c,v 1.20.6.1 2011/06/06 09:06:00 jruoho Exp $	*/

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.20.6.1 2011/06/06 09:06:00 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/pci/pcivar.h>
#include <dev/ofw/openfirm.h>

#include <machine/autoconf.h>

#include <powerpc/pic/picvar.h>

int	mainbus_match(device_t, cfdata_t, void *);
void	mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mainbus, 0,
    mainbus_match, mainbus_attach, NULL, NULL);

/*
 * Probe for the mainbus; always succeeds.
 */
int
mainbus_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

/*
 * Attach the mainbus.
 */
void
mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct ofbus_attach_args oba;
	struct confargs ca;
	int node, i;
	u_int32_t reg[4];
	char name[32];

	printf("\n");

	for (i = 0; i < 2; i++) {
		ca.ca_name = "cpu";
		ca.ca_reg = reg;
		reg[0] = i;
		config_found(self, &ca, NULL);
	}

	pic_finish_setup();

	node = OF_peer(0);
	if (node) {
		oba.oba_busname = "ofw";
		oba.oba_phandle = node;
		config_found(self, &oba, NULL);
	}

	for (node = OF_child(OF_finddevice("/")); node; node = OF_peer(node)) {
		memset(name, 0, sizeof(name));
		if (OF_getprop(node, "name", name, sizeof(name)) == -1)
			continue;

		ca.ca_name = name;
		ca.ca_node = node;
		ca.ca_nreg = OF_getprop(node, "reg", reg, sizeof(reg));
		ca.ca_reg  = reg;
		config_found(self, &ca, NULL);
	}

#ifdef MAMBO
	ca.ca_name="com";
	config_found(self, &ca, NULL);
#endif

}
