/*	$NetBSD: ibm_machdep.c,v 1.12.4.1 2006/06/19 03:45:06 chap Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus J. Klein.
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

#include <machine/intr.h>
#include <machine/platform.h>

extern struct prep_pci_chipset *prep_pct;

void pci_intr_fixup_ibm_6015(void);

void
pci_intr_fixup_ibm_6015(void)
{
	struct prep_pci_chipset_businfo *pbi;
	prop_dictionary_t dict, sub;
	prop_number_t intr_num;
	int i, j;
	char key[20];

	/* this works because the 6015 has only 1 PCI bus native */
	pbi = SIMPLEQ_FIRST(&prep_pct->pc_pbi);

	dict = prop_dictionary_create_with_capacity(16);
	KASSERT(dict != NULL);
	(void)prop_dictionary_set(pbi->pbi_properties, "prep-pci-intrmap",
	    dict);
	sub = prop_dictionary_create_with_capacity(4);
	KASSERT(sub != NULL);

	intr_num = prop_number_create_integer(13);
	for (j = 0; j < 4; j++) {
		sprintf(key, "pin-%c", 'A' + j);
		prop_dictionary_set(sub, key, intr_num);
	}
	prop_object_release(intr_num);
	prop_dictionary_set(dict, "devfunc-12", sub);
	prop_object_release(sub);

	/* devices 13-19 all share IRQ 15 */
	for (i = 13; i < 20; i++) {
		sub = prop_dictionary_create_with_capacity(4);
		intr_num = prop_number_create_integer(15);
		sprintf(key, "devfunc-%d", i);
		for (j = 0; j < 4; j++) {
			sprintf(key, "pin-%c", 'A' + j);
			prop_dictionary_set(sub, key, intr_num);
		}
		prop_dictionary_set(dict, key, sub);
		prop_object_release(intr_num);
		prop_object_release(sub);
	}
	prop_object_release(dict);
}
