/*-
 * Copyright (c) 2011 Department of Software Engineering,
 *		      University of Szeged, Hungary
 * Copyright (c) 2011 Adam Hoka <ahoka@NetBSD.org>
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the Department of Software Engineering, University of Szeged, Hungary
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Device specific functions for legacy Micron NAND chips
 *
 * Currently supported:
 * MT29F2G08AACWP, MT29F4G08BACWP, MT29F8G08FACWP
 */

#include "nand.h"
#include "onfi.h"

int
nand_read_parameters_micron(device_t self, struct nand_chip *chip)
{
	uint8_t byte;

	KASSERT(chip->nc_manf_id == NAND_MFR_MICRON);

	nand_select(self, true);
	nand_command(self, ONFI_READ_ID);
	nand_address(self, 0x00);

	switch (chip->nc_manf_id) {
		/* three dummy reads */
		nand_read_byte(self, &byte); /* vendor */
		nand_read_byte(self, &byte); /* device */
		nand_read_byte(self, &byte); /* unused */

		/* this is the interesting one */
		nand_read_byte(self, &byte);
		/* TODO actually get info */
		nand_select(self, false);
		return 1;
		
		break;
	default:
		nand_select(self, false);
		return 1;
	}
	
	chip->nc_num_luns = 1;
	chip->nc_lun_blocks = chip->nc_size / chip->nc_block_size;

	nand_select(self, false);

	return 0;
}
