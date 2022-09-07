/*	$NetBSD: emuxki_boards.c,v 1.1 2022/09/07 03:34:43 khorben Exp $	*/

/*-
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yannick Montulet, and by Andrew Doran.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: emuxki_boards.c,v 1.1 2022/09/07 03:34:43 khorben Exp $");

#include <dev/pci/emuxkivar.h>
#include <dev/pci/emuxki_boards.h>

/* generic fallbacks must be after any corresponding precise match */
static const struct emuxki_board emuxki_boards[] = {
	{
		.sb_board = "SB1550",
		.sb_name = "Sound Blaster Audigy Rx",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBAUDIGY4,
		.sb_subsystem = 0x10241102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2 | EMUXKI_AUDIGY2_CA0108,
	},
	{
		.sb_board = "SB0610",
		.sb_name = "Sound Blaster Audigy 4",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBAUDIGY4,
		.sb_subsystem = 0x10211102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2 | EMUXKI_AUDIGY2_CA0108,
	},
	{
		.sb_board = "SB0400",
		.sb_name = "Sound Blaster Audigy 2 Value",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBAUDIGY4,
		.sb_subsystem = 0x10011102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2 | EMUXKI_AUDIGY2_CA0108,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Audigy 2 Value",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBAUDIGY4,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2 | EMUXKI_AUDIGY2_CA0108,
	},
	{
		.sb_board = "SB0350",
		.sb_name = "Sound Blaster Audigy 2 ZS",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_subsystem = 0x20021102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2,
	},
	{
		.sb_board = "SB0350",
		.sb_name = "Sound Blaster Audigy 2",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_subsystem = 0x20051102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2,
	},
	{
		.sb_board = "SB0240",
		.sb_name = "Sound Blaster Audigy 2",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_subsystem = 0x10021102,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Audigy 2",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_revision = 0x8,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Audigy 2",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_revision = 0x4,
		.sb_flags = EMUXKI_AUDIGY | EMUXKI_AUDIGY2,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Audigy",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_AUDIGY,
		.sb_flags = EMUXKI_AUDIGY,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Live! 2",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBLIVE2,
		.sb_flags = EMUXKI_SBLIVE,
	},
	{
		.sb_board = "CT4870",
		.sb_name = "Sound Blaster Live! Value",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBLIVE,
		.sb_subsystem = 0x80281102,
		.sb_flags = EMUXKI_SBLIVE,
	},
	{
		.sb_board = "CT4790",
		.sb_name = "Sound Blaster PCI512",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBLIVE,
		.sb_subsystem = 0x80231102,
		.sb_flags = EMUXKI_SBLIVE,
	},
	{
		.sb_board = "PC545",
		.sb_name = "Sound Blaster E-mu APS",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBLIVE,
		.sb_subsystem = 0x40011102,
		.sb_flags = EMUXKI_APS,
	},
	{
		.sb_board = "generic",
		.sb_name = "Sound Blaster Live!",
		.sb_vendor = PCI_VENDOR_CREATIVELABS,
		.sb_product = PCI_PRODUCT_CREATIVELABS_SBLIVE,
		.sb_flags = EMUXKI_SBLIVE,
	},
};

const struct emuxki_board *
emuxki_board_lookup(pci_vendor_id_t vendor, pci_product_id_t product,
		    uint32_t subsystem, uint8_t revision)
{
	const struct emuxki_board *sb;
	unsigned int i;

	for (i = 0; i < __arraycount(emuxki_boards); i++) {
		sb = &emuxki_boards[i];

		/* precise match */
		if (vendor == sb->sb_vendor && product == sb->sb_product &&
		    subsystem == sb->sb_subsystem && revision == sb->sb_revision)
			return sb;

		/* generic fallback (same subsystem, any revision) */
		if (vendor == sb->sb_vendor && product == sb->sb_product &&
		    subsystem == sb->sb_subsystem && sb->sb_revision == 0)
			return sb;

		/* generic fallback (any subsystem, specific revision) */
		if (vendor == sb->sb_vendor && product == sb->sb_product &&
		    sb->sb_subsystem == 0 && revision == sb->sb_revision)
			return sb;

		/* generic fallback (same vendor, same product) */
		if (vendor == sb->sb_vendor && product == sb->sb_product &&
		    sb->sb_subsystem == 0 && sb->sb_revision == 0)
			return sb;
	}

	return NULL;
}
