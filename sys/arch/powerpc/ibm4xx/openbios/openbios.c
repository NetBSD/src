/*	$NetBSD: openbios.c,v 1.1.14.2 2005/09/15 14:28:44 riz Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: openbios.c,v 1.1.14.2 2005/09/15 14:28:44 riz Exp $");

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


	/* Initialize board properties database */
	board_info_init();

	if (board_info_set("user-config-version",
		&board_bios.usr_config_ver, 
		sizeof(board_bios.usr_config_ver), PROP_CONST, 0))
		panic("setting user-config-version");

	if (board_info_set("rom-software-version",
		&board_bios.rom_sw_ver, 
		sizeof(board_bios.rom_sw_ver), PROP_CONST, 0))
		panic("setting rom-software-version");

	if (board_info_set("mem-size",
		&board_bios.mem_size, 
		sizeof(board_bios.mem_size), PROP_CONST, 0))
		panic("setting mem-size");

	if (board_info_set("emac0-mac-addr",
		&board_bios.mac_address_local, 
		sizeof(board_bios.mac_address_local), PROP_CONST, 0))
		panic("setting emac0-mac-addr");

	if (board_info_set("sip0-mac-addr",
		&board_bios.mac_address_pci, 
		sizeof(board_bios.mac_address_pci), PROP_CONST, 0))
		panic("setting sip0-mac-addr");

	if (board_info_set("processor-frequency",
		&board_bios.processor_speed, 
		sizeof(board_bios.processor_speed), PROP_CONST, 0))
		panic("setting processor-frequency");

	if (board_info_set("plb-frequency",
		&board_bios.plb_speed, 
		sizeof(board_bios.plb_speed), PROP_CONST, 0))
		panic("setting plb-frequency");

	if (board_info_set("pci-frequency",
		&board_bios.pci_speed, 
		sizeof(board_bios.pci_speed), PROP_CONST, 0))
		panic("setting pci-frequency");
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
