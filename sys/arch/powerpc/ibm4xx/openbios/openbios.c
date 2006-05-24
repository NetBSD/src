/*	$NetBSD: openbios.c,v 1.2.12.1 2006/05/24 15:48:20 tron Exp $	*/

/*
 * Copyright (c) 2004 Shigeyuki Fukushima.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: openbios.c,v 1.2.12.1 2006/05/24 15:48:20 tron Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>

#include <powerpc/ibm4xx/openbios.h>

/*
 * Board configuration structure from the OpenBIOS.
 *
 * Supported (XXX):
 *    405GPr 1.2 ROM Monitor (5/25/02)
 */
struct board_bios_data {
	unsigned char	usr_config_ver[4];
	unsigned char	rom_sw_ver[30];
	unsigned int	mem_size;
	unsigned char	mac_address_local[6];
	unsigned char	mac_address_pci[6];
	unsigned int	processor_speed;
	unsigned int	plb_speed;
	unsigned int	pci_speed;
};

static struct board_bios_data board_bios;

void
openbios_board_init(void *info_block, u_int startkernel)
{

        /* Initialize cache info for memcpy, etc. */
        cpu_probe_cache();

	/* Save info block */
	memcpy(&board_bios, info_block, sizeof(board_bios));
}

unsigned int
openbios_board_memsize_get(void)
{
	return board_bios.mem_size;
}

void
openbios_board_info_set(void)
{
	prop_number_t pn;
	prop_string_t ps;
	prop_data_t pd;

	/* Initialize board properties database */
	board_info_init();

	ps = prop_string_create_cstring_nocopy(board_bios.usr_config_ver);
	KASSERT(ps != NULL);
	if (prop_dictionary_set(board_properties, "user-config-version",
				ps) == FALSE)
		panic("setting user-config-version");
	prop_object_release(ps);

	ps = prop_string_create_cstring_nocopy(board_bios.rom_sw_ver);
	KASSERT(ps != NULL);
	if (prop_dictionary_set(board_properties, "rom-software-version",
				ps) == FALSE)
		panic("setting rom-software-version");
	prop_object_release(ps);

	pn = prop_number_create_integer(board_bios.mem_size);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "mem-size", pn) == FALSE)
		panic("setting mem-size");
	prop_object_release(pn);

	pd = prop_data_create_data_nocopy(board_bios.mac_address_local,
					  sizeof(board_bios.mac_address_local));
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "emac0-mac-addr",
				pd) == FALSE)
		panic("setting emac0-mac-addr");
	prop_object_release(pd);

	pd = prop_data_create_data_nocopy(board_bios.mac_address_pci,
					  sizeof(board_bios.mac_address_pci));
	KASSERT(pd != NULL);
	if (prop_dictionary_set(board_properties, "sip0-mac-addr",
				pd) == FALSE)
		panic("setting sip0-mac-addr");
	prop_object_release(pd);

	pn = prop_number_create_integer(board_bios.processor_speed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "processor-frequency",
				pn) == FALSE)
		panic("setting processor-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(board_bios.plb_speed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "plb-frequency", pn) == FALSE)
		panic("setting plb-frequency");
	prop_object_release(pn);

	pn = prop_number_create_integer(board_bios.pci_speed);
	KASSERT(pn != NULL);
	if (prop_dictionary_set(board_properties, "pci-frequency", pn) == FALSE)
		panic("setting pci-frequency");
	prop_object_release(pn);
}


void
openbios_board_print(void)
{

	printf("Board config data:\n");
	printf("  usr_config_ver = %s\n", board_bios.usr_config_ver);
	printf("  rom_sw_ver = %s\n", board_bios.rom_sw_ver);
	printf("  mem_size = %u\n", board_bios.mem_size);
	printf("  mac_address_local = %02x:%02x:%02x:%02x:%02x:%02x\n",
	    board_bios.mac_address_local[0], board_bios.mac_address_local[1],
	    board_bios.mac_address_local[2], board_bios.mac_address_local[3],
	    board_bios.mac_address_local[4], board_bios.mac_address_local[5]);
	printf("  mac_address_pci = %02x:%02x:%02x:%02x:%02x:%02x\n",
	    board_bios.mac_address_pci[0], board_bios.mac_address_pci[1],
	    board_bios.mac_address_pci[2], board_bios.mac_address_pci[3],
	    board_bios.mac_address_pci[4], board_bios.mac_address_pci[5]);
	printf("  processor_speed = %u\n", board_bios.processor_speed);
	printf("  plb_speed = %u\n", board_bios.plb_speed);
	printf("  pci_speed = %u\n", board_bios.pci_speed);
}
