/*	$NetBSD: prompatch.c,v 1.1 2001/06/21 03:13:05 uwe Exp $ */

/*
 * Copyright (c) 2001 Valeriy E. Ushakov
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
 * 3. The name of the author may not be used to endorse or promote products
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

#include <sys/param.h>
#include <lib/libsa/stand.h>
#include <machine/promlib.h>

void prom_patch(void);

/*
 * Each patch entry is processed by:
 * printf(message);  prom_interpret(patch);  printf("\n");
 */
struct patch_entry {
	char *message;
	char *patch;
};

/*
 * PROM patches to apply to machine matching name/promvers.
 */
struct prom_patch {
	char *name;		/* "name" of the root node */
	int promvers;		/* prom_version() */
	struct patch_entry *patches;
};



/*
 * Patches for JavaStation 1 with OBP 3.* (OpenFirmware).
 * PROM in these machines is crippled in many ways.
 */
static struct patch_entry patch_js1_of[] = {

/*
 * JS1/OF has no cpu node in the device tree.  Create one to save us a
 * _lot_ of headache in cpu.c and mainbus_attach.  Mostly copied from
 * OBP2.  While clock-frequency is usually at the root node, add it
 * here for brevity as kernel checks cpu node for this property anyway.
 */
{ "cpu: creating node ", /* NB: space at the end is intentional */
	"0 0 0 0 \" /\" begin-package"
	" \" FMI,MB86904\" device-name" /* NB: will print the name */
	" \" cpu\" device-type"
	" 0 \" mid\" integer-property"

	" 8 \" sparc-version\" integer-property"
	" 4 \" version\" integer-property"
	" 0 \" implementation\" integer-property"
	" 4 \" psr-version\" integer-property"
 	" 0 \" psr-implementation\" integer-property"

	" d# 100000000 \" clock-frequency\" integer-property"

	" d# 4096 \" page-size\" integer-property"
	" d# 256 \" mmu-nctx\" integer-property"

	" 2 \" ncaches\" integer-property"

	" d# 512 \" icache-nlines\" integer-property"
	" d# 32 \" icache-line-size\" integer-property"
	" 1 \" icache-associativity\" integer-property"

	" d# 512 \" dcache-nlines\" integer-property"
	" d# 16 \" dcache-line-size\" integer-property"
	" 1 \" dcache-associativity\" integer-property"

	" 0 0 \" cache-physical?\" property"
	" 0 0 \" cache-coherence?\" property"

	" end-package"
},

/*
 * mk48txx_attach needs a model name, spare clockattach from guesswork.
 */
{ "eeprom: adding \"model\"",
	"dev /obio/eeprom \" mk48t08\" model device-end"
},

/* 
 * "interrupts" property is bogusly zero, delete it and let
 * sbus_get_intr fallback to correct "intr" property
 */
{ "le: deleting bogus \"interrupts\"",
	"dev /sbus/ledma/le \" interrupts\" delete-property device-end"
},

/*
 * Give "su" (com port) "interrupts" property.
 */
{ "su: adding \"interrupts\"",
	"dev /obio/su d# 13 \" interrupts\" integer-property device-end"
},
    
/*
 * Give "8042" (pckbc) "interrupts" property.
 */
{ "8042: adding \"interrupts\"",
	"dev /obio/8042 d# 13 \" interrupts\" integer-property device-end"
},
    
{ NULL, NULL }
}; /* patch_js1_of */



static struct prom_patch prom_patch_tab[] = {
	{ "SUNW,JDM1", PROM_OPENFIRM, patch_js1_of },
	{ NULL, 0, NULL }
};


/*
 * Check if this machine needs tweaks to its PROM.  It's simpler to
 * fix PROM than to invent workarounds in the kernel code.  We do this
 * patching in the secondary boot to avoid wasting space in the
 * kernel.
 */
void
prom_patch(void)
{
	char namebuf[32];
	char *propval;
	struct prom_patch *p;

	propval = getpropstringA(prom_findroot(), "name",
				 namebuf, sizeof(namebuf));
	if (propval == NULL)
		return;

	for (p = prom_patch_tab; p->name != NULL; ++p) {
		if (p->promvers == prom_version()
		    && strcmp(p->name, namebuf) == 0) {
			struct patch_entry *e;

			printf("Patching PROM v%d for %s\n",
			       p->promvers, p->name);
			for (e = p->patches; e->message != NULL; ++e) {
				printf("%s", e->message);
				prom_interpret(e->patch);
				printf("\n");
			}
			return;
		}
	}
}
